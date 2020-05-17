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

/// @file FirFilter.cpp
//
//////////////////////////////////////////////////////////////////////
#include "firFilter.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "bpskDecoder.h"


CFirFilter::CFirFilter() :
m_freqPass(0),
    m_freqStop(0),
    m_numTaps(0),
    m_sampleRate(0),
    m_filterIndex(0),
    m_filterHistoryRe(NULL),
    m_filterHistoryIm(NULL),
    m_filterValues(NULL),
    m_filterWindow(NULL),
    m_filterType(FT_LOW_PASS),
    m_windowType(WT_RECTANGULAR)
{

}

CFirFilter::~CFirFilter(void)
{
    Free();
}

BOOL CFirFilter::Initialise(WindowType windowType, FilterType filterType, INT sampleRate, INT numTaps, FLOAT freqCutOff, FLOAT signalBandwidth)
{
    if(FT_LOW_PASS != filterType && FT_HIGH_PASS != filterType)
    {
        return FALSE;
    }
    return Initialise(windowType, filterType,sampleRate, numTaps, freqCutOff, freqCutOff, signalBandwidth);
}

BOOL CFirFilter::Initialise(WindowType windowType, FilterType filterType, INT sampleRate, INT numTaps, FLOAT freqPass, FLOAT freqStop, FLOAT signalBandwidth)
{
    if(numTaps<=0)
    {
        return FALSE;
    }

    Free();

    m_numTaps = numTaps;
    m_sampleRate = sampleRate;
    m_windowType = windowType;
    m_filterType = filterType;

    // allocate memory for the number of taps requested
    AllocateBuffers(numTaps);

    if(!CalculateWindow(windowType))
    {
        Free();
        return FALSE;
    }        

    return Reinitialise(signalBandwidth, freqPass, freqStop);
}

BOOL CFirFilter::Initialise(const FLOAT* filterValues, int numTaps)
{
    if(numTaps<=0 || NULL == filterValues)
    {
        return FALSE;
    }

    m_filterType = FT_FROM_VALUES;
    m_numTaps = numTaps;

    // allocate memory for the number of taps requested
    AllocateBuffers(numTaps);

    memcpy(m_filterValues, filterValues, sizeof(FLOAT)*numTaps);
    memcpy(m_filterValues+numTaps, filterValues, sizeof(FLOAT)*numTaps);

    return TRUE;
}

void CFirFilter::AllocateBuffers(int numTaps)
{
    m_filterWindow = new FLOAT[numTaps];
    m_filterHistoryRe = new FLOAT[numTaps];
    m_filterHistoryIm = new FLOAT[numTaps];
    // duplicate the tap values so we "wrap around" the end when applying (so it can be done in one loop)
    m_filterValues = new FLOAT[numTaps*2];

    for (int i=0;i<m_numTaps;i++)
    {
        m_filterHistoryRe[i]=0.0F;
        m_filterHistoryIm[i]=0.0F;
    }
}

void CFirFilter::Free()
{
    m_numTaps = 0;    
    m_sampleRate = 0;

    if(NULL != m_filterValues)
    {
        delete [] m_filterValues;
        m_filterValues = NULL;
    }

    if(NULL != m_filterWindow)
    {
        delete [] m_filterWindow;
        m_filterWindow = NULL;
    }

    if(NULL != m_filterHistoryRe)
    {
        delete [] m_filterHistoryRe;
        m_filterHistoryRe = NULL;
    }

    if(NULL != m_filterHistoryIm)
    {
        delete [] m_filterHistoryIm;
        m_filterHistoryIm = NULL;
    }
}

BOOL CFirFilter::Reinitialise(FLOAT signalBandwidth, FLOAT freqCutOff)
{
    if(FT_LOW_PASS != m_filterType && FT_HIGH_PASS != m_filterType)
    {
        return FALSE;
    }
    return Reinitialise(signalBandwidth, freqCutOff, freqCutOff);
}

BOOL CFirFilter::Reinitialise(FLOAT signalBandwidth, FLOAT freqPass, FLOAT freqStop)
{
    //const FLOAT SIGNAL_BANDWIDTH = 3190;
    m_freqPass = freqPass;
    m_freqStop = freqStop;
    FLOAT bandwidth = freqStop-freqPass;

    FLOAT freqCentre = freqPass + (bandwidth/2);
    FLOAT space = bandwidth-signalBandwidth;
    m_signalMinPass = freqCentre-(space/2);
    m_signalMaxPass = freqCentre+(space/2);

    BOOL success;
    switch(m_filterType)
    {
    case FT_LOW_PASS:
        success = CalculateOneTransSinc( freqStop );
        break;
    case FT_HIGH_PASS:
        success = CalculateOneTransSinc( freqPass );
        break;
    case FT_BAND_PASS: // drop through
    case FT_BAND_STOP:
        success = CalculateTwoTransSinc( freqPass, freqStop );
        break;
    default:
        success = FALSE;
        break;
    }

    if(success && WT_RECTANGULAR != m_windowType)
    {
        success = ApplyWindow();
    }

    return success;

    /*
    int windowLength = 127;
    FLOAT sampFreq = 48000;

    // Low and high pass filters
    FLOAT transFreq = 2800;

    // Kaiser Window
    int kaiserWindowLength;
    FLOAT beta;

    calculateKaiserParams(0.005, 400, sampFreq, &kaiserWindowLength, &beta);

    lpf = create1TransSinc(kaiserWindowLength, transFreq, sampFreq, LOW_PASS);
    FLOAT *lpf_kaiser = createKaiserWindow(lpf, NULL, kaiserWindowLength, beta);

    outputFFT("lpf-kaiser.dat", lpf_kaiser, kaiserWindowLength, sampFreq);

    return 0;
    */
}

// add the input sample and generate an output sample
FLOAT CFirFilter::ProcessSample(const FLOAT& inSample)
{
	// use a circular buffer & index shifting
	m_filterHistoryRe[m_filterIndex] = inSample;	

	const FLOAT *filter = m_filterValues + m_numTaps - m_filterIndex;
	FLOAT re = 0.0F;

	for (int i = 0; i<m_numTaps; i++)
	{
		re += m_filterHistoryRe[i] * (*filter);
		filter++;
	}
	if (--m_filterIndex < 0)
	{
		m_filterIndex = m_numTaps - 1;
	}
	return re;	
}

// add the input sample and generate an output sample
void CFirFilter::ProcessSample(const COMPLEXSTRUCT& inSample, COMPLEXSTRUCT& outSample)
{
    // use a circular buffer & index shifting
    m_filterHistoryRe[m_filterIndex] = inSample.fRe;
    m_filterHistoryIm[m_filterIndex] = inSample.fIm;

    const FLOAT *filter=m_filterValues + m_numTaps - m_filterIndex;

    FLOAT re=0.0F, im=0.0F;
    
    for (int i=0;i<m_numTaps;i++)
    {
        re+=m_filterHistoryRe[i]*(*filter);
        im+=m_filterHistoryIm[i]*(*filter);
        filter++;
        
    }
    if (--m_filterIndex < 0)
    {
        m_filterIndex = m_numTaps-1;
    }

    outSample.fIm = im;
    outSample.fRe = re;
}

void CFirFilter::ProcessSample(const COMPLEXSTRUCT& inSample)
{
    m_filterHistoryRe[m_filterIndex] = inSample.fRe;
    m_filterHistoryIm[m_filterIndex] = inSample.fIm;
    if (--m_filterIndex < 0)
    {
        m_filterIndex = m_numTaps-1;
    }
}

// Create sinc function for filter with 1 transition - Low and High pass filters
BOOL CFirFilter::CalculateOneTransSinc(FLOAT transFreq)
{
    int n;
    int length = m_numTaps;
    FLOAT sampFreq = m_sampleRate;

    if (FT_LOW_PASS != m_filterType && FT_HIGH_PASS != m_filterType) {
        return FALSE;
    }

    if(NULL == m_filterValues)
    {
        return FALSE;
    }

    // Calculate the normalised transistion frequency. As transFreq should be
    // less than or equal to sampFreq / 2, ft should be less than 0.5
    FLOAT ft = transFreq / sampFreq;


    FLOAT m_2 = 0.5F * (length-1);
    int halfLength = length / 2;

    // Set centre tap, if present
    // This avoids a divide by zero
    if (2*halfLength != length) 
    {
        FLOAT val = 2.0F * ft;

        // If we want a high pass filter, subtract sinc function from a dirac pulse
        if (FT_HIGH_PASS == m_filterType)
        {
            val = 1.0F - val;
        }

        m_filterValues[halfLength] = val;
        m_filterValues[length+halfLength] = val;
    }
    else if (FT_HIGH_PASS == m_filterType) 
    {
        //fprintf(stderr, "create1TransSinc: For high pass filter, window length must be odd\n");
        return FALSE;
    }

    // This has the effect of inverting all weight values
    if (FT_HIGH_PASS == m_filterType) 
    {
        ft = -ft;
    }

    // Calculate taps
    // Due to symmetry, only need to calculate half the window
    for (n=0 ; n<halfLength ; n++) 
    {
        FLOAT val = sin(TWO_PI * ft * (n-m_2)) / (PI * (n-m_2));

        m_filterValues[n] = val;
        m_filterValues[length-n-1] = val;
        // duplicate the tap values so we "wrap around" the end when applying (so it can be done in one loop)
        m_filterValues[length + n] = val;
        m_filterValues[length + (length-n-1)] = val;
    }

    return TRUE;
}



// Create two sinc functions for filter with 2 transitions - Band pass and band stop filters
BOOL CFirFilter::CalculateTwoTransSinc(FLOAT trans1Freq, FLOAT trans2Freq)
{
    int n;
    int length = m_numTaps;
    FLOAT sampFreq = m_sampleRate;

    if (FT_BAND_PASS != m_filterType && FT_BAND_STOP != m_filterType) {
        return FALSE;
    }

    // Calculate the normalised transistion frequencies.
    FLOAT ft1 = trans1Freq / sampFreq;
    FLOAT ft2 = trans2Freq / sampFreq;


    FLOAT m_2 = 0.5F * (length-1);
    int halfLength = length / 2;

    // Set centre tap, if present
    // This avoids a divide by zero
    if (2*halfLength != length) {
        FLOAT val = 2.0F * (ft2 - ft1);

        // If we want a band stop filter, subtract sinc functions from a dirac pulse
        if (FT_BAND_STOP == m_filterType) 
        {    
            val = 1.0F - val;
        }

        m_filterValues[halfLength] = val;
        m_filterValues[length+halfLength] = val;
    }
    else 
    {
        //fprintf(stderr, "create1TransSinc: For band pass and band stop filters, window length must be odd\n");
        return FALSE;
    }

    // Swap transition points if Band Stop
    if (FT_BAND_STOP == m_filterType) 
    {
        FLOAT tmp = ft1;
        ft1 = ft2; ft2 = tmp;
    }

    // Calculate taps
    // Due to symmetry, only need to calculate half the window
    for (n=0 ; n<halfLength ; n++) {
        FLOAT val1 = sin(2.0F * PI * ft1 * (n-m_2)) / (PI * (n-m_2));
        FLOAT val2 = sin(2.0F * PI * ft2 * (n-m_2)) / (PI * (n-m_2));

        m_filterValues[n] = val2 - val1;
        m_filterValues[length-n-1] = val2 - val1;
        // duplicate the tap values so we "wrap around" the end when applying (so it can be done in one loop)
        m_filterValues[length + n] = val2 - val1;
        m_filterValues[length + (length-n-1)] = val2 - val1;
    }

    return TRUE;
}

// Create a set of window weights
// windowType - The window type
BOOL CFirFilter::CalculateWindow(WindowType type)
{    
    if (NULL == m_filterWindow || m_numTaps <= 0) 
    {
        return FALSE;
    }

    int n;
    int m = m_numTaps - 1;
    int halfLength = m_numTaps / 2;

    // Calculate taps
    // Due to symmetry, only need to calculate half the window
    switch (type)
    {
    case WT_RECTANGULAR:
        for (n=0 ; n<m_numTaps ; n++) 
        {
            m_filterWindow[n] = 1.0F;
        }
        break;

    case WT_BARTLETT:
        for (n=0 ; n<=halfLength ; n++) 
        {
            FLOAT tmp = (FLOAT) n - (FLOAT)m / 2;
            FLOAT val = 1.0F - (2.0F * fabs(tmp))/m;
            m_filterWindow[n] = val;
            m_filterWindow[m_numTaps-n-1] = val;
        }
        break;

    case WT_HANNING:
        for (n=0 ; n<=halfLength ; n++) 
        {
            FLOAT val = 0.5F - 0.5F * cos(2.0F * PI * n / m);
            m_filterWindow[n] = val;
            m_filterWindow[m_numTaps-n-1] = val;
        }
        break;

    case WT_HAMMING:
        for (n=0 ; n<=halfLength ; n++) 
        {
            FLOAT val = 0.54F - 0.46F * cos(2.0F * PI * n / m);
            m_filterWindow[n] = val;
            m_filterWindow[m_numTaps-n-1] = val;
        }
        break;

    case WT_BLACKMAN:
        for (n=0 ; n<=halfLength ; n++) 
        {
            FLOAT val = 0.42F - 0.5F * cos(2.0F * PI * n / m) + 0.08F * cos(4.0F * PI * n / m);
            m_filterWindow[n] = val;
            m_filterWindow[m_numTaps-n-1] = val;
        }
        break;

    default:
        return FALSE;
        break;
    }

    return TRUE;
}

BOOL CFirFilter::ApplyWindow()
{
    // apply the current window to the filter tap values
    if (NULL == m_filterValues || NULL == m_filterWindow) 
    {
        return FALSE;
    }

    for (int n=0 ; n<m_numTaps ; n++) 
    {
        m_filterValues[n] *= m_filterWindow[n];
        // apply to duplicated filter values as well
        m_filterValues[n+m_numTaps] *= m_filterWindow[n];
    }

    return TRUE;
}

BOOL CFirFilter::SelfTest()
{
    COMPLEXSTRUCT one = {1.0,1.0};
    COMPLEXSTRUCT zero = {0.0, 0.0};
    COMPLEXSTRUCT out;
    
    for(int i=0;i<3;i++)
    {
        ProcessSample(one, out);
        for (int n=0 ; n<m_numTaps ; n++) 
        {
            if(out.fRe != m_filterValues[n] || out.fIm != m_filterValues[n])
            {
                return FALSE;
            }
            ProcessSample(zero, out);
        }
    }

    ProcessSample(zero, out);

    FLOAT total = 0.0;
    for (int n=0 ; n<m_numTaps; n++) 
    {
        total += 1.0F * m_filterValues[n];
        ProcessSample(one, out);
        if(total != out.fIm)
        {
            return FALSE;
        }
    }

    return TRUE;
}


int CFirFilter::OutputFFT(CHAR *filename, FLOAT *window, INT windowLength, FLOAT sampFreq)
{    
    int i;
    FILE *fp;
    float *in;
    fftwf_complex *out;
    fftwf_plan plan;
    int result = 0;

    // If the window length is short, zero padding will be used
    int fftSize = (windowLength < 2048) ? 2048 : windowLength;

    // Calculate size of result data
    int resultSize = (fftSize / 2) + 1;

    // Allocate memory to hold input and output data
    in = (float *) fftwf_malloc(fftSize * sizeof(float));
    out = (fftwf_complex *) fftwf_malloc(resultSize * sizeof(fftwf_complex));
    if (in == NULL || out == NULL) {
        result = 1;
        fprintf(stderr, "outputFFT: Could not allocate input/output data\n");
        goto finalise;
    }

    // Create the plan and check for success
    plan = fftwf_plan_dft_r2c_1d(fftSize, in, out, FFTW_MEASURE); 
    if (plan == NULL) {
        result = 1;
        fprintf(stderr, "outputFFT: Could not create plan\n");
        goto finalise;
    }

    // Copy window and add zero padding (if required)
    for (i=0 ; i<windowLength ; i++) in[i] = window[i];
    for ( ; i<fftSize ; i++) in[i] = 0;

    // Perform fft
    fftwf_execute(plan);

    // Open file for writing
    fp = fopen((const char *)filename, "w");
    if (fp == NULL) {
        result = 1;
        fprintf(stderr, "outputFFT: Could open output file for writing\n");
        goto finalise;
    }

    // Output result
    for (i=0 ; i<resultSize ; i++)
    {
        FLOAT freq = sampFreq * i / fftSize;
        FLOAT mag = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
        FLOAT magdB = 20 * log10(mag);
        FLOAT phase = atan2(out[i][1], out[i][0]);
        fprintf(fp, "%f, %f, %f, %f\n", freq, mag, magdB, phase);
    }

    fprintf(fp, "\n\nFilter Co-efficients:\n\n");
    for (i=0 ; i<windowLength; i++)
    {
        fprintf(fp, "    %gF, /* tap# %03d */ \n", window[i],i);
    }

    // Perform any cleaning up
finalise:
    if (plan != NULL) fftwf_destroy_plan(plan);
    if (in != NULL) fftwf_free(in);
    if (out != NULL) fftwf_free(out);
    if (fp != NULL) fclose(fp);

    return result;
}


/*
// Transition Width (transWidth) is given in Hz
// Sampling Frequency (sampFreq) is given in Hz
// Window Length (windowLength) will be set
void calculateKaiserParams(FLOAT ripple, FLOAT transWidth, FLOAT sampFreq, int *windowLength, FLOAT *beta)
{
    // Calculate delta w
    FLOAT dw = 2 * PI * transWidth / sampFreq;

    // Calculate ripple dB
    FLOAT a = -20.0 * log10(ripple);

    // Calculate filter order
    int m;
    if (a>21) m = ceil((a-7.95) / (2.285*dw));
    else m = ceil(5.79/dw);

    *windowLength = m + 1;

    if (a<=21) *beta = 0.0;
    else if (a<=50) *beta = 0.5842 * pow(a-21, 0.4) + 0.07886 * (a-21);
    else *beta = 0.1102 * (a-8.7);
}

FLOAT *createKaiserWindow(FLOAT *in, FLOAT *out, int windowLength, FLOAT beta)
{
    FLOAT m_2 = (FLOAT)(windowLength-1) / 2.0;
    FLOAT denom = modZeroBessel(beta);                    // Denominator of Kaiser function

    // If output buffer has not been allocated, allocate memory now
    if (out == NULL) {
        out = (FLOAT *) malloc(windowLength * sizeof(FLOAT));
        if (out == NULL) {
            fprintf(stderr, "Could not allocate memory for window\n");
            return NULL;
        }
    }

    int n;
    for (n=0 ; n<windowLength ; n++)
    {
        FLOAT val = ((n) - m_2) / m_2;
        val = 1 - (val * val);
        out[n] = modZeroBessel(beta * sqrt(val)) / denom;
    }

    // If input has been given, multiply with out
    if (in != NULL) {
        for (n=0 ; n<windowLength ; n++) {
            out[n] *= in[n];
        }
    }

    return out;
}

FLOAT modZeroBessel(FLOAT x)
{
    int i;

    FLOAT x_2 = x/2;
    FLOAT num = 1;
    FLOAT fact = 1;
    FLOAT result = 1;

    for (i=1 ; i<20 ; i++) {
        num *= x_2 * x_2;
        fact *= i;
        result += num / (fact * fact);
        //        printf("%f %f %f\n", num, fact, result);
    }

    return result;
}
*/
