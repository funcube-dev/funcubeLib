//    Copyright 2013 (c) AMSAT-UK
//
//    This file is part of FUNcube-Lib.
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

/// @file DecodeWorker.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include <queue>
#include "funcubeLib.h"
#include "memPool.h"
#include "stopWatch.h"
#include "rollingAverage.h"
#include "rollingAverageComplex.h"
#include "idecodeResult.h"
#include "iaudioPublisher.h"
#include "firFilter.h"
#include "overlappedFft.h"
#include "peakDetect.h"

// forward Declaration
class CBpskDecoder;

class CDecodeWorker : public IWorkerThreadClient
{
public:
	CDecodeWorker(IDecodeResult* listener, CBpskDecoder* decoder, DWORD samplesPerBuffer);
    ~CDecodeWorker(void);

    BOOL ReadyForAudioInput();
    BOOL PushAudioInput(fc::AutoBufPtr ptr);
    fc::AutoBufPtr PopAudioOutput();
    // How available is this worker for retuning:
    // <100 not available
    // =>100 is available
    UINT GetAvailabilityFactor();

	FLOAT GetTuneFrequency();
    void SetTuneFrequency(FLOAT signalFreq);
    void EnableAutoTune(BOOL enable);
    void EnableAudioOut(BOOL enable);
    
private:
	const size_t MAX_INPUT_QUEUE = 512;

    // IWorkerThreadClient
    HRESULT Execute(DWORD_PTR dwParam, HANDLE hObject);
    HRESULT CloseHandle(_In_ HANDLE hHandle);
    
	void PublishAudioOutput(fc::AutoBufPtr ptr);
    fc::AutoBufPtr CollectAudioInput();
	
    IDecodeResult* m_resultListener;	
	CBpskDecoder* m_decoder;

    CEvent m_workAvailable;
    CWorkerThread<Win32ThreadTraits> m_workerThread;
    
    std::recursive_mutex m_lock;

    std::queue<fc::AutoBufPtr> m_queueAudioOut;
    std::queue<fc::AutoBufPtr> m_queueAudioIn;
	
	DWORD m_samplesPerBuffer;
};

