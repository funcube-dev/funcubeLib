#include <math.h>
#include "Fft.h"
#include <functional>

CFft::CFft(void) :
		m_numBins(0),
		m_samplesPerBuffer(0),
		m_writeIdx(0),
		m_outputValid(FALSE)
{
}

CFft::~CFft(void)
{
	Shutdown();
}

BOOL CFft::Initialise(const UINT numBins, const UINT samplesPerBuffer)
{
	// check bin count is power of 2
	if(numBins==0 || (numBins & (numBins-1)) != 0)
	{
		return FALSE;
	}
	if(numBins%samplesPerBuffer != 0  || samplesPerBuffer > numBins)
	{
		return FALSE;
	}

	m_numBins = numBins;
	m_samplesPerBuffer = samplesPerBuffer;
	m_writeIdx = 0;
	
	m_hannWindow = std::unique_ptr<FLOAT[]>(new FLOAT[m_numBins]);
    FLOAT n = 0.0F;
    for(size_t idx=0;idx<m_numBins;++idx)
    {
        m_hannWindow[idx] = 0.5F * (1.0F - cos( (TWO_PI * n++ ) / m_numBins )); // hanning        
    }

	size_t fftBytes = sizeof(fftwf_complex) * numBins;
	m_halfFftBytes = fftBytes/2;

	m_fftIn = fft_unique_ptr((fftwf_complex*)fftwf_malloc(fftBytes), FftFreeFunctor());	
    m_fftOut = fft_unique_ptr((fftwf_complex*)fftwf_malloc(fftBytes), FftFreeFunctor());

    m_fftPlan = fftwf_plan_dft_1d(numBins, m_fftIn.get(), m_fftOut.get(), FFTW_FORWARD, FFTW_MEASURE);

	return TRUE;
}

void CFft::Shutdown()
{
	m_numBins = 0;
	m_samplesPerBuffer = 0;
}

void CFft::Clear()
{
	m_outputValid = FALSE;
	m_writeIdx = 0;
}

BOOL CFft::ZeroFill(UINT sampleCount)
{
	BOOL ignored=FALSE;
	return ZeroFill(sampleCount, ignored);
}

BOOL CFft::ZeroFill(UINT sampleCount, BOOL& resultUpdated)
{
	resultUpdated = FALSE;

	// check m_samplesPerBuffer is a multiple of sample count
	if(sampleCount%m_samplesPerBuffer != 0)
	{
		return FALSE;
	}

	size_t count;
	for(count=0;count<sampleCount && m_writeIdx <= m_numBins; count++)
	{
		m_fftIn[m_writeIdx][0] = 0.0;
		m_fftIn[m_writeIdx][1] = 0.0;
		m_writeIdx++;
	}

	resultUpdated = CheckAndExecute();

	return count==sampleCount;

}

BOOL CFft::Add(const COMPLEXSTRUCT* samples, const UINT sampleCount, BOOL& resultUpdated)
{
	resultUpdated = FALSE;
	if(sampleCount != m_samplesPerBuffer)
	{
		return FALSE;
	}

	size_t count;
	for(count=0;count<sampleCount && m_writeIdx <= m_numBins; count++)
	{		
		m_fftIn[m_writeIdx][0] = samples->fRe * m_hannWindow[m_writeIdx];
		m_fftIn[m_writeIdx][1] = samples->fIm * m_hannWindow[m_writeIdx];
		m_writeIdx++;
		samples++;
	}

	resultUpdated = CheckAndExecute();	

	return count==sampleCount;
}

BOOL CFft::CheckAndExecute()
{
	BOOL executed = FALSE;

	if(m_writeIdx == m_numBins)
	{
		fftwf_execute(m_fftPlan);
		m_outputValid = TRUE;
		m_writeIdx = 0;
		executed = TRUE;
	}

	return executed;
}

BOOL CFft::CopyResult(COMPLEXSTRUCT* samplesOut, const UINT sampleCount)
{
	if(sampleCount != m_numBins || !m_outputValid)
	{
		return FALSE;
	}
	// result is currently ordered from 0 ... +freq -freq ... 0 so copy half at a time
	// to put in order -freq ... 0 ... +freq
	memcpy(samplesOut, ((PBYTE)m_fftOut[0])+m_halfFftBytes, m_halfFftBytes);
	memcpy(((PBYTE)samplesOut)+m_halfFftBytes, m_fftOut[0], m_halfFftBytes);

	return TRUE;
}

BOOL CFft::SumResult(COMPLEXSTRUCT* samplesOut, const UINT sampleCount)
{
	if(sampleCount != m_numBins || !m_outputValid)
	{
		return FALSE;
	}
	size_t halfSamples = m_numBins / 2;
	
	COMPLEXSTRUCT *srcPos, *srcNeg, *destPos, *destNeg;
	
	srcPos = srcNeg = (COMPLEXSTRUCT*)m_fftOut[0];
	destPos = destNeg = samplesOut;

	// position the buffers
	destPos += halfSamples;
	srcNeg += halfSamples;
	
	for(size_t count=0; count < halfSamples; ++count)
	{
		*destPos++ += *srcPos++;
		*destNeg++ += *srcNeg++;
	}

	return TRUE;
}
