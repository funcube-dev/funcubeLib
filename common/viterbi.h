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

/// @file Viterbi.h
//
//////////////////////////////////////////////////////////////////////
/* Viterbi decoder for arbitrary convolutional code
 * viterbi27 and viterbi37 for the r=1/2 and r=1/3 K=7 codes are faster
 * Copyright 1997 Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL) 
 */
#pragma once

#include "FECConstants.h"

class CViterbi
{
public:
    CViterbi(void);
    virtual ~CViterbi(void);

    int Decode27(
        unsigned char *insymbols,     // Raw deinterleaved input symbols
        unsigned int insymbols_size,  // size of input symbol buffer
        unsigned char *outdata,       // Decoded output data
        unsigned int outdata_size,    // size of output buffer
        unsigned int nbits );          // Number of output bits

private:
    unsigned long m_paths[NBITS*2*sizeof(unsigned long)];
};

