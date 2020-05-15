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

/// @file MemPool.h
//
//////////////////////////////////////////////////////////////////////
#pragma once

#ifdef LINUX
#include "wintypes.h"
#else
#include <windows.h>
#endif
#include "AutoPtrEx.h"
#include <stack>
#include <memory>
#include <mutex>

namespace fc
{
    //forward declaration
    class CMemPool;
    class CMemBuf;

    typedef std::shared_ptr<CMemBuf> AutoBufPtr;
    static const std::shared_ptr<CMemBuf> NULL_MEMBUF_PTR;

	class CMemBuf
	{
	public:
		CMemBuf(DWORD capacity) {
			m_capacity = capacity;
			m_size = 0;
			m_pdata = new BYTE[capacity];
			memset(m_pdata, 0, m_capacity);
		}
		virtual ~CMemBuf() {
			delete[] m_pdata;
		}

		inline PBYTE Data() { return m_pdata; }		

		inline DWORD& Size() { return m_size; }
		inline DWORD& Capacity() { return m_capacity; }

	private:
		DWORD m_capacity;
		DWORD m_size;
		PBYTE m_pdata;
	};

    class CMemPool
    {
    public:
        CMemPool();
        virtual ~CMemPool();

        enum MEMTYPE { MEM_GROW, MEM_FIXED };
        /// Initialise a mempool of dwAllocBytes
        BOOL Initialise(DWORD bufferCount, DWORD bufferSize, MEMTYPE type = MEM_FIXED);

        BOOL Shutdown();

		size_t GetBufferSize() { return m_bufferSize; }
		size_t GetBufferCount() { return m_bufferCount; }
		size_t GetFreeBuffers() { m_lock.lock(); size_t ret=m_stack.size(); m_lock.unlock(); return ret; }

		AutoBufPtr GetBuffer();
    private:    
		typedef std::stack<CMemBuf*> STACK_MEMBUF;

		void PutBuffer(CMemBuf* buf);
        
        /// Add as many buffers of m_dwSize as possibile
        void AddBuffers(DWORD count, DWORD size);

        /// Remove all the buffers
        BOOL RemoveBuffers();
        /// control access to the buffers
        std::recursive_mutex m_lock;
        /// container for the allocated mem chunks
        STACK_MEMBUF m_stack;        
        /// the size in bytes of the current buffers.
        DWORD m_bufferSize;
        /// total number of buffers created
        DWORD m_bufferCount;
    };

}
