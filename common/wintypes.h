//    Copyright 2014 (c) AMSAT-UK
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

/// @file wintypes.h
//
//////////////////////////////////////////////////////////////////////

// Windows-specific type names for Posix platforms
#pragma once

// Get ISO C99 standard integer types, define Windows specific names
#include <stdint.h>
#include <wchar.h>

// cannot use typedef as these have additional type words prepended where used.. *sigh*
#define __int8 char
#define __int16 short
#define __int32 int

// ensure we have memory and string bits
#include <string.h>

// the usual suspects..
typedef void VOID;
typedef int BOOL;
typedef void * HANDLE;
typedef HANDLE * PHANDLE;
typedef char CHAR;
typedef CHAR * PCHAR;
typedef uint8_t BYTE;
typedef uint8_t byte;
typedef BYTE * PBYTE;
typedef int16_t SHORT;
typedef uint16_t USHORT;
typedef int INT;
typedef unsigned int UINT;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef uint32_t DWORD;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;
typedef float FLOAT;
typedef double DOUBLE;

typedef const CHAR * LPCSTR;


#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif

// MSVC 'interface' keyword hacks
#ifdef __cplusplus
#define interface struct
#endif

// DLL interface ugliness
#define __stdcall
#ifdef __cplusplus
#define STDAPI_(type) extern "C" type
#else
#define STDAPI_(type) extern type
#endif
#define STDMETHODIMP_(type) type

// Exception handling macros..
#define _THROW0	throw

// Interlocked memory-barrier voodoo
// @see: http://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html#Atomic-Builtins
#define InterlockedExchangeAdd(pvar, add) __sync_fetch_and_add(pvar, add)
#define InterlockedExchangeSubtract(pvar, sub) __sync_fetch_and_sub(pvar, sub)

#ifdef __cplusplus
// Use POSIX threads to emulate ATL/MFC/Win32 functions.. fun eh?
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

// interruptible thread sleep..
#define SleepEx(t, i) { struct timespec _t = {0,(t)*1000000L}; nanosleep(&_t, NULL); }
#define Sleep(t) SleepEx(t, 0)

// real-time clock for delay calculations
#define GetTickCount()	timeGetTime()
DWORD inline timeGetTime() {
	struct timespec _t; clock_gettime(CLOCK_MONOTONIC, &_t); return (DWORD)(_t.tv_sec*1000L + _t.tv_nsec/1000000L);
}

// CWorkerThread and friends
typedef LONG HRESULT;
typedef DWORD * DWORD_PTR;
typedef void Win32ThreadTraits;
#define S_FALSE FALSE
#define S_OK TRUE
#define E_FAIL FALSE
#define _In_
#define _Out_
#define _Inout_

class IWorkerThreadClient
{
public:
	virtual HRESULT Execute(DWORD_PTR dwParam, HANDLE hObject) = 0;
	virtual HRESULT CloseHandle(_In_ HANDLE hHandle) = 0;
};


class CEvent
{
public:
	void Create(void*, BOOL, BOOL, void*) {
		sem_init(&m_sem, 0, 0);
	};
	void Reset() {
	};
	void Set() {
		sem_post(&m_sem);
	};
	void Wait() {
		sem_wait(&m_sem);
	};

	operator HANDLE() { return (HANDLE)this; };
private:
	sem_t m_sem;
};

template <class T>
class CWorkerThread
{
public:
	CWorkerThread() {
		m_tid = 0;
		m_pEvent = NULL;
		m_dwTimeout = 0L;
		m_pClient = NULL;
		m_pData = NULL;
	};
	~CWorkerThread() {
		if (m_tid!=0)
			pthread_cancel(m_tid);
	};
	HRESULT Initialize() {
		return S_OK;
	};
	HRESULT AddHandle(HANDLE hnd, IWorkerThreadClient* wrk, void* arg) {
		if (m_tid==0) {
			m_pData = arg;
			m_pClient = wrk;
			m_pEvent = (CEvent *)hnd;
			pthread_create(&m_tid, NULL, CWorkerThread::_thread, this);
			return S_OK;
		}
		return E_FAIL;
	};
	HRESULT AddTimer(DWORD dwInt, IWorkerThreadClient *wrk, void *arg, HANDLE *pHnd) {
		if (m_tid==0) {
			m_pData = arg;
			m_pClient = wrk;
			m_dwTimeout = dwInt;
			*pHnd = (HANDLE)this;
			pthread_create(&m_tid, NULL, CWorkerThread::_thread, this);
			return S_OK;
		}
		return E_FAIL;
	};
	HRESULT RemoveHandle(HANDLE hnd) {
		if (m_tid!=0 && hnd==(HANDLE)m_pEvent) {
			CEvent *t = m_pEvent;
			m_pEvent = NULL;
			t->Set();
			pthread_join(m_tid, NULL);
			m_tid = 0;
		} else if(m_tid!=0 && hnd==(HANDLE)this) {
			m_dwTimeout = 0L;
			pthread_join(m_tid, NULL);
			m_tid = 0;
		}
		return S_OK;
	};
private:
	static void* _thread(void *arg) {
		CWorkerThread *pMe = (CWorkerThread *)arg;
		pMe->Run();
		return NULL;
	};
	void Run() {
		BOOL ok = TRUE;
		// wait until we have something to call back (or get terminated)
		while (ok) {
			ok = FALSE;
			if (m_pEvent!=NULL) {
				m_pEvent->Wait();
				if (m_pEvent!=NULL) {
					m_pClient->Execute((DWORD_PTR)m_pData, m_pEvent);
					ok = TRUE;
				}
			}
			if (m_dwTimeout!=0L) {
				struct timespec ts = { m_dwTimeout/1000, (m_dwTimeout%1000) * 1000000L };
				nanosleep(&ts, NULL);
				if (m_dwTimeout!=0L) {
					m_pClient->Execute((DWORD_PTR)m_pData, (HANDLE)this);
					ok = TRUE;
				}
			}
		}
	};
	pthread_t m_tid;
	CEvent *m_pEvent;
	DWORD m_dwTimeout;
	IWorkerThreadClient *m_pClient;
	void *m_pData;
};

#endif

// debug output
#define OutputDebugStringA(str) fputs((str), stderr);fflush(stderr);
//#define OutputDebugStringA(str) {};
