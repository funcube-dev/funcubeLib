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

/// @file 
//
//////////////////////////////////////////////////////////////////////
#include "funcubeLib.h"

#include "fecConstants.h"
#include "viterbi.h"
#include "codecAO40.h"

#include "fec.h"

CFec::CFec(void)
{
    m_viterbi = new CViterbi();
    m_ao40 = new CCodecAO40();
    m_errorCount = 0;
}

CFec::~CFec(void)
{
    delete m_viterbi;
    delete m_ao40;
}

// take a buffer de-interleave it, result stored in m_symbols
void CFec::Deinterleave(unsigned char *raw)
{
    /* Input  array:  raw   */
    /* Output array:  symbols */
    int col,row;
    int coltop,rowstart;

    coltop=0;
    for(col=1;col<ROWS;col++) /* Skip first column as it's the sync vector */
    {          
        rowstart=0;
        for(row=0;row<COLUMNS;row++)
        {
            m_symbols[coltop+row]=raw[rowstart+col];  /* coltop=col*65 ; rowstart=row*80 */
            rowstart+=ROWS;
        }
        coltop+=COLUMNS;
    }
}

BOOL CFec::Decode(unsigned char *raw, unsigned char *decoded_data)
{    
    // Step 1: De-interleave
    Deinterleave(raw);

    // Step 2: Viterbi decoder
    m_viterbi->Decode27(m_symbols, SYMPBLOCK, m_vitdecdata, NBITS_OUT, NBITS);    

    // Step 3: RS decoder
    int rserrs = m_ao40->decode(m_vitdecdata, decoded_data);
    
    // Step 4: Optional: Re-encode o/p and count errors
    if(rserrs != -1)
    {            
		m_ao40->encode(decoded_data, BLOCKSIZE);
        m_errorCount = m_ao40->count_errors(raw);
    }
    else
    {
        m_errorCount = -1;
    }

    return rserrs != -1;
}

BOOL  CFec::Encode(unsigned char *decodedIn, const unsigned int inBytesSize, unsigned char *rawOut, const unsigned int outByteSize) {
	if (inBytesSize != BLOCKSIZE) {
		return FALSE;
	}
	if (outByteSize != SYMPBLOCK) {
		return FALSE;
	}

	memcpy(rawOut, m_ao40->encode(decodedIn, inBytesSize), outByteSize);
	return TRUE;
}


