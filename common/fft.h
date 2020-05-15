#pragma once

#include "FuncubeLib.h"
#include <memory>

class CFft
{
public:
	CFft(void);
	~CFft(void);

	// create buffers/configure FFT sizes
	BOOL Initialise(UINT numBins, UINT samplesPerBuffer);
	// release buffers free memory
	void Shutdown();

	// keep the same configuration but reset buffers/counters
	void Clear();
	
	// add a buffer of samples, the fft is executed if sufficent samples are available
	BOOL Add(const COMPLEXSTRUCT* samples, UINT sampleCount, BOOL& resultUpdated);
	// write zeros to the buffer, the fft is executed if sufficent samples are available
	BOOL ZeroFill(UINT sampleCount, BOOL& resultUpdated);
	BOOL ZeroFill(UINT sampleCount);

	BOOL CopyResult(COMPLEXSTRUCT* samples, UINT sampleCount);
	BOOL SumResult(COMPLEXSTRUCT* samples, UINT sampleCount);

	BOOL IsOutputValid() { return m_outputValid; }

private:
	BOOL CheckAndExecute();
	// declare customer deleter for use with unique_ptr
	struct FftFreeFunctor {
		void operator()(fftwf_complex* ptr) { 
			fftwf_free(ptr); 
		}
	};
	typedef std::unique_ptr<fftwf_complex[], FftFreeFunctor> fft_unique_ptr;
	fft_unique_ptr m_fftIn;
	fft_unique_ptr m_fftOut;
    fftwf_plan m_fftPlan;
	size_t m_halfFftBytes;
	UINT m_numBins;
	UINT m_samplesPerBuffer;
	UINT m_writeIdx;
	BOOL m_outputValid;

	std::unique_ptr<FLOAT[]> m_hannWindow;
};

