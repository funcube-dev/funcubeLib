//    Copyright 2013 (c) AMSAT-UK
//
//    This file is part of FUNcubeLib.
//
//    FUNcubeLib is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    FUNcubeLib is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with FUNcubeLib If not, see <http://www.gnu.org/licenses/>.
//

/// @file DecodeManager.cpp
//
//////////////////////////////////////////////////////////////////////
#include "FuncubeLib.h"
#include "AudioBufferConfig.h"
#include "portaudio.h"
#include <math.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "BpskDecoder.h"

#include "DecodeManager.h"

using namespace std;
using namespace fc;

CDecodeManager::CDecodeManager(IDecodeResult* listener)
{
	m_decodeResultListener = listener;
    m_manualTuneBin=0;
    m_requiredAudioCount = 24;
    m_lowLimitBin = 0;
	m_highLimitBin = 0;

    m_lowLimitBinLastDecode  = m_lowLimitBin;
    m_highLimitBinLastDecode = m_highLimitBin;

	m_freeTime.SetAverageSamples(100);
	m_freeTime.SetAverage(50.0);

    m_rangeChanged = TRUE;

    m_autoTuneState = FFTTunerState_On;
    m_rangeLimitHz = FFTRangeLimit_Slope;

    m_DCOffSetRemove=TRUE;    
    m_decoderNum = 3;
    m_peakDetectDelaySecs = 1.0;
    m_peakDetectMaxCalc = 2;
}

CDecodeManager::~CDecodeManager(void)
{
    m_workerThread.RemoveHandle(m_workAvailable);
	m_fftResult = NULL;

    m_memPoolOut.Shutdown();
    m_memPoolIn.Shutdown();
	m_memPoolFft.Shutdown();
}

void CDecodeManager::Initialise()
{    
    //OutputDebugStringA("\n\nINIT|\n\n");
    m_workAvailable.Create(NULL, FALSE, FALSE, NULL);
    m_workAvailable.Reset();
    m_workerThread.Initialize();
    m_workerThread.AddHandle(m_workAvailable, this, NULL);    
}

BOOL CDecodeManager::Start(UINT sampleRateIn)
{
    //OutputDebugStringA("\n\nST|\n\n");
	m_bufferConfig.SetSampleRate(sampleRateIn);

	// average gain and DC offset over half an FFT buffers worth of time
	m_aveDCOffset.SetAverageSamples(m_bufferConfig.NumBinsFftHalf());
	m_aveGain.SetAverageSamples(m_bufferConfig.NumBinsFftHalf());

	m_memPoolOut.Initialise(64, m_bufferConfig.BytesAudioOutBuffer());
	m_memPoolIn.Initialise(64, m_bufferConfig.BytesAudioInBuffer());
	m_memPoolFft.Initialise(4, m_bufferConfig.BytesFftBuffer());

    // initialise all our decoders up front (won't be using them all initialy)
	for (size_t i = 0; i < MAX_DECODERS; i++)
	{
        auto decodeWorker = make_shared<CDecodeWorker>(
			m_decodeResultListener,
			new CBpskDecoder(
				this,
                this,
				FALSE, 
				m_bufferConfig.SampleRateIn(), 
				m_bufferConfig.FreqRangeFft()), m_bufferConfig.FramesPerBufferIn());
        m_decoders.push_back(decodeWorker);
	}

	m_fft.Initialise(m_bufferConfig.NumBinsFft(), m_bufferConfig.FramesPerBufferIn());
    // detect upto 32 peaks no closer than ~500Hz apart
	m_peakDetect.Initialise(m_bufferConfig.NumBinsFft(), 32, 50);

	m_peakProcessThread.Initialise(&m_peakDetect, m_bufferConfig.NumBinsFft());

    // set the sample rate for the sample based timer
    m_lastPeakDetectTime.Initialise(sampleRateIn);

	m_stopWatch.Start();

	return TRUE;
}

FLOAT BinToFreq(ULONG bin) 
{ 
	static FLOAT result = (HZ_PER_BIN*bin) - 96000.0F;
	return  result;
}

ULONG FreqToBin(FLOAT freq) 
{ 
	static ULONG result = ((freq + 96000.0F) / HZ_PER_BIN);
	return  result;
}

void CDecodeManager::OnDecodeSuccess(byte* buffer, INT bufferSize, FLOAT frequency, INT errorCount)
{
    static byte lastResult[256];
    if(NULL!=m_decodeResultListener)
    {
        if(memcmp(lastResult,buffer, min(256,bufferSize))!=0)
        {
            memcpy(lastResult,buffer, min(256,bufferSize));
            LogMessage(4, "decode manager callback");
            m_decodeResultListener->OnDecodeSuccess(buffer, bufferSize, frequency, errorCount);
        }
    }
}

BOOL CDecodeManager::ReadyForAudioInput()
{    
    MutexGuard guard(m_bufferLock);
    
    BOOL full = m_queueAudioIn.size()>8;        
    return !full;
}

BOOL CDecodeManager::PushAudioInput(AutoBufPtr ptr, BOOL overwrite)
{
    MutexGuard guard(m_bufferLock);
    if(m_queueAudioIn.size()>8)  // discard oldest, if we have 8 sample buffers waiting
    {
        if(overwrite)
        {
            m_queueAudioIn.pop();
        }
        else
        {            
            return FALSE;
        }
    }
    m_queueAudioIn.push(ptr);    
    m_workAvailable.Set();

    return TRUE;
}

AutoBufPtr CDecodeManager::CollectAudioInput()
{
    MutexGuard guard(m_bufferLock);

    AutoBufPtr ret;    
    if(m_queueAudioIn.size()>0)
    {        
        ret = m_queueAudioIn.front();
        m_queueAudioIn.pop();
    }    
    
    return ret;
}

void CDecodeManager::GetFftBuffer(COMPLEXSTRUCT* resultBuffer, UINT sampleCount)
{
    MutexGuard guard(m_bufferLock);
    if(NULL!=resultBuffer)
    {        
		m_fft.CopyResult(resultBuffer, sampleCount);
    }    
}

BOOL CDecodeManager::GetWorkerFreqs(std::vector<FLOAT>& workerFreqs, const ULONG requestedCount)
{	
    ULONG count = min(requestedCount, m_decoderNum);
    // if manual tuning just push the first decoder freq
    if (m_autoTuneState != FFTTunerState_On) {
        count = 1;
    }

    workerFreqs.clear();
    for (UINT idx = 0; idx < count; idx++) {
        workerFreqs.push_back(m_decoders[idx]->GetTuneFrequency());
    }

	return workerFreqs.size() > 0;
}

BOOL CDecodeManager::GetWorkerAvail(std::vector<UINT>& workerAvail, const ULONG requestedCount)
{
    ULONG count = min(requestedCount, m_decoderNum);
    // if manual tuning just push the first decoder freq
    if (m_autoTuneState != FFTTunerState_On) {
        count = 1;
    }

    workerAvail.clear();
    for (UINT idx = 0; idx < count; idx++) {
        workerAvail.push_back(m_decoders[idx]->GetAvailabilityFactor());
    }

    return workerAvail.size() > 0;
}


void CDecodeManager::PublishAudioOutput(AutoBufPtr ptr)
{
    MutexGuard guard(m_bufferLock);

    if(m_queueAudioOut.size()>5)  // discard oldest, if we have 3 sample buffers waiting
    {
        m_queueAudioOut.pop();
    }
    m_queueAudioOut.push(ptr);        
}

AutoBufPtr CDecodeManager::PopAudioOutput()
{
    MutexGuard guard(m_bufferLock);

    AutoBufPtr ret;    
    if(m_queueAudioOut.size()>m_requiredAudioCount)
    {
        m_requiredAudioCount = 0;
        ret = m_queueAudioOut.front();
        m_queueAudioOut.pop();        
    }
    else
    {
        // if we run out, make sure we have 3 sample buffers ready before we start up again
        m_requiredAudioCount = 3;        
    }

    return ret;
}

fc::AutoBufPtr CDecodeManager::GetEmptyInputAudioBuffer()
{
    return m_memPoolIn.GetBuffer();
}

fc::AutoBufPtr CDecodeManager::GetEmptyOutputAudioBuffer()
{
    return m_memPoolOut.GetBuffer();
}

void CDecodeManager::SetAutoTuneRange(ULONG lowBin, ULONG highBin)
{
    if(lowBin>highBin)
    {
        return;
    }
    if(highBin>m_bufferConfig.NumBinsFft())
    {
        return;
    }

    m_rangeChanged = TRUE;

    m_lowLimitBin = lowBin;
    m_highLimitBin = highBin;

	m_peakDetect.SetDetectLimits(m_lowLimitBin, m_highLimitBin);
    m_lowLimitBinLastDecode = m_lowLimitBin;
    m_highLimitBinLastDecode = m_highLimitBin;
}

void CDecodeManager::SetLastDecodeTuneRangeFreq(FLOAT lowFreq, FLOAT highFreq)
{
    ULONG lowBin = FreqToBin(lowFreq);
    ULONG highBin = FreqToBin(highFreq);

    if(lowBin>highBin)
    {
        return;
    }
    if(highBin>m_bufferConfig.NumBinsFft())
    {
        return;
    }

    m_rangeChanged = TRUE;

    m_lowLimitBinLastDecode = lowBin;
    m_highLimitBinLastDecode = highBin;
}

void CDecodeManager::NormaliseGainRemoveDCOffset(COMPLEXSTRUCT* pSamples, INT sampleCount) {
    COMPLEXSTRUCT dcOffset, scale;
    for (int index = 0; index < sampleCount; ++index)
    {
        m_aveDCOffset.Average(*pSamples, &dcOffset);
        pSamples->fRe -= dcOffset.fRe;
        pSamples->fIm -= dcOffset.fIm;

        m_aveGain.Average(fabsf(pSamples->fRe), fabsf(pSamples->fIm), &scale);
        pSamples->fRe *= (0.5 / scale.fRe);
        pSamples->fIm *= (0.5 / scale.fIm);

        pSamples++;
    }    
}

void CDecodeManager::GenerateFft(COMPLEXSTRUCT* pSamples, INT sampleCount) {
    BOOL fftChanged = FALSE;
    // add sample until there is enough to produce a new fft
    if (m_fft.Add(pSamples, sampleCount, fftChanged) && fftChanged) {
        // grab the new fft data and pass to peak detector for procssing
        AutoBufPtr fftResultBuf = m_memPoolFft.GetBuffer();
        m_fft.CopyResult((COMPLEXSTRUCT*)fftResultBuf->Data(), m_bufferConfig.NumBinsFft());
        m_peakProcessThread.PushFftInput(fftResultBuf);
    }
}

HRESULT CDecodeManager::Execute(DWORD_PTR dwParam, HANDLE hObject)
{
    // record how much time was spent before we got called
    DOUBLE elapsedFreeTime = m_stopWatch.GetElapsedSeconds(TRUE);

    //OutputDebugStringA("X|");
        
    m_bufferLock.lock();

    AutoBufPtr inSamples = CollectAudioInput();
    if(NULL == inSamples.get() || NULL == inSamples->Data())
    {
        //OutputDebugStringA("*1*|");
        m_bufferLock.unlock();
        return S_FALSE;
    }
	
    INT sampleCount = inSamples->Size()/sizeof(COMPLEXSTRUCT);
    COMPLEXSTRUCT* pSamples = (COMPLEXSTRUCT*)inSamples->Data();
	if (m_bufferConfig.FramesPerBufferIn() != sampleCount)
	{
        //OutputDebugStringA("*2*|");
        m_bufferLock.unlock();
		return S_FALSE;
	}
    // update timer (based on samples not elapsedtime)    
    m_lastPeakDetectTime.IncrementCount(sampleCount);

    NormaliseGainRemoveDCOffset(pSamples, sampleCount);
    GenerateFft(pSamples, sampleCount);
	    
    m_bufferLock.unlock();

    if (m_autoTuneState == FFTTunerState_On) {
        //OutputDebugStringA("A|");
        // trigger a new peak detection
        CheckPeakDetection();

        std::vector<UINT> peakPos;
        if (m_peakProcessThread.PeakCalcUpdated()) {
            //OutputDebugStringA("U|");
            if(m_peakProcessThread.GetPeaks(peakPos)) {
                //OutputDebugStringA("P|");
                // sort the worker list into:
                //      working workers, most recently decoded first
                //      available workers, most recently tuned first
                std::sort(std::begin(m_decoders), std::end(m_decoders), [](const auto a, const auto b) {
                    return a->GetAvailabilityFactor() < b->GetAvailabilityFactor();
                });

                auto peakIter = peakPos.begin();
                for (UINT idx = 0; idx < m_decoderNum && peakIter != peakPos.end(); idx++) {
                    if (m_decoders[idx]->GetAvailabilityFactor() > 100) {
                        // enable auto tune and set frequency
                        m_decoders[idx]->EnableAutoTune(TRUE);
                        m_decoders[idx]->SetTuneFrequency(m_bufferConfig.BinToFreq(*peakIter++));
                        //OutputDebugStringA("S|");
                    }
                }
            }
        }

        for (UINT i = 0; i < m_decoderNum; i++) {            
            //m_decoders[i]->EnableAudioOut(i == 0);
            // feed all the decoders the data, shared_ptr so no copy needed
            m_decoders[i]->PushAudioInput(inSamples);            
        }        
    }
    else {
        // manual tune, update one decoder, no peak detection etc...
        //OutputDebugStringA("M|");
        m_decoders[0]->EnableAutoTune(FALSE);
        m_decoders[0]->EnableAudioOut(TRUE);
        m_decoders[0]->SetTuneFrequency(m_bufferConfig.BinToFreq(m_manualTuneBin));
        m_decoders[0]->PushAudioInput(inSamples);
    }
    	
    LogBusyTime(elapsedFreeTime);
    return S_OK;
}

void CDecodeManager::CheckPeakDetection()
{
    // only trigger peak detection at most once per second    
    if (m_lastPeakDetectTime.IsStarted() && m_lastPeakDetectTime.GetElapsedSeconds() < m_peakDetectDelaySecs) {
        return;
    }
    m_lastPeakDetectTime.Restart();
    OutputDebugStringA("K|");

    // start available decoder count with all of them
    UINT availDecoders = m_decoderNum;
    std::vector<UINT> excludeBins;
    // how many workers need re-purposing (availability factor >= 100)
    for (UINT idx = 0; idx < m_decoderNum; idx++) {
        OutputDebugStringA("D");
        if (m_decoders[idx]->GetAvailabilityFactor() < 100) {
            OutputDebugStringA("N");
            // exclude frequencies that already have a decoder working on them
            excludeBins.push_back(FreqToBin(m_decoders[idx]->GetTuneFrequency()));
            // remove an available decoder
            availDecoders--;
        }
        else {
            OutputDebugStringA("F");
        }        
    }
    
    // if there are any free/avaiable decoders, trigger a peek detect
    // excluding all the decoders that are currently busy
    if (availDecoders > 0) {
        OutputDebugStringA("T");
        m_peakProcessThread.TriggerPeakCalc(min(availDecoders, m_peakDetectMaxCalc), excludeBins);
    }

    OutputDebugStringA("|\n");
}

void CDecodeManager::LogBusyTime(DOUBLE elapsedFreeTime)
{
    DOUBLE elapsedProcessingTime = m_stopWatch.GetElapsedSeconds(TRUE);

    DOUBLE totalTime = elapsedFreeTime + elapsedProcessingTime;
    DOUBLE percentConv = 100 / totalTime;
    DOUBLE freePercent = elapsedFreeTime * percentConv;
    m_freeTime.Average(freePercent);
    if (m_freeTime.GetAverage() < 5)
    {
        LOG_MESSAGE(5, "freetime " << freePercent);
    }
}

HRESULT CDecodeManager::CloseHandle(_In_ HANDLE hHandle)
{
    // Dont do anything the handle is looked after by the CEvent class
    //::CloseHandle(hHandle);
    return S_OK;
}

void CDecodeManager::SetFftRangeLimit(FFTRangeLimit limit) 
{ 
    m_rangeLimitHz = limit;
};

void CDecodeManager::SetAutoTuneState(FFTTunerState tunerState) 
{ 
    m_autoTuneState = tunerState; 
};

void CDecodeManager::RemoveDCOffset(BOOL enable) 
{ 
    m_DCOffSetRemove = enable; 
}