#include "FuncubeLib.h"

#include "gmock\gmock.h" 
#include "gtest\gtest.h"

#include "fft.h"
#include "OverlappedFft.h"
#include "Oscillator.h"
#include "FirFilter.h"
#include <vector>
#include "PeakDetect.h"


TEST(PeakDetectTests, InsertSort)
{
	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = 2048;
	COMPLEXSTRUCT fftData[NUM_BINS];

	int size = sizeof(fftData);
	memset(&fftData, 0, size);
	
	fftData[0].fRe = 0.1F;
	fftData[1].fRe = 1.0F;
	fftData[2].fRe = 0.1F;
	fftData[3].fRe = 0.1F;
	fftData[4].fRe = 2.0F;
	fftData[5].fRe = 0.1F;
	fftData[6].fRe = 0.1F;
	fftData[7].fRe = 3.0F;
	fftData[8].fRe = 0.1F;
	fftData[9].fRe = 0.1F;
	fftData[10].fRe = 4.0F;
	fftData[11].fRe = 0.1F;
	fftData[12].fRe = 4.1F; // should be ignored as too close to bin 13
	fftData[13].fRe = 5.0F;
	fftData[14].fRe = 0.1F;
	fftData[15].fRe = 0.1F;
	
	CPeakDetect peaks;
	peaks.Initialise(NUM_BINS, 4, 3, 25, 1);

	peaks.Process(&fftData[0], NUM_BINS);	
	std::vector<UINT> peakBin;
	ULONG peakCount=4;
	peaks.CalcPeaks(peakCount);
	peaks.GetPeaks(peakBin, peakCount);

	UINT expectedBin[4] = { 13,10,7,4 };

	EXPECT_EQ(expectedBin[0], peakBin[0]);
	EXPECT_EQ(expectedBin[1], peakBin[1]);
	EXPECT_EQ(expectedBin[2], peakBin[2]);
}

TEST(PeakDetectTests, SinglePeakDetect)
{
	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = 2048;
	COMPLEXSTRUCT fftData[NUM_BINS];

	int size = sizeof(fftData);
	memset(&fftData, 0, size);
	
	fftData[EXPECTED_BIN].fRe = 42.0F;
	fftData[EXPECTED_BIN].fIm = 42.0F;	

	CPeakDetect peaks;
	peaks.Initialise(NUM_BINS, 4, 1, 1, 1);

	peaks.Process(&fftData[0], NUM_BINS);
	std::vector<UINT> peakBin;
	ULONG peakCount=1;
	peaks.CalcPeaks(peakCount);
	peaks.GetPeaks(peakBin, peakCount);
	
	EXPECT_EQ(EXPECTED_BIN, peakBin[0]);

	
	//std::cout << "[          ] peakValue[" << peakPos[0] << "] = " << peakValue[0] << std::endl;
	//std::cout << "[          ] peakValue[" << peakPos[1] << "] = " << peakValue[1] << std::endl;
	//std::cout << "[          ] peakValue[" << peakPos[2] << "] = " << peakValue[2] << std::endl;
		
}