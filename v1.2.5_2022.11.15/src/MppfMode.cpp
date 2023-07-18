/* Copyright (c) 2015-2022 Qualcomm Technologies, Inc. All Rights Reserved
 * 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The above license is used as a license under copyright only. Please reference VESA Policy #200D for patent licensing terms.
 */

/**
\file       MppfMode.cpp
\brief      Midpoint Prediction Fallback Mode
*/

#include "MppfMode.h"
#include "TypeDef.h"
#include "Misc.h"

extern Bool enEncTracer;
extern Bool enDecTracer;
extern FILE* encTracer;
extern FILE* decTracer;

/**
    MPPF mode constructor

    @param bpp compressed bitrate
    @param bitDepth bit depth
    @param isEncoder flag to indicate encoder
    @param chromaFormat chroma sampling format
    @param csc color space
*/
MppfMode::MppfMode (Int bpp, Int bitDepth, Bool isEncoder, ChromaFormat chromaFormat, ColorSpace csc)
    : Mode (kModeTypeMPPF, csc, (csc == kRgb ? kRgb : kYCbCr), bpp, bitDepth, isEncoder, chromaFormat)
{
    for (Int c = 0; c < g_numComps; c++) {
        m_midpoint[c] = new Int[4];
    }
}

/**
    Destructor
*/
MppfMode::~MppfMode () {
    ClearMppSuffixMapping ();
    for (UInt c = 0; c < g_numComps; c++) {
        if (m_qResBufA[c]) { delete[] m_qResBufA[c]; m_qResBufA[c] = NULL; }
        if (m_qResBufB[c]) { delete[] m_qResBufB[c]; m_qResBufB[c] = NULL; }
        if (m_recBufA[c]) { delete[] m_recBufA[c]; m_recBufA[c] = NULL; }
        if (m_recBufB[c]) { delete[] m_recBufB[c]; m_recBufB[c] = NULL; }
		if (m_recBufC[c]) { delete[] m_recBufC[c]; m_recBufC[c] = NULL; }
        if (m_midpoint[c]) { delete[] m_midpoint[c]; m_midpoint[c] = NULL; }
		if (!m_isEncoder) {
			if (m_nextBlockQuantRes[c]) { delete[] m_nextBlockQuantRes[c]; m_nextBlockQuantRes[c] = NULL; }
		}
	}
}

/**
    Init MPPF mode
*/
Void MppfMode::InitModeSpecific () {
    SetMppSuffixMapping ();
    // set memory for temp buffers
    for (UInt c = 0; c < g_numComps; c++) {
        UInt numPx = getWidth (c) * getHeight (c);
        m_qResBufA[c] = new Int[numPx];
        m_qResBufB[c] = new Int[numPx];
        m_recBufA[c] = new Int[numPx];
        m_recBufB[c] = new Int[numPx];
		m_recBufC[c] = new Int[numPx];
    }

    m_minBlockBits = g_maxModeHeaderLen + g_maxFlatHeaderLen;
	m_minBlockBits += 1; // MPPF Type flag
    switch (m_origSrcCsc) {
    case kRgb:
        m_mppfAdaptiveNumTrials = 2;
        m_mppfAdaptiveCsc[0] = kRgb;
        m_mppfAdaptiveCsc[1] = kYCoCg;
        SetMppfBitsPerComp (0, m_pps->GetMppfBitsPerCompA (0), m_pps->GetMppfBitsPerCompA (1), m_pps->GetMppfBitsPerCompA (2));
        SetMppfBitsPerComp (1, m_pps->GetMppfBitsPerCompB (0), m_pps->GetMppfBitsPerCompB (1), m_pps->GetMppfBitsPerCompB (2));

        for (Int k = 0; k < g_numComps; k++) {
            m_minBlockBits += (m_bitsPerCompA[k] * m_blkWidth * m_blkHeight);
        }
        break;
    case kYCbCr:
        m_mppfAdaptiveNumTrials = 1;
        m_mppfAdaptiveCsc[0] = kYCbCr;
        SetMppfBitsPerComp (0, m_pps->GetMppfBitsPerCompA (0), m_pps->GetMppfBitsPerCompA (1), m_pps->GetMppfBitsPerCompA (2));
        for (Int k = 0; k < g_numComps; k++) {
            m_minBlockBits += m_bitsPerCompA[k] * ((m_blkWidth * m_blkHeight) >> (g_compScaleX[k][m_chromaFormat] + g_compScaleY[k][m_chromaFormat]));
        }
        break;
    default:
        fprintf (stderr, "MppfMode::InitModeSpecific(): unsupported input color format: %d.\n", m_origSrcCsc);
        exit (EXIT_FAILURE);
    }
}

/**
    Set MPPF substream mapping
*/
Void MppfMode::SetMppSuffixMapping () {
    for (UInt k = 0; k < g_numComps; k++) {
        UInt numPx = getWidth (k) * getHeight (k);
        m_mppSuffixSsmMapping[k] = new Int[numPx];
        switch (m_chromaFormat) {
        case k444:
            memcpy (m_mppSuffixSsmMapping[k], g_mppSsmMapping_444[k], numPx * sizeof (Int));
            break;
        case k422:
            memcpy (m_mppSuffixSsmMapping[k], g_mppSsmMapping_422[k], numPx * sizeof (Int));
            break;
        case k420:
            memcpy (m_mppSuffixSsmMapping[k], g_mppSsmMapping_420[k], numPx * sizeof (Int));
            break;
        }
    }
}

/**
    Clear storage for next block quantized residuals
*/
Void MppfMode::ClearMppSuffixMapping () {
	for (UInt k = 0; k < g_numComps; k++) {
		if (m_mppSuffixSsmMapping[k]) {
			delete[] m_mppSuffixSsmMapping[k];
			m_mppSuffixSsmMapping[k] = NULL;
		}
	}
}

/**
    Allocate storage for next block quantized residuals
*/
Void MppfMode::InitNextBlockCache () {
    for (UInt c = 0; c < g_numComps; c++) {
		Int numPx = getWidth (c) * getHeight (c);
        m_nextBlockQuantRes[c] = new Int[numPx];
        memset (m_nextBlockQuantRes[c], 0, numPx * sizeof (Int));
    }
}

/**
    Copy quantized residuals for next block to current block (for decoder)
*/
Void MppfMode::CopyCachedSuffixBits () {
    for (UInt k = 0; k < g_numComps; k++) {
        Int *pQuant = GetQuantBlkBuf (k);
        Int *pNextBlockQuant = m_nextBlockQuantRes[k];
        for (UInt x = 0; x < getWidth (k) * getHeight (k); x++) {
            if (m_mppSuffixSsmMapping[k][x] == 0) {
                pQuant[x] = pNextBlockQuant[x];
                pNextBlockQuant[x] = 0;
            }
        }
    }
}

/**
    Parse MPPF quantized residuals

    @param isSsmIdx0 flag to indicate substream 0
*/
Void MppfMode::DecodeMppfSuffixBits (Bool isSsmIdx0) {
    // read quantized residual from bitstream
    ColorSpace curSuffixCsc;
    UInt curMppfIndex;
    if (isSsmIdx0) {
        curSuffixCsc = m_mppfAdaptiveCsc[m_mppfIndexNext];
        curMppfIndex = m_mppfIndexNext;
    }
    else {
        curSuffixCsc = m_mppfAdaptiveCsc[m_mppfIndex];
        curMppfIndex = m_mppfIndex;
    }
    m_csc = curSuffixCsc;

    for (UInt k = 0; k < g_numComps; k++) {
        Int bitDepth = (curSuffixCsc == kYCoCg && k > 0 ? m_bitDepth + 1 : m_bitDepth);
        Int *pQuant = GetQuantBlkBuf (k);
        Int *pNextBlockQuant = m_nextBlockQuantRes[k];
        UInt numPx = getWidth (k) * getHeight (k);
        if (isSsmIdx0) {
            Int curStepSize = GetStepSize (curMppfIndex, k);
            Int bits = bitDepth - curStepSize;
            Int a = bits - 1;
            Int minCode = -(1 << a);
            Int maxCode = (1 << a) - 1;
            // parse suffixes from substream 0
            for (UInt x = 0; x < numPx; x++) {
                Int curSsmIdx = m_mppSuffixSsmMapping[k][x];
                if (curSsmIdx == 0) {
                    UInt val = m_ssm->ReadFromFunnelShifter (curSsmIdx, bits);
                    pNextBlockQuant[x] = (Int)(val + minCode);
                }
            }
        }
        else {
            Int curStepSize = GetStepSize (curMppfIndex, k);
            Int bits = bitDepth - curStepSize;
            Int a = bits - 1;
            Int minCode = -(1 << a);
            Int maxCode = (1 << a) - 1;
            // parse suffixes from substreams 1..3
            for (UInt x = 0; x < numPx; x++) {
                Int curSsmIdx = m_mppSuffixSsmMapping[k][x];
                if (curSsmIdx != 0) {
                    UInt val = m_ssm->ReadFromFunnelShifter (curSsmIdx, bits);
                    pQuant[x] = (Int)(val + minCode);
                }
            }
        }
    }
}

/**
    Get step size for specified component

    @param index MPPF index
    @param k component index
    @return step size
*/
Int MppfMode::GetStepSize (UInt index, UInt k) {
    Int bitDepth = 0;
    Int compBits = (index == 0 ? m_bitsPerCompA[k] : m_bitsPerCompB[k]);

    switch (m_csc) {
    case kRgb:
    case kYCbCr:
        bitDepth = m_bitDepth;
        break;
    case kYCoCg:
        bitDepth = (k == 0 ? m_bitDepth : m_bitDepth + 1);
        break;
    default:
        assert (false);
    }
    return (bitDepth - compBits);
}

/**
    Update QP
*/
Void MppfMode::UpdateQp () {
    m_stepSize = (m_bitDepth - 1);
    m_bias = 1 << (m_stepSize - 1);
}

/**
    Test encode MPPF block
*/
Void MppfMode::Test () {
    Int bits, distortion, rdCost;
    UInt* tempRate = new UInt[m_mppfAdaptiveNumTrials];
    UInt* tempDistortion = new UInt[m_mppfAdaptiveNumTrials];
    UInt* tempRdCost = new UInt[m_mppfAdaptiveNumTrials];

    for (UInt testMppfIndex = 0; testMppfIndex < m_mppfAdaptiveNumTrials; testMppfIndex++) {
        bits = 0;
        distortion = 0;
        rdCost = 0;
        m_mppfIndex = testMppfIndex;
        m_csc = m_mppfAdaptiveCsc[m_mppfIndex];
        bits += EncodeModeHeader (false);
        bits += EncodeFlatness (false);
        bits += EncodeMppfType (false);
        InitCurBlock ();
		ComputeMidpointUnified ();
        for (Int k = 0; k < g_numComps; k++) {
            Int width = getWidth (k);
            Int height = getHeight (k);
            Int numPx = width * height;
            Int bitDepth = (GetCsc () == kYCoCg && k > 0 ? m_bitDepth + 1 : m_bitDepth);
            Int clipMax = (1 << m_bitDepth) - 1;
            Int clipMin = (GetCsc () == kYCoCg && k > 0) ? -(1 << m_bitDepth) : 0;
            Int* pCur = GetCurBlkBuf (k);
            Int* pRes = GetResBlkBuf (k);
            Int* pRec = (m_mppfIndex == 0 ? m_recBufA[k] : m_recBufB[k]);
            Int* pQuant = (m_mppfIndex == 0 ? m_qResBufA[k] : m_qResBufB[k]);
            Int curStepSize = GetStepSize (m_mppfIndex, k);
			Int curBias = curStepSize == 0 ? 0 : 1 << (curStepSize - 1);
            Int a = bitDepth - curStepSize - 1;
            Int minCode = -(1 << a);
            Int maxCode = (1 << a) - 1;
            Int curSample = 0;
            Int curSampleError = 0;
            UInt pxIdx = 0;
			UInt numSubBlocks = (m_chromaFormat == k444 || k == 0 ? 4 : 2);
			UInt numSubBlockSamples = (m_chromaFormat == k420 && k > 0 ? 2 : 4);

            for (UInt subBlock = 0; subBlock < numSubBlocks; subBlock++) {
                Int error[2][2] = { 0 };
                UInt xOffset = (subBlock << 1);
                Int mp = GetMidpoint (k, subBlock);

                // pixel A
                pxIdx = xOffset;
                curSample = pCur[pxIdx];
                pRes[pxIdx] = curSample - mp;
                pQuant[pxIdx] = Clip3<Int> (minCode, maxCode, (pRes[pxIdx] > 0) ? (pRes[pxIdx] + curBias) >> curStepSize : -((curBias - pRes[pxIdx]) >> curStepSize));
                pRec[pxIdx] = Clip3<Int> (clipMin, clipMax, (pQuant[pxIdx] << curStepSize) + mp);
                curSampleError = curSample - pRec[pxIdx];
                error[0][1] += ((curSampleError + 1) >> 1); // diffuse error to B
                error[1][0] += ((curSampleError + 1) >> 1); // diffuse error to C
                bits += (bitDepth - curStepSize);

                // pixel B
                pxIdx = xOffset + 1;
				curSample = pCur[pxIdx];
				pRes[pxIdx] = curSample + error[0][1] - mp;
                pQuant[pxIdx] = Clip3<Int> (minCode, maxCode, (pRes[pxIdx] > 0) ? (pRes[pxIdx] + curBias) >> curStepSize : -((curBias - pRes[pxIdx]) >> curStepSize));
                pRec[pxIdx] = Clip3<Int> (clipMin, clipMax, (pQuant[pxIdx] << curStepSize) + mp);
                curSampleError = curSample - pRec[pxIdx];
                error[1][0] += ((curSampleError + 1) >> 1); // diffuse error to C
                error[1][1] += ((curSampleError + 1) >> 1); // diffuse error to D
                bits += (bitDepth - curStepSize);

                // pixel C
				if (!(k > 0 && m_chromaFormat == k420)) {
					pxIdx = getWidth (k) + xOffset;
					curSample = pCur[pxIdx];
					pRes[pxIdx] = curSample + error[1][0] - mp;
					pQuant[pxIdx] = Clip3<Int> (minCode, maxCode, (pRes[pxIdx] > 0) ? (pRes[pxIdx] + curBias) >> curStepSize : -((curBias - pRes[pxIdx]) >> curStepSize));
					pRec[pxIdx] = Clip3<Int> (clipMin, clipMax, (pQuant[pxIdx] << curStepSize) + mp);
					curSampleError = curSample - pRec[pxIdx];
					error[1][1] += ((curSampleError + 1) >> 1); // diffuse error to D
					bits += (bitDepth - curStepSize);

					// pixel D
					pxIdx = getWidth (k) + xOffset + 1;
					curSample = pCur[pxIdx];
					pRes[pxIdx] = curSample + error[1][1] - mp;
					pQuant[pxIdx] = Clip3<Int> (minCode, maxCode, (pRes[pxIdx] > 0) ? (pRes[pxIdx] + curBias) >> curStepSize : -((curBias - pRes[pxIdx]) >> curStepSize));
					pRec[pxIdx] = Clip3<Int> (clipMin, clipMax, (pQuant[pxIdx] << curStepSize) + mp);
					bits += (bitDepth - curStepSize);
				}
            }
        }
        m_rateWithoutPadding = bits;
        
        // if MPPF, temp buffer YCoCg-->RGB
        if (m_csc == kYCoCg) {
            TempRecBufferYCoCgToRgb ();
        }

		if (m_csc == kYCoCg) {
			CopyTempRecBuffer (2);
			CurBlockYCoCgToRgb ();
			m_csc = kRgb;
			distortion = ComputeCurBlockDistortion ();
			m_csc = kYCoCg;
		}
		else {
			CopyTempRecBuffer (m_mppfIndex);
			distortion = ComputeCurBlockDistortion ();
		}
        tempRate[testMppfIndex] = bits;
        tempDistortion[testMppfIndex] = distortion;
        tempRdCost[testMppfIndex] = ComputeRdCost (bits, distortion);
    }

    if (m_mppfAdaptiveNumTrials > 1) {
        m_mppfIndex = (tempDistortion[0] < tempDistortion[1] ? 0 : 1);
    }
    else {
        m_mppfIndex = 0;
    }

    // set selected MPPF type to the final rec buffer
	if (m_origSrcCsc == kRgb && m_mppfIndex == 0) {
		TempRecBufferRgbToYCoCg ();
		CopyTempRecBuffer (2);
	}
	else {
		CopyTempRecBuffer (m_mppfIndex);
	}

    CopyTempQuantBuffer (m_mppfIndex);
    m_rate = tempRate[m_mppfIndex];
    m_distortion = tempDistortion[m_mppfIndex];
    m_rdCost = tempRdCost[m_mppfIndex];
    m_csc = m_mppfAdaptiveCsc[m_mppfIndex];

    delete[] tempRate;
    delete[] tempDistortion;
    delete[] tempRdCost;
}

/**
    Encode MPPF index to bitstream

    @param isFinal flag to indicate final encode
    @return number of bits needed to encode
*/
Int MppfMode::EncodeMppfType (Bool isFinal) {
    Int bits = 0;
    if (m_csc == kYCbCr) {
        return bits;
    }
    bits += 1;
    UInt substreamId = 0;
    if (isFinal) {
        m_ssm->GetCurSyntaxElement (substreamId)->AddSyntaxWord (m_mppfIndex, 1);
    }
    return bits;
}

/**
    Encode MPPF block
*/
Void MppfMode::Encode () {
    m_qp = 0;
    m_ssm->SetCurSyntaxElementMode (std::string ("MPPF"));
    UInt* prevBits = new UInt[m_ssm->GetNumSubStreams ()];
    UInt* postBits = new UInt[m_ssm->GetNumSubStreams ()];
    UInt prevBitsSum = 0;
    UInt postBitsSum = 0;
    for (UInt ss = 0; ss < m_ssm->GetNumSubStreams (); ss++) {
        prevBits[ss] = m_ssm->GetCurSyntaxElement (ss)->GetNumBits ();
        prevBitsSum += prevBits[ss];
    }
    EncodeModeHeader (true);
    EncodeFlatness (true);
    EncodeMppfType (true);
    for (Int k = 0; k < g_numComps; k++) {
        Int width = getWidth (k);
        Int height = getHeight (k);
        Int *ptrQuant = GetQuantBlkBuf (k);
        Int compBitDepth = (m_csc == kYCoCg && k > 0) ? m_bitDepth + 1 : m_bitDepth;
        Int compBits = compBitDepth - GetStepSize (m_mppfIndex, k);
        Int minCode = -(1 << (compBits - 1));
        for (Int x = 0; x < (width * height); x++) {
            Int key = ptrQuant[x];
            Int curSs = m_mppSuffixSsmMapping[k][x];
            assert (curSs >= 0);
            m_ssm->GetCurSyntaxElement (curSs)->AddSyntaxWord ((UInt)(key - minCode), compBits);
        }
    }
    for (UInt ss = 0; ss < m_ssm->GetNumSubStreams (); ss++) {
        postBits[ss] = m_ssm->GetCurSyntaxElement (ss)->GetNumBits ();
        postBitsSum += postBits[ss];
    }
    Int curBits = postBitsSum;
    delete[] prevBits;
    delete[] postBits;

	//! copy to reconstructed frame buffer !//
	if (m_csc == kYCoCg || m_csc == kRgb) {
		CopyToYCoCgBuffer (m_posX);
	}
	else {
		CopyToRecFrame (m_posX, m_posY);
	}

    //! debug tracer !//
    if (enEncTracer) {
        fprintf (encTracer, "MPPF: csc = %s, index = %d\n", g_colorSpaceNames[m_csc].c_str (), m_mppfIndex);
        fprintf (encTracer, "MPPF: stepSize = [%d, %d, %d]\n", GetStepSize (m_mppfIndex, 0), GetStepSize (m_mppfIndex, 1), GetStepSize (m_mppfIndex, 2));
        /*
        fprintf (encTracer, "MPPF: mp0 = [%d, %d, %d]\n", GetMidpoint (0, 0), GetMidpoint (1, 0), GetMidpoint (2, 0));
        fprintf (encTracer, "MPPF: mp1 = [%d, %d, %d]\n", GetMidpoint (0, 1), GetMidpoint (1, 1), GetMidpoint (2, 1));
        fprintf (encTracer, "MPPF: mp2 = [%d, %d, %d]\n", GetMidpoint (0, 2), GetMidpoint (1, 2), GetMidpoint (2, 2));
        fprintf (encTracer, "MPPF: mp3 = [%d, %d, %d]\n", GetMidpoint (0, 3), GetMidpoint (1, 3), GetMidpoint (2, 3));
        */
        debugTracerWriteArray3 (encTracer, "MPPF", "Quant", m_blkWidth * m_blkHeight, m_qResBlk, m_chromaFormat);
        debugTracerWriteArray3 (encTracer, "MPPF", "RecBlk", m_blkWidth * m_blkHeight, m_recBlk, m_chromaFormat);
    }
}

/**
    Decode MPPF block
*/
Void MppfMode::Decode () {
    m_qp = 0;
    DecodeMppfSuffixBits (false);
	ComputeMidpointUnified ();
    for (Int k = 0; k < g_numComps; k++) {
        Int numPx = getWidth (k) * getHeight (k);
        Int bitDepth = (GetCsc () == kYCoCg && k > 0 ? m_bitDepth + 1 : m_bitDepth);
        Int curStepSize = GetStepSize (m_mppfIndex, k);
        Int curBias = 1 << (curStepSize - 1);
        Int bits = bitDepth - curStepSize;
        Int minCode = -(1 << (bits - 1));
        Int clipMax = (1 << m_bitDepth) - 1;
        Int clipMin = (GetCsc () == kYCoCg && k > 0) ? -(1 << m_bitDepth) : 0;
        Int *pRec = GetRecBlkBuf (k);
        Int* pQuant = GetQuantBlkBuf (k);
        for (UInt y = 0; y < getHeight (k); y++) {
            for (UInt x = 0; x < getWidth (k); x++) {
                Int index = y * getWidth (k) + x;
                Int mp = GetMidpoint (k, x >> 1);
                Int key = pQuant[index];
                Int deQuant = (key << curStepSize);
                Int a = bits - 1;
                Int offset = 1 << (curStepSize - 1);
                Int recon = Clip3<Int> (clipMin, clipMax, mp + deQuant);
                pRec[index] = (Int)recon;
            }
        }
    }

    //! copy to reconstructed frame buffer !//
	if (m_csc == kRgb) {
		RecBlockRgbToYCoCg ();
	}

	//! copy to reconstructed frame buffer !//
	if (m_csc == kYCoCg || m_csc == kRgb) {
		CopyToYCoCgBuffer (m_posX);
	}
	else {
		CopyToRecFrame (m_posX, m_posY);
	}

    //! debug tracer !//
    if (enDecTracer) {
        fprintf (decTracer, "MPPF: csc = %s, index = %d\n", g_colorSpaceNames[m_csc].c_str (), m_mppfIndex);
        fprintf (decTracer, "MPPF: stepSize = [%d, %d, %d]\n", GetStepSize (m_mppfIndex, 0), GetStepSize (m_mppfIndex, 1), GetStepSize (m_mppfIndex, 2));
        /*
        fprintf (decTracer, "MPPF: mp0 = [%d, %d, %d]\n", GetMidpoint (0, 0), GetMidpoint (1, 0), GetMidpoint (2, 0));
        fprintf (decTracer, "MPPF: mp1 = [%d, %d, %d]\n", GetMidpoint (0, 1), GetMidpoint (1, 1), GetMidpoint (2, 1));
        fprintf (decTracer, "MPPF: mp2 = [%d, %d, %d]\n", GetMidpoint (0, 2), GetMidpoint (1, 2), GetMidpoint (2, 2));
        fprintf (decTracer, "MPPF: mp3 = [%d, %d, %d]\n", GetMidpoint (0, 3), GetMidpoint (1, 3), GetMidpoint (2, 3));
        */
        debugTracerWriteArray3 (decTracer, "MPPF", "Quant", m_blkWidth * m_blkHeight, m_qResBlk, m_chromaFormat);
        debugTracerWriteArray3 (decTracer, "MPPF", "RecBlk", m_blkWidth * m_blkHeight, m_recBlk, m_chromaFormat);
    }
}

/**
    Compute midpoints
*/
Void MppfMode::ComputeMidpointUnified () {
	InitNeighbors ();
	UInt numSubBlocks = 4;
	UInt numSubBlocksChroma = (m_chromaFormat == k444 ? 4 : 2);
	for (UInt sb = 0; sb < numSubBlocks; sb++) {
		Int mean[3] = { 0, 0, 0 };

		if (m_posX == 0 && m_posY == 0) {
			// first block in slice
			for (Int k = 0; k < g_numComps; k++) {
				if (m_chromaFormat != k444 && k > 0 && sb >= numSubBlocksChroma) {
					continue;
				}
				mean[k] = (m_csc == kYCoCg && k > 0 ? 0 : 1 << (m_bitDepth - 1));
			}
		}
		else if (m_posY == 0) {
			//! FBLS !//
			for (Int k = 0; k < g_numComps; k++) {
				if (m_chromaFormat != k444 && k > 0 && sb >= numSubBlocksChroma) {
					continue;
				}
				Int *pSrc;
				Int stride;
				if (m_csc == kYCoCg) {
					pSrc = m_recBufferYCoCg->getBuf (k);
					stride = m_recBufferYCoCg->getWidth (k);
				}
				else {
					pSrc = m_pRecVideoFrame->getBuf (k); // top-left corner of image
					stride = m_pRecVideoFrame->getWidth (k);
				}
				Int posX = getPosX (k) + (sb << 1);
				Int posY = getPosY (k);
				Int width = getWidth (k);
				Int height = getHeight (k);
				Int blockOffsetInFrame = (posY * stride + posX - width);
				pSrc += blockOffsetInFrame;
				Int compMean = 0;
				if (m_chromaFormat == k420 && k > 0) {
					for (Int i = 0; i < 2; i++) {
						compMean += pSrc[i];
					}
					mean[k] = (compMean >> 1);
				}
				else {
					for (Int j = 0; j < 2; j++) {
						for (Int i = 0; i < 2; i++) {
							compMean += pSrc[j * stride + i];
						}
					}
					mean[k] = (compMean >> 2);
				}
			}
		}
		else {
			//! NFBLS !//
			for (Int k = 0; k < g_numComps; k++) {
				if (m_chromaFormat != k444 && k > 0 && sb >= numSubBlocksChroma) {
					continue;
				}
				Int *pSrc = m_pRecVideoFrame->getBuf (k); // top-left corner of image
				Int stride = m_pRecVideoFrame->getWidth (k);
				Int posX = getPosX (k) + (sb << 1);
				Int posY = getPosY (k);
				Int width = getWidth (k);
				Int height = getHeight (k);
				Int blockOffsetInFrame = ((posY - 1) * stride + posX);
				pSrc += blockOffsetInFrame;
				Int compMean = 0;
				for (Int i = 0; i < 2; i++) {
					compMean += pSrc[i];
				}
				mean[k] = (compMean >> 1);
			}

			//! convert mean from RGB->YCoCg !//
			if (m_csc == kYCoCg) {
				Int Co = mean[0] - mean[2];
				Int temp = mean[2] + (Co >> 1);
				Int Cg = mean[1] - temp;
				Int Y = temp + (Cg >> 1);
				mean[0] = Y;
				mean[1] = Co;
				mean[2] = Cg;
			}
		}

		// compute MPP predictor from the mean
		for (UInt k = 0; k < g_numComps; k++) {
			Int middle = (m_csc == kYCoCg && k > 0) ? 0 : 1 << (m_bitDepth - 1);
			Int curStepSize = GetStepSize (m_mppfIndex, k);
			Int curBias = curStepSize == 0 ? 0 : 1 << (curStepSize - 1);
			Int maxClip = std::min<Int> ((1 << m_bitDepth) - 1, middle + 2 * curBias);
			Int mp = Clip3<Int> (middle, maxClip, mean[k] + (2 * curBias));
			SetMidpoint (k, sb, mp);
		}
	}
}

/**
    CSC for temp reconstructed buffer (YCoCg -> RGB)

    @param mppfIndex MPPF Index
*/
Void MppfMode::TempRecBufferYCoCgToRgb () {
    Int temp, R, G, B;
	Int *srcY = m_recBufB[0];
	Int *srcCo = m_recBufB[1];
	Int *srcCg = m_recBufB[2];
	Int *dstR = m_recBufC[0];
	Int *dstG = m_recBufC[1];
	Int *dstB = m_recBufC[2];
    Int numPx = m_blkWidth * m_blkHeight;
    for (Int x = 0; x < numPx; x++) {
		temp = srcY[x] - (srcCg[x] >> 1);
		G = srcCg[x] + temp;
		B = temp - (srcCo[x] >> 1);
		R = srcCo[x] + B;
		dstR[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, R);
		dstG[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, G);
		dstB[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, B);
    }
}

Void MppfMode::TempRecBufferRgbToYCoCg () {
	Int *srcR = m_recBufA[0];
	Int *srcG = m_recBufA[1];
	Int *srcB = m_recBufA[2];
	Int *dstY = m_recBufC[0];
	Int *dstCo = m_recBufC[1];
	Int *dstCg = m_recBufC[2];
	Int numPx = m_blkWidth * m_blkHeight;
	for (Int x = 0; x < numPx; x++) {
		Int Co = srcR[x] - srcB[x];
		Int tmp = srcB[x] + (Co >> 1);
		Int Cg = srcG[x] - tmp;
		dstY[x] = tmp + (Cg >> 1); // Y
		dstCo[x] = Co; // Co
		dstCg[x] = Cg; // Cg
	}
}

/**
    Copy temp reconstructed buffer to final reconstructed buffer

    @param mppfIndex MPPF index
*/
Void MppfMode::CopyTempRecBuffer (UInt mppfIndex) {
    for (UInt c = 0; c < g_numComps; c++) {
        for (UInt x = 0; x < getWidth (c) * getHeight (c); x++) {
			switch (mppfIndex) {
				case 0:
					m_recBlk[c][x] = m_recBufA[c][x];
					break;
				case 1:
					m_recBlk[c][x] = m_recBufB[c][x];
					break;
				case 2:
					m_recBlk[c][x] = m_recBufC[c][x];
					break;
			}
        }
    }
}

/**
    Copy temp quantized residual buffer to final quantized residual buffer

    @param mppfIndex MPPF index
*/
Void MppfMode::CopyTempQuantBuffer (UInt mppfIndex) {
    for (UInt c = 0; c < g_numComps; c++) {
        for (UInt x = 0; x < getWidth (c) * getHeight (c); x++) {
            m_qResBlk[c][x] = (mppfIndex == 0 ? m_qResBufA[c][x] : m_qResBufB[c][x]);
        }
    }
}

/**
    Set MPPF step sizes

    @param i MPPF index
    @param ss0 step size for component 0
    @param ss1 step size for component 1
    @param ss2 step size for component 2
*/
Void MppfMode::SetMppfBitsPerComp (UInt idx, UInt ss0, UInt ss1, UInt ss2) {
    switch (idx) {
    case 0:
        m_bitsPerCompA[0] = ss0;
        m_bitsPerCompA[1] = ss1;
        m_bitsPerCompA[2] = ss2;
        break;
    case 1:
        m_bitsPerCompB[0] = ss0;
        m_bitsPerCompB[1] = ss1;
        m_bitsPerCompB[2] = ss2;
        break;
    }
}