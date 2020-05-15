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
#include "MemPool.h"

// forward Declaration
class CPeakDetect;

class CPeakDetectWorker : public IWorkerThreadClient
{
public:
	CPeakDetectWorker();
    ~CPeakDetectWorker(void);

    BOOL Initialise(CPeakDetect* peakDetect, DWORD binsPerBuffer);

	BOOL ReadyForInput();    
    void PushFftInput(fc::AutoBufPtr ptr);
    void TriggerPeakCalc(UINT numCalcPeaks, std::vector<UINT>& excludeBins);
    BOOL PeakCalcUpdated(BOOL resetFlag = TRUE);
    BOOL GetPeaks(std::vector<UINT>& peakBins);
private:
	const size_t MAX_INPUT_QUEUE = 2;
    // IWorkerThreadClient
    HRESULT Execute(DWORD_PTR dwParam, HANDLE hObject);
    HRESULT CloseHandle(_In_ HANDLE hHandle);
    
    fc::AutoBufPtr CollectFftInput();

    // flag to indicate a peak calculation is needed
    BOOL m_calcNeeded;
    BOOL m_calcComplete;
    UINT m_numCalcPeaks;
	CPeakDetect* m_peakDetect;

    CEvent m_workAvailable;
    CWorkerThread<Win32ThreadTraits> m_workerThread;
    
    std::recursive_mutex m_lock;

    std::queue<fc::AutoBufPtr> m_queue;
	DWORD m_binsPerBuffer;
};

