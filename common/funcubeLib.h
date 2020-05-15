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

/// @file FuncubeLib.h
//
//////////////////////////////////////////////////////////////////////
#pragma once

#ifndef STRICT
#define STRICT
#endif

#define WIN32_LEAN_AND_MEAN
#define NOCOMM

#if defined(_WIN32) || defined(_WIN64)
#include "targetver.h"
#include "windows.h"
#include <atlbase.h>
#include <atlsync.h>
#include <atlutil.h>
using namespace ATL;
#endif

#ifdef LINUX
#include "wintypes.h"
#endif

#include "portaudio.h"
#include "fftw3.h"
#include <mutex>

typedef signed __int8 S8;
typedef signed __int16 S16;
typedef signed __int32 S32;
typedef unsigned __int8 U8;
typedef unsigned __int16 U16;
typedef unsigned __int32 U32;

typedef float SAMPLE;

struct COMPLEXSTRUCT {
    float fRe, fIm;

	COMPLEXSTRUCT& operator >>(const COMPLEXSTRUCT& rhs) {
		fRe += rhs.fRe;
		fIm += rhs.fIm;
		return *this;
	};
	struct COMPLEXSTRUCT& operator+=(const COMPLEXSTRUCT& rhs) { fRe += rhs.fRe; fIm += rhs.fIm; return *this; }
	struct COMPLEXSTRUCT& operator+=(const float& rhs) { fRe += rhs; fIm += rhs; return *this; }
};

const FLOAT HZ_PER_BIN = 11.71875F;

const DOUBLE PI = 3.14159265358979323846;
const DOUBLE HALF_PI = 1.57079632679489661923;
const DOUBLE TWO_PI = 6.28318530717958647692;

#define BLOCK_SIZE 256
#define FEC_BLOCK_SIZE 650

#define PREAMBLE_SIZE 768 // Number of bit times for preamble - 5 seconds = 6000 bits, 5200 FEC bits + 32 sync vector bits = 5232, 6000 - 5232 = 768 preamble bits

#define SYNC_VECTOR 0x1acffc1d // Standard CCSDS sync vector
#define SYNC_VECTOR_SIZE 32 // Standard CCSDS sync vector

#ifdef __cplusplus
#include <iostream>
#include <sstream>
void LogMessage(int level, const std::string& message);
#endif

typedef void (__stdcall * OnDataCallback)(BYTE* buffer, ULONG bufferSize, FLOAT frequency, INT errorCount);
typedef void (__stdcall * OnDataReadyCallback)();
typedef void (__stdcall * OnLogMessageCallback)(int logLevel, const CHAR *logMessage);
typedef BOOL (__stdcall * DeviceEnumCallback)(LPCSTR name, LPCSTR apiName, int index, int inChannels, int outChannels);

typedef std::lock_guard<std::recursive_mutex> MutexGuard;

#define LOG_MESSAGE(_T_, _S_) {std::stringstream message;message << _S_;LogMessage(_T_, message.str());}
#define LOG_ERROR(_S_) LOG_MESSAGE(1,_S_)
#define LOG_WARNING(_S_) LOG_MESSAGE(2,_S_)
#define LOG_INFO(_S_) LOG_MESSAGE(3,_S_)
#define LOG_DEBUG(_S_) LOG_MESSAGE(4,_S_)

#define MASK_FFT_RANGE_LIMIT 0x07
#define MASK_FFT_TUNER 0x30
#define MASK_PREMIX_ENABLE 0x40

enum FFTRangeLimit {
    FFTRangeLimit_Off   = 0x00,
    FFTRangeLimit_010   = 0x01,
    FFTRangeLimit_020   = 0x02,
    FFTRangeLimit_040   = 0x03,
    FFTRangeLimit_080   = 0x04,
    FFTRangeLimit_140   = 0x05,
    FFTRangeLimit_200   = 0x06,
    FFTRangeLimit_Slope = 0x07
};

enum FFTTunerState {
    FFTTunerState_Off  = 0x00,
    FFTTunerState_On   = 0x10,
    FFTTunerState_Lock = 0x20
};

STDAPI_(UINT) Library_GetVersion();
STDAPI_(BOOL) Library_CheckFftwExists();
STDAPI_(BOOL) Library_SetOnLogMessageCallback(OnLogMessageCallback callbackProc);

STDAPI_(BOOL) Dongle_Initialize();
STDAPI_(BOOL) Dongle_Shutdown();
STDAPI_(BOOL) Dongle_Exists();
STDAPI_(ULONG) Dongle_GetFrequency();
STDAPI_(BOOL) Dongle_SetFrequency(ULONG frequency);
STDAPI_(BOOL) Dongle_BiasTEnable(BOOL enable);

STDAPI_(BOOL) Decode_Initialize();
STDAPI_(BOOL) Decode_Shutdown();
STDAPI_(BOOL) Decode_EnumDevices(DeviceEnumCallback callbackProc);
STDAPI_(BOOL) Decode_Start(const char* inDeviceName, const char* outDeviceName, BOOL stereoIn, BOOL isFCDongle);
STDAPI_(BOOL) Decode_StartByIndex(int inDeviceIdx, int outDeviceIdx, BOOL stereoIn, BOOL isFCDongle);
STDAPI_(BOOL) Decode_Stop();
STDAPI_(VOID) Decode_SetManualTuneFrequency(FLOAT newVal);
STDAPI_(VOID) Decode_SetAutoTuneFrequencyRange(FLOAT low,  FLOAT high);
STDAPI_(BOOL) Decode_CollectFftOutput(FLOAT* buffer, ULONG* bufferSize);
STDAPI_(BOOL) Decode_CollectFftOutputByRange(FLOAT freqStart, FLOAT freqEnd, FLOAT* buffer, ULONG* bufferSize);
STDAPI_(BOOL) Decode_SetOnDataCallback(OnDataCallback);
STDAPI_(VOID) Decode_EnableMonitorAudio(BOOL enable);
STDAPI_(VOID) Decode_RemoveDCOffset(BOOL remove);
STDAPI_(BOOL) Decode_RefreshAudioDevices();
STDAPI_(BOOL) Decode_IsStarted();
STDAPI_(BOOL) Decode_SetTrackingParams(int mode);
STDAPI_(VOID) Decode_SetPeakDetectParams(const UINT maxCalc, const DOUBLE delaySecs);
STDAPI_(BOOL) Decode_SetWorkerCount(ULONG num);
STDAPI_(BOOL) Decode_GetWorkerPeaks(FLOAT* frequency, ULONG* bufferLength);
STDAPI_(BOOL) Decode_GetWorkerAvailability(UINT* availability, ULONG* bufferLength);
STDAPI_(BOOL) Decode_ExcludePeaks(FLOAT* frequency, ULONG* bufferLength);
STDAPI_(BOOL) Decode_SetOnDataReadyCallback(OnDataReadyCallback);
STDAPI_(BOOL) Decode_CollectLastData(BYTE* buffer, ULONG* bufferSize, FLOAT* freq, INT* errorCount);

STDAPI_(const char *) Decode_LastError();

STDAPI_(BOOL) Encode_Initialize();
STDAPI_(BOOL) Encode_Shutdown();
STDAPI_(BOOL) Encode_CanCollect();
STDAPI_(BOOL) Encode_AllDataCollected();
STDAPI_(BOOL) Encode_CollectSamples(BYTE* buffer, ULONG* bufferSize);
STDAPI_(BOOL) Encode_PushData(const BYTE* buffer, const ULONG bufferSize);
