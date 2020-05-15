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

/// @file StopWatch.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#if defined(_WIN32) || defined(_WIN64)
#include <WTypes.h>
#endif
#ifdef LINUX
#include "wintypes.h"
#include <time.h>
#endif

class CStopWatch
{
public:
	CStopWatch(void);
	~CStopWatch(void);

	void Start();
	void Stop();
	void Restart();
	
	void Clear();
	BOOL IsStarted() { return m_started; }

	double GetElapsedSeconds(BOOL reset = FALSE);

private:
#if defined(_WIN32) || defined(_WIN64)
	LARGE_INTEGER m_start;
	LARGE_INTEGER m_stop;
	LARGE_INTEGER m_frequency;
#endif
#ifdef LINUX
	struct timespec m_start;
	struct timespec m_stop;
	struct timespec m_frequency;
#endif
	BOOL m_started;
	BOOL m_everStarted;

	void CaptureElapsed();
};

