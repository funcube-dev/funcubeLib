#include "funcubeLib.h"

#include "gmock\gmock.h" 
#include "gtest\gtest.h"

#include "fft.h"
#include "overlappedFft.h"
#include "oscillator.h"
#include "firFilter.h"

/*
TEST(PeakTrackerTests, InsertSort)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ = 16384.0F;

	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = (SIGNAL_FREQ / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;

	int size = sizeof(sizeof(COMPLEXSTRUCT));

	COscillator* osc = new COscillator();
	COverlappedFft fft;

	osc->Initialise(SIGNAL_FREQ, TEST_RATE+5);
	BOOL InitResult = fft.Initialise(NUM_BINS, 1);
		
	COMPLEXSTRUCT sample;
	BOOL resultUpdated;
	for(size_t count=0;count<NUM_BINS;count++)
	{
		osc->GetSample(sample);
		resultUpdated = FALSE;
		fft.Add(&sample, 1, resultUpdated);
	}

	EXPECT_EQ(TRUE, resultUpdated);


	COMPLEXSTRUCT samples[NUM_BINS];
	fft.CopyResult(samples, NUM_BINS);
	FLOAT peakValue = 0.0;
	UINT peakPos = 0;
	for(size_t idx=0;idx<NUM_BINS;idx++)
	{
		FLOAT value = sqrt(samples[idx].fRe*samples[idx].fRe + samples[idx].fIm*samples[idx].fIm);
		if(value > peakValue)
		{
			peakPos = idx;
			peakValue = value;
		}
	}

	fft.Shutdown();

	std::cout << "[          ] peakValue = " << peakValue << std::endl;
	
	EXPECT_EQ(EXPECTED_BIN, peakPos);
	EXPECT_EQ(TRUE, InitResult);


	
	//std::cout << "[          ] peakValue[" << peakPos[0] << "] = " << peakValue[0] << std::endl;
	//std::cout << "[          ] peakValue[" << peakPos[1] << "] = " << peakValue[1] << std::endl;
	//std::cout << "[          ] peakValue[" << peakPos[2] << "] = " << peakValue[2] << std::endl;
		
}


TEST(PeakTrackerTests, SinglePeakDetect)
{
	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = 2048;
	COMPLEXSTRUCT fftData[NUM_BINS];

	int size = sizeof(fftData);
	memset(&fftData, 0, size);
	
	fftData[EXPECTED_BIN].fRe = 42.0F;
	fftData[EXPECTED_BIN].fIm = 42.0F;	

	//CPeakTracker tracker;
	//tracker.Initialise(NUM_BINS, 4, 3000, 96000, 1024, CPeakTracker::SampleType_Complex);

	//tracker.Process(&fftData[0], NUM_BINS);
	FLOAT peakBin;
	ULONG peakCount=1;
	//tracker.GetPeaks(&peakBin, &peakCount);
	
	EXPECT_EQ(EXPECTED_BIN, peakBin);
		
	//std::cout << "[          ] peakValue[" << peakPos[0] << "] = " << peakValue[0] << std::endl;
	//std::cout << "[          ] peakValue[" << peakPos[1] << "] = " << peakValue[1] << std::endl;
	//std::cout << "[          ] peakValue[" << peakPos[2] << "] = " << peakValue[2] << std::endl;
		
}
*/