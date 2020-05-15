#pragma once
#include "funcubeLib.h"

class CAudioBufferConfig
{
public:	
	CAudioBufferConfig() {
		SetSampleRate(96000);
	}

	void SetSampleRate(UINT sampleRateIn) {
		m_sampleRateIn = sampleRateIn;
		m_sampleRateOut = SAMPLE_RATE_OUT;
		m_fftSize = (FLOAT)sampleRateIn / HZ_PER_BIN;
		m_fftFreqRange = m_fftSize * HZ_PER_BIN; // aka sampleRate
		m_fftHalfFreqRange = m_fftFreqRange / 2.0;
		m_fftHalfSize = m_fftSize / 2;
		m_fftBufferBytes = sizeof(COMPLEXSTRUCT) * m_fftSize;
		m_fftHalfBufferBytes = sizeof(COMPLEXSTRUCT) * m_fftHalfSize;
		m_framesPerBufferIn = m_fftSize / 2;
		m_framesPerBufferOut = FRAMES_PER_BUFFER_OUT;
		m_audioInBufferBytes = sizeof(COMPLEXSTRUCT) * m_framesPerBufferIn;
		m_audioOutBufferBytes = sizeof(SAMPLE) * m_framesPerBufferOut;
	}
	~CAudioBufferConfig() { ; }

public:
	inline FLOAT BinToFreq(ULONG bin) { return  (HZ_PER_BIN*bin) - m_fftHalfFreqRange; }
	inline ULONG FreqToBin(FLOAT freq) { return  ((freq + m_fftHalfFreqRange) / HZ_PER_BIN); }
	inline LONG SecondsToInSamples(FLOAT secs) { return m_sampleRateIn*secs; }

	const UINT SampleRateIn() { return m_sampleRateIn; }
	const UINT SampleRateOut() { return m_sampleRateOut; }
	const UINT NumBinsFft() { return m_fftSize; }
	const UINT NumBinsFftHalf() { return m_fftHalfSize; }
	const UINT FramesPerBufferIn() { return m_framesPerBufferIn; }
	const UINT FramesPerBufferOut() { return m_framesPerBufferOut; }	
	const UINT BytesFftBuffer() { return m_fftBufferBytes; }
	const UINT BytesFftHalfBuffer() { return m_fftHalfBufferBytes; }
	const UINT BytesAudioInBuffer() { return m_audioInBufferBytes; }
	const UINT BytesAudioOutBuffer() { return m_audioOutBufferBytes; }
	const UINT BytesAudioInBufferMax() { return MAX_COMPLEX_BUFFER_BYTES; }
	const FLOAT FreqRangeFft() { return m_fftFreqRange; }
	const FLOAT FreqRangeFftHalf() { return m_fftHalfFreqRange; }
	const UINT NumBinsFftMax() { return MAX_FFT_SIZE; }
	
private:
	const UINT SAMPLE_RATE_OUT = 9600;
	const UINT FRAMES_PER_BUFFER_OUT = 512;
	const UINT MAX_FFT_SIZE = 16384;
	const UINT MAX_COMPLEX_BUFFER_BYTES = sizeof(COMPLEXSTRUCT) * MAX_FFT_SIZE;	
	
	UINT m_sampleRateIn;
	UINT m_sampleRateOut;

	UINT m_fftSize;
	UINT m_fftHalfSize;

	UINT m_fftBufferBytes;
	UINT m_fftHalfBufferBytes;

	FLOAT m_fftFreqRange;
	FLOAT m_fftHalfFreqRange;

	UINT m_framesPerBufferIn;
	UINT m_framesPerBufferOut;

	UINT m_audioInBufferBytes;
	UINT m_audioOutBufferBytes;
};
