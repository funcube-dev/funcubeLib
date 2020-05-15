#include "FuncubeLib.h"

#include "gmock\gmock.h" 
#include "gtest\gtest.h"

#include "fft.h"
#include "OverlappedFft.h"
#include "Oscillator.h"
#include "FirFilter.h"
#include "PeakDetect.h"
/*
TEST(FftTests, OscillatorSetFreq)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ = 16384.0F;

	BOOL initResult = TRUE;
	COscillator osc;
	
	osc.Initialise(SIGNAL_FREQ, TEST_RATE);
			
	FLOAT phaseInc = osc.GetPhaseInc();
	FLOAT freq = osc.GetFrequency();
	
	EXPECT_EQ(SIGNAL_FREQ, freq);
}

TEST(FftTests, Oscillator)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ = 16384.0F;

	BOOL initResult = TRUE;
	COscillator osc;
	
	osc.Initialise(SIGNAL_FREQ, TEST_RATE);
			
	FLOAT expectedSin = 0.0;	
	FLOAT valueSin = 0.0;
	FLOAT errorSin = 0.0;

	FLOAT expectedCos = 0.0;
	FLOAT valueCos = 0.0;	
	FLOAT errorCos = 0.0;

	FLOAT maxErrorCos = 0.0;
	FLOAT maxErrorSin = 0.0;

	FLOAT error;

	UINT steps = 1000;
	//char blar[256];
	for(size_t count=0;count<steps;count++)
	{
		FLOAT rads = (TWO_PI/steps)*count;
		expectedSin = sin(rads);
		expectedCos = cos(rads);
		//valueCos = osc.fcosLI(rads);
		//valueSin = osc.fsinLI(rads);

		error = abs(valueSin-expectedSin);		
		if(error>maxErrorSin) maxErrorSin = error;		
		errorSin += error;

		error = abs(valueCos-expectedCos);				
		if(error>maxErrorCos) maxErrorCos = error;
		errorCos += error;
		
	}

	BOOL cumulativeSinErrorLow = errorSin<0.0000001*steps;
	BOOL cumulativeCosErrorLow = errorCos<0.0000001*steps;

	BOOL maxSinErrorLow = maxErrorSin<0.0000005;
	BOOL maxCosErrorLow = maxErrorCos<0.0000005;
	
	EXPECT_EQ(TRUE, cumulativeSinErrorLow);
	EXPECT_EQ(TRUE, cumulativeCosErrorLow);

	EXPECT_EQ(TRUE, maxSinErrorLow);
	EXPECT_EQ(TRUE, maxCosErrorLow);
}

TEST(FftTests, OverlappedInitialise)
{
	COverlappedFft ovl;
	BOOL initialised = ovl.Initialise(4096, 2048);

	ovl.Shutdown();

	EXPECT_EQ(TRUE, initialised);
}

TEST(FftTests, OverlappedInitialiseBad)
{
	COverlappedFft ovl;
	BOOL initialisedInvalid1 = ovl.Initialise(4096, 4096);
	BOOL initialisedInvalid2 = ovl.Initialise(1000, 500);
	BOOL initialisedSuccess = ovl.Initialise(32, 16);

	ovl.Shutdown();

	EXPECT_EQ(FALSE, initialisedInvalid1);
	EXPECT_EQ(FALSE, initialisedInvalid2);

	EXPECT_EQ(TRUE, initialisedSuccess);
	
}

TEST(FftTests, SpectrumDisplayCopy)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ = 16384.0F;

	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = (SIGNAL_FREQ / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;

	int size = sizeof(sizeof(COMPLEXSTRUCT));

	COscillator* osc = new COscillator();
	CFft fft;
	
	osc->Initialise(SIGNAL_FREQ, TEST_RATE);
	BOOL InitResult = fft.Initialise(NUM_BINS, 1);
		
	COMPLEXSTRUCT sample;
	BOOL resultUpdated = FALSE;
	for(size_t count=0;count<NUM_BINS;count++)
	{
		osc->GetSample(sample);
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

	EXPECT_EQ(EXPECTED_BIN, peakPos);
	EXPECT_EQ(TRUE, InitResult);
}

TEST(FftTests, SpectrumDisplaySum)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ = 16384.0F;

	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = (SIGNAL_FREQ / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;

	COscillator* osc = new COscillator();
	CFft fft;

	osc->Initialise(SIGNAL_FREQ, TEST_RATE);
	BOOL InitResult = fft.Initialise(NUM_BINS, 1);
		
	COMPLEXSTRUCT sample;
	BOOL resultUpdated = FALSE;
	for(size_t count=0;count<NUM_BINS;count++)
	{
		osc->GetSample(sample);
		fft.Add(&sample, 1, resultUpdated);
	}

	EXPECT_EQ(TRUE, resultUpdated);

	COMPLEXSTRUCT samples[NUM_BINS];
	COMPLEXSTRUCT expected[NUM_BINS];	
	
	fft.CopyResult(samples, NUM_BINS);
	
	for(size_t idx=0;idx<NUM_BINS;idx++)
	{
		FLOAT val = idx;
		
		expected[idx].fRe = samples[idx].fRe + val;
		expected[idx].fIm = samples[idx].fIm + val;

		samples[idx].fRe = val;
		samples[idx].fIm = val;
	}

	fft.SumResult(samples, NUM_BINS);
	
	fft.Shutdown();

	UINT equalsCount = 0;
	for(size_t idx=0;idx<NUM_BINS;idx++)
	{		
		if(expected[idx].fRe==samples[idx].fRe && expected[idx].fIm==samples[idx].fIm)
		{
			equalsCount++;
		}
	}
	
	EXPECT_EQ(NUM_BINS, equalsCount);
	EXPECT_EQ(TRUE, InitResult);
}

TEST(FftTests, SpectrumDisplayOverlapped)
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
}

TEST(FftTests, SpectrumDisplayOverlappedMulti)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ1 = 10000.0F;
	const FLOAT SIGNAL_FREQ2 = 16384.0F;
	const FLOAT SIGNAL_FREQ3 = 20000.0F;

	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = (SIGNAL_FREQ1 / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;

	int size = sizeof(sizeof(COMPLEXSTRUCT));

	COscillator* osc1 = new COscillator();
	COscillator* osc2 = new COscillator();
	COscillator* osc3 = new COscillator();

	COverlappedFft fft;
	CFirFilter fir;
	fir.Initialise(CFirFilter::WT_RECTANGULAR, CFirFilter::FT_BAND_PASS, TEST_RATE, 103, 15000.0F, 17000.0F);

	osc1->Initialise(SIGNAL_FREQ1, TEST_RATE);
	osc2->Initialise(SIGNAL_FREQ2, TEST_RATE);
	osc3->Initialise(SIGNAL_FREQ3, TEST_RATE);

	BOOL InitResult = fft.Initialise(NUM_BINS, 1);
		
	COMPLEXSTRUCT sample, s1, s2, s3;
	BOOL resultUpdated;
	for(size_t count=0;count<NUM_BINS;count++)
	{
		osc1->GetSample(s1);
		osc2->GetSample(s2);
		osc3->GetSample(s3);

		sample.fIm = s1.fIm + s2.fIm + s3.fIm;
		sample.fRe = s1.fRe + s2.fRe + s3.fRe;

		fir.ProcessSample(sample, sample);

		resultUpdated = FALSE;
		fft.Add(&sample, 1, resultUpdated);
	}

	EXPECT_EQ(TRUE, resultUpdated);

	COMPLEXSTRUCT samples[NUM_BINS];
	fft.CopyResult(samples, NUM_BINS);
	FLOAT peakValue[3] = { 0.0F, 0.0F, 0.0F };
	UINT peakPos[3] = {0,0,0};
	for(size_t idx=0;idx<NUM_BINS;idx++)
	{
		FLOAT value = sqrt(samples[idx].fRe*samples[idx].fRe + samples[idx].fIm*samples[idx].fIm);		
		
		if(idx>2300 && idx<2500) 
		{
			if(value > peakValue[0])
			{
				peakPos[0] = idx;
				peakValue[0] = value;
			}
		}
		if(idx>2500 && idx<2800) 
		{
			if(value > peakValue[1])
			{
				peakPos[1] = idx;
				peakValue[1] = value;
			}
		}
		if(idx>2800 && idx<3000) 
		{
			if(value > peakValue[2])
			{
				peakPos[2] = idx;
				peakValue[2] = value;
			}
		}
		
		//std::cout << idx << ", " << value << std::endl;
	}

	fft.Shutdown();

	std::cout << "[          ] peakValue[" << peakPos[0] << "] = " << peakValue[0] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[1] << "] = " << peakValue[1] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[2] << "] = " << peakValue[2] << std::endl;
	
	EXPECT_EQ(EXPECTED_BIN, peakPos[0]);
	EXPECT_EQ(TRUE, InitResult);
}




///////////////////////////


TEST(FftTests, SpectrumDisplayOverlappedMultiBartlett)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ1 = 10000.0F;
	const FLOAT SIGNAL_FREQ2 = 16384.0F;
	const FLOAT SIGNAL_FREQ3 = 20000.0F;

	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = (SIGNAL_FREQ1 / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;

	int size = sizeof(sizeof(COMPLEXSTRUCT));

	COscillator* osc1 = new COscillator();
	COscillator* osc2 = new COscillator();
	COscillator* osc3 = new COscillator();

	COverlappedFft fft;
	CFirFilter fir;
	fir.Initialise(CFirFilter::WT_BARTLETT, CFirFilter::FT_BAND_PASS, TEST_RATE, 103, 15000.0F, 17000.0F);

	osc1->Initialise(SIGNAL_FREQ1, TEST_RATE);
	osc2->Initialise(SIGNAL_FREQ2, TEST_RATE);
	osc3->Initialise(SIGNAL_FREQ3, TEST_RATE);

	BOOL InitResult = fft.Initialise(NUM_BINS, 1);
		
	COMPLEXSTRUCT sample, s1, s2, s3;
	BOOL resultUpdated;
	for(size_t count=0;count<NUM_BINS;count++)
	{
		osc1->GetSample(s1);
		osc2->GetSample(s2);
		osc3->GetSample(s3);

		sample.fIm = s1.fIm + s2.fIm + s3.fIm;
		sample.fRe = s1.fRe + s2.fRe + s3.fRe;

		fir.ProcessSample(sample, sample);

		resultUpdated = FALSE;
		fft.Add(&sample, 1, resultUpdated);
	}

	EXPECT_EQ(TRUE, resultUpdated);

	COMPLEXSTRUCT samples[NUM_BINS];
	fft.CopyResult(samples, NUM_BINS);
	FLOAT peakValue[3] = { 0.0F, 0.0F, 0.0F };
	UINT peakPos[3] = {0,0,0};
	for(size_t idx=0;idx<NUM_BINS;idx++)
	{
		FLOAT value = sqrt(samples[idx].fRe*samples[idx].fRe + samples[idx].fIm*samples[idx].fIm);		
		
		if(idx>2300 && idx<2500) 
		{
			if(value > peakValue[0])
			{
				peakPos[0] = idx;
				peakValue[0] = value;
			}
		}
		if(idx>2500 && idx<2800) 
		{
			if(value > peakValue[1])
			{
				peakPos[1] = idx;
				peakValue[1] = value;
			}
		}
		if(idx>2800 && idx<3000) 
		{
			if(value > peakValue[2])
			{
				peakPos[2] = idx;
				peakValue[2] = value;
			}
		}
		
		//std::cout << idx << ", " << value << std::endl;
	}

	fft.Shutdown();

	std::cout << "[          ] peakValue[" << peakPos[0] << "] = " << peakValue[0] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[1] << "] = " << peakValue[1] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[2] << "] = " << peakValue[2] << std::endl;
	
	EXPECT_EQ(EXPECTED_BIN, peakPos[0]);
	EXPECT_EQ(TRUE, InitResult);
}

////////////////////////////

TEST(FftTests, SpectrumDisplayOverlappedMultiBlackman)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ1 = 10000.0F;
	const FLOAT SIGNAL_FREQ2 = 16384.0F;
	const FLOAT SIGNAL_FREQ3 = 20000.0F;

	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = (SIGNAL_FREQ1 / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;

	int size = sizeof(sizeof(COMPLEXSTRUCT));

	COscillator* osc1 = new COscillator();
	COscillator* osc2 = new COscillator();
	COscillator* osc3 = new COscillator();

	COverlappedFft fft;
	CFirFilter fir;
	fir.Initialise(CFirFilter::WT_BLACKMAN, CFirFilter::FT_BAND_PASS, TEST_RATE, 103, 15000.0F, 17000.0F);

	osc1->Initialise(SIGNAL_FREQ1, TEST_RATE);
	osc2->Initialise(SIGNAL_FREQ2, TEST_RATE);
	osc3->Initialise(SIGNAL_FREQ3, TEST_RATE);

	BOOL InitResult = fft.Initialise(NUM_BINS, 1);
		
	COMPLEXSTRUCT sample, s1, s2, s3;
	BOOL resultUpdated;
	for(size_t count=0;count<NUM_BINS;count++)
	{
		osc1->GetSample(s1);
		osc2->GetSample(s2);
		osc3->GetSample(s3);

		sample.fIm = s1.fIm + s2.fIm + s3.fIm;
		sample.fRe = s1.fRe + s2.fRe + s3.fRe;

		fir.ProcessSample(sample, sample);

		resultUpdated = FALSE;
		fft.Add(&sample, 1, resultUpdated);
	}

	EXPECT_EQ(TRUE, resultUpdated);

	COMPLEXSTRUCT samples[NUM_BINS];
	fft.CopyResult(samples, NUM_BINS);
	FLOAT peakValue[3] = { 0.0F, 0.0F, 0.0F };
	UINT peakPos[3] = {0,0,0};
	for(size_t idx=0;idx<NUM_BINS;idx++)
	{
		FLOAT value = sqrt(samples[idx].fRe*samples[idx].fRe + samples[idx].fIm*samples[idx].fIm);		
		
		if(idx>2300 && idx<2500) 
		{
			if(value > peakValue[0])
			{
				peakPos[0] = idx;
				peakValue[0] = value;
			}
		}
		if(idx>2500 && idx<2800) 
		{
			if(value > peakValue[1])
			{
				peakPos[1] = idx;
				peakValue[1] = value;
			}
		}
		if(idx>2800 && idx<3000) 
		{
			if(value > peakValue[2])
			{
				peakPos[2] = idx;
				peakValue[2] = value;
			}
		}
		
		//std::cout << idx << ", " << value << std::endl;
	}

	fft.Shutdown();

	std::cout << "[          ] peakValue[" << peakPos[0] << "] = " << peakValue[0] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[1] << "] = " << peakValue[1] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[2] << "] = " << peakValue[2] << std::endl;
	
	EXPECT_EQ(EXPECTED_BIN, peakPos[0]);
	EXPECT_EQ(TRUE, InitResult);
}

/////////////////////////////////////////

TEST(FftTests, SpectrumDisplayOverlappedMultiHamming)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ1 = 10000.0F;
	const FLOAT SIGNAL_FREQ2 = 16384.0F;
	const FLOAT SIGNAL_FREQ3 = 20000.0F;

	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = (SIGNAL_FREQ1 / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;

	int size = sizeof(sizeof(COMPLEXSTRUCT));

	COscillator* osc1 = new COscillator();
	COscillator* osc2 = new COscillator();
	COscillator* osc3 = new COscillator();

	COverlappedFft fft;
	CFirFilter fir;
	fir.Initialise(CFirFilter::WT_HAMMING, CFirFilter::FT_BAND_PASS, TEST_RATE, 103, 15000.0F, 17000.0F);

	osc1->Initialise(SIGNAL_FREQ1, TEST_RATE);
	osc2->Initialise(SIGNAL_FREQ2, TEST_RATE);
	osc3->Initialise(SIGNAL_FREQ3, TEST_RATE);

	BOOL InitResult = fft.Initialise(NUM_BINS, 1);
		
	COMPLEXSTRUCT sample, s1, s2, s3;
	BOOL resultUpdated;
	for(size_t count=0;count<NUM_BINS;count++)
	{
		osc1->GetSample(s1);
		osc2->GetSample(s2);
		osc3->GetSample(s3);

		sample.fIm = s1.fIm + s2.fIm + s3.fIm;
		sample.fRe = s1.fRe + s2.fRe + s3.fRe;

		fir.ProcessSample(sample, sample);

		resultUpdated = FALSE;
		fft.Add(&sample, 1, resultUpdated);
	}

	EXPECT_EQ(TRUE, resultUpdated);

	COMPLEXSTRUCT samples[NUM_BINS];
	fft.CopyResult(samples, NUM_BINS);
	FLOAT peakValue[3] = { 0.0F, 0.0F, 0.0F };
	UINT peakPos[3] = {0,0,0};
	for(size_t idx=0;idx<NUM_BINS;idx++)
	{
		FLOAT value = sqrt(samples[idx].fRe*samples[idx].fRe + samples[idx].fIm*samples[idx].fIm);		
		
		if(idx>2300 && idx<2500) 
		{
			if(value > peakValue[0])
			{
				peakPos[0] = idx;
				peakValue[0] = value;
			}
		}
		if(idx>2500 && idx<2800) 
		{
			if(value > peakValue[1])
			{
				peakPos[1] = idx;
				peakValue[1] = value;
			}
		}
		if(idx>2800 && idx<3000) 
		{
			if(value > peakValue[2])
			{
				peakPos[2] = idx;
				peakValue[2] = value;
			}
		}
		
		//std::cout << idx << ", " << value << std::endl;
	}

	fft.Shutdown();

	std::cout << "[          ] peakValue[" << peakPos[0] << "] = " << peakValue[0] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[1] << "] = " << peakValue[1] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[2] << "] = " << peakValue[2] << std::endl;
	
	EXPECT_EQ(EXPECTED_BIN, peakPos[0]);
	EXPECT_EQ(TRUE, InitResult);
}

///////////////////////////////////

TEST(FftTests, SpectrumDisplayOverlappedMultiHanning)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ1 = 10000.0F;
	const FLOAT SIGNAL_FREQ2 = 16384.0F;
	const FLOAT SIGNAL_FREQ3 = 20000.0F;

	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = (SIGNAL_FREQ1 / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;

	int size = sizeof(sizeof(COMPLEXSTRUCT));

	COscillator* osc1 = new COscillator();
	COscillator* osc2 = new COscillator();
	COscillator* osc3 = new COscillator();

	COverlappedFft fft;
	CFirFilter fir;
	fir.Initialise(CFirFilter::WT_HANNING, CFirFilter::FT_BAND_PASS, TEST_RATE, 103, 16184.0F, 16584.0F);

	osc1->Initialise(SIGNAL_FREQ1, TEST_RATE);
	osc2->Initialise(SIGNAL_FREQ2, TEST_RATE);
	osc3->Initialise(SIGNAL_FREQ3, TEST_RATE);

	BOOL InitResult = fft.Initialise(NUM_BINS, 1);
		
	COMPLEXSTRUCT sample, s1, s2, s3;
	BOOL resultUpdated;
	for(size_t count=0;count<NUM_BINS;count++)
	{
		osc1->GetSample(s1);
		osc2->GetSample(s2);
		osc3->GetSample(s3);

		sample.fIm = s1.fIm + s2.fIm + s3.fIm;
		sample.fRe = s1.fRe + s2.fRe + s3.fRe;

		fir.ProcessSample(sample, sample);

		resultUpdated = FALSE;
		fft.Add(&sample, 1, resultUpdated);
	}

	EXPECT_EQ(TRUE, resultUpdated);

	COMPLEXSTRUCT samples[NUM_BINS];
	fft.CopyResult(samples, NUM_BINS);
	FLOAT peakValue[3] = { 0.0F, 0.0F, 0.0F };
	UINT peakPos[3] = {0,0,0};
	FLOAT runningTotal=sqrt(samples[0].fRe*samples[0].fRe + samples[0].fIm*samples[0].fIm);
	runningTotal+=sqrt(samples[1].fRe*samples[1].fRe + samples[1].fIm*samples[1].fIm);
	runningTotal+=sqrt(samples[2].fRe*samples[2].fRe + samples[2].fIm*samples[2].fIm);
	runningTotal+=sqrt(samples[3].fRe*samples[3].fRe + samples[3].fIm*samples[3].fIm);
	runningTotal+=sqrt(samples[4].fRe*samples[4].fRe + samples[4].fIm*samples[4].fIm);
	for(size_t idx=0;idx<NUM_BINS-5;idx++)
	{
//		runningTotal +=
		FLOAT value = sqrt(samples[idx].fRe*samples[idx].fRe + samples[idx].fIm*samples[idx].fIm);
		
		if(idx*2>2300 && idx<2500) 
		{
			if(value > peakValue[0])
			{
				peakPos[0] = idx;
				peakValue[0] = value;
			}
		}
		if(idx>2500 && idx<2800) 
		{
			if(value > peakValue[1])
			{
				peakPos[1] = idx;
				peakValue[1] = value;
			}
		}
		if(idx>2800 && idx<3000) 
		{
			if(value > peakValue[2])
			{
				peakPos[2] = idx;
				peakValue[2] = value;
			}
		}
		
		//std::cout << idx << ", " << value << std::endl;
	}

	fft.Shutdown();

	std::cout << "[          ] peakValue[" << peakPos[0] << "] = " << peakValue[0] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[1] << "] = " << peakValue[1] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[2] << "] = " << peakValue[2] << std::endl;
	
	EXPECT_EQ(EXPECTED_BIN, peakPos[0]);
	EXPECT_EQ(TRUE, InitResult);
}


TEST(FftTests, SpectrumDisplayPeakDetect)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ1 = 10000.0F;
	const FLOAT SIGNAL_FREQ2 = 16384.0F;
	const FLOAT SIGNAL_FREQ3 = 20000.0F;

	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN = (SIGNAL_FREQ1 / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;

	int size = sizeof(sizeof(COMPLEXSTRUCT));
		
	COscillator* osc1 = new COscillator();
	COscillator* osc2 = new COscillator();
	COscillator* osc3 = new COscillator();

	COverlappedFft fft;
	CFirFilter fir;
	fir.Initialise(CFirFilter::WT_RECTANGULAR, CFirFilter::FT_BAND_PASS, TEST_RATE, 103, 15000.0F, 17000.0F);

	osc1->Initialise(SIGNAL_FREQ1, TEST_RATE);
	osc2->Initialise(SIGNAL_FREQ2, TEST_RATE);
	osc3->Initialise(SIGNAL_FREQ3, TEST_RATE);

	BOOL InitResult = fft.Initialise(NUM_BINS, 1);
		
	COMPLEXSTRUCT sample, s1, s2, s3;
	BOOL resultUpdated;
	for(size_t count=0;count<NUM_BINS;count++)
	{
		osc1->GetSample(s1);
		osc2->GetSample(s2);
		osc3->GetSample(s3);

		sample.fIm = s1.fIm + s2.fIm + s3.fIm;
		sample.fRe = s1.fRe + s2.fRe + s3.fRe;

		fir.ProcessSample(sample, sample);

		resultUpdated = FALSE;
		fft.Add(&sample, 1, resultUpdated);
	}

	EXPECT_EQ(TRUE, resultUpdated);

	COMPLEXSTRUCT samples[NUM_BINS];
	fft.CopyResult(samples, NUM_BINS);
	
	FLOAT peakValue[3] = { 0.0F, 0.0F, 0.0F };
	UINT peakPos[3] = {0,0,0};
	for(size_t idx=0;idx<NUM_BINS;idx++)
	{
		FLOAT value = sqrt(samples[idx].fRe*samples[idx].fRe + samples[idx].fIm*samples[idx].fIm);		
		
		if(idx>2300 && idx<2500) 
		{
			if(value > peakValue[0])
			{
				peakPos[0] = idx;
				peakValue[0] = value;
			}
		}
		if(idx>2500 && idx<2800) 
		{
			if(value > peakValue[1])
			{
				peakPos[1] = idx;
				peakValue[1] = value;
			}
		}
		if(idx>2800 && idx<3000) 
		{
			if(value > peakValue[2])
			{
				peakPos[2] = idx;
				peakValue[2] = value;
			}
		}
		
		//std::cout << idx << ", " << value << std::endl;
	}

	fft.Shutdown();



	std::cout << "[          ] peakValue[" << peakPos[0] << "] = " << peakValue[0] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[1] << "] = " << peakValue[1] << std::endl;
	std::cout << "[          ] peakValue[" << peakPos[2] << "] = " << peakValue[2] << std::endl;
	
	EXPECT_EQ(EXPECTED_BIN, peakPos[0]);
	EXPECT_EQ(TRUE, InitResult);
}

TEST(FftTests, SpectrumDisplayAltPeakDetect)
{
	const FLOAT TEST_RATE = 96000.0F;
	const FLOAT SIGNAL_FREQ1 = 10500.0F;
	const FLOAT SIGNAL_FREQ2 = 16384.0F;
	const FLOAT SIGNAL_FREQ3 = 20500.0F;

	const UINT NUM_BINS = 4096;
	const UINT EXPECTED_BIN1 = (SIGNAL_FREQ1 / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;
	const UINT EXPECTED_BIN2 = (SIGNAL_FREQ2 / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;
	const UINT EXPECTED_BIN3 = (SIGNAL_FREQ3 / (TEST_RATE/NUM_BINS)) + NUM_BINS/2;

	int size = sizeof(sizeof(COMPLEXSTRUCT));
		
	COscillator* osc1 = new COscillator();
	COscillator* osc2 = new COscillator();
	COscillator* osc3 = new COscillator();

	COverlappedFft fft;
	CFirFilter fir;
	CPeakDetect peaks;
	fir.Initialise(CFirFilter::WT_RECTANGULAR, CFirFilter::FT_BAND_PASS, TEST_RATE, 103, 15000.0F, 17000.0F);		
	peaks.Initialise(NUM_BINS, 4);
	
	osc1->Initialise(SIGNAL_FREQ1, TEST_RATE);
	osc2->Initialise(SIGNAL_FREQ2, TEST_RATE);
	osc3->Initialise(SIGNAL_FREQ3, TEST_RATE);

	BOOL InitResult = fft.Initialise(NUM_BINS, 1);
		
	COMPLEXSTRUCT sample, s1, s2, s3;
	BOOL resultUpdated;
	for(size_t count=0;count<NUM_BINS;count++)
	{
		osc1->GetSample(s1);
		osc2->GetSample(s2);
		osc3->GetSample(s3);

		sample.fIm = s1.fIm + s2.fIm + s3.fIm;
		sample.fRe = s1.fRe + s2.fRe + s3.fRe;

		fir.ProcessSample(sample, sample);

		resultUpdated = FALSE;
		fft.Add(&sample, 1, resultUpdated);
	}

	EXPECT_EQ(TRUE, resultUpdated);

	COMPLEXSTRUCT samples[NUM_BINS];
	fft.CopyResult(samples, NUM_BINS);

	peaks.Process(samples, NUM_BINS);
	
	UINT peakBin[3];
	FLOAT peakValue[3];

	ULONG peakCount=3;
	//peaks.GetPeaks(&peakBin[0], &peakValue[0], &peakCount);
	
	fft.Shutdown();


	std::cout << "[          ] peakValue[" << peakBin[0] << "] = " << peakValue[0] << std::endl;
	std::cout << "[          ] peakValue[" << peakBin[1] << "] = " << peakValue[1] << std::endl;
	std::cout << "[          ] peakValue[" << peakBin[2] << "] = " << peakValue[2] << std::endl;
	
	EXPECT_EQ(EXPECTED_BIN2, peakBin[0]);
	EXPECT_EQ(EXPECTED_BIN1, peakBin[1]);
	EXPECT_EQ(EXPECTED_BIN3, peakBin[2]);

	EXPECT_EQ(TRUE, InitResult);
}
*/