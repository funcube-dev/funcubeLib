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

/// @file BpskDecoder.h
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "decodeManager.h"
#include "memPool.h"
#include "viterbi.h"
#include "codecAO40.h"
#include "firFilter.h"
#include "oscillator.h"
#include "fec.h"
#include "sampleStopWatch.h"
#include "idecoder.h"

#define BIT_PHASE_INC_4K8 ((FLOAT)(1.0F / SAMPLE_RATE_4K8));
#define RX_CARRIER_FREQ_4K8 4800
#define BIT_RATE_4K8 4800
#define BIT_TIME_4K8 (1.0F/BIT_RATE_4K8)
#define SAMPLE_RATE_4K8 19200
#define SAMPLES_PER_BIT_4K8 (SAMPLE_RATE_4K8/BIT_RATE_4K8)
#define INTERMEDIATE_FREQ_4K8 (RX_CARRIER_FREQ_4K8*2)
#define SIGNAL_BANDWIDTH_4K8 6000.0F
#define SIGNAL_HALF_BANDWIDTH_4K8 3000.0F

#define BIT_PHASE_INC_1K2 ((FLOAT)(1.0F / SAMPLE_RATE_1K2));
#define RX_CARRIER_FREQ_1K2 1200
#define BIT_RATE_1K2 1200
#define BIT_TIME_1K2 (1.0F/BIT_RATE_1K2)
#define SAMPLE_RATE_1K2 9600
#define SAMPLES_PER_BIT_1K2 (SAMPLE_RATE_1K2/BIT_RATE_1K2)
#define INTERMEDIATE_FREQ_1K2 (RX_CARRIER_FREQ_1K2*2)
#define SIGNAL_BANDWIDTH_1K2 3000.0F
#define SIGNAL_HALF_BANDWIDTH_1K2 1500.0F

#define DOWN_SAMPLE_FILTER_SIZE 85
#define DOWN_MIX_FILTER_SIZE 127

class CBpskDecoder : public IDecoder
{
public:
    CBpskDecoder(IDecodeResult* listener, IAudioPublisher* publisher, BOOL is4k8Mode, UINT sampleRate, FLOAT fftFreqRange);
    ~CBpskDecoder(void);

    void ProcessAudio(const COMPLEXSTRUCT* sampleData, int sampleCount) override;

    void SetTuneFrequency(FLOAT signalFrequency) override;
    FLOAT GetTuneFrequency() override { return m_enableAutoTune ? m_signalFrequency + m_adjustFrequency : m_signalFrequency; }

    DOUBLE LastRetuneElapsedSecs() override { return m_lastRetuneTime.GetElapsedSeconds(); }

    FLOAT LastDecodeFrequency() { return m_lastDecodeFreq; }
    DOUBLE LastDecodeElapsedSecs() { return m_lastDecodeTime.GetElapsedSeconds(); }
    DOUBLE LastDecodeFreqPerSecChange() { return m_lastDecodeFreqPerSecChange; }

    void EnableAudioOut(BOOL enable) override { m_enableAudioOut = enable; }
    void EnableAutoTune(BOOL enable) override { m_enableAutoTune = enable; }

private:    
    void RxDownSample(const COMPLEXSTRUCT& samp);
	void RxMixToZero(const COMPLEXSTRUCT& samp);    
    void RxPutNextUCSample(const COMPLEXSTRUCT& samp);
    
    inline void AddOutSample(const COMPLEXSTRUCT& samp);

    BOOL m_enableAudioOut;
    BOOL m_enableAutoTune;

	INT m_bitRate;
	FLOAT m_bitTime;
	FLOAT m_bitPhaseInc;
	FLOAT m_decimatedSampleRate;
	FLOAT m_sampleRate;
	FLOAT m_fftFreqRange;
	INT m_samplesPerBit;
	FLOAT m_intermediateFreq;
	FLOAT m_signalBandwidth;
	FLOAT m_signalHalfBandwidth;
	FLOAT m_carrierFreq;
	const INT* m_halfBitTable;
	const FLOAT* m_rsMatchedFilter;
	INT m_rsMatchedFilterSize;

    IDecodeResult* m_decodeResultListener;
    IAudioPublisher* m_audioPublisher;
    FLOAT m_signalFrequency;
    FLOAT m_adjustFrequency;    
    INT m_errorCount;
    
    BOOL m_mixToZero;
    
    int m_bitPos;
    int m_peakPos;
    int m_newPeakPos;

    FLOAT m_bitPhasePos;

    COMPLEXSTRUCT m_csLastIQ;

    int m_downSampleCount;
    
    U8 m_softBits[5200];
    U8 m_decodedBytes[BLOCK_SIZE];
    S8 m_hardBits[5200];
            
    CFirFilter m_lowPassDecimating;    
    CFirFilter m_matchedFilter;

    CRollingAverage m_syncEnergyAvg4k8[SAMPLES_PER_BIT_4K8+2];
	CRollingAverage m_syncEnergyAvg1k2[SAMPLES_PER_BIT_1K2+2];
	CRollingAverage* m_syncEnergyAvg;
    CRollingAverage m_freqError;

	COscillator m_zeroDownMix;
	COscillator m_audioOutMix;

    CFec m_fec;

    fc::AutoBufPtr m_audioOut;    
    CSampleStopWatch m_lastDecodeTime;
    CSampleStopWatch m_lastRetuneTime;
    DOUBLE m_lastDecodeFreqPerSecChange;
    
    FLOAT m_lastDecodeFreq;
};