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

/// @file Oscillator.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "FuncubeLib.h"

class COscillator
{
public:
    COscillator(void);
    virtual ~COscillator(void);

    void Initialise(FLOAT frequency, FLOAT sampleRate);

	const SAMPLE GetSample();
    void GetSample(COMPLEXSTRUCT& outSample);
    void MixSample(const COMPLEXSTRUCT& inSample, COMPLEXSTRUCT& outSample);
    void MixSample(const FLOAT& inSample, COMPLEXSTRUCT& outSample);
	void MixSample(const COMPLEXSTRUCT& inSample, FLOAT& outSample);	
	void MixSample(const FLOAT& inSample, FLOAT& outSample);

    FLOAT GetFrequency() { SyncFrequencyFromPhaseInc(); return m_frequency; }
    void SetFrequency(FLOAT frequency);
    FLOAT AddFrequency(FLOAT incrementValue);

    FLOAT GetPhaseInc() { SyncPhaseIncFromFrequency(); return m_phaseInc; }
	void SetPhaseInc(FLOAT phaseInc);
    FLOAT AddPhaseInc(FLOAT incrementValue);

    FLOAT GetPhase() { return m_currentPhase; }
    FLOAT AddPhase(FLOAT incrementValue);

	inline FLOAT fcos(FLOAT rads) { return cos(rads); }
	inline FLOAT fsin(FLOAT rads) { return sin(rads); }

private:
	void SyncFrequencyFromPhaseInc();
	void SyncPhaseIncFromFrequency();

	DOUBLE m_frequency;
    DOUBLE m_sampleRate;
    DOUBLE m_phaseInc;
    DOUBLE m_currentPhase;

    BOOL m_validFreq;
    BOOL m_validPhaseInc;
};
