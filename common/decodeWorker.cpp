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
#include "funcubeLib.h"
#include "portaudio.h"
#include <math.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include "bpskDecoder.h"

#include "decodeWorker.h"

using namespace std;
using namespace fc;

CDecodeWorker::CDecodeWorker(IDecodeResult* listener, CBpskDecoder* decoder, DWORD samplesPerBuffer)
{
	m_resultListener = listener;
	m_decoder = decoder;

	m_samplesPerBuffer = samplesPerBuffer;

	m_workAvailable.Create(NULL, FALSE, FALSE, NULL);
	m_workAvailable.Reset();
	m_workerThread.Initialize();
	m_workerThread.AddHandle(m_workAvailable, this, NULL);
}

CDecodeWorker::~CDecodeWorker(void)
{
    m_workerThread.RemoveHandle(m_workAvailable);
}

FLOAT CDecodeWorker::GetTuneFrequency()
{ 
	MutexGuard guard(m_lock);
	if (m_decoder == NULL) {
		return 0.0F;
	}	
	return m_decoder->GetTuneFrequency(); 
}

void CDecodeWorker::EnableAutoTune(BOOL enable) 
{ 
	MutexGuard guard(m_lock);
	if (m_decoder == NULL) {
		return;
	}
	m_decoder->EnableAutoTune(enable);	
}

void CDecodeWorker::EnableAudioOut(BOOL enable) 
{ 
	MutexGuard guard(m_lock);
	if (m_decoder == NULL) {
		return;
	}
	m_decoder->EnableAudioOut(enable); 	
}

void CDecodeWorker::SetTuneFrequency(FLOAT signalFreq)
{
	MutexGuard guard(m_lock);
	if (m_decoder == NULL) {
		return;
	}
	char buf[256];
	sprintf(&buf[0], "S:%g", signalFreq);
	OutputDebugStringA(buf);
	m_decoder->SetTuneFrequency(signalFreq);
}

BOOL CDecodeWorker::ReadyForAudioInput()
{    
	MutexGuard guard(m_lock);
    BOOL full = m_queueAudioIn.size()>MAX_INPUT_QUEUE;	
    return !full;
}

UINT CDecodeWorker::GetAvailabilityFactor() {
	DOUBLE decodeSecs, retuneSecs;	
	// locked
	{		
		MutexGuard guard(m_lock);
		if (m_decoder == NULL) {
			return 200;
		}
		decodeSecs = m_decoder->LastDecodeElapsedSecs();
		retuneSecs = m_decoder->LastRetuneElapsedSecs();
	}

	// never been tuned, 
	if (retuneSecs < 0) {
		return 200;
	}

	// factor < 50 we have decoded
	UINT availFactor = 0;
	// scale elapsed since decode so that 5 * the frame duration is equivalent to
	// not having decoded
	static const DOUBLE scaleDecode = 50.0 / (4.3 * 5);
	availFactor += decodeSecs < 0.0 ? 50 : decodeSecs*scaleDecode;
	if (availFactor < 50) {
		// we have recently decoded so ignore retune time
		//OutputDebugStringA("R|");
		return availFactor;
	}
	// factor >=50 and <100 not decoded, but recently retuned

	// scale elapsed since retune so that 2 * the frame duration takes it over
	// the 100 available to be retuned threshold. add 50 to previously decoded
	// workers to take them over the 100.
	static const DOUBLE scaleRetune = 50.0 / (4.3 * 2);
	availFactor += decodeSecs<0.0 ? retuneSecs * scaleRetune : 50;
	// factor > 100 available for retune, non decoded workers age quicker then decoded workers

	return availFactor;
}

BOOL CDecodeWorker::PushAudioInput(AutoBufPtr ptr)
{
	MutexGuard guard(m_lock);

    if(m_queueAudioIn.size()>MAX_INPUT_QUEUE)  // discard oldest, if we have too many buffers queued
    {
		m_queueAudioIn.pop();
    }
    m_queueAudioIn.push(ptr);

	// set the work available handle, causes Execute to run
    m_workAvailable.Set();
    return TRUE;
}

AutoBufPtr CDecodeWorker::PopAudioOutput()
{
	MutexGuard guard(m_lock);

	AutoBufPtr ret;	
	if (m_queueAudioOut.size()>0)
	{
		ret = m_queueAudioOut.front();
		m_queueAudioOut.pop();
	}
	
	return ret;
}

AutoBufPtr CDecodeWorker::CollectAudioInput()
{
	MutexGuard guard(m_lock);

    AutoBufPtr ret;	
    if(m_queueAudioIn.size()>0)
    {        
        ret = m_queueAudioIn.front();
        m_queueAudioIn.pop();
    }
    return ret;
}

void CDecodeWorker::PublishAudioOutput(AutoBufPtr ptr)
{
	MutexGuard guard(m_lock);

    if(m_queueAudioOut.size()>5)  // discard oldest, if we have 5 sample buffers waiting
    {
        m_queueAudioOut.pop();
    }
    m_queueAudioOut.push(ptr);	
}

HRESULT CDecodeWorker::Execute(DWORD_PTR dwParam, HANDLE hObject)
{
	// locked
	{
		MutexGuard guard(m_lock);
		if (m_decoder == NULL)
		{
			return S_OK;
		}
	}

	IAudioPublisher* audioPublisher = NULL;
	BOOL autoTune = FALSE;
	while (1) {
		
		AutoBufPtr ptr = CollectAudioInput();
		if (NULL == ptr.get())
		{			
			break;
		}
		if (sizeof(COMPLEXSTRUCT)*m_samplesPerBuffer != ptr->Size()) {			
			return S_FALSE;
		}		

		// locked
		{
			MutexGuard guard(m_lock);
			m_decoder->ProcessAudio(
				(COMPLEXSTRUCT*)ptr->Data(),
				m_samplesPerBuffer);
		}
		
	}
    return S_OK;
}

HRESULT CDecodeWorker::CloseHandle(_In_ HANDLE hHandle)
{
    // Dont do anything the handle is looked after by the CEvent class
    //::CloseHandle(hHandle);
    return S_OK;
}