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
#include "math.h"
#include "Oscillator.h"


COscillator::COscillator(void)
{
    m_currentPhase = 0.0;
}

COscillator::~COscillator(void)
{	
}

void COscillator::Initialise(FLOAT frequency, FLOAT sampleRate)
{    
    m_sampleRate = sampleRate;
    SetFrequency(frequency);
}

void COscillator::SetFrequency(FLOAT frequency)
{
    if(m_frequency != frequency)
    {
        m_validPhaseInc = FALSE;
        m_frequency = frequency;    
        SyncPhaseIncFromFrequency();
    }
}

void COscillator::SetPhaseInc(FLOAT phaseInc)
{
    if(m_phaseInc != phaseInc)
    {
        m_validFreq = FALSE;
        m_phaseInc = phaseInc;
        SyncFrequencyFromPhaseInc();
    }
}

const SAMPLE COscillator::GetSample()
{
	COMPLEXSTRUCT samp;
	GetSample(samp);
	return samp.fRe;
}

void COscillator::GetSample(COMPLEXSTRUCT& outSample)
{
	outSample.fRe = fcos(m_currentPhase);
	outSample.fIm = fsin(m_currentPhase);

    m_currentPhase+=m_phaseInc;
    if (m_currentPhase>=TWO_PI)
    {
        m_currentPhase-=TWO_PI;
    } 
    else if(m_currentPhase<0)
    {
        m_currentPhase+=TWO_PI;
    }
}

void COscillator::MixSample(const COMPLEXSTRUCT& inSample, COMPLEXSTRUCT& outSample)
{    
    COMPLEXSTRUCT oscSample;
    GetSample(oscSample);

    // inSample might == outSample so 
    COMPLEXSTRUCT mixed;
    mixed.fRe = oscSample.fRe*inSample.fRe+oscSample.fIm*inSample.fIm;
    mixed.fIm = oscSample.fRe*inSample.fIm-oscSample.fIm*inSample.fRe;
    
    outSample = mixed;
}

void COscillator::MixSample(const FLOAT& inSample, COMPLEXSTRUCT& outSample)
{
    GetSample(outSample);
    outSample.fRe*=inSample;
    outSample.fIm*=inSample;
}

void COscillator::MixSample(const FLOAT& inSample, FLOAT& outSample)
{
	COMPLEXSTRUCT oscSample;
	GetSample(oscSample);
	outSample = oscSample.fRe * inSample;
}

void COscillator::MixSample(const COMPLEXSTRUCT& inSample, FLOAT& outSample)
{
	COMPLEXSTRUCT oscSample;
	GetSample(oscSample);
		
	outSample = oscSample.fRe*inSample.fRe + oscSample.fIm*inSample.fIm;	
}

void COscillator::SyncPhaseIncFromFrequency()
{
    if(m_validPhaseInc)
    {
        return;
    }

    m_phaseInc=(m_frequency*TWO_PI)/m_sampleRate;
	// dont try if the frequency is too high
    if(m_phaseInc>PI || m_phaseInc<-PI)
    {
        m_phaseInc = 0;
    }
    
    m_validPhaseInc = TRUE;
}

void COscillator::SyncFrequencyFromPhaseInc()
{
    if(m_validFreq)
    {
        return;
    }

	// check if the frequency is too high
    if(m_phaseInc>PI || m_phaseInc<-PI)
    {
        m_phaseInc = 0;
    }

	m_frequency = (m_sampleRate*m_phaseInc)/TWO_PI;
}

FLOAT COscillator::AddFrequency(FLOAT incrementValue)
{
    SyncFrequencyFromPhaseInc();
    m_frequency+=incrementValue;
    m_validPhaseInc=FALSE;
    m_validFreq=TRUE;

    return m_frequency;
}
    
FLOAT COscillator::AddPhaseInc(FLOAT incrementValue)
{
    SyncPhaseIncFromFrequency();
    m_phaseInc+=incrementValue;
    m_validPhaseInc=TRUE;
    m_validFreq=FALSE;

	// check if the frequency is too high
    if(m_phaseInc>PI || m_phaseInc<-PI)
    {   
        m_phaseInc = 0;
    }

    return m_phaseInc;
}

FLOAT COscillator::AddPhase(FLOAT incrementValue) 
{ 
	m_currentPhase += incrementValue; 

	while(m_currentPhase>=TWO_PI)
    {
        m_currentPhase-=TWO_PI;
    }
	while(m_currentPhase<0)
    {
        m_currentPhase+=TWO_PI;
    }

	return m_currentPhase;
}