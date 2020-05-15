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

/// @file CRollingAverageComplex.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "FuncubeLib.h"
#include "RollingAverage.h"
#include "complex.h"
#include "fftw3.h"

class CRollingAverageComplex
{
public:
	CRollingAverageComplex(void) { }
	CRollingAverageComplex(const COMPLEXSTRUCT initialValue) {
		m_aveRe.SetAverage(initialValue.fRe);
		m_aveIm.SetAverage(initialValue.fIm);
	}

	~CRollingAverageComplex(void) { }

	COMPLEXSTRUCT GetAverageFactor() {
		COMPLEXSTRUCT ret = { m_aveRe.GetAverageFactor(), m_aveIm.GetAverageFactor() };
		return ret;
	}
	void SetAverageFactor(const COMPLEXSTRUCT value) {
		m_aveRe.SetAverageFactor(value.fRe);
		m_aveIm.SetAverageFactor(value.fIm);
	}
	void SetAverageFactor(const FLOAT value) {
		m_aveRe.SetAverageFactor(value);
		m_aveIm.SetAverageFactor(value);
	}

	COMPLEXSTRUCT GetAverageSamples() {
		COMPLEXSTRUCT ret = { m_aveRe.GetAverageSamples(), m_aveIm.GetAverageSamples() };
		return ret;
	}
    void SetAverageSamples(const COMPLEXSTRUCT value)	{
		m_aveRe.SetAverageSamples(value.fRe);
		m_aveIm.SetAverageSamples(value.fIm);
	}
	void SetAverageSamples(const FLOAT value) {
		m_aveRe.SetAverageSamples(value);
		m_aveIm.SetAverageSamples(value);
	}

	inline void Average(const COMPLEXSTRUCT value) {
		m_aveRe.Average(value.fRe);
		m_aveIm.Average(value.fIm);
	}
	inline void Average(const FLOAT re, const FLOAT im) {
		m_aveRe.Average(re);
		m_aveIm.Average(im);
	}
	inline void Average(const COMPLEXSTRUCT value, COMPLEXSTRUCT* out) {
		out->fRe = m_aveRe.Average(value.fRe);
		out->fIm = m_aveIm.Average(value.fIm);
	}
	inline void Average(const FLOAT re, const FLOAT im, COMPLEXSTRUCT* out) {
		out->fRe = m_aveRe.Average(re);
		out->fIm = m_aveIm.Average(im);
	}
	COMPLEXSTRUCT GetAverage() {
		COMPLEXSTRUCT ret = { m_aveRe.GetAverage(), m_aveIm.GetAverage() };
		return ret;
	}
    void SetAverage(const COMPLEXSTRUCT value) {
		m_aveRe.SetAverage(value.fRe);
		m_aveIm.SetAverage(value.fIm);
	}
	void SetAverage(FLOAT value) {
		m_aveRe.SetAverage(value);
		m_aveIm.SetAverage(value);
	}
    
private:
	CRollingAverage m_aveRe;
	CRollingAverage m_aveIm;
};