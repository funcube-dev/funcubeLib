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

/// @file BpskDecoder.cpp
//
//////////////////////////////////////////////////////////////////////
#include "funcubeLib.h"

#include "viterbi.h"
#include "codecAO40.h"
#include "bpskDecoder.h"
#include <string.h>
#include <math.h>

#define INTERLEAVE_ROWS       80            /* Block interleaver rows */
#define INTERLEAVE_COLUMNS    65            /* Block interleaver columns */

const INT BLOCK_COUNT = 640;

// indexed by 
const INT halfBitTable1k2[SAMPLES_PER_BIT_1K2] = {4,5,6,7,0,1,2,3};
const INT halfBitTable4k8[SAMPLES_PER_BIT_4K8] = { 2,3,0,1 };

const S8 SYNC_VECTOR_VALUES[INTERLEAVE_COLUMNS]={1,1,1,1,1,1,1,-1,-1,-1,-1,1,1,1,-1,1,1,1,1,-1,-1,1,-1,1,1,-1,-1,1,-1,-1,1,-1,-1,-1,-1,-1,-1,1,-1,-1,-1,1,-1,-1,1,1,-1,-1,-1,1,-1,1,1,1,-1,1,-1,1,1,-1,1,1,-1,-1,-1};

const INT RX_MATCHED_FILTER_SIZE_1K2 = 65;
const FLOAT RX_MATCHED_FILTER_VALUES_1K2[RX_MATCHED_FILTER_SIZE_1K2]=
{
    -0.0101130691F,-0.0086975143F,-0.0038246093F,+0.0033563764F,+0.0107237026F,+0.0157790936F,+0.0164594107F,+0.0119213911F,
    +0.0030315224F,-0.0076488191F,-0.0164594107F,-0.0197184277F,-0.0150109226F,-0.0023082460F,+0.0154712381F,+0.0327423589F,
    +0.0424493086F,+0.0379940454F,+0.0154712381F,-0.0243701991F,-0.0750320094F,-0.1244834076F,-0.1568500423F,-0.1553748911F,
    -0.1061032953F,-0.0015013786F,+0.1568500423F,+0.3572048240F,+0.5786381191F,+0.7940228249F,+0.9744923010F,+1.0945250059F,
    +1.1366117829F,+1.0945250059F,+0.9744923010F,+0.7940228249F,+0.5786381191F,+0.3572048240F,+0.1568500423F,-0.0015013786F,
    -0.1061032953F,-0.1553748911F,-0.1568500423F,-0.1244834076F,-0.0750320094F,-0.0243701991F,+0.0154712381F,+0.0379940454F,
    +0.0424493086F,+0.0327423589F,+0.0154712381F,-0.0023082460F,-0.0150109226F,-0.0197184277F,-0.0164594107F,-0.0076488191F,
    +0.0030315224F,+0.0119213911F,+0.0164594107F,+0.0157790936F,+0.0107237026F,+0.0033563764F,-0.0038246093F,-0.0086975143F,
    -0.0101130691F
};

const INT RX_MATCHED_FILTER_SIZE_4K8 = 11;
const FLOAT RX_MATCHED_FILTER_VALUES_4K8[RX_MATCHED_FILTER_SIZE_4K8] =
{
	-0.0130F, -0.0194,	0.0595,	0.395,	0.898,	1.15, 0.898,	0.395,	0.0595, -0.0194, -0.0130F
};

CBpskDecoder::CBpskDecoder(IDecodeResult* listener, IAudioPublisher* publisher, BOOL is4k8Mode, UINT sampleRate, FLOAT fftFreqRange)
{
    m_enableAudioOut = FALSE;    
    m_enableAutoTune = TRUE;
    m_decodeResultListener = listener;
    m_audioPublisher = publisher;
	m_sampleRate = sampleRate;
	m_fftFreqRange = fftFreqRange;
    m_lastDecodeTime.Initialise(sampleRate);
    m_lastRetuneTime.Initialise(sampleRate);
	
	if (is4k8Mode) {
		m_bitRate = BIT_RATE_4K8;
		m_bitTime = BIT_TIME_4K8;
		m_bitPhaseInc = BIT_PHASE_INC_4K8;
		m_decimatedSampleRate = SAMPLE_RATE_4K8;
		m_samplesPerBit = SAMPLES_PER_BIT_4K8;
		m_intermediateFreq = INTERMEDIATE_FREQ_4K8;
		m_signalBandwidth = SIGNAL_BANDWIDTH_4K8;
		m_signalHalfBandwidth = SIGNAL_HALF_BANDWIDTH_4K8;
		m_carrierFreq = RX_CARRIER_FREQ_4K8;
		m_halfBitTable = &halfBitTable4k8[0];
		m_syncEnergyAvg = &m_syncEnergyAvg4k8[0];
		m_rsMatchedFilter = &RX_MATCHED_FILTER_VALUES_4K8[0];
		m_rsMatchedFilterSize = RX_MATCHED_FILTER_SIZE_4K8;
	}
	else 
	{
		m_bitRate = BIT_RATE_1K2;
		m_bitTime = BIT_TIME_1K2;
		m_bitPhaseInc = BIT_PHASE_INC_1K2;
		m_decimatedSampleRate = SAMPLE_RATE_1K2;
		m_samplesPerBit = SAMPLES_PER_BIT_1K2;
		m_intermediateFreq = INTERMEDIATE_FREQ_1K2;
		m_signalBandwidth = SIGNAL_BANDWIDTH_1K2;
		m_signalHalfBandwidth = SIGNAL_HALF_BANDWIDTH_1K2;
		m_carrierFreq = RX_CARRIER_FREQ_1K2;
		m_halfBitTable = &halfBitTable1k2[0];
		m_syncEnergyAvg = &m_syncEnergyAvg1k2[0];
		m_rsMatchedFilter = &RX_MATCHED_FILTER_VALUES_1K2[0];
		m_rsMatchedFilterSize = RX_MATCHED_FILTER_SIZE_1K2;
	}

    m_peakPos=0;
    m_newPeakPos=0;
    m_bitPos=0;
    m_bitPhasePos=0.0F;
    m_csLastIQ.fRe = 0.0F;
    m_csLastIQ.fIm = 0.0F;

    m_downSampleCount = 0;
    m_adjustFrequency = 0.0F;
	m_signalFrequency = 0.0F;
    m_errorCount = -1;
    m_mixToZero = TRUE;    
    
    m_lastDecodeFreqPerSecChange = 0.0;

    m_freqError = 0.0F;

    memset(m_hardBits,0, sizeof(m_hardBits));

    m_freqError.SetAverageSamples(59.0F);

    for (int i=0;i<m_samplesPerBit+2;i++)
    {
        m_syncEnergyAvg[i].SetAverageSamples(144.0F);
    }
	    
    m_lastDecodeFreq = m_fftFreqRange + 1.0;

    m_lowPassDecimating.Initialise(
		CFirFilter::WT_BLACKMAN,
        CFirFilter::FT_LOW_PASS,
        m_sampleRate,
        131,
        m_signalHalfBandwidth*1.2,
		m_signalBandwidth);

    m_matchedFilter.Initialise(m_rsMatchedFilter, m_rsMatchedFilterSize);

    m_zeroDownMix.Initialise(1000, m_sampleRate);
	m_audioOutMix.Initialise(m_intermediateFreq, m_decimatedSampleRate);
}

CBpskDecoder::~CBpskDecoder(void)
{
}

void CBpskDecoder::SetTuneFrequency(FLOAT signalFrequency) {
    if (m_signalFrequency != signalFrequency)
    {
        m_signalFrequency = signalFrequency;
        m_adjustFrequency = 0.0;
        m_freqError.SetAverage(0.0F);
        m_lastRetuneTime.Restart();
        // As we have re-tuned existing decode times etc count for 
        // nothing, so reset them
        m_lastDecodeTime.Clear();
        m_lastDecodeFreqPerSecChange = 0.0;
    }
}

void CBpskDecoder::ProcessAudio(const COMPLEXSTRUCT* sampleData, int sampleCount)
{   
    // update the timers (based on samples not elapsedtime)    
    m_lastDecodeTime.IncrementCount(sampleCount);
    m_lastRetuneTime.IncrementCount(sampleCount);
        
    FLOAT centreFrequency = m_signalFrequency;

    if(m_enableAutoTune)
    {
        centreFrequency+=m_adjustFrequency;
    }
    // do we need to skip the final mix to zero (signal already there)
    m_mixToZero = !(centreFrequency < 50 && centreFrequency > -50);

    // use freq that matches the signal so ends up at zero        
    m_zeroDownMix.SetFrequency(centreFrequency);
        
    for(ULONG i=0;i<sampleCount;i++)
    {
        RxMixToZero(sampleData[i]);       
    }
}

inline void CBpskDecoder::AddOutSample(const COMPLEXSTRUCT& samp)
{
    if (!m_enableAudioOut || NULL == m_audioPublisher)
    {
        return;
    }
        
    if(NULL == m_audioOut.get())
    {
        m_audioOut = m_audioPublisher->GetEmptyOutputAudioBuffer();
    }

	SAMPLE realSamp;
	m_audioOutMix.MixSample(samp, realSamp);
	*(SAMPLE*)(m_audioOut->Data() + m_audioOut->Size()) = realSamp;
	m_audioOut->Size() += sizeof(realSamp);
	if (m_audioOut->Size() == m_audioOut->Capacity())
	{
		m_audioPublisher->PublishAudioOutput(m_audioOut);
		m_audioOut = m_audioPublisher->GetEmptyOutputAudioBuffer();
	}    
}

void CBpskDecoder::RxMixToZero(const COMPLEXSTRUCT& samp) // Mix from carrier to zero
{
	// Complex downconversion...    
	COMPLEXSTRUCT sampMixed;
	if (m_mixToZero)
	{
		m_zeroDownMix.MixSample(samp, sampMixed);
	}
	else
	{
		sampMixed = samp;
	}

	RxDownSample(sampMixed);
}

void CBpskDecoder::RxDownSample(const COMPLEXSTRUCT& samp) // Downsample from 96000 to 9600SPS
{    
	// Only need to produce a filtered result every now and again
    if (--m_downSampleCount < 0)
    {   
		// TODO check this is a whole multiple else we'll have problems...
        m_downSampleCount = m_sampleRate / m_decimatedSampleRate - 1;

        COMPLEXSTRUCT filteredSamp;
        m_lowPassDecimating.ProcessSample(samp, filteredSamp);

		AddOutSample(filteredSamp);
        RxPutNextUCSample(filteredSamp);
    }
    else
    {
        m_lowPassDecimating.ProcessSample(samp);
    }
}

void CBpskDecoder::RxPutNextUCSample(const COMPLEXSTRUCT& sampMixed)
{
    // Takes upconverted samples
    FLOAT fRe=0.0F;
    FLOAT fIm=0.0F;
    int i;    
	COMPLEXSTRUCT sampOut;

    m_matchedFilter.ProcessSample(sampMixed, sampOut);
    
    fRe=sampOut.fRe;
    fIm=sampOut.fIm;

    // Bit clock detection
    {
        FLOAT fEnergy = sqrt(fRe*fRe + fIm*fIm);

        m_syncEnergyAvg[m_bitPos].Average(fEnergy);
        if (m_bitPos==m_peakPos)
        {
            COMPLEXSTRUCT csVect;
            FLOAT fEnergy2;

            csVect.fRe=-(m_csLastIQ.fRe*fRe+ m_csLastIQ.fIm*fIm); // DBPSK!
            csVect.fIm=(m_csLastIQ.fRe*fIm- m_csLastIQ.fIm*fRe); // For use in AFC
            
            m_csLastIQ.fRe=fRe;
            m_csLastIQ.fIm=fIm;

            fEnergy2=(FLOAT)sqrt(csVect.fRe*csVect.fRe+csVect.fIm*csVect.fIm);
            if(fEnergy2!=0.0)
            {
                if(csVect.fRe<0.0F)
                {
					m_freqError.Average((csVect.fIm) / fEnergy2);
                }
                else
                {
					m_freqError.Average(-csVect.fIm / fEnergy2);
                    
                }                
                m_adjustFrequency += m_freqError.GetAverage();				
                            
                // Insert CalcPhaseError + other stuff here
                BOOL bBit=csVect.fRe<0.0F;
                
                memmove(m_hardBits, m_hardBits+1, 5200-1);
                memmove(m_softBits, m_softBits+1, 5200-1);

                m_hardBits[5200-1]=(bBit ? 1 : -1);                

                // scale 1.0 to -1.0 value to the range 32-224
                // for input to the soft decision viterbi
                m_softBits[5200-1] = (U8)128 - csVect.fRe/fEnergy2 * 96.0;

                int i, nSum=0;;
                for (i=0;i<INTERLEAVE_COLUMNS;i++)
                {
                    nSum+=m_hardBits[i*INTERLEAVE_ROWS]*SYNC_VECTOR_VALUES[i];                        
                }
        
                if (nSum>37)
                {
                    if (m_fec.Decode(m_softBits,m_decodedBytes))
                    {
                        // calculate rate of change of frequency since last decode                        
						FLOAT decodeFreq = m_signalFrequency;
						if (m_enableAutoTune)
						{
							decodeFreq += m_adjustFrequency;
						}
                        FLOAT decodeFreqDiff = m_lastDecodeFreq - decodeFreq;
                        if (m_lastDecodeFreq > m_fftFreqRange) {
                            FLOAT elapsedSecs = LastDecodeElapsedSecs();
                            m_lastDecodeFreqPerSecChange = elapsedSecs > 0.0 ? decodeFreqDiff / elapsedSecs : 0.0;
                        }                        
						m_lastDecodeFreq = decodeFreq;
                        // update last decode time, prevents worker being re-purposed
                        m_lastDecodeTime.Start();

                        if (NULL != m_decodeResultListener)
                        {
                            m_decodeResultListener->OnDecodeSuccess(m_decodedBytes, sizeof(m_decodedBytes), m_lastDecodeFreq, m_fec.LastErrorCount());
                        }
                    }                    
                }
            }
		}

        // switch to the new canditate bit position at the best time (half way into next bit)
        if (m_bitPos==m_halfBitTable[m_peakPos] && m_peakPos!=m_newPeakPos)
        {
            m_peakPos = m_newPeakPos;
        }

        m_bitPos++;
        if (m_bitPos>=m_samplesPerBit)
        {
            m_bitPos=0;
        }
		m_bitPhasePos += m_bitPhaseInc;

        // update candidate bit position at bit_time intervals
        if (m_bitPhasePos >= m_bitTime) // Each bit time...
        {
            m_bitPhasePos=(FLOAT)(m_bitPhasePos-m_bitTime);
            m_bitPos=0;

            DOUBLE fMax=m_syncEnergyAvg[0].GetAverage(); // seed with first value
            m_newPeakPos=0; // the seed max is from index 0
            for (i=1;i<m_samplesPerBit;i++) // Find max energy over past samples
            {    
                if (m_syncEnergyAvg[i].GetAverage()>fMax)
                {
                    m_newPeakPos=i;
                    fMax=m_syncEnergyAvg[i].GetAverage();
                }
            }
        }
    }
}
