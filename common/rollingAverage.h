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

/// @file RollingAverage.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "funcubeLib.h"

class CRollingAverage
{
public:
    CRollingAverage(void);
    CRollingAverage(const FLOAT initialValue);
    ~CRollingAverage(void);

    FLOAT GetAverageFactor();
    void SetAverageFactor(const FLOAT value);

    FLOAT GetAverageSamples();
    void SetAverageSamples(const FLOAT value);
	
	inline FLOAT Average(const FLOAT value) { m_average = (m_averageFactor * value) + (m_inverseAverageFactor * m_average); return m_average; }
	inline void ExternalAverage(FLOAT &average, const FLOAT value) { average = (m_averageFactor * value) + (m_inverseAverageFactor * average); }
    FLOAT GetAverage() { return m_average; }	
	void SetAverage(const FLOAT value);
    
private:
    FLOAT m_average;
    FLOAT m_averageFactor;
    FLOAT m_inverseAverageFactor;
};