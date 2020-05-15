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

/// @file Decode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "FuncubeLib.h"
#include "portaudio.h"
#include <math.h>
#include <stdlib.h>
#include <string>
#include "DecodeManager.h"
#include "MemPool.h"
#include "GuardInterlock.h"

#include "Decode.h"

using namespace std;
using namespace fc;

// CDecode
CDecode::CDecode()
{
	m_lockWorking = 0;

	m_streamIn = NULL;
	m_streamOut = NULL;
	m_isComplexSignal=FALSE;
	m_stereoIn=TRUE;
	m_audioOut=FALSE;

	m_decodeManager=NULL;
	m_audioSource=NULL;
	m_fftBuffer=NULL;

	m_averagePeakPower = 0;
	m_averagePower = 0;
	m_fftResultSize = 4096;
	m_fftResultFreqRange = 48000.0F;

	m_lastErrorText=NULL;
}

CDecode::~CDecode()
{
}

BOOL CDecode::Initialise()
{
	if(NULL != m_decodeManager)
	{
		// already initialised is ok!
		return TRUE;
	}

	m_decodeManager = new CDecodeManager((IDecodeResult*)this);
	m_decodeManager->Initialise();

	m_averageInput = 0.0F;
	
	// allocate the max we will ever need
	m_fftBuffer = new COMPLEXSTRUCT[m_bufferConfig.BytesAudioInBufferMax()];

	return TRUE;
}

BOOL CDecode::ReinitialisePortAudio()
{
	if(NULL!=m_streamIn || NULL!=m_streamOut)
	{
		m_lastErrorText = "Streaming in progress";
		return FALSE;
	}

	Pa_Terminate();

	PaError initRes = Pa_Initialize();
	if (initRes!=paNoError)
	{
		m_lastErrorText = Pa_GetErrorText(initRes);
		return FALSE;
	}
	return TRUE;
}

BOOL CDecode::EnumDevices(DeviceEnumCallback callbackProc)
{
        if (NULL == callbackProc)
                return FALSE;
        if (!ReinitialisePortAudio())
                return FALSE;

        const PaDeviceInfo* deviceInfo;
		const PaHostApiInfo* apiInfo;
        for(PaDeviceIndex idx = 0; idx<Pa_GetDeviceCount()-1 ;idx++)
        {
            deviceInfo = Pa_GetDeviceInfo(idx);
			apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            // Note: keeping it simples will only work if wide characters are in ASCII range,
            // otherwise need to use locale and facets
            string name(deviceInfo->name);
            wstring wideDeviceName;
            wideDeviceName.assign(name.begin(), name.end());
			string apiName(apiInfo->name);
			wstring wideApiName;
			wideApiName.assign(apiName.begin(), apiName.end());
            if (!callbackProc((LPCSTR)wideDeviceName.c_str(), (LPCSTR)wideApiName.c_str(), (int)idx, deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels))
			{
				m_lastErrorText = "EnumDevices callback returned FALSE";
				return FALSE;
			}
        }
        return TRUE;
}

BOOL CDecode::Start(LPCSTR inDeviceName, LPCSTR outDeviceName, BOOL stereoIn, BOOL isFCDongle)
{
	if(!ReinitialisePortAudio())
	{
		return FALSE;
	}

	PaDeviceIndex inDeviceIdx = -1;
	PaDeviceIndex outDeviceIdx = -1;

	string inDevice(inDeviceName);
	string outDevice(outDeviceName);

	if(inDevice.compare("default")==0)
	{
		inDeviceIdx = Pa_GetDefaultInputDevice();
	}

	if(outDevice.compare("default")==0)
	{
		outDeviceIdx = Pa_GetDefaultOutputDevice();
	}

	const PaDeviceInfo* deviceInfo;
	for(PaDeviceIndex idx = 0; idx<Pa_GetDeviceCount()-1 && (-1==inDeviceIdx || -1==outDeviceIdx);idx++)
	{        
		deviceInfo = Pa_GetDeviceInfo(idx);
		string name(deviceInfo->name);
		size_t value = name.find(inDevice);
		if(-1==inDeviceIdx && name.find(inDevice)!=string::npos && deviceInfo->maxInputChannels>0)
		{
			inDeviceIdx = idx;            
		}
		if(-1==outDeviceIdx && name.find(outDevice)!=string::npos && deviceInfo->maxOutputChannels>0)
		{
			outDeviceIdx = idx;            
		}
	}

	if(-1==inDeviceIdx || -1==outDeviceIdx)
	{
		m_lastErrorText = "Unable to find specified device(s)";
		return FALSE;
	}

	return Start(inDeviceIdx, outDeviceIdx, stereoIn, isFCDongle);
}

PaDeviceIndex CDecode::CheckFCDDeviceIndex(PaDeviceIndex inDeviceIdx)
{
	PaDeviceIndex checkedIdx = -1;

	const PaDeviceInfo* deviceInfo;
	if(inDeviceIdx>-1)
	{
		deviceInfo = Pa_GetDeviceInfo(inDeviceIdx);
		if(NULL != deviceInfo)
		{
			string name(deviceInfo->name);            
			size_t value = name.find("FUNcube");
			if(name.find("FUNcube")!=string::npos)
			{
				checkedIdx = inDeviceIdx;
			}
		}        
	}

	if(-1 == checkedIdx)
	{

		for(PaDeviceIndex idx = 0; idx<Pa_GetDeviceCount()-1 && -1==checkedIdx;idx++)
		{        
			deviceInfo = Pa_GetDeviceInfo(idx);
			if(NULL != deviceInfo)
			{
				string name(deviceInfo->name);            
				size_t value = name.find("FUNcube");
				if(name.find("FUNcube")!=string::npos)
				{
					checkedIdx = idx;
				}
			}
		}
	}

	return checkedIdx;
}

BOOL CDecode::Start(int inDeviceIdx, int outDeviceIdx, BOOL stereoIn, BOOL isFCDongle)
{	
	if(!ReinitialisePortAudio())
	{
		return FALSE;
	}        

	m_fftResultSize = 4096;
	m_fftResultFreqRange = 48000.0F;
		

	m_isComplexSignal = isFCDongle;    
	m_stereoIn = stereoIn;

	if(-1 == inDeviceIdx)
	{
		inDeviceIdx = Pa_GetDefaultInputDevice();
        LOG_DEBUG("defaultInDevice:" << inDeviceIdx);
	}

	if(-1 == outDeviceIdx)
	{
		outDeviceIdx = Pa_GetDefaultOutputDevice();
        LOG_DEBUG("defaultOutDevice:" << outDeviceIdx);
	}

	// only error if no input device available, not fatal if no output device
	if(-1==inDeviceIdx)
	{
		m_lastErrorText = "No input audio device specified and no defaults available";
		return FALSE;
	}

	const PaDeviceInfo* inDeviceInfo = Pa_GetDeviceInfo(inDeviceIdx);
	
	PaStreamParameters inStreamParam;
	inStreamParam.channelCount = m_stereoIn?2:1;
	inStreamParam.device = inDeviceIdx;
	inStreamParam.sampleFormat = paFloat32;
	inStreamParam.suggestedLatency = inDeviceInfo->defaultLowInputLatency;
	inStreamParam.hostApiSpecificStreamInfo = NULL;

	PaError inResult = paInvalidSampleRate;
	UINT sampleRates[5] = { 192000, 96000, 48000, 24000 };
	UINT sampleRateIn;
	for (UINT sampleRateIndex=0;sampleRateIndex<5 && inResult == paInvalidSampleRate; ++sampleRateIndex) 
	{
		sampleRateIn = sampleRates[sampleRateIndex];
		inResult = Pa_IsFormatSupported(&inStreamParam, NULL, sampleRateIn);
		if (inResult == paFormatIsSupported)
		{
            LOG_DEBUG("sampleRate:" << sampleRateIn);
			m_bufferConfig.SetSampleRate(sampleRateIn);
			inResult = Pa_OpenStream
			(
				&m_streamIn,
				&inStreamParam,
				NULL,
				sampleRateIn,
				m_bufferConfig.FramesPerBufferIn(),
				0,                     /* stream flags */
				StaticCallbackIn,
				this
			);
		}
	}

	PaError outResult=paNoError;
	if(paNoError == inResult && -1!=outDeviceIdx)
	{
		const PaDeviceInfo* outDeviceInfo = Pa_GetDeviceInfo(outDeviceIdx);
		PaStreamParameters outStreamParam;
		outStreamParam.channelCount = 1;
		outStreamParam.device = outDeviceIdx;
		outStreamParam.sampleFormat = paFloat32;
		outStreamParam.suggestedLatency = outDeviceInfo->defaultLowOutputLatency;
		outStreamParam.hostApiSpecificStreamInfo = NULL;

		outResult = Pa_IsFormatSupported( NULL, &outStreamParam, m_bufferConfig.SampleRateOut());
		if( outResult == paFormatIsSupported )
		{
			outResult = Pa_OpenStream
			(
				&m_streamOut,
				NULL,
				&outStreamParam,
				m_bufferConfig.SampleRateOut(),
				m_bufferConfig.FramesPerBufferOut(),
				0,                     /* stream flags */        
				StaticCallbackOut,
				this
				);
		}
	}
	// average output volume over 2 frames
	m_outputVolumeNormalise.SetAverageSamples(m_bufferConfig.FramesPerBufferOut() * 2);
	m_outputVolumeNormalise.SetAverage(0.0);

	// Before we start the streams initialise the DecodeManager
	if (!m_decodeManager->Start(m_bufferConfig.SampleRateIn()))
	{
		m_lastErrorText = "Failed to start decode manger.";
		return FALSE;
	}

	if (paNoError == inResult)
	{
		inResult = Pa_StartStream(m_streamIn);
	}
	else
	{
		m_lastErrorText = Pa_GetErrorText(inResult);
	}

	if (paNoError == outResult)
	{
		outResult = Pa_StartStream(m_streamOut);
	}
	else
	{
		m_lastErrorText = Pa_GetErrorText(outResult);
	}

	// capture is whats important, don't error if we can't output, so long as we can capture...
	return paNoError == inResult;
}

BOOL CDecode::Stop()
{
	if(NULL != m_streamIn)
	{
		Pa_StopStream(m_streamIn);        
		Pa_CloseStream(m_streamIn);        
		m_streamIn = NULL;
	}
	if(NULL != m_streamOut)
	{
		Pa_StopStream(m_streamOut);        
		Pa_CloseStream(m_streamOut);        
		m_streamOut = NULL;
	}
	if(NULL != m_audioSource)
	{
		m_audioSource->Stop();
		m_audioSource = NULL;
	}    
	return TRUE;
}

BOOL CDecode::Shutdown()
{
	Stop();

	// prevent callback from running by getting the lock
	InterlockedExchangeAdd(&m_lockWorking, 1UL);
	Pa_Terminate();
	InterlockedExchangeSubtract(&m_lockWorking, 1UL);

	delete m_decodeManager;
	m_decodeManager = NULL;

	delete[] m_fftBuffer;
	m_fftBuffer = NULL;

	return TRUE;
}

int CDecode::StaticCallbackIn(const void *inputBuffer,
	void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	return ((CDecode*)userData)->CallbackIn(inputBuffer,outputBuffer,framesPerBuffer,timeInfo,statusFlags);
}

int CDecode::StaticCallbackOut(const void *inputBuffer,
	void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	return ((CDecode*)userData)->CallbackOut(inputBuffer,outputBuffer,framesPerBuffer,timeInfo,statusFlags);
}

/* receive an audio buffer from elsewhere:
 * Returns: 0 = ok (or not able to process, but this isn't fatal), negative = error
 */
int CDecode::AudioBufferReady(const void *inputBuffer, unsigned long framesPerBuffer)
{
	// grab the working lock, bail if already taken
	CGuardInterlock<ULONG> _lock(&m_lockWorking);
	if(_lock.Previous()>0)
	{
		return 0;
	}

	SAMPLE *in = (SAMPLE*)inputBuffer;
	if(NULL == in)
	{
		return 0;
	}
	fc::AutoBufPtr ptr = m_decodeManager->GetEmptyInputAudioBuffer();

	if(NULL == ptr.get())
	{
		LogMessage(1, "Audio Input failed to get buffer");
		return -1;
	}

	if(ptr->Capacity()<framesPerBuffer*sizeof(SAMPLE))
	{
		LogMessage(1, "Audio Input buffer size error");
		return -2;
	}

	SAMPLE *buffer = (SAMPLE*)ptr->Data();

	if(m_stereoIn)
	{
		for(ULONG i=0; i<framesPerBuffer; i++ )
		{	
			*buffer++ = *in++; // real				
			*buffer++ = *in++; // imaginary				
			ptr->Size()+=sizeof(SAMPLE)*2;
		}
	}
	else
	{
		for(ULONG i=0; i<framesPerBuffer; i++ )
		{	
			*buffer++ = *in++; // real
			*buffer++ = 0; // imaginary
			ptr->Size()+=sizeof(SAMPLE)*2;
		}
	}

	if(m_audioSource->IsRealtime())
	{
		m_decodeManager->PushAudioInput(ptr);
	}
	else
	{
		// if not realtime, block don't overwrite samples
		while(!m_decodeManager->ReadyForAudioInput())
		{	
			Sleep(10);
		}

		if(!m_decodeManager->PushAudioInput(ptr, FALSE))
		{
			return -3;
		}
	}
	return 0;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
int CDecode::CallbackIn(const void *inputBuffer,
	void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags)
{    
	CGuardInterlock<volatile ULONG> _lock(&m_lockWorking);
	if(_lock.Previous()>0)
	{
		return -1;
	}

	SAMPLE s;

	SAMPLE *in = (SAMPLE*)inputBuffer;
	fc::AutoBufPtr ptr = m_decodeManager->GetEmptyInputAudioBuffer();

	if(NULL == ptr.get())
	{
		//LOG_ERROR << "Audio Input failed to get buffer";
		return 0;
	}

	if(ptr->Capacity()<framesPerBuffer*sizeof(SAMPLE))
	{
		//LOG_ERROR << "Audio Input buffer size error expecting > : "  << framesPerBuffer*sizeof(SAMPLE) << " got: " << ptr->Capacity();
		return -1;
	}

	SAMPLE *buffer = (SAMPLE*)ptr->Data();

	if(in==0)
	{
		return 0;
	}

	(void) timeInfo; /* Prevent unused variable warning. */    
	(void) outputBuffer;

	if(m_stereoIn)
	{
		for(ULONG i=0; i<framesPerBuffer; i++ )
		{    
			*buffer++ = s = *in++; // real                
			*buffer++ = s = *in++; // imaginary                
			ptr->Size()+=sizeof(SAMPLE)*2;
		}
	}
	else
	{
		for(ULONG i=0; i<framesPerBuffer; i++ )
		{    
			*buffer++ = s = *in++; // real
			*buffer++ = 0; // imaginary
			ptr->Size()+=sizeof(SAMPLE)*2;
		}
	}

	m_decodeManager->PushAudioInput(ptr);

	return 0;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
int CDecode::CallbackOut(const void *inputBuffer,
	void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags)
{    
	(void) timeInfo; /* Prevent unused variable warning. */    
	(void) inputBuffer;

	if (!m_audioOut) {
		memset(outputBuffer, 0, framesPerBuffer * sizeof(SAMPLE));		
		return 0;
	}
	
	AutoBufPtr ptr = m_decodeManager->PopAudioOutput();
	if (NULL == ptr.get())
	{
		return 0;
	}
	if (ptr->Size() != framesPerBuffer * sizeof(SAMPLE))
	{
		//LOG_ERROR << "Audio Output buffer size error expecting: "  << framesPerBuffer*sizeof(SAMPLE) << " got: " << ptr->Size();
		return -1;
	}

	SAMPLE *buffer = (SAMPLE*)ptr->Data();
	SAMPLE *out = (SAMPLE*)outputBuffer;
	SAMPLE fSample;
	for (ULONG i = 0; i<framesPerBuffer; i++)
	{
		fSample = *buffer++;
		fSample *= (0.5F / m_outputVolumeNormalise.Average(fSample<0 ? -fSample : fSample)) + 0.5F;
		fSample = min(0.5F, max(-0.5F, fSample));
		*out++ = fSample;
	}

	return 0;
}

void CDecode::SetManualTuneFrequency(FLOAT newVal)
{
	ULONG manualTuneBin = m_bufferConfig.FreqToBin(newVal);
	m_decodeManager->SetManualTuneBin(manualTuneBin);
}

void CDecode::SetAutoTuneFrequencyRange(FLOAT low, FLOAT high)
{
	// turn on autoCentreFreq is newVal zero,
	// otherwise use the given value
	ULONG lowBin = m_bufferConfig.FreqToBin(low);
	ULONG highBin = m_bufferConfig.FreqToBin(high);

	m_decodeManager->SetAutoTuneRange(lowBin, highBin);
}

BOOL CDecode::CollectFftOutput(FLOAT* outBuffer, ULONG* outBufferElements)
{
	LONG bufferLength = *outBufferElements;
	*outBufferElements=-1;

	// copy data into buffer
	int samplesPerLine = m_fftResultSize/bufferLength;

	if(samplesPerLine<1)
	{
		samplesPerLine=1;
	}

	m_decodeManager->GetFftBuffer(m_fftBuffer, m_fftResultSize);

	int i=0;
	int j=0;
	float value=0.0;
	*outBufferElements = 0;
	while(i<m_fftResultSize && j<bufferLength)
	{
		int count=0;
		outBuffer[j]=0.0;
		while(count++<samplesPerLine && i<m_fftResultSize)
		{    
			value = 20*log(sqrt(m_fftBuffer[i].fRe*m_fftBuffer[i].fRe) + (m_fftBuffer[i].fIm*m_fftBuffer[i].fIm));
			if(value>outBuffer[j])
			{                
				outBuffer[j]=+value;                
			}
			i++;            
		}

		(*outBufferElements)++;
		j++;
	}

	return S_OK;
}

BOOL CDecode::CollectFftOutput(FLOAT freqStart, FLOAT freqEnd, FLOAT* outBuffer, ULONG* outBufferElements)
{
	LONG bufferLength = *outBufferElements;
	*outBufferElements=-1;
	
	// check we are not out of range	
	//if(freqStart<0 || freqEnd<0 || freqStart>m_bufferConfig.FreqRangeFft() || freqEnd>m_bufferConfig.FreqRangeFft())
	///{
	//	m_lastErrorText = "Frequency out of range";
	//	return FALSE;
	//}

	int binStart = m_bufferConfig.FreqToBin(freqStart);
	int binEnd = m_bufferConfig.FreqToBin(freqEnd);
	int numBins = binEnd-binStart;
	int maxBin = m_bufferConfig.NumBinsFft();

	// check some but not too much bandwidth has been requested
	if(numBins<0 || numBins>maxBin)
	{
		m_lastErrorText = "Frequency range reversed or too wide";
		return FALSE;
	}

	// copy data into buffer
	int samplesPerLine = numBins/bufferLength;

	if(samplesPerLine<1)
	{
		samplesPerLine=1;
	}

	m_decodeManager->GetFftBuffer(m_fftBuffer, maxBin);

	int binPos=0;
	int bufPos=0;
	float value=0.0;
	*outBufferElements = 0;
	while(binPos<maxBin && bufPos<bufferLength)
	{
		int count=0;
		outBuffer[bufPos]=0.0;
		while(count++<samplesPerLine && binPos<maxBin)
		{    
			value = 20*log(sqrt(m_fftBuffer[binPos].fRe*m_fftBuffer[binPos].fRe) + (m_fftBuffer[binPos].fIm*m_fftBuffer[binPos].fIm));
			if(value>outBuffer[bufPos])
			{                
				outBuffer[bufPos] = value;                
			}
			binPos++;
		}

		(*outBufferElements)++;
		bufPos++;
	}

	return S_OK;
}

void CDecode::OnDecodeSuccess(byte* buffer, INT bufferSize, FLOAT frequency, INT errorCount)
{
	if(BLOCK_SIZE==bufferSize) {
		memcpy(&m_lastData[0], buffer, BLOCK_SIZE);
		m_lastFrequency = frequency;
    	m_lastErrorCount = errorCount;    
	}

	if(NULL != m_decodeSuccessCallback)
	{
		LogMessage(4, "data callback");
		m_decodeSuccessCallback(buffer, bufferSize, frequency, errorCount);
	}
	if(NULL != m_decodeReadyCallback)
	{
		LogMessage(4, "data rady callback");
		m_decodeReadyCallback();
	}	
}

BOOL CDecode::SetOnDataCallback(OnDataCallback callbackProc)
{
	m_decodeSuccessCallback = callbackProc;
	return TRUE;
}

BOOL CDecode::RemoveDCOffset(BOOL enable)
{
	if(NULL == m_decodeManager)
	{
		return FALSE;
	}

	m_decodeManager->RemoveDCOffset(enable);

	return TRUE;

}

BOOL CDecode::SetTrackingParams(int mode)
{
	if(NULL == m_decodeManager)
	{
		return FALSE;
	}

	m_decodeManager->SetAutoTuneState((FFTTunerState)(mode&MASK_FFT_TUNER));
	m_decodeManager->SetFftRangeLimit((FFTRangeLimit)(mode&MASK_FFT_RANGE_LIMIT));

	return TRUE;
}

BOOL CDecode::SetWorkerCount(ULONG num) {
	if (NULL == m_decodeManager)
	{
		return FALSE;
	}
	if (num > 32) 
	{
		return FALSE;
	}

	// set the number of workers that should be running/decoding
	m_decodeManager->SetWorkerCount(num);
	return TRUE;
}

void CDecode::SetPeakDetectParams(const UINT maxCalc, const DOUBLE delaySecs) 
{ 
	if (NULL == m_decodeManager) {
		return;
	}
	m_decodeManager->SetPeakDetectParams(maxCalc, delaySecs);
}

BOOL CDecode::GetWorkerPeaks(FLOAT* frequency, ULONG* bufferLength)
{	
	if (NULL == m_decodeManager)
	{
		return FALSE;
	}
	ULONG peakCount = min((ULONG)(MAX_DECODERS - 1), *bufferLength);
	*bufferLength = 0;

	std::vector<FLOAT> peakPos;
	if (m_decodeManager->GetWorkerFreqs(peakPos, peakCount))
	{
		for (UINT idx = 0; idx < peakCount && idx < peakPos.size(); idx++) 
		{
			frequency[idx] = peakPos[idx];
		}
	}
	*bufferLength = peakPos.size();
	return *bufferLength > 0;
}


BOOL CDecode::GetWorkerAvailability(UINT* availability, ULONG* bufferLength)
{
	if (NULL == m_decodeManager)
	{
		return FALSE;
	}
	ULONG count = min((ULONG)(MAX_DECODERS - 1), *bufferLength);
	*bufferLength = 0;

	std::vector<UINT> avail;
	if (m_decodeManager->GetWorkerAvail(avail, count))
	{
		for (UINT idx = 0; idx < count && idx < avail.size(); idx++)
		{
			availability[idx] = avail[idx];
		}
	}
	*bufferLength = avail.size();
	return *bufferLength > 0;
}

BOOL CDecode::SetOnDataReadyCallback(OnDataReadyCallback callbackProc)
{
	m_decodeReadyCallback = callbackProc;
	return TRUE;
}

BOOL CDecode::CollectLastData(BYTE* buffer, ULONG* bufferSize, FLOAT* freq, INT* errs) {
	if(*bufferSize<BLOCK_SIZE) {
		*bufferSize = BLOCK_SIZE;
		return FALSE;
	}
	memcpy(buffer, &m_lastData[0], BLOCK_SIZE);
	*bufferSize=BLOCK_SIZE;
	*freq = m_lastFrequency;
	*errs = m_lastErrorCount;
	return TRUE;
}