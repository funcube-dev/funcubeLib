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

/// @file IEncoder.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
class IDecoder
{
public:	
	// Process another batch of audio
	virtual void ProcessAudio(const COMPLEXSTRUCT* sampleData, int sampleCount) = 0;

	// Update the frequency at which decode occurs
	virtual void SetTuneFrequency(FLOAT signalFrequency) = 0;
	virtual FLOAT GetTuneFrequency() = 0;

	// Enable/disable adjusting tuned frequency based on error feedback
	virtual void EnableAutoTune(BOOL enable) = 0;
	// Enable publishing audio out samples
	virtual void EnableAudioOut(BOOL enable) = 0;
	
	virtual DOUBLE LastRetuneElapsedSecs() = 0;

	virtual FLOAT LastDecodeFrequency() = 0;
	virtual DOUBLE LastDecodeElapsedSecs() = 0;
	virtual DOUBLE LastDecodeFreqPerSecChange() = 0;
};