#pragma once

#include "funcubeLib.h"
#include "fft.h"
#include <mutex>
#ifdef LINUX
#include "wintypes.h"
#endif

class COverlappedFft
{
public:
	COverlappedFft();
	~COverlappedFft(void);

	// create buffers/configure FFT sizes
	BOOL Initialise(UINT numBins, UINT samplesPerBuffer);
	// release buffers free memory
	void Shutdown();

	// keep the same configuration but reset buffers/counters
	BOOL Clear();

	BOOL Add(const COMPLEXSTRUCT* samples, UINT sampleCount, BOOL& resultUpdated);

	BOOL CopyResult(COMPLEXSTRUCT* samples, UINT sampleCount);
	BOOL SumResult(COMPLEXSTRUCT* samples, UINT sampleCount);

	BOOL IsOutputValid();

private:
	void Swap();

	/// control access to the buffers
	std::recursive_mutex m_lock;

	CFft* m_current;
	CFft* m_previous;

	UINT m_overlapSize;	
};

