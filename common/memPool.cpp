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

/// @file MemPool.cpp
//
//////////////////////////////////////////////////////////////////////
#include "memPool.h"
#include <stdlib.h>
#include <string.h>
#include <functional>

using namespace fc;

const int MIN_BUFFER_SIZE = 64;

//////////////////////////////////////////////////////////////////////
// CMemPool Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMemPool::CMemPool()
{
    m_bufferSize=0;
    m_bufferCount=0;
}

CMemPool::~CMemPool()
{
    Shutdown();
}

BOOL CMemPool::Initialise(DWORD count, DWORD size, MEMTYPE type)
{
	m_bufferSize = size;
    AddBuffers(count, size);

    return TRUE;
}

BOOL CMemPool::Shutdown()
{
	std::lock_guard<std::recursive_mutex> guard(m_lock);
    if(m_stack.size()!=m_bufferCount)
    {
		// shutdown failed outstanding buffers		
        return FALSE;
    }	
    RemoveBuffers();	

    return TRUE;
}

AutoBufPtr CMemPool::GetBuffer()
{
	CMemBuf* pbuf = NULL;
	m_lock.lock();
	if (m_stack.size() > 0) {
		pbuf = m_stack.top();
		m_stack.pop();
	}
	m_lock.unlock();
	if (NULL == pbuf) {
		return NULL_MEMBUF_PTR;
	}

	return std::shared_ptr<CMemBuf>(pbuf,
		std::bind(&CMemPool::PutBuffer, this, std::placeholders::_1));
}

void CMemPool::PutBuffer(CMemBuf* buf)
{
	std::lock_guard<std::recursive_mutex> guard(m_lock);
	if (NULL != buf) 
	{
		buf->Size() = 0;
		m_stack.push(buf);
	}	
}


void CMemPool::AddBuffers(DWORD count, DWORD size)
{
	std::lock_guard<std::recursive_mutex> guard(m_lock);
	for (DWORD i = 0; i < count; i++) {
		m_stack.push(new CMemBuf(size));		
		m_bufferCount++;
	}	
}

BOOL CMemPool::RemoveBuffers()
{       
	std::lock_guard<std::recursive_mutex> guard(m_lock);
    while(m_stack.size()>0)
    {        
		delete m_stack.top();
        m_stack.pop();
		--m_bufferCount;
    }	
        
    return m_bufferCount==0;
}
