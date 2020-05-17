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

/// @file Encode.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include <queue>
#include "funcubeLib.h"
#include "memPool.h"
#include "fec.h"
#include "firFilter.h"
#include "oscillator.h"
#include "iencoder.h"
#include "encodeWorker.h"


// CEncode
class CEncode : public IEncoder {
public:
	CEncode();
    virtual ~CEncode();

    BOOL Initialise();
    BOOL Shutdown();

	BOOL CanCollect();	
	BOOL AllDataCollected();

	BOOL CollectSamples(BYTE* buffer, ULONG* bufferSize);
	BOOL PushData(const BYTE* buffer, const ULONG bufferSize);
	
	// IEncode interface
	BOOL ReadyForInput();	
	BOOL SetInputBuffer(fc::AutoBufPtr);	
	fc::AutoBufPtr GenerateNextBuffer();
	
protected:
	static const LONG SampleRate = 48000;
	static const LONG SampleSizeBytes = 4;
	static const LONG BitRate = 1200;
	static const LONG PreambleBits = 768; // Number of bit times for preamble - 5 seconds = 6000 bits, 5200 FEC bits + 32 sync vector bits = 5232, 6000 - 5232 = 768 preamble bits
	static const LONG PostambleBits = 16;
	static const U32 SyncVector = 0x1acffc1d;
	static const LONG SyncVectorBits = 32;
	static const LONG FecBits = 5200;
	static const LONG SamplesPerTxBit = SampleRate / BitRate; // 48000 / 1200 == 40
	static const LONG BytesPerTxBit = SamplesPerTxBit * SampleSizeBytes; // 40 * 4 = 160
	static const LONG BytesPerDataBlock = 256;
	static const LONG TxBufferBytes = 5200;

	fc::CMemPool m_poolDataIn;
	fc::CMemPool m_poolSamplesOut;

	unsigned char m_txbuf[TxBufferBytes];
	BYTE GetNextTxBit();

	enum { esInit, esPreamble, esSync, esFec, esPostamble, esComplete } m_encodeState = esComplete;

	CFec m_fec;
	CFirFilter m_matchedFilter;
	COscillator m_symbolClock;
	CEncodeWorker m_encodeWorker;
};

