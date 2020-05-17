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

/// @file Fec.h
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "fecConstants.h"

#ifndef BOOL
typedef int                 BOOL;
#endif

// forward declarations
class CViterbi;
class CCodecAO40;

class CFec
{
public:
    CFec(void);
    ~CFec(void);

    BOOL Decode(U8 *rawIn, U8 *decodedOut);
	BOOL Encode(U8 *decodedIn, U32 inBytesSize, U8 *rawOut, U32 outByteSize);

    int LastErrorCount() { return m_errorCount; }
    float LastErrorPercent() { return (100.0F * m_errorCount) / SYMPBLOCK; }
    
private:
    unsigned char m_symbols[SYMPBLOCK] ;    /* de-interleaved sync+symbols */
    unsigned char m_vitdecdata[NBITS_OUT] ;  /* array for Viterbi decoder output data */

    void Deinterleave(unsigned char *raw);

    CViterbi* m_viterbi;
	CCodecAO40* m_ao40;
    int m_errorCount;
};

