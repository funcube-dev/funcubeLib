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

/// @file PeakDetect.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "funcubeLib.h"
#include "rollingAverage.h"
#include <vector>

class CPeakDetect
{
public:
    CPeakDetect();
    ~CPeakDetect(void);
	
    BOOL Initialise(const UINT numBins, const UINT maxPeaks, const UINT minDistance=11, const UINT sampleSmoothing=25, const UINT windowSize=189);

	BOOL SetDetectLimits(const UINT lowBin, const UINT highBin);

	void SetMaxPeakCount(const UINT maxPeaks);
	UINT GetMaxPeakCount() { return m_maxPeaks; }

	BOOL CalcPeaks(const ULONG numPeaks);
	BOOL GetPeaks(std::vector<UINT>& peakBins);
	void ExcludePeaks(std::vector<UINT>& peakBins);
	    
	BOOL Process(const COMPLEXSTRUCT* fftSamples, const UINT sampleCount);

	BOOL CopyResult(COMPLEXSTRUCT* samples, const UINT sampleCount);

private:    
    volatile BOOL m_lockAve;
	FLOAT* m_peakValues;
	UINT* m_peakBins;
	std::vector<UINT> m_excludeBins;
	FLOAT* m_binsAve;
	FLOAT* m_binsSumFuncube;
	FLOAT* m_binsPower;
	FLOAT* m_windowFuncube;

	UINT m_numPeaks;
	UINT m_maxPeaks;
	UINT m_numBins;
	UINT m_minDistance;	
	UINT m_windowSize;
	UINT m_lowLimitBin;
    UINT m_highLimitBin;
	CRollingAverage m_ave;

	void CalculateWindows(UINT size);
	void ApplyWindowFunction();
	
	BOOL TooClose(UINT bin1, UINT bin2);
	BOOL IsLonePeak(UINT bin);
	BOOL IsInRange(UINT bin);
	BOOL FindPeak(FLOAT* bins, UINT numBins);
};
