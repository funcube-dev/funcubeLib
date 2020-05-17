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
#include "rollingAverage.h"

CRollingAverage::CRollingAverage(void)
{
    CRollingAverage(0.0F);    
}

CRollingAverage::CRollingAverage(FLOAT initialValue)
{
    // about 5 significant samples
    m_averageFactor = 0.3F;
    m_inverseAverageFactor = 0.7F;

    m_average = initialValue;
}

CRollingAverage::~CRollingAverage(void)
{
}
 
FLOAT CRollingAverage::GetAverageFactor()
{
    return m_averageFactor;
}

void CRollingAverage::SetAverage(FLOAT value) 
{ 
    m_average = value;    
}

void CRollingAverage::SetAverageFactor(FLOAT value)
{
    m_averageFactor = value;
    // calculate this here so we don't need to for each value added
    m_inverseAverageFactor = 1.0F - m_averageFactor;
}

FLOAT CRollingAverage::GetAverageSamples()
{
    return (2.0F / m_averageFactor)-1.0F;
}

void CRollingAverage::SetAverageSamples(FLOAT value)
{
    m_averageFactor = 2.0F / (value + 1.0F);
    // calculate this here so we don't need to for each value added
    m_inverseAverageFactor = 1.0F - m_averageFactor;
}