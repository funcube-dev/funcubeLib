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

/// @file PeakDetectWorker.cpp
//
//////////////////////////////////////////////////////////////////////
#include "FuncubeLib.h"
#include "portaudio.h"
#include <math.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include "StopWatch.h"
#include "PeakDetect.h"

#include "PeakDetectWorker.h"

using namespace std;
using namespace fc;

CPeakDetectWorker::CPeakDetectWorker()
{
	m_peakDetect = NULL;
	m_calcNeeded = FALSE;

	m_workAvailable.Create(NULL, FALSE, FALSE, NULL);
	m_workAvailable.Reset();

	m_workerThread.Initialize();
	m_workerThread.AddHandle(m_workAvailable, this, NULL);
}

CPeakDetectWorker::~CPeakDetectWorker(void)
{
    m_workerThread.RemoveHandle(m_workAvailable);
}

BOOL CPeakDetectWorker::Initialise(CPeakDetect* peakDetect, DWORD binsPerBuffer)
{
	m_peakDetect = peakDetect;
	m_binsPerBuffer = binsPerBuffer;

    return m_peakDetect!=NULL;
}

BOOL CPeakDetectWorker::ReadyForInput()
{    
	MutexGuard guard(m_lock);
    BOOL full = m_queue.size()>MAX_INPUT_QUEUE;
    return !full;
}

BOOL CPeakDetectWorker::GetPeaks(std::vector<UINT>& peakBins) 
{
	MutexGuard guard(m_lock);
	if(m_peakDetect == NULL) {
		return FALSE;
	}

	m_peakDetect->GetPeaks(peakBins);

	return TRUE;
}

void CPeakDetectWorker::PushFftInput(AutoBufPtr ptr)
{
	MutexGuard guard(m_lock);
    if(m_queue.size()>MAX_INPUT_QUEUE)  // discard oldest, if we have too many buffers queued
    {
		m_queue.pop();
    }
    m_queue.push(ptr);

	// set the work available handle, causes Execute to run
    m_workAvailable.Set();	
}

void CPeakDetectWorker::TriggerPeakCalc(UINT numCalcPeaks, std::vector<UINT> &excludeBins)
{
	MutexGuard guard(m_lock);
	if(m_peakDetect == NULL) {
		return;
	}

	m_peakDetect->ExcludePeaks(excludeBins);
	m_numCalcPeaks = numCalcPeaks;
	// indicate we want a peak calculation done
	m_calcNeeded = TRUE;

	// set the work available handle, causes Execute to run
	m_workAvailable.Set();	
}

BOOL CPeakDetectWorker::PeakCalcUpdated(BOOL resetFlag) 
{
	MutexGuard guard(m_lock);
	BOOL ret = m_calcComplete;
	if (resetFlag) {
		m_calcComplete = FALSE;
	}
	return ret;
}

AutoBufPtr CPeakDetectWorker::CollectFftInput()
{
	MutexGuard guard(m_lock);
    AutoBufPtr ret;	
    if(m_queue.size()>0)
    {        
        ret = m_queue.front();
        m_queue.pop();
    }
    return ret;
}

HRESULT CPeakDetectWorker::Execute(DWORD_PTR dwParam, HANDLE hObject)
{
	// locked
	{
		MutexGuard guard(m_lock);
		if (m_peakDetect == NULL)
		{
			return S_OK;
		}
	}

	UINT numCalcPeaks=0;
	// loop till queue is empty then break
	while (1) {
		AutoBufPtr ptr = CollectFftInput();
		if (NULL == ptr.get())
		{
			// check/reset and grab the value before we leave the lock
			if (m_calcNeeded) 
			{
				m_calcNeeded = FALSE;
				numCalcPeaks = m_numCalcPeaks;
			}
			// break out of the loop when nothing to process
			break;
		}
		if (sizeof(COMPLEXSTRUCT)*m_binsPerBuffer != ptr->Capacity()) {
			return S_FALSE;
		}		

		// locked
		{
			MutexGuard guard(m_lock);
			m_peakDetect->Process(
				(COMPLEXSTRUCT*)ptr->Data(),
				m_binsPerBuffer);
		}
	}

	// will only have a value if a calculation is needed
	if (numCalcPeaks)
	{
		MutexGuard guard(m_lock);
		m_peakDetect->CalcPeaks(numCalcPeaks);
		// update signal that we have calculated		
		m_calcComplete = TRUE;		
	}

    return S_OK;
}

HRESULT CPeakDetectWorker::CloseHandle(_In_ HANDLE hHandle)
{
    // Dont do anything the handle is looked after by the CEvent class
    //::CloseHandle(hHandle);
    return S_OK;
}