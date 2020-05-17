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

/// @file Decode.h
//
//////////////////////////////////////////////////////////////////////
#include <string.h>
#include "viterbi.h"

/* --------------- */
/* Viterbi decoder */
/* --------------- */


/* Viterbi decoder for arbitrary convolutional code
 * viterbi27 and viterbi37 for the r=1/2 and r=1/3 K=7 codes are faster
 * Copyright 1997 Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL) 
 */

/* This is a bare bones <2,7> Viterbi decoder, adapted from a general purpose model.
 * It is not optimised in any way, neither as to coding (for example the memcopy should
 * be achievable simply by swapping pointers), nor as to simplifying the metric table,
 * nor as to using any machine-specific smarts.  On contemporary machines, in this application,
 * the execution time is negligible.  Many ideas for optimisation are contained in PK's www pages.
 * The real ADC is 8-bit, though in practice 4-bits are actually sufficient.
 * Descriptions of the Viterbi decoder algorithm can be found in virtually any book
 * entitled "Digital Communications".  (JRM)
 */


/* Table of symbol pair emitted for each state of the convolutional
 encoder's shift register */ 
const int viterbi_syms[]={
  1, 2, 3, 0, 2, 1, 0, 3, 2, 1, 0, 3, 1, 2, 3, 0,
  1, 2, 3, 0, 2, 1, 0, 3, 2, 1, 0, 3, 1, 2, 3, 0,
  0, 3, 2, 1, 3, 0, 1, 2, 3, 0, 1, 2, 0, 3, 2, 1,
  0, 3, 2, 1, 3, 0, 1, 2, 3, 0, 1, 2, 0, 3, 2, 1,
  2, 1, 0, 3, 1, 2, 3, 0, 1, 2, 3, 0, 2, 1, 0, 3,
  2, 1, 0, 3, 1, 2, 3, 0, 1, 2, 3, 0, 2, 1, 0, 3,
  3, 0, 1, 2, 0, 3, 2, 1, 0, 3, 2, 1, 3, 0, 1, 2,
  3, 0, 1, 2, 0, 3, 2, 1, 0, 3, 2, 1, 3, 0, 1, 2, 
};

/* Tables for Viterbi r=1/2 k=7 decoder to CCSDS standard */
/* ------------------------------------------------------ */

/* Metric table, [sent sym][rx symbol] */
/* This metric table is for an 8-bit ADC, which is total overkill!
   Simplify later.  128-i and i-128 would probably do!  jrm */

const int viterbi_mettab[2][256]={
  20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
  20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
  20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
  20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
  20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
  20,   20,   20,   20,   20,   20,   20,   19,   19,   19,   19,   19,   19,   19,   19,   19,
  19,   19,   18,   18,   18,   18,   18,   18,   17,   17,   17,   16,   16,   16,   15,   15,
  14,   14,   13,   13,   12,   11,   10,   10,    9,    8,    7,    6,    5,    3,    2,    1,
  -1,   -2,   -4,   -5,   -7,   -9,  -11,  -13,  -15,  -17,  -19,  -21,  -23,  -25,  -28,  -30,
 -32,  -35,  -37,  -40,  -42,  -45,  -47,  -50,  -52,  -55,  -58,  -60,  -63,  -66,  -68,  -71,
 -74,  -77,  -79,  -82,  -85,  -88,  -90,  -93,  -96,  -99, -102, -104, -107, -110, -113, -116,
-119, -121, -124, -127, -130, -133, -136, -138, -141, -144, -147, -150, -153, -155, -158, -161,
-164, -167, -170, -172, -175, -178, -181, -184, -187, -190, -192, -195, -198, -201, -204, -207,
-210, -212, -215, -218, -221, -224, -227, -229, -232, -235, -238, -241, -244, -247, -249, -252,
-255, -258, -261, -264, -267, -269, -272, -275, -278, -281, -284, -286, -289, -292, -295, -298,
-301, -304, -306, -309, -312, -315, -318, -320, -324, -326, -329, -332, -335, -337, -341, -372,

-372, -341, -338, -335, -332, -329, -326, -324, -321, -318, -315, -312, -309, -306, -304, -301,
-298, -295, -292, -289, -286, -284, -281, -278, -275, -272, -269, -267, -264, -261, -258, -255,
-252, -249, -247, -244, -241, -238, -235, -232, -229, -227, -224, -221, -218, -215, -212, -210,
-207, -204, -201, -198, -195, -192, -190, -187, -184, -181, -178, -175, -172, -170, -167, -164,
-161, -158, -155, -153, -150, -147, -144, -141, -138, -136, -133, -130, -127, -124, -121, -119,
-116, -113, -110, -107, -104, -102,  -99,  -96,  -93,  -90,  -88,  -85,  -82,  -79,  -77,  -74,
 -71,  -68,  -66,  -63,  -60,  -58,  -55,  -52,  -50,  -47,  -45,  -42,  -40,  -37,  -35,  -32,
 -30,  -28,  -25,  -23,  -21,  -19,  -17,  -15,  -13,  -11,   -9,   -7,   -5,   -4,   -2,   -1,
   1,    2,    3,    5,    6,    7,    8,    9,   10,   10,   11,   12,   13,   13,   14,   14,
  15,   15,   16,   16,   16,   17,   17,   17,   18,   18,   18,   18,   18,   18,   19,   19,
  19,   19,   19,   19,   19,   19,   19,   19,   19,   20,   20,   20,   20,   20,   20,   20,
  20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
  20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
  20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
  20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
  20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,   20,
};


CViterbi::CViterbi(void)
{	
}

CViterbi::~CViterbi(void)
{	
}

int CViterbi::Decode27(	
	unsigned char *insymbols,     // Raw deinterleaved input symbols
	unsigned int insymbols_size,  // size of input symbol buffer
	unsigned char *outdata,       // Decoded output data
	unsigned int outdata_size,    // size of output buffer
	unsigned int nbits )		  // Number of output bits
{
	// TODO check we don't try and read too much from insymbols

	if(NBITS != nbits) // paths buffer is sized for only NBITS
	{
		return -1;
	}

	unsigned int bitcnt = 0;
	int beststate,i,j;
	long cmetric[64], nmetric[64];    /* 2^(K-1) */
	unsigned long *pp;
	long m0,m1,mask;
	int mets[4];                     /* 2^N */        
		
	memset(m_paths, 0, sizeof(m_paths));
	pp = m_paths;

	/* Initialize starting metrics to prefer 0 state */
	cmetric[0] = 0;
	for(i=1;i< 64;i++)
		cmetric[i] = -999999;

	for(;;){
		/* Read 2 input symbols and compute the 4 branch metrics */
		for(i=0;i<4;i++){
			mets[i] = 0;
			for(j=0;j<2;j++){
				mets[i] += viterbi_mettab[(i >> (1-j)) & 1][insymbols[j]];
			}
		}
		insymbols += 2;
		mask = 1;
		for(i=0;i<64;i+=2){
			int b1,b2;

			b1 = mets[viterbi_syms[i]];
			nmetric[i] = m0 = cmetric[i/2] + b1;
			b2 = mets[viterbi_syms[i+1]];
			b1 -= b2;
			m1 = cmetric[(i/2) + (1<<(K-2))] + b2;
			if(m1 > m0){
				nmetric[i] = m1;
				*pp |= mask;
			}
			m0 -= b1;
			nmetric[i+1] = m0;
			m1 += b1;
			if(m1 > m0){
				nmetric[i+1] = m1;
				*pp |= mask << 1;
			}
			mask = (mask << 2) & 0xffffffff;
			if(mask == 0){
				mask = 1;
				pp++;
			}
		}
		if(mask != 1)
			pp++;
		if(++bitcnt == nbits){
			beststate = 0;

			break;
		}
		memcpy(cmetric,nmetric,sizeof(cmetric));
	}
	pp -= 2;
	/* Chain back from terminal state to produce decoded data */
	memset(outdata,0,nbits/8);
	for(i=nbits-K;i >= 0;i--){
		if(pp[beststate >> 5] & (1L << (beststate & 31))){
			beststate |= (1 << (K-1));
			outdata[i>>3] |= 0x80 >> (i&7);
		}
		beststate >>= 1;
		pp -= 2;
	}        
	return 0;
}
