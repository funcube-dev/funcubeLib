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

/// @file Encode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "funcubeLib.h"
#include "portaudio.h"
#include <math.h>
#include <stdlib.h>
#include <string>
#include "decodeManager.h"
#include "memPool.h"
#include "guardInterlock.h"

#include "encode.h"

using namespace std;
using namespace fc;


const FLOAT MATCHED_FILTER_GAIN_1K2_48K = 40.05981371;
const INT MATCHED_FILTER_SIZE_1K2_48K = 321;
const FLOAT RX_MATCHED_FILTER_VALUES_1K2_48K[MATCHED_FILTER_SIZE_1K2_48K] ={ 
-0.0100141923, -0.0100217989, -0.0098867691, -0.0096065642,-0.0091805576, -0.0086100936, -0.0078985212, -0.0070512015,-0.0060754895, -0.0049806870, -0.0037779701, -0.0024802883,-0.0011022378, +0.0003400904, +0.0018292869, +0.0033468221,
+0.0048732656, +0.0063885226, +0.0078720861, +0.0093033021,+0.0106616430, +0.0119269882, +0.0130799066, +0.0141019378,+0.0149758691, +0.0156860036, +0.0162184159, +0.0165611921,+0.0167046502, +0.0166415375, +0.0163672019, +0.0158797337,
+0.0151800762, +0.0142721016, +0.0131626508, +0.0118615354,+0.0103815008, +0.0087381495, +0.0069498247, +0.0050374538,+0.0030243537, +0.0009359978, -0.0012002526, -0.0033554499,-0.0054993941, -0.0076009925, -0.0096286446, -0.0115506466,
-0.0133356112, -0.0149528983, -0.0163730513, -0.0175682344,-0.0185126644, -0.0191830324, -0.0195589095, -0.0196231314,-0.0193621556, -0.0187663867, -0.0178304646, -0.0165535107,-0.0149393275, -0.0129965481, -0.0107387312, -0.0081844003,
-0.0053570218, -0.0022849235, +0.0009988506, +0.0044567479,+0.0080469773, +0.0117238420, +0.0154381314, +0.0191375722,+0.0227673326, +0.0262705788, +0.0295890773, +0.0326638384,+0.0354357955, +0.0378465135, +0.0398389195, +0.0413580488,
+0.0423517983, +0.0427716792, +0.0425735615, +0.0417184011,+0.0401729419, +0.0379103841, +0.0349110103, +0.0311627625,+0.0266617606, +0.0214127569, +0.0154295182, +0.0087351302,+0.0013622191, -0.0066469170, -0.0152402674, -0.0243561648,
-0.0339234398, -0.0438616744, -0.0540815559, -0.0644853304,-0.0749673540, -0.0854147400, -0.0957080974, -0.1057223567,-0.1153276777, -0.1243904322, -0.1327742546, -0.1403411518,-0.1469526643, -0.1524710674, -0.1567606038, -0.1596887352,
-0.1611274038, -0.1609542890, -0.1590540519, -0.1553195513,-0.1496530223, -0.1419672059, -0.1321864156, -0.1202475334,-0.1061009223, -0.0897112457, -0.0710581861, -0.0501370517,-0.0269592669, -0.0015527367, +0.0260379179, +0.0557512595,
+0.0875090710, +0.1212165329, +0.1567625396, +0.1940201572,+0.2328472179, +0.2730870505, +0.3145693409, +0.3571111176,+0.4005178560, +0.4445846920, +0.4890977376, +0.5338354877,+0.5785703066, +0.6230699832, +0.6670993425, +0.7104218996,
+0.7528015429, +0.7940042326, +0.8337997006, +0.8719631363,+0.9082768461, +0.9425318696, +0.9745295417, +1.0040829843,+1.0310185176, +1.0551769757, +1.0764149177, +1.0946057222,+1.1096405559, +1.1214292087, +1.1299007866, +1.1350042571,
+1.1367088425, +1.1350042571, +1.1299007866, +1.1214292087,+1.1096405559, +1.0946057222, +1.0764149177, +1.0551769757,+1.0310185176, +1.0040829843, +0.9745295417, +0.9425318696,+0.9082768461, +0.8719631363, +0.8337997006, +0.7940042326,
+0.7528015429, +0.7104218996, +0.6670993425, +0.6230699832,+0.5785703066, +0.5338354877, +0.4890977376, +0.4445846920,+0.4005178560, +0.3571111176, +0.3145693409, +0.2730870505,+0.2328472179, +0.1940201572, +0.1567625396, +0.1212165329,
+0.0875090710, +0.0557512595, +0.0260379179, -0.0015527367,-0.0269592669, -0.0501370517, -0.0710581861, -0.0897112457,-0.1061009223, -0.1202475334, -0.1321864156, -0.1419672059,-0.1496530223, -0.1553195513, -0.1590540519, -0.1609542890,
-0.1611274038, -0.1596887352, -0.1567606038, -0.1524710674,-0.1469526643, -0.1403411518, -0.1327742546, -0.1243904322,-0.1153276777, -0.1057223567, -0.0957080974, -0.0854147400,-0.0749673540, -0.0644853304, -0.0540815559, -0.0438616744,
-0.0339234398, -0.0243561648, -0.0152402674, -0.0066469170,+0.0013622191, +0.0087351302, +0.0154295182, +0.0214127569,+0.0266617606, +0.0311627625, +0.0349110103, +0.0379103841,+0.0401729419, +0.0417184011, +0.0425735615, +0.0427716792,
+0.0423517983, +0.0413580488, +0.0398389195, +0.0378465135,+0.0354357955, +0.0326638384, +0.0295890773, +0.0262705788,+0.0227673326, +0.0191375722, +0.0154381314, +0.0117238420,+0.0080469773, +0.0044567479, +0.0009988506, -0.0022849235,
-0.0053570218, -0.0081844003, -0.0107387312, -0.0129965481,-0.0149393275, -0.0165535107, -0.0178304646, -0.0187663867,-0.0193621556, -0.0196231314, -0.0195589095, -0.0191830324,-0.0185126644, -0.0175682344, -0.0163730513, -0.0149528983,
-0.0133356112, -0.0115506466, -0.0096286446, -0.0076009925,-0.0054993941, -0.0033554499, -0.0012002526, +0.0009359978,+0.0030243537, +0.0050374538, +0.0069498247, +0.0087381495,+0.0103815008, +0.0118615354, +0.0131626508, +0.0142721016,
+0.0151800762, +0.0158797337, +0.0163672019, +0.0166415375,+0.0167046502, +0.0165611921, +0.0162184159, +0.0156860036,+0.0149758691, +0.0141019378, +0.0130799066, +0.0119269882,+0.0106616430, +0.0093033021, +0.0078720861, +0.0063885226,
+0.0048732656, +0.0033468221, +0.0018292869, +0.0003400904,-0.0011022378, -0.0024802883, -0.0037779701, -0.0049806870,-0.0060754895, -0.0070512015, -0.0078985212, -0.0086100936,-0.0091805576, -0.0096065642, -0.0098867691, -0.0100217989,
-0.0100141923,
};

/*
const FLOAT MATCHED_FILTER_GAIN_1K2_48K = 39.59535057;
const INT MATCHED_FILTER_SIZE_1K2_48K = 161;
const FLOAT RX_MATCHED_FILTER_VALUES_1K2_48K[MATCHED_FILTER_SIZE_1K2_48K] = { 
+0.0423517983, +0.0427716792, +0.0425735615, +0.0417184011, +0.0401729419, +0.0379103841, +0.0349110103, +0.0311627625, +0.0266617606, +0.0214127569, +0.0154295182, +0.0087351302, +0.0013622191, -0.0066469170, -0.0152402674, -0.0243561648,
-0.0339234398, -0.0438616744, -0.0540815559, -0.0644853304, -0.0749673540, -0.0854147400, -0.0957080974, -0.1057223567, -0.1153276777, -0.1243904322, -0.1327742546, -0.1403411518, -0.1469526643, -0.1524710674, -0.1567606038, -0.1596887352,
-0.1611274038, -0.1609542890, -0.1590540519, -0.1553195513, -0.1496530223, -0.1419672059, -0.1321864156, -0.1202475334, -0.1061009223, -0.0897112457, -0.0710581861, -0.0501370517, -0.0269592669, -0.0015527367, +0.0260379179, +0.0557512595,
+0.0875090710, +0.1212165329, +0.1567625396, +0.1940201572, +0.2328472179, +0.2730870505, +0.3145693409, +0.3571111176, +0.4005178560, +0.4445846920, +0.4890977376, +0.5338354877, +0.5785703066, +0.6230699832, +0.6670993425, +0.7104218996,
+0.7528015429, +0.7940042326, +0.8337997006, +0.8719631363, +0.9082768461, +0.9425318696, +0.9745295417, +1.0040829843, +1.0310185176, +1.0551769757, +1.0764149177, +1.0946057222, +1.1096405559, +1.1214292087, +1.1299007866, +1.1350042571,
+1.1367088425, +1.1350042571, +1.1299007866, +1.1214292087, +1.1096405559, +1.0946057222, +1.0764149177, +1.0551769757, +1.0310185176, +1.0040829843, +0.9745295417, +0.9425318696, +0.9082768461, +0.8719631363, +0.8337997006, +0.7940042326,
+0.7528015429, +0.7104218996, +0.6670993425, +0.6230699832, +0.5785703066, +0.5338354877, +0.4890977376, +0.4445846920, +0.4005178560, +0.3571111176, +0.3145693409, +0.2730870505, +0.2328472179, +0.1940201572, +0.1567625396, +0.1212165329,
+0.0875090710, +0.0557512595, +0.0260379179, -0.0015527367, -0.0269592669, -0.0501370517, -0.0710581861, -0.0897112457, -0.1061009223, -0.1202475334, -0.1321864156, -0.1419672059, -0.1496530223, -0.1553195513, -0.1590540519, -0.1609542890,
-0.1611274038, -0.1596887352, -0.1567606038, -0.1524710674, -0.1469526643, -0.1403411518, -0.1327742546, -0.1243904322, -0.1153276777, -0.1057223567, -0.0957080974, -0.0854147400, -0.0749673540, -0.0644853304, -0.0540815559, -0.0438616744,
-0.0339234398, -0.0243561648, -0.0152402674, -0.0066469170, +0.0013622191, +0.0087351302, +0.0154295182, +0.0214127569, +0.0266617606, +0.0311627625, +0.0349110103, +0.0379103841, +0.0401729419, +0.0417184011, +0.0425735615, +0.0427716792,
+0.0423517983
};
*/

// CEncode
CEncode::CEncode()
{
	m_matchedFilter.Initialise(RX_MATCHED_FILTER_VALUES_1K2_48K, MATCHED_FILTER_SIZE_1K2_48K);
}

CEncode::~CEncode()
{
}

BOOL CEncode::Initialise()
{
	// start in esComplete state so we are ready to accept an input buffer
	m_encodeState = esComplete;

	m_encodeWorker.Initialise(this);

	// ensure there are 64 buffers 256 bytes in size
	m_poolDataIn.Initialise(64, BytesPerDataBlock);
	m_poolSamplesOut.Initialise(256, BytesPerTxBit);
	m_symbolClock.Initialise(2400, SampleRate);
	return TRUE;
}

BOOL CEncode::Shutdown()
{
	m_encodeWorker.Shutdown();
	return TRUE;
}

BOOL CEncode::CanCollect()
{
	return m_encodeWorker.OutputQueueLength() > 0;
}

BOOL CEncode::AllDataCollected() {
	return m_encodeWorker.InputQueueLength() == 0 && ReadyForInput() && m_encodeWorker.OutputQueueLength()==0;
}

BOOL CEncode::CollectSamples(BYTE* buffer, ULONG* bufferSize)
{
	// ensure buffer is a multiple of the sample size
	if (*bufferSize % BytesPerTxBit != 0) {
		return FALSE;
	}

	ULONG bufferCapacity = *bufferSize;
	*bufferSize = 0;
	while (*bufferSize < bufferCapacity) {
		AutoBufPtr ptr = m_encodeWorker.PopOutput();
		if (NULL == ptr.get() || ptr->Capacity()+*bufferSize>bufferCapacity) {
			return *bufferSize > 0 ? TRUE : FALSE;
		}

		memcpy((void*)&buffer[*bufferSize], ptr->Data(), ptr->Capacity());
		*bufferSize += ptr->Capacity();
	}
	return TRUE;
}

BOOL CEncode::PushData(const BYTE* buffer, const ULONG bufferSize) {
	AutoBufPtr ptr = m_poolDataIn.GetBuffer();
	if (NULL == ptr.get()) {
		return FALSE;
	}
	if (bufferSize != ptr->Capacity()) {
		return FALSE;
	}
	memcpy(ptr->Data(), buffer, ptr->Capacity());
	ptr->Size() = ptr->Capacity();
	if (!m_encodeWorker.PushInput(ptr)) {
		return FALSE;
	}
	return TRUE;
}

BOOL CEncode::ReadyForInput() {
	return m_encodeState == esComplete;
}

BOOL CEncode::SetInputBuffer(fc::AutoBufPtr buf) {
	// encode the 256 byte buffer into the 5200 byte tx buffer
	if (!m_fec.Encode(buf->Data(), buf->Size(), m_txbuf, TxBufferBytes)) {
		return FALSE;
	}		
	m_encodeState = esInit;

	return TRUE;
}
fc::AutoBufPtr CEncode::GenerateNextBuffer() {
	static int bufcount = 0;
	// grab the next bit output 0 or 180 degrees depending on value (0.5 / 40.0598137 MATCHED_FILTER_GAIN_1K2_48K) 
	FLOAT value = GetNextTxBit()==0?0.01248F:-0.01248F;
	AutoBufPtr samples = m_poolSamplesOut.GetBuffer();
	if (NULL == samples.get()) {
		return samples;
	}
	//cerr << "buffer:" << bufcount++ << endl;

	UINT sampleCount = samples->Capacity() / SampleSizeBytes;
	FLOAT* sampleData = (FLOAT*)samples->Data();
	for (UINT i = 0; i < sampleCount; i++) {
		sampleData[i] = m_matchedFilter.ProcessSample(value);
	}
	samples->Size() = sampleCount*SampleSizeBytes;
	return samples;
}

BYTE CEncode::GetNextTxBit() {
	static LONG bitsRemaining = 0;
	static BYTE bitState = 0;
	static U32 syncVector = 0;	

	BYTE bitValue = 0;
	switch (m_encodeState) {
	case esInit: //drop through to preamble
		cerr << "esInit:" << endl;
		bitsRemaining = PreambleBits; // reset bits remaining
		m_encodeState = esPreamble; // transition straight to preamble (and drop through to it)
		cerr << "esPreamble:" << bitsRemaining << endl;
	case esPreamble: 		
		if (--bitsRemaining == 0) {
			m_encodeState = esSync;
			bitsRemaining = SyncVectorBits;
			syncVector = SYNC_VECTOR;
			cerr << "esSync:" << bitsRemaining << endl;
		}		
		break;	
	case esSync:		
		bitValue = (syncVector & 0x80000000) != 0 ? 1 : 0;
		syncVector <<= 1; // shift to the next syncbit		
		if (--bitsRemaining == 0) {
			m_encodeState = esFec;
			bitsRemaining = FecBits;
			cerr << "esFec:" << bitsRemaining << endl;
		}		
		break;
	case esFec:		
		bitValue = m_txbuf[TxBufferBytes-(--bitsRemaining)] != 0 ? 1 : 0;
		if (bitsRemaining == 0) {
			m_encodeState = esPostamble;
			bitsRemaining = PostambleBits;
			cerr << "esPostamble:" << bitsRemaining << endl;
		}		
		break;
	case esPostamble:		
		if (--bitsRemaining == 0) {
			m_encodeState = esComplete;
			cerr << "esComplete:" << endl;
		}		
		break;
	case esComplete:		
		// only transition out when fresh data buffer added (SetInputBuffer)
		break;
	}

	// differential on zero bits so flip it
	if (bitValue == 0) {
		bitState = !bitState;
	}

	return bitState;
}