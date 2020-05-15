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

/// @file DecodeManager.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include <queue>
#include "FuncubeLib.h"
#include "AudioBufferConfig.h"
#include "MemPool.h"
#include "StopWatch.h"
#include "RollingAverage.h"
#include "RollingAverageComplex.h"
#include "IDecodeResult.h"
#include "IAudioPublisher.h"
#include "OverlappedFft.h"
#include "PeakDetect.h"
#include "DecodeWorker.h"
#include "PeakDetectWorker.h"
#include "SampleStopWatch.h"
#include <vector>

#define MAX_DECODERS 32

class CDecodeManager : public IWorkerThreadClient, IDecodeResult, IAudioPublisher
{
public:
    CDecodeManager(IDecodeResult* listener);
    ~CDecodeManager(void);

    void Initialise();
	BOOL Start(UINT sampleRateIn);
	BOOL Stop();
    
    BOOL ReadyForAudioInput();
    BOOL PushAudioInput(fc::AutoBufPtr ptr, BOOL overwrite=TRUE);
    void PopAudioOutput(SAMPLE* realResult);

    fc::AutoBufPtr PopAudioOutput();

    void GetFftBuffer(COMPLEXSTRUCT* complexResultBuffer, UINT sampleCount);

    //ULONG GetCentreBin() { return m_centreBin; }
    void SetManualTuneBin(ULONG newVal) { m_manualTuneBin = newVal; }
    BOOL GetWorkerFreqs(std::vector<FLOAT>& workerFreqs, const ULONG requestedCount);
    BOOL GetWorkerAvail(std::vector<UINT>& workerAvail, const ULONG requestedCount);
	void SetWorkerCount(const ULONG workersCount) { m_decoderNum = workersCount; }
    void SetPeakDetectParams(const UINT maxCalc, const DOUBLE delaySecs) { m_peakDetectMaxCalc = maxCalc; m_peakDetectDelaySecs = delaySecs; }

    void SetAutoTuneRange(ULONG lowBin, ULONG highBin);

    virtual void OnDecodeSuccess(byte* buffer, INT bufferSize, FLOAT frequency, INT errorCount);
    virtual void PublishAudioOutput(fc::AutoBufPtr ptr);
    virtual fc::AutoBufPtr GetEmptyOutputAudioBuffer();
    virtual fc::AutoBufPtr GetEmptyInputAudioBuffer();

    void SetFftRangeLimit(FFTRangeLimit limit);
    void SetAutoTuneState(FFTTunerState tunerState);
    void RemoveDCOffset(BOOL enable);
    
private:    
    inline FLOAT BinToFreq(ULONG bin) { return  (HZ_PER_BIN*bin)-96000.0F; }
    inline UINT FreqToBin(FLOAT freq) { return  ((freq+96000.0F)/HZ_PER_BIN); }
    void SetLastDecodeTuneRangeFreq(FLOAT lowFreq, FLOAT highFreq);

    // IWorkerThreadClient
    HRESULT Execute(DWORD_PTR dwParam, HANDLE hObject);
    HRESULT CloseHandle(_In_ HANDLE hHandle);

    void NormaliseGainRemoveDCOffset(COMPLEXSTRUCT* pSamples, INT sampleCount);
    void GenerateFft(COMPLEXSTRUCT* pSamples, INT sampleCount);
    void CheckPeakDetection();

    void LogBusyTime(DOUBLE elapsedFreeTime);

    fc::AutoBufPtr CollectAudioInput();

    IDecodeResult* m_decodeResultListener;
    std::vector<std::shared_ptr<CDecodeWorker>> m_decoders;
    ULONG m_decoderNum;
    
	COMPLEXSTRUCT* m_fftResult;

	ULONG m_manualTuneBin;
    // min time to wait between peak calculations
    DOUBLE m_peakDetectDelaySecs;
    // max number of peaks to calc at a time
    UINT m_peakDetectMaxCalc;
    CSampleStopWatch m_lastPeakDetectTime;

    ULONG m_lowLimitBin;
	ULONG m_highLimitBin;

    ULONG m_lowLimitBinLastDecode;
    ULONG m_highLimitBinLastDecode;

    CEvent m_workAvailable;
    CWorkerThread<Win32ThreadTraits> m_workerThread;

	CPeakDetectWorker m_peakProcessThread;
    
    std::recursive_mutex m_bufferLock;

    fc::CMemPool m_memPoolOut;
    std::queue<fc::AutoBufPtr> m_queueAudioOut;
    DWORD m_requiredAudioCount;

    fc::CMemPool m_memPoolIn;
    std::queue<fc::AutoBufPtr> m_queueAudioIn;
    CStopWatch m_stopWatch;

	fc::CMemPool m_memPoolFft;

	CAudioBufferConfig m_bufferConfig;
	COverlappedFft m_fft;
	CPeakDetect m_peakDetect;

    CRollingAverageComplex m_aveDCOffset;
	CRollingAverageComplex m_aveGain;
	CRollingAverage m_freeTime;

    BOOL m_rangeChanged;

    FFTTunerState m_autoTuneState;
    FFTRangeLimit m_rangeLimitHz;

    BOOL m_DCOffSetRemove;
};
