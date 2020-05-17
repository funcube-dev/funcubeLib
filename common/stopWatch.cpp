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

/// @file StopWatch.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stopWatch.h"

CStopWatch::CStopWatch(void)
{
#if defined(_WIN32) || defined(_WIN64)
    QueryPerformanceFrequency( &m_frequency ) ;
#endif
#ifdef LINUX
    clock_getres( CLOCK_MONOTONIC, &m_frequency );
#endif
    Clear();
}


CStopWatch::~CStopWatch(void)
{
}

void CStopWatch::Start() 
{
    m_started = TRUE;
    m_everStarted = TRUE;
#if defined(_WIN32) || defined(_WIN64)
    QueryPerformanceCounter(&m_start);
#endif
#ifdef LINUX
    clock_gettime( CLOCK_MONOTONIC, &m_start );
#endif
}

void CStopWatch::Stop() 
{
	m_started = FALSE;
	CaptureElapsed();
}

void CStopWatch::Restart()
{
    Start();
}

void CStopWatch::Clear() {
    m_started = FALSE;
    m_everStarted = FALSE;
#if defined(_WIN32) || defined(_WIN64)
    m_start.QuadPart = 0;
    m_stop.QuadPart = 0;    
#endif
#ifdef LINUX
    m_start = { 0 };
    m_stop = { 0 };    
#endif
}

void CStopWatch::CaptureElapsed() 
{
#if defined(_WIN32) || defined(_WIN64)
	QueryPerformanceCounter(&m_stop);
#endif
#ifdef LINUX
	clock_gettime(CLOCK_MONOTONIC, &m_stop);
#endif
}

double CStopWatch::GetElapsedSeconds(BOOL restart)
{
    if (!m_everStarted) {
        // indicate we've never been started
        return -1.0;
    }

	if (m_started) {
		CaptureElapsed();
	}

#if defined(_WIN32) || defined(_WIN64)
    DOUBLE diff = m_stop.QuadPart - m_start.QuadPart;
    diff /= (double)m_frequency.QuadPart;
#endif
#ifdef LINUX
    struct timespec ts;
    if (m_stop.tv_nsec<m_start.tv_nsec)
    {
        ts.tv_sec = m_stop.tv_sec-m_start.tv_sec-1;
        ts.tv_nsec = 1000000000+m_stop.tv_nsec-m_start.tv_nsec;
    }
    else
    {
        ts.tv_sec = m_stop.tv_sec-m_start.tv_sec;
        ts.tv_nsec = m_stop.tv_nsec-m_start.tv_nsec;
    }
    DOUBLE diff = (DOUBLE)ts.tv_sec + ((DOUBLE)ts.tv_nsec)/1000000000.0;
#endif
    if(restart)
    {
        Restart();
    }
    return (diff); 
}
