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

/// @file EncodeWorker.cpp
//
//////////////////////////////////////////////////////////////////////
#include "funcubeLib.h"
#include <math.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include "encodeWorker.h"

using namespace std;
using namespace fc;

CEncodeWorker::CEncodeWorker()
{	
	m_shuttingDown = FALSE;
	m_encoder = NULL;

	m_workAvailable.Create(NULL, FALSE, FALSE, NULL);
	m_workAvailable.Reset();
	m_workerThread.Initialize();
	m_workerThread.AddHandle(m_workAvailable, this, NULL);
}

CEncodeWorker::~CEncodeWorker(void)
{
	m_shuttingDown = TRUE;
	m_workerThread.RemoveHandle(m_workAvailable);    
}

BOOL CEncodeWorker::Initialise(IEncoder* encoder)
{   
	m_encoder = encoder;
    return TRUE;
}

BOOL CEncodeWorker::PushInput(AutoBufPtr ptr)
{
	MutexGuard guard(m_queueLock);

    if(m_queueIn.size()>MAX_INPUT_QUEUE)  // inform caller we couldn't queue the buffer
    {
		return FALSE;
    }
    m_queueIn.push(ptr);	

	// set the work available handle, causes Execute to run
    m_workAvailable.Set();
	Execute(NULL, NULL);
	return TRUE;
}

AutoBufPtr CEncodeWorker::PopOutput()
{
	MutexGuard guard(m_queueLock);

	AutoBufPtr ret;	
	if (m_queueOut.size()>0)
	{
		ret = m_queueOut.front();
		m_queueOut.pop();
	}
	// set the work available handle, causes Execute to run (make sure the output queue is kept topped up)
	m_workAvailable.Set();
	return ret;
}

BOOL CEncodeWorker::CanCollectInput() {
	MutexGuard guard(m_queueLock);	
	return m_queueIn.size() > 0;
}

AutoBufPtr CEncodeWorker::CollectInput()
{
	MutexGuard guard(m_queueLock);

    AutoBufPtr ret;	
    if(m_queueIn.size()>0)
    {        
        ret = m_queueIn.front();
        m_queueIn.pop();
    }
    return ret;
}

BOOL CEncodeWorker::CanPublishOutput() {	
	MutexGuard guard(m_queueLock);	
	return m_queueOut.size() < MAX_OUTPUT_QUEUE;
}

BOOL CEncodeWorker::PublishOutput(AutoBufPtr ptr)
{
	MutexGuard guard(m_queueLock);
    if(m_queueOut.size()>MAX_OUTPUT_QUEUE)  // inform caller if we already have enough output buffers queued
    {	
		return FALSE;       
    }
    m_queueOut.push(ptr);
	return TRUE;
}

HRESULT CEncodeWorker::Execute(DWORD_PTR dwParam, HANDLE hObject)
{
	MutexGuard guard(m_queueLock);

	// there's a new input block and we are ready to process it
	if (CanCollectInput() && m_encoder->ReadyForInput()) {
		AutoBufPtr inputBuf = CollectInput();
		
		// set a new input buffer (do work outside the lock)
		m_queueLock.unlock();
		m_encoder->SetInputBuffer(inputBuf);
		m_queueLock.lock();
	}

	// we have space in the output queue and we've not finnished the current input block
	// or we have a nearly empty output queue
	while (!m_shuttingDown && CanPublishOutput() && !m_encoder->ReadyForInput()) {
		// grab a new input buffer (outside the lock)
		m_queueLock.unlock();
		AutoBufPtr outputBuf = m_encoder->GenerateNextBuffer();
		m_queueLock.lock();

		PublishOutput(outputBuf);
	}	
	
	return S_OK;
}

HRESULT CEncodeWorker::CloseHandle(_In_ HANDLE hHandle)
{
    // Dont do anything the handle is looked after by the CEvent class
    //::CloseHandle(hHandle);
    return S_OK;
}