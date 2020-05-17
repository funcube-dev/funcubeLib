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

/// @file EncodeWorker.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include <queue>
#include "funcubeLib.h"
#include "memPool.h"
#include "iencoder.h"

// forward Declaration
class IEncoder;

class CEncodeWorker : public IWorkerThreadClient
{
public:
	CEncodeWorker();
    ~CEncodeWorker(void);

    BOOL Initialise(IEncoder* encoder);
	BOOL Shutdown() { return TRUE; };
        
    BOOL PushInput(fc::AutoBufPtr ptr);
    fc::AutoBufPtr PopOutput();
	ULONG InputQueueLength() { MutexGuard guard(m_queueLock); return m_queueIn.size(); };
	ULONG OutputQueueLength() { MutexGuard guard(m_queueLock); return m_queueOut.size(); };

private:
	const size_t MAX_OUTPUT_QUEUE = 64;
	const size_t MAX_INPUT_QUEUE = 128;

    // IWorkerThreadClient
    HRESULT Execute(DWORD_PTR dwParam, HANDLE hObject);
    HRESULT CloseHandle(_In_ HANDLE hHandle);
    
	BOOL CanPublishOutput();
	BOOL PublishOutput(fc::AutoBufPtr ptr);
	BOOL CanCollectInput();
    fc::AutoBufPtr CollectInput();

	IEncoder* m_encoder;

    CEvent m_workAvailable;
    CWorkerThread<Win32ThreadTraits> m_workerThread;
    
    std::recursive_mutex m_queueLock;

    std::queue<fc::AutoBufPtr> m_queueOut;
    std::queue<fc::AutoBufPtr> m_queueIn;	

	BOOL m_shuttingDown;
};

