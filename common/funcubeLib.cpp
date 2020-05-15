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

/// @file FuncubeLib.cpp
//
//////////////////////////////////////////////////////////////////////
#include "FuncubeLib.h"
#include "Dongle.h"
#include "DecodeManager.h"
#include "Decode.h"
#include "Encode.h"
#include "VersionNo.h"

using namespace fc;

static CDongle* m_pDongle = NULL;

STDMETHODIMP_(UINT) Library_GetVersion()
{
    UINT result = SVN_REV;
    return result;
}

STDMETHODIMP_(BOOL) Library_CheckFftwExists()
{
    BOOL success = TRUE;
#if defined(_WIN32) || defined(_WIN64)
    // C++ exception handling will not work with fftw! so using
    __try
    {
        // fftw_version doesnt seem to be exported from the
        // pre-built binary so just test these work
        BYTE* mem = (BYTE*)fftwf_malloc(32);
        fftwf_free(mem);
    }
    __except(true)
    {
        success = FALSE;
    }
#endif
    return success;
}

static OnLogMessageCallback g_logMessageCallback = NULL;
STDMETHODIMP_(BOOL) Library_SetOnLogMessageCallback(OnLogMessageCallback callbackProc)
{
    g_logMessageCallback = callbackProc;
    return TRUE;
}

void LogMessage(int level, const std::string& message)
{
    if(NULL != g_logMessageCallback)
    {
        g_logMessageCallback(level, message.c_str());
    }
}

STDMETHODIMP_(BOOL) Dongle_Initialize() 
{ 
    if(NULL == m_pDongle)
    {
        m_pDongle = new CDongle();        
    } 
    return TRUE; 
}

STDMETHODIMP_(BOOL) Dongle_Shutdown() 
{ 
    if(NULL != m_pDongle)
    {
        delete m_pDongle;
        m_pDongle = NULL;
    }
    return TRUE; 
}
STDMETHODIMP_(BOOL) Dongle_Exists() 
{ 
    if(NULL == m_pDongle)
    {
        return FALSE;
    }

    return m_pDongle->Exists();
}


static ULONG s_frequency; // shared betweeen get/set not thread safe

STDMETHODIMP_(ULONG) Dongle_GetFrequency() 
{
    static DWORD tickCount=0;    
    if(NULL == m_pDongle)
    {
        return FALSE;
    }
    
    if(GetTickCount()>tickCount)
    {
        // only get the tick count every 5 seconds
        s_frequency = m_pDongle->GetFrequency();
        tickCount = GetTickCount()+5000;
    }
     
    return s_frequency;
}

STDMETHODIMP_(BOOL) Dongle_SetFrequency(ULONG frequency) 
{
    if(NULL == m_pDongle)
    {
        return FALSE;
    }
    s_frequency = frequency;
    return m_pDongle->SetFrequency(frequency);
}

STDMETHODIMP_(BOOL) Dongle_BiasTEnable(BOOL enable)
{
    if(NULL == m_pDongle)
    {
        return FALSE;
    }
    
    return m_pDongle->BiasTEnable(enable);
}


static CDecode* m_pDecode = NULL;

STDMETHODIMP_(BOOL) Decode_Initialize() 
{ 
    if(NULL == m_pDecode)
    {
        m_pDecode = new CDecode();
        if(!m_pDecode->Initialise())
        {
            delete m_pDecode;
            m_pDecode = NULL;
        }
    }
    return NULL != m_pDecode; 
}

STDMETHODIMP_(BOOL) Decode_Shutdown() 
{ 
    if(NULL != m_pDecode)
    {
        m_pDecode->Shutdown();
        delete m_pDecode;
        m_pDecode = NULL;
    }

    return TRUE; 
}

STDMETHODIMP_(BOOL) Decode_EnumDevices(DeviceEnumCallback callbackProc) 
{
	if(NULL == m_pDecode)
	{
		return FALSE;
	}

	return m_pDecode->EnumDevices(callbackProc);
}

STDMETHODIMP_(BOOL) Decode_Start(const char* inDeviceName, const char* outDeviceName, BOOL stereoIn, BOOL isFCDongle)
{ 
    if(NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->Start(inDeviceName, outDeviceName, stereoIn, isFCDongle);
}

STDMETHODIMP_(BOOL) Decode_StartByIndex(int inDeviceIdx, int outDeviceIdx, BOOL stereoIn, BOOL isFCDongle) 
{ 
    if(NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->Start(inDeviceIdx, outDeviceIdx, stereoIn, isFCDongle);
}

STDMETHODIMP_(BOOL) Decode_Stop() 
{ 
    if(NULL == m_pDecode)
    {
        return FALSE;
    }
    return m_pDecode->Stop();    
}

STDMETHODIMP_(VOID) Decode_SetManualTuneFrequency(FLOAT newVal)
{
    if(NULL != m_pDecode)
    {
        m_pDecode->SetManualTuneFrequency(newVal);
    }
}

STDMETHODIMP_(VOID) Decode_SetAutoTuneFrequencyRange(FLOAT low,  FLOAT high)
{
    if(NULL != m_pDecode)
    {
        m_pDecode->SetAutoTuneFrequencyRange(low, high);
    }
}

STDMETHODIMP_(BOOL) Decode_CollectFftOutputByRange(FLOAT freqStart, FLOAT freqEnd, FLOAT* buffer, ULONG* bufferSize)
{
    if(NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->CollectFftOutput(freqStart, freqEnd, buffer, bufferSize);
}


STDMETHODIMP_(BOOL) Decode_CollectFftOutput(FLOAT* buffer, ULONG* bufferSize)
{
    if(NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->CollectFftOutput(buffer, bufferSize);
}

STDMETHODIMP_(BOOL) Decode_SetOnDataCallback(OnDataCallback callbackProc)
{
    if(NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->SetOnDataCallback(callbackProc);
}

STDMETHODIMP_(VOID) Decode_EnableMonitorAudio(BOOL enable)
{
    if(NULL == m_pDecode)
    {
        return;
    }

    m_pDecode->SetAudioOut(enable);
}

STDMETHODIMP_(VOID) Decode_RemoveDCOffset(BOOL remove)
{
    if(NULL == m_pDecode)
    {
        return;
    }

    m_pDecode->RemoveDCOffset(remove);
}

STDMETHODIMP_(BOOL) Decode_RefreshAudioDevices()
{
    if(NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->ReinitialisePortAudio();
}

STDMETHODIMP_(BOOL) Decode_IsStarted()
{
    if(NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->IsStarted();
}

STDMETHODIMP_(BOOL) Decode_SetTrackingParams(int mode)
{
    if(NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->SetTrackingParams(mode);
}

STDMETHODIMP_(VOID) Decode_SetPeakDetectParams(const UINT maxCalc, const DOUBLE delaySecs)
{
    if (NULL == m_pDecode)
    {
        return;
    }

    m_pDecode->SetPeakDetectParams(maxCalc, delaySecs);
}

STDMETHODIMP_(BOOL) Decode_SetWorkerCount(ULONG num)
{
    if (NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->SetWorkerCount(num);
}

STDMETHODIMP_(BOOL) Decode_GetWorkerPeaks(FLOAT* frequency, ULONG* bufferLength)
{
	if (NULL == m_pDecode)
	{
		return FALSE;
	}

	return m_pDecode->GetWorkerPeaks(frequency, bufferLength);
}

STDMETHODIMP_(BOOL) Decode_GetWorkerAvailability(UINT* availability, ULONG* bufferLength)
{
    if (NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->GetWorkerAvailability(availability, bufferLength);
}

STDMETHODIMP_(BOOL) Decode_ExcludePeaks(FLOAT* frequency, ULONG* bufferLength)
{
    if (NULL == m_pDecode)
    {
        return FALSE;
    }

    return FALSE;
}

STDMETHODIMP_(const char *) Decode_LastError()
{
       if(NULL == m_pDecode)
       {
               return NULL;
       }

       return m_pDecode->LastError();
}

STDMETHODIMP_(BOOL) Decode_SetOnDataReadyCallback(OnDataReadyCallback callbackProc)
{
    if(NULL == m_pDecode)
    {
        return FALSE;
    }

    return m_pDecode->SetOnDataReadyCallback(callbackProc);
}

STDMETHODIMP_(BOOL) Decode_CollectLastData(BYTE* buffer, ULONG* bufferSize, FLOAT* freq, INT* errs)
{
	if (NULL == m_pDecode)
	{
		return FALSE;
	}

	return m_pDecode->CollectLastData(buffer, bufferSize, freq, errs);
}



static CEncode* m_pEncode = NULL;

STDMETHODIMP_(BOOL) Encode_Initialize()
{
	if (NULL == m_pEncode)
	{
		m_pEncode = new CEncode();
		if (!m_pEncode->Initialise())
		{
			delete m_pEncode;
			m_pEncode = NULL;
		}
	}
	return NULL != m_pEncode;
}

STDMETHODIMP_(BOOL) Encode_Shutdown()
{
	if (NULL != m_pEncode)
	{
		m_pEncode->Shutdown();
		delete m_pEncode;
		m_pEncode = NULL;
	}

	return TRUE;
}

STDMETHODIMP_(BOOL) Encode_CanCollect()
{
	if (NULL == m_pEncode)
	{
		return FALSE;
	}

	return m_pEncode->CanCollect();
}

STDMETHODIMP_(BOOL) Encode_AllDataCollected()
{
	if (NULL == m_pEncode)
	{
		return FALSE;
	}

	return m_pEncode->AllDataCollected();
}

STDMETHODIMP_(BOOL) Encode_CollectSamples(BYTE* buffer, ULONG* bufferSize)
{
	if (NULL == m_pEncode)
	{
		return FALSE;
	}

	return m_pEncode->CollectSamples(buffer, bufferSize);
}

STDMETHODIMP_(BOOL) Encode_PushData(const BYTE* buffer, const ULONG bufferSize)
{
	if (NULL == m_pEncode)
	{
		return FALSE;
	}

	return m_pEncode->PushData(buffer, bufferSize);
}