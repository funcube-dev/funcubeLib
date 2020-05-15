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
#include "SampleStopWatch.h"

CSampleStopWatch::CSampleStopWatch(void)
{
    Clear();
}

CSampleStopWatch::~CSampleStopWatch(void)
{
}

void CSampleStopWatch::Initialise(LONG sampleRate) 
{
    m_sampleRate = sampleRate;
    Clear();
}

void CSampleStopWatch::Start()
{
    m_started = TRUE;
    m_everStarted = TRUE;
    m_start = m_sampleCount;
}

void CSampleStopWatch::Stop()
{
	m_started = FALSE;
    m_stop = m_sampleCount;
}

// start just with a name that makes it 
// clear the use intention
void CSampleStopWatch::Restart()
{
    Start();
}

// put back the stop watch to the state as it
// if it had just been created
void CSampleStopWatch::Clear()
{    
    m_start = 0;
    m_stop = 0;
    m_sampleCount = 0;
    m_started = FALSE;
    m_everStarted = FALSE;
}

DOUBLE CSampleStopWatch::GetElapsedSeconds(BOOL restart)
{
    if (!m_everStarted) {
        // indicate we've never been started
        return -1.0;
    }

	if (m_started) {
        m_stop = m_sampleCount;
	}

    DOUBLE diff = m_stop - m_start;
    diff /= (DOUBLE)m_sampleRate;

    if(restart)
    {
        Restart();
    }
    return (diff); 
}
