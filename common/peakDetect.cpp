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

/// @file PeakDetect.cpp
//
//////////////////////////////////////////////////////////////////////
#include "funcubeLib.h"
#include "portaudio.h"
#include <math.h>
#include <iostream>
#include <sstream>

#include "peakDetect.h"

using namespace std;

CPeakDetect::CPeakDetect()
{
    m_lockAve = FALSE;
	m_peakBins = NULL;
	m_peakValues = NULL;
	m_binsAve = NULL;
	m_binsPower = NULL;
	m_windowFuncube = NULL;	
	m_maxPeaks = 16;
	m_numPeaks=0;
	m_numBins=0;
	m_lowLimitBin=0;
    m_highLimitBin=0;	
	m_minDistance=0;
}

CPeakDetect::~CPeakDetect(void)
{
	delete[] m_binsAve;
	delete[] m_binsPower;
	delete[] m_peakValues;
	delete[] m_peakBins;
	delete[] m_windowFuncube;	
}

BOOL CPeakDetect::Initialise(UINT numBins, UINT maxPeaks, UINT minDistance, UINT sampleSmoothing, UINT windowSize)
{
	// must be odd number
	if (windowSize % 2 == 0) {
		return FALSE;
	}

    m_numBins = numBins;
	m_minDistance = minDistance;
	m_windowSize = windowSize;
	SetMaxPeakCount(maxPeaks);
	m_binsAve = new FLOAT[m_numBins];
	m_binsPower = new FLOAT[m_numBins];
	m_binsSumFuncube = new FLOAT[m_numBins];
	m_lowLimitBin = 0;
	m_highLimitBin = m_numBins;
	m_ave.SetAverageSamples(sampleSmoothing);
	CalculateWindows(m_windowSize);
	for (UINT idx=0; idx < m_numBins; idx++) {
		m_binsAve[idx] = m_binsPower[idx] = m_binsSumFuncube[idx] = 0.0F;
	}
    return TRUE;
}

// Generate two window functions
// A blackman window, but with no negatives, two peaks and a raised bit in the middle (for the funcube window)
// Ascii art below, sort of M like:
//     /\ /\
//   _/  -  \_

void CPeakDetect::CalculateWindows(UINT size) {
	// can only be NULL or an address so safe to just delete
	delete[] m_windowFuncube;

	m_windowFuncube = new FLOAT[size];	

	// Can't make a funcube looking signal in under 10 bins so just go for a rectangularish one
	if (size < 10) {
		for (UINT idx = 0; idx < size; idx++)
		{
			m_windowFuncube[idx] = 0.9F;
		}
		m_windowFuncube[size/2] = 1.0F;
		return;
	}

	// there are two peaks so generate
	// blackman with half and then mirror it
	UINT numTapsPerPeak = size / 2;
	UINT m = numTapsPerPeak - 1;
	// the coefs for the peak can also be mirrored (so only generate half)
	// but with a little offset added to make the raised middle
	UINT halfLength = numTapsPerPeak / 2;

	FLOAT val;
	FLOAT sum = 0.0;
	// M shaped so indexs are for each of the four slopes, outside 1, middle 1, middle 2 and outside 2
	UINT pk1Out, pk1Mid, pk2Mid, pk2Out;
	// set the very middle to the fixed rasied value
	m_windowFuncube[numTapsPerPeak] = 0.1F;
	for (UINT idx = 0; idx <= halfLength; idx++)
	{
		pk1Out = idx;
		pk1Mid = numTapsPerPeak - idx - 1;
		pk2Mid = pk1Out + numTapsPerPeak + 1; // add one to skip over the very middle tap
		pk2Out = pk1Mid + numTapsPerPeak + 1;
		
		val = 0.42F - 0.5F * cos(TWO_PI * idx / m) + 0.08F * cos(4.0F * PI * idx / m);
		if (val < 0.0F) val = 0.0F;
		m_windowFuncube[pk1Mid] = val + 0.1F;
		m_windowFuncube[pk2Mid] = val + 0.1F;
		// set these last as the middle tap of each peak will be set
		// by both the mid and the outer, but the mids have a little bit
		// extra added that we don't want, so use the outers.
		m_windowFuncube[pk1Out] = val;
		m_windowFuncube[pk2Out] = val;
	}
}

void CPeakDetect::SetMaxPeakCount(UINT maxPeaks) {
	m_maxPeaks = maxPeaks;
	m_numPeaks = 0;
	
	delete[] m_peakValues;
	delete[] m_peakBins;
	m_peakValues = new FLOAT[m_maxPeaks];
	m_peakBins = new UINT[m_maxPeaks];
}


BOOL CPeakDetect::SetDetectLimits(UINT lowBin, UINT highBin)
{
    if(lowBin>highBin)
    {
        return FALSE;
    }
    
    m_lowLimitBin = lowBin;
    m_highLimitBin = highBin;

	return TRUE;
}

void CPeakDetect::ApplyWindowFunction() {
	// lock averaged power spectra while we process them
	m_lockAve = TRUE;

	if (m_windowSize == 1) {
		// no window to apply so just copy
		memcpy(m_binsSumFuncube, m_binsAve, sizeof(FLOAT) * m_numBins);
		m_lockAve = FALSE;
		return;
	}

	// apply window function along power spectrum, save
	// smoothed power spectra..
	INT halfWindow = m_windowSize / 2;
	for (INT idx = 0; idx < m_numBins; idx++)
	{
		FLOAT* pave = &m_binsAve[max(0, idx - halfWindow)];
		FLOAT* pwin = &m_windowFuncube[max(0, halfWindow - idx)];
		FLOAT* pwinEnd = &m_windowFuncube[min(m_windowSize, (m_numBins - idx) + halfWindow)];

		m_binsSumFuncube[idx] = 0.0;
		while (pwin < pwinEnd) {
			m_binsSumFuncube[idx] += *pave++ * *pwin++;
		}		
	}

	m_lockAve = FALSE;
}

BOOL CPeakDetect::CalcPeaks(ULONG numPeaks) 
{
	if (numPeaks == 0) {
		OutputDebugStringA("Z|");
	}
	OutputDebugStringA("C|");
	ApplyWindowFunction();

    // find minimum number of required peaks in smoothed power spectra, zero remainder
	BOOL found = TRUE;
	ULONG ubound = min((numPeaks), m_maxPeaks);
	for (m_numPeaks = 0; m_numPeaks < ubound; m_numPeaks++) {
		m_peakBins[m_numPeaks] = 0;
		m_peakValues[m_numPeaks] = 0.0;

		if (!FindPeak(m_binsSumFuncube, m_numBins)) {
			found = FALSE;
			break;
		}
	}
	// clear unfound peaks
	for (ULONG idx = m_numPeaks; idx < m_maxPeaks; idx++)
	{
		m_peakBins[idx] = 0;
		m_peakValues[idx] = 0.0;
	}

	return found;
}

BOOL CPeakDetect::GetPeaks(vector<UINT>& peakBins) {
	ULONG idx;	
	for (idx = 0; idx< m_numPeaks; idx++)
	{
		//std::cout << "[          ] out  [" << m_peakBins[idx] << "] = " << m_peakValues[idx] << std::endl;
		peakBins.push_back(m_peakBins[idx]);
	}	
	return m_numPeaks>0;
}

void CPeakDetect::ExcludePeaks(std::vector<UINT>& excludeBins) {
	// take a copy of the bins to exclude (next time we calculate)
	m_excludeBins = excludeBins;
}

BOOL CPeakDetect::FindPeak(FLOAT* bins, UINT numBins)
{
	BOOL found = FALSE;
	FLOAT prev, cur, next;
	prev = bins[0];
	cur = bins[1];
	for (UINT idx = 2; idx<numBins; idx++)
	{
		next = bins[idx];

		if (prev < cur && cur > next)
		{
			if (cur>m_peakValues[m_numPeaks] && IsInRange(idx-1) && IsLonePeak(idx - 1))
			{
				m_peakValues[m_numPeaks] = cur;
				m_peakBins[m_numPeaks] = idx - 1;
				found = TRUE;
			}
		}
		prev = cur;
		cur = next;
	}
	return found;
}

// check if a pair of bins are too close to each other (i.e. are part of the same peak)
BOOL CPeakDetect::TooClose(UINT bin1, UINT bin2) {
	// if no min distance, too close when equal
	if (0 == m_minDistance) {
		return bin1 == bin2;
	}
	// check difference
	return labs(((LONG)bin1 - bin2)) < m_minDistance;
}

// check if specified bin is on its own (TRUE), or too close to any others (FALSE)
BOOL CPeakDetect::IsLonePeak(UINT bin) 
{
	for (UINT i = 0; i < m_numPeaks; i++)
	{
		if (TooClose(bin, m_peakBins[i]))
			return FALSE;
	}
	for (const UINT& exclude : m_excludeBins) {
		if (TooClose(bin, exclude))
			return FALSE;
	}

	return TRUE;
}

// check if specified bin is inside the range limits
BOOL CPeakDetect::IsInRange(UINT bin)
{
	if (bin < m_lowLimitBin) 
	{
		return FALSE;	
	}

	if (bin > m_highLimitBin)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CPeakDetect::CopyResult(COMPLEXSTRUCT* samplesOut, const UINT sampleCount)
{
	if (sampleCount != m_numBins)
	{
		return FALSE;
	}
	for (int i = 0; i < m_numBins; i++) {
		samplesOut[i].fIm = m_binsSumFuncube[i];
		samplesOut[i].fRe = m_binsSumFuncube[i];
	}
	return TRUE;
}

// process FFT spectrum into time averaged power spectrum
BOOL CPeakDetect::Process(const COMPLEXSTRUCT* fft, UINT sampleCount)
{
	if (sampleCount != m_numBins)
	{
		return FALSE;
	}
	if (0 == m_maxPeaks) 
	{
		return FALSE;
	}

	// calc power at each bin (unless locked out)
    if (!m_lockAve)
    {
	    for (UINT idx = 0; idx<m_numBins; idx++)
	    {
		    m_binsPower[idx] = fft[idx].fRe*fft[idx].fRe + fft[idx].fIm*fft[idx].fIm;
		    // update average power at each bin
		    m_ave.ExternalAverage(m_binsAve[idx], m_binsPower[idx]);
	    }
    }

	return TRUE;
}

