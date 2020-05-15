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

/// @file Decode.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "FuncubeLib.h"
#include "AudioBufferConfig.h"
#include "IAudioSource.h"
#include "RollingAverage.h"
#include "DecodeManager.h"

// CDecode
class CDecode :
    public IDecodeResult,
    public IAudioSink
{
public:
    CDecode();
    virtual ~CDecode();

    BOOL Initialise();
    BOOL Shutdown();

    BOOL EnumDevices(DeviceEnumCallback callbackProc);
    BOOL Start(LPCSTR inDeviceName, LPCSTR outDeviceName, BOOL stereoIn, BOOL isFCDongle);
    BOOL Start(int inDeviceIdx, int outDeviceIdx, BOOL stereoIn, BOOL isFCDongle);
    
    BOOL IsStarted() { return NULL!= m_streamIn || NULL != m_streamOut || NULL != m_audioSource; }
    BOOL Stop();

    virtual void OnDecodeSuccess(byte* buffer, INT bufferSize, FLOAT frequency, INT errorCount);
    
    PaDeviceIndex CheckFCDDeviceIndex(PaDeviceIndex inDeviceIdx);    
    void SetManualTuneFrequency(FLOAT newVal);
    void SetAutoTuneFrequencyRange(FLOAT low, FLOAT high);
    BOOL CollectFftOutput(FLOAT* buffer, ULONG* outBufferElements);
    BOOL CollectFftOutput(FLOAT freqStart, FLOAT freqEnd, FLOAT* outBuffer, ULONG* outBufferElements);
    BOOL SetOnDataCallback(OnDataCallback callbackProc);
    void SetAudioOut(BOOL enable) { m_audioOut = enable; }
    BOOL ReinitialisePortAudio();
    BOOL RemoveDCOffset(BOOL enable);
    BOOL SetTrackingParams(int mode);
	BOOL GetWorkerPeaks(FLOAT* frequency, ULONG* bufferLength);
    BOOL GetWorkerAvailability(UINT* frequency, ULONG* bufferLength);
    BOOL SetWorkerCount(ULONG num);
    void SetPeakDetectParams(const UINT maxCalc, const DOUBLE delaySecs);

    BOOL SetOnDataReadyCallback(OnDataReadyCallback callbackProc);
    BOOL CollectLastData(BYTE* buffer, ULONG* bufferSize, FLOAT* freq, INT* errs);
    
    int AudioBufferReady(const void *inputBuffer, unsigned long framesPerBuffer);

    const char *LastError() { return m_lastErrorText; }

private:
    static int StaticCallbackIn(const void *inputBuffer,void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData);
    static int StaticCallbackOut(const void *inputBuffer,void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData);
    int CallbackIn(const void *inputBuffer,void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags);
    int CallbackOut(const void *inputBuffer,void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags);

    IAudioSource* m_audioSource;    
	CAudioBufferConfig m_bufferConfig;
    
    CDecodeManager* m_decodeManager;
    PaStream* m_streamIn;
    PaStream* m_streamOut;
    BOOL m_isComplexSignal;
    BOOL m_stereoIn;
    BOOL m_audioOut;
    float m_averageInput;    
    float m_averagePower;
    float m_averagePeakPower;

    CRollingAverage m_outputVolumeNormalise;

	COMPLEXSTRUCT* m_fftBuffer;
    int m_fftResultSize;
    FLOAT m_fftResultFreqRange;

    OnDataCallback m_decodeSuccessCallback = NULL;
    OnDataReadyCallback m_decodeReadyCallback = NULL;
    volatile ULONG m_lockWorking;

    BYTE m_lastData[BLOCK_SIZE];
    INT m_lastErrorCount;
    FLOAT m_lastFrequency;

    const char *m_lastErrorText;
};

