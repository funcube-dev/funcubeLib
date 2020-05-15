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

/// @file GuardInterlock.h
//
//////////////////////////////////////////////////////////////////////

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include "windows.h"
#endif
#ifdef LINUX
#include "wintypes.h"
#endif

// Guarded lock
template <class T>
class CGuardInterlock {
public:
	CGuardInterlock(volatile T *pLock) {
		m_pLock = pLock;
		m_prev = (T)InterlockedExchangeAdd(m_pLock, 1);
	}
	~CGuardInterlock() {
		InterlockedExchangeSubtract(m_pLock, 1);
	}
	T Previous() { return m_prev; }
private:
	volatile T *m_pLock;
	T m_prev;
};
