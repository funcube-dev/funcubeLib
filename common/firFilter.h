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

/// @file FirFilter.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "funcubeLib.h"

class CFirFilter
{
public:
    CFirFilter(void);
    virtual ~CFirFilter(void);
    
    enum WindowType {WT_RECTANGULAR, WT_BARTLETT, WT_HANNING, WT_HAMMING, WT_BLACKMAN};
    enum FilterType {FT_LOW_PASS, FT_HIGH_PASS, FT_BAND_PASS, FT_BAND_STOP, FT_FROM_VALUES};

    BOOL Initialise(WindowType windowType, FilterType filterType, int sampleRate, int numTaps, FLOAT freqPass, FLOAT freqStop, FLOAT signalBandwidth);
    BOOL Initialise(WindowType windowType, FilterType filterType, int sampleRate, int numTaps, FLOAT freqCutOff, FLOAT signalBandwidth);
    BOOL Initialise(const FLOAT* filterValues, int numTaps);
    BOOL Reinitialise(FLOAT signalBandwidth, FLOAT freqCutOff);
    BOOL Reinitialise(FLOAT signalBandwidth, FLOAT freqPass, FLOAT freqStop);

    BOOL IsInPassBand(FLOAT freq) { return m_signalMinPass<freq && m_signalMaxPass>freq; }

    FLOAT PassFrequency() { return m_freqPass; }
    FLOAT StopFrequency() { return m_freqStop; }

    void Free();

    void ProcessSample(const COMPLEXSTRUCT& inSample, COMPLEXSTRUCT& outSample);
    // add the input sample without generating an output (for decimating filters)
    void ProcessSample(const COMPLEXSTRUCT& inSample);

	// add a real only sample return the output sample
	FLOAT ProcessSample(const FLOAT& inSample);

    BOOL SelfTest();
private:

    void AllocateBuffers(int numTaps);
    BOOL CalculateOneTransSinc(FLOAT transFreq);
    BOOL CalculateTwoTransSinc(FLOAT trans1Freq, FLOAT trans2Freq);

    BOOL CalculateWindow(WindowType type);
    BOOL ApplyWindow();

    int OutputFFT(CHAR *filename, FLOAT *window, INT windowLength, FLOAT sampFreq);
    
    //void CalculateKaiserParams(FLOAT ripple, FLOAT transWidth, FLOAT sampFreq, int *windowLength, FLOAT *beta);
    //FLOAT *CreateKaiserWindow(FLOAT *in, FLOAT *out, int windowLength, FLOAT beta);
    //FLOAT ModZeroBessel(FLOAT x);
    //int outputFFT(char *filename, FLOAT *window, int windowLength, FLOAT sampFreq);
    
    FilterType m_filterType;
    WindowType m_windowType;
    
    FLOAT m_signalMinPass;
    FLOAT m_signalMaxPass;

    FLOAT m_freqPass;
    FLOAT m_freqStop;    
    
    INT m_numTaps;
    FLOAT *m_filterValues;
    FLOAT *m_filterWindow;
    
    INT m_filterIndex;
    FLOAT* m_filterHistoryRe;
    FLOAT* m_filterHistoryIm;

    INT m_sampleRate;
};
