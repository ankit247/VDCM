/* Copyright (c) 2015-2018 Qualcomm Technologies, Inc. All Rights Reserved
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

/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2012, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Copyright (c) 2015-2017 MediaTek Inc., RDO weighting are modified and/or implemented by MediaTek Inc. based on TransMode.cpp
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
\file       TransMode.cpp
\brief      Transform Mode
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <math.h>
#include <memory.h>
#include <assert.h>
#include "TransMode.h"
#include "IntraPrediction.h"
#include "Misc.h"
#include "TypeDef.h"

extern Bool enEncTracer;
extern Bool enDecTracer;
extern FILE* encTracer;
extern FILE* decTracer;

/**
    Transform mode constructor

    @param bpp compressed bitrate
    @param bitDepth bit depth
    @param isEncoder flag to indicate encoder or decoder
    @param chromaFormat chroma sampling format
    @param csc color space
    */
TransMode::TransMode (Int bpp, Int bitDepth, Bool isEncoder, ChromaFormat chromaFormat, ColorSpace csc, ColorSpace targetCsc)
    : Mode (kModeTypeTransform, csc, targetCsc, bpp, bitDepth, isEncoder, chromaFormat)
    , m_transBitDepth (m_bitDepth)
    , m_nextBlockBestIntraPredIdx (kIntraUndefined)
    , m_nextBlockIsFls (true) {
    // stub
}

/**
    Destructor
*/
TransMode::~TransMode () {
    delete m_intraPred;
	for (UInt intra = 0; intra < 4; intra++) {
		for (UInt k = 0; k < g_numComps; k++) {
			delete[] m_intraBufferPred[intra][k];
			delete[] m_intraBufferRes[intra][k];
			delete[] m_intraBufferDctCoeff[intra][k];
			delete[] m_intraBufferQuant[intra][k];
			delete[] m_intraBufferDeQuant[intra][k];
			delete[] m_intraBufferRec[intra][k];
			
		}
		delete[] m_intraBufferPred[intra];
		delete[] m_intraBufferRes[intra];
		delete[] m_intraBufferDctCoeff[intra];
		delete[] m_intraBufferQuant[intra];
		delete[] m_intraBufferDeQuant[intra];
		delete[] m_intraBufferRec[intra];
	}
    delete[] m_intraBufferPred;
    delete[] m_intraBufferRes;
    delete[] m_intraBufferDctCoeff;
    delete[] m_intraBufferQuant;
    delete[] m_intraBufferDeQuant;
    delete[] m_intraBufferRec;
}

/**
    Initialize transform mode
*/
Void TransMode::InitModeSpecific () {
    m_intraPred = new IntraPred (m_blkWidth, m_blkHeight, m_bitDepth, m_chromaFormat, m_csc);
    m_maxTrDynamicRange = (g_minBpc <= 10) ? g_maxTrDynamicRange[0] : g_maxTrDynamicRange[1];
	m_intraBufferPred = new Int**[4];
	m_intraBufferRes = new Int**[4];
	m_intraBufferDctCoeff = new Int**[4];
	m_intraBufferQuant = new Int**[4];
	m_intraBufferDeQuant = new Int**[4];
	m_intraBufferRec = new Int**[4];
	for (UInt intra = 0; intra < 4; intra++) {
		m_intraBufferPred[intra] = new Int*[g_numComps];
		m_intraBufferRes[intra] = new Int*[g_numComps];
		m_intraBufferDctCoeff[intra] = new Int*[g_numComps];
		m_intraBufferQuant[intra] = new Int*[g_numComps];
		m_intraBufferDeQuant[intra] = new Int*[g_numComps];
		m_intraBufferRec[intra] = new Int*[g_numComps];
		for (UInt k = 0; k < g_numComps; k++) {
			UInt numPx = getWidth (k) * getHeight (k);
			m_intraBufferPred[intra][k] = new Int[numPx];
			m_intraBufferRes[intra][k] = new Int[numPx];
			m_intraBufferDctCoeff[intra][k] = new Int[numPx];
			m_intraBufferQuant[intra][k] = new Int[numPx];
			m_intraBufferDeQuant[intra][k] = new Int[numPx];
			m_intraBufferRec[intra][k] = new Int[numPx];
		}
	}
}


/**
    Test transform mode
*/
Void TransMode::Test () {
	UInt bitsSubstream0 = 0;
	m_bestIntraPredIdx = kIntraUndefined;
	Int* bestNModes;
	m_ec->SetModeType (kEcTransform);
	m_ec->SetUnderflowPreventionMode (m_underflowPreventionMode);
	bitsSubstream0 += EncodeModeHeader (false);
	bitsSubstream0 += EncodeFlatness (false);
	ResetBuffers ();
	m_intraPred->SetFirstLineInSlice (m_isFls);
	InitCurBlock ();
	InitNeighbors ();

	//! set up the intra predictor !//
	for (Int k = 0; k < g_numComps; k++) {
		m_intraPred->InitNeighbours (GetNeighborsLeft (k), GetNeighborsAbove (k), GetNeighborsLeftAbove (k), k);
	}
	if (m_isFls) {
		m_intraPred->PerfPrediction (kIntraFbls);
	}
	else {
		for (UInt predIndex = 0; predIndex < g_intra_numTotalModes; predIndex++) {
			m_intraPred->PerfPrediction ((IntraPredictorType)predIndex); // do all the predictions; dc, horizontal, vertical and diagonal
		}
		m_intraPred->FindBestNPredictors (GetCurBlkBuf (0), GetCurBlkBuf (1), GetCurBlkBuf (2));
		bestNModes = m_intraPred->GetMPMBuffer ();
		bitsSubstream0 += g_intra_numTotalModesBits;
	}

	//! test all intra predictors in bestNModes !//
	Int numIntraPredModes = m_isFls ? 1 : g_intra_numModesSecondPass;

	Bool intraPredictorIsValid[4] = { false, false, false, false };
	UInt intraPredictorRates[4] = { 0, 0, 0, 0 };
	UInt intraPredictorDistortions[4] = { 0, 0, 0, 0 };
	UInt intraPredictorRdCost[4] = { 0, 0, 0, 0 };

	for (Int intraIdx = 0; intraIdx < numIntraPredModes; intraIdx++) {
		UInt bitsPerSsm[4] = { bitsSubstream0, 0, 0, 0 };
		IntraPredictorType curIntraPred = m_isFls ? kIntraFbls : (IntraPredictorType)bestNModes[intraIdx];
		intraPredictorIsValid[intraIdx] = true;
		CalculateResidue (curIntraPred);
		Int sad[g_numComps] = { 0, 0, 0 };
		for (Int k = 0; k < g_numComps; k++) {
			Int numPx = getHeight (k)*getWidth (k);
			Int blkStride = m_blkWidth >> g_compScaleX[k][m_chromaFormat];
			m_transBitDepth = m_bitDepth + ((m_csc == kYCoCg && k > 0) ? 2 : 1);
			std::memcpy (GetIntraBufferPred (intraIdx, k), m_predBlk[k], numPx * sizeof (Int));
			std::memcpy (GetIntraBufferRes (intraIdx, k), m_resBlk[k], numPx * sizeof (Int));
			DctLlmForward (GetIntraBufferRes (intraIdx, k), GetIntraBufferDctCoeff (intraIdx, k), getWidth (k), getHeight (k));
			QuantLlm (GetIntraBufferDctCoeff (intraIdx, k), GetIntraBufferQuant (intraIdx, k), getWidth (k), getHeight (k), k);
			DeQuantLlm (GetIntraBufferQuant (intraIdx, k), GetIntraBufferDeQuant (intraIdx, k), getWidth (k), getHeight (k), k);
			DctLlmInverse (GetIntraBufferDeQuant (intraIdx, k), GetIntraBufferRec (intraIdx, k), getWidth (k), getHeight (k));

			//! clip reconstructed residual samples !//
			Int curCompBitDepth = m_bitDepth + (m_csc == kYCoCg && k > 0 ? 1 : 0);
			Int maxRecRes = (1 << curCompBitDepth) - 1;
			Int minRecRes = -((1 << curCompBitDepth) - 1);
			Int* recResBuffer = GetIntraBufferRec (intraIdx, k);
			for (UInt s = 0; s < (getWidth(k) * getHeight(k)); s++) {
				recResBuffer[s] = Clip3<Int> (minRecRes, maxRecRes, recResBuffer[s]);
			}

			for (UInt s = 0; s < (getWidth (k) * getHeight (k)); s++) {
				sad[k] += abs (GetIntraBufferRes (intraIdx, k)[s] - GetIntraBufferRec (intraIdx, k)[s]);
			}
			AddPredValue (GetIntraBufferRec (intraIdx, k), m_intraPred->GetIntraBuffer (curIntraPred, k), getWidth (k), getHeight (k), k);

			//! entropy coding !//
			m_ec->SetPos (m_posX, m_posY);
			bitsPerSsm[k + 1] = m_ec->EncodeTransformCoefficients (GetIntraBufferQuant (intraIdx, k), k, false, m_ssm);
			for (UInt i = 0; i < 4; i++) {
				if (m_ec->GetBitsPerGroup (i) > g_ec_groupSizeLimit) {
					intraPredictorIsValid[intraIdx] = false;
					break;
				}
			}
			if (bitsPerSsm[k + 1] > m_ssm->GetMaxSeSize ()) {
				intraPredictorIsValid[intraIdx] = false;
			}
		}

		//! calculate rate, distortion and rdCost for this intra predictor !//
		for (UInt ssmIdx = 0; ssmIdx < 4; ssmIdx++) {
			intraPredictorRates[intraIdx] += bitsPerSsm[ssmIdx];
		}
		switch (m_csc) {
			case kYCoCg:
				intraPredictorDistortions[intraIdx] = ((sad[0] * g_ycocgWeights[0] + 128) >> 8) + ((sad[1] * g_ycocgWeights[1] + 128) >> 8) + ((sad[2] * g_ycocgWeights[2] + 128) >> 8);
				break;
			case kYCbCr:
			case kRgb:
				intraPredictorDistortions[intraIdx] = sad[0] + sad[1] + sad[2];
				break;
		}
		intraPredictorRdCost[intraIdx] = ComputeRdCost (intraPredictorRates[intraIdx], intraPredictorDistortions[intraIdx]);
	}

	//! select best intra predictor !//
	UInt bestRdCost = LARGE_INT;
	Int bestIntraIdx = -1;
	UInt numValidIntra = 0;
	UInt bestRate = LARGE_INT;
	for (Int intraIdx = 0; intraIdx < numIntraPredModes; intraIdx++) {
		Bool a = (intraPredictorRdCost[intraIdx] < bestRdCost);
		Bool b = (intraPredictorRdCost[intraIdx] == bestRdCost) && (intraPredictorRates[intraIdx] < bestRate);
		if (intraPredictorIsValid[intraIdx] && (a || b)) {
			bestIntraIdx = intraIdx;
			bestRate = intraPredictorRates[intraIdx];
			bestRdCost = intraPredictorRdCost[intraIdx];
			numValidIntra++;
		}
	}
	
	//! copy buffers !//
	m_bestIntraPredIdx = m_isFls ? kIntraFbls : (IntraPredictorType)bestNModes[bestIntraIdx];
	if (numValidIntra > 0) {
		for (UInt k = 0; k < g_numComps; k++) {
			Int numPx = getHeight (k)*getWidth (k);
			std::memcpy (m_qResBlk[k], GetIntraBufferQuant (bestIntraIdx, k), numPx * sizeof (Int));
			std::memcpy (m_recBlk[k], GetIntraBufferRec (bestIntraIdx, k), numPx * sizeof (Int));
		}
		m_rate = intraPredictorRates[bestIntraIdx];
		m_distortion = intraPredictorDistortions[bestIntraIdx];
		m_rdCost = intraPredictorRdCost[bestIntraIdx];
	}
	else {
		m_rate = LARGE_INT;
		m_distortion = LARGE_INT;
		m_rdCost = LARGE_INT;
	}
}

/**
    Encode transform mode block
*/
Void TransMode::Encode () {
    m_ec->SetModeType (kEcTransform);
    m_ec->SetUnderflowPreventionMode (m_underflowPreventionMode);
    m_ssm->SetCurSyntaxElementMode (std::string ("XFORM"));
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
    if (!m_isFls) {
        EncodeBestIntraPredictor ();
    }

    // EC 
    for (Int k = 0; k < g_numComps; k++) {
        Int* pQuantCoeff = GetQuantBlkBuf (k);
        m_ec->EncodeTransformCoefficients (pQuantCoeff, k, true, m_ssm);
    }

    for (UInt ss = 0; ss < m_ssm->GetNumSubStreams (); ss++) {
        postBits[ss] = m_ssm->GetCurSyntaxElement (ss)->GetNumBits ();
        postBitsSum += postBits[ss];
    }
    m_rateWithoutPadding = postBitsSum - prevBitsSum;

	if (m_underflowPreventionMode) {
		m_rateWithoutPadding -= (9 * m_pps->GetRcStuffingBits ());
	}

    delete[] prevBits;
    delete[] postBits;

    //! copy to reconstructed frame buffer !//
	if (m_csc == kYCoCg) {
		CopyToYCoCgBuffer (m_posX);
	}
	else {
		CopyToRecFrame (m_posX, m_posY);
	}

    //! debug tracer !//
    if (enEncTracer) {
        fprintf (encTracer, "XFORM: colorSpace = %s, intraPredictor = %s\n", g_colorSpaceNames[m_csc].c_str (), g_intraModeNames[m_bestIntraPredIdx].c_str ());
        debugTracerWriteArray3 (encTracer, "XFORM", "NborLeft", m_neighborsLeftLen, m_neighborsLeft, m_chromaFormat);
        debugTracerWriteArray3 (encTracer, "XFORM", "NborAbove", m_neighborsAboveLen, m_neighborsAbove, m_chromaFormat);
        debugTracerWriteArray3 (encTracer, "XFORM", "NborLeftAbove", m_neighborsAboveLeftLen, m_neighborsLeftAbove, m_chromaFormat);
        debugTracerWriteArray3 (encTracer, "XFORM", "QuantBlk", m_blkWidth*m_blkHeight, m_qResBlk, m_chromaFormat);
        debugTracerWriteArray3 (encTracer, "XFORM", "RecBlk", m_blkWidth*m_blkHeight, m_recBlk, m_chromaFormat);
    }
}

/**
    Decode transform mode block
*/
Void TransMode::Decode () {
    m_ec->SetModeType (kEcTransform);
    m_ec->SetUnderflowPreventionMode (m_underflowPreventionMode);
    m_intraPred->SetFirstLineInSlice (m_isFls);

    InitNeighbors ();
    for (Int k = 0; k < g_numComps; k++) {
        m_intraPred->InitNeighbours (GetNeighborsLeft (k), GetNeighborsAbove (k), GetNeighborsLeftAbove (k), k);
    }

    //do best intra prediction 
    m_intraPred->PerfPrediction (m_bestIntraPredIdx);

    //copy best intra predictor values
    for (UInt k = 0; k < g_numComps; k++) {
        Int *ptrPred = GetPredBlkBuf (k);
        m_intraPred->AssignPredValue (ptrPred, m_bestIntraPredIdx, k);
    }

    //decode quantized residuals
    for (Int k = 0; k < g_numComps; k++) {
        Int* pQuant = GetQuantBlkBuf (k);
        m_ec->DecodeTransformCoefficients (pQuant, k, m_ssm);
    }

    //dequantize, inverse tranform and adding predictor values
    Reconstruct ();

    //! debug tracer !//
    if (enDecTracer) {
        fprintf (decTracer, "XFORM: colorSpace = %s, intraPredictor = %s\n", g_colorSpaceNames[m_csc].c_str (), g_intraModeNames[m_bestIntraPredIdx].c_str ());
        debugTracerWriteArray3 (decTracer, "XFORM", "NborLeft", m_neighborsLeftLen, m_neighborsLeft, m_chromaFormat);
        debugTracerWriteArray3 (decTracer, "XFORM", "NborAbove", m_neighborsAboveLen, m_neighborsAbove, m_chromaFormat);
        debugTracerWriteArray3 (decTracer, "XFORM", "NborLeftAbove", m_neighborsAboveLeftLen, m_neighborsLeftAbove, m_chromaFormat);
        debugTracerWriteArray3 (decTracer, "XFORM", "QuantBlk", m_blkWidth*m_blkHeight, m_qResBlk, m_chromaFormat);
        debugTracerWriteArray3 (decTracer, "XFORM", "RecBlk", m_blkWidth*m_blkHeight, m_recBlk, m_chromaFormat);
    }

	//! copy to reconstructed frame buffer !//
	if (m_csc == kYCoCg) {
		CopyToYCoCgBuffer (m_posX);
	}
	else {
		CopyToRecFrame (m_posX, m_posY);
	}
}

/**
    Reconstruct block
*/
Void TransMode::Reconstruct () {
    for (Int k = 0; k < g_numComps; k++) {
        Int blkStride = m_blkWidth >> g_compScaleX[k][m_chromaFormat];
        if (((getPosX (k) + getWidth (k)) > getSrcWidth (k)) || ((getPosY (k) + getHeight (k)) > getSrcHeight (k))) {
            return;
        }

        // set the bitdepth
        m_transBitDepth = m_bitDepth + ((m_csc == kYCoCg && k > 0) ? 2 : 1);

        if (!m_isEncoder) {
            // dequant
            DeQuantLlm (GetQuantBlkBuf (k), GetDeQuantBlkBuf (k), getWidth (k), getHeight (k), k);
        }

        // inverse transform
        DctLlmInverse (GetDeQuantBlkBuf (k), GetRecBlkBuf (k), getWidth (k), getHeight (k));

        // add predictor back to the residue
        AddPredValue (GetRecBlkBuf (k), GetPredBlkBuf (k), getWidth (k), getHeight (k), k);
    }
}

/**
    Parse best intra predictor from bitstream
*/
Void TransMode::DecodeBestIntraPredictor () {
    IntraPredictorType predictor;
    Bool isIntraSignaled = !m_nextBlockIsFls;
    if (isIntraSignaled) {
        UInt curSsmIdx = 0;
        predictor = (IntraPredictorType)DecodeFixedLengthCodeSsm (g_intra_numTotalModesBits, curSsmIdx);
    }
    else {
        predictor = kIntraFbls;
    }
    m_nextBlockBestIntraPredIdx = predictor;
}

/**
    Update QP
*/
Void TransMode::UpdateQp () {
    SetQp (m_rateControl->GetQp ());
}

/**
    Swap current block and next block intra predictor (for decoder)
*/
Void TransMode::NextBlockIntraSwap () {
    m_bestIntraPredIdx = m_nextBlockBestIntraPredIdx;
}

/**
    Calculate residue for best intra predictor
    */
Void TransMode::CalculateResidue () {
    for (Int k = 0; k < g_numComps; k++) {
        Int* ptrOrg = GetCurBlkBuf (k);
        Int* ptrPred = GetPredBlkBuf (k);
        Int* ptrPredError = GetResBlkBuf (k);
        m_intraPred->AssignPredValue (ptrPred, m_bestIntraPredIdx, k);
        for (UInt x = 0; x < getWidth (k) * getHeight (k); x++) {
            ptrPredError[x] = ptrOrg[x] - ptrPred[x];
        }
    }
}

/**
    Calculate residue for selected intra predictor

    @param predictor selected intra predictor
    */
Void TransMode::CalculateResidue (IntraPredictorType predictor) {
    for (Int k = 0; k < g_numComps; k++) {
        Int* ptrOrg = GetCurBlkBuf (k);
        Int* ptrPred = GetPredBlkBuf (k);
        Int* ptrPredError = GetResBlkBuf (k);
        m_intraPred->AssignPredValue (ptrPred, predictor, k);
        for (UInt x = 0; x < getWidth (k) * getHeight (k); x++) {
            ptrPredError[x] = ptrOrg[x] - ptrPred[x];
        }
    }
}

/**
    Encode best intra predictor to bitstream
    */
Void TransMode::EncodeBestIntraPredictor () {
    UInt substreamId = 0;
    UInt code = (UInt)m_bestIntraPredIdx;
    m_ssm->GetCurSyntaxElement (substreamId)->AddSyntaxWord (code, g_intra_numTotalModesBits);
}

/**
    Add predicted block to reconstructed residual block

    @param pRec reconstructed residuals
    @param pPred intra-predicted block
    @param width block width
    @param height block height
    @param k component index
    */
Void TransMode::AddPredValue (Int* pRec, Int* pPred, UInt width, UInt height, UInt k) {
    Int max = (1 << m_bitDepth) - 1;
    Int min = (m_csc == kYCoCg && k > 0 ? -1 * (1 << m_bitDepth) : 0);
    for (UInt x = 0; x < width * height; x++) {
        pRec[x] = Clip3<Int> (min, max, pRec[x] + pPred[x]);
    }
}

/**
    Forward 2D DCT transform (SOIT)

    @param pBlk array of intra predicted residuals
    @param pCoeff array of transform coefficients
    @param width block width
    @param height block height
    */
Void TransMode::DctLlmForward (Int* pBlk, Int* pCoeff, UInt width, UInt height) {
    UInt numSamples = width * height;
    Int* tempArray = new Int[numSamples];
    // Pre-shift left by DCT8_2D_UP_SHIFT
    for (UInt s = 0; s < numSamples; s++) {
        tempArray[s] = (pBlk[s] << g_dctPreShift);
    }

    // Forward DCT for each row
    for (UInt row = 0; row < height; row++) {
        switch (width) {
        case 4:
            dct4_II_fw_LLM_int (&tempArray[row * width]); // transform applied in-place
            break;
        case 8:
            dct8_II_fw_LLM_int (&tempArray[row * width]); // transform applied in-place
            break;
        default:
            fprintf (stderr, "TransMode::DctLlmForward(): unsupported width: %d.\n", width);
            exit (EXIT_FAILURE);
        }
        std::memcpy (&pCoeff[row * width], &tempArray[row * width], width * sizeof (Int));
    }

    // Forward Haar for each column
    if (height > 1) {
        for (UInt col = 0; col < width; col++) {
            Int temp = pCoeff[col] + pCoeff[col + width];
            pCoeff[col + width] = pCoeff[col] - pCoeff[col + width];
            pCoeff[col] = temp;
        }
    }

    // Post-shift right by DCT8_2D_UP_SHIFT
    for (UInt s = 0; s < numSamples; s++) {
        pCoeff[s] = (pCoeff[s] + g_dctPreShiftRound) >> g_dctPreShift;
    }
    delete[] tempArray;
}

/**
    Inverse 2D DCT transform (SOIT)

    @param pCoeff array of reconstructed transform coefficients
    @param pBlk array of reconstructed predicted residuals
    @param width block width
    @param height block height
    */
Void TransMode::DctLlmInverse (Int* pCoeff, Int* pBlk, UInt width, UInt height) {
    Int* tempArray = new Int[width * height];

    // Haar on columns
    if (height > 1) {
        for (UInt col = 0; col < width; col++) {
            tempArray[col] = pCoeff[col] + pCoeff[col + width];
            tempArray[col + width] = pCoeff[col] - pCoeff[col + width];
        }
    }
    else {
        for (UInt col = 0; col < width; col++) {
            tempArray[col] = pCoeff[col];
        }
    }

    // inverse DCT for each row
    for (UInt row = 0; row < height; row++) {
        switch (width) {
        case 4:
            dct4_II_bw_LLM_int (&tempArray[row * width]); // transform applied in-place
            break;
        case 8:
            dct8_II_bw_LLM_int (&tempArray[row * width]); // transform applied in-place
            break;
        default:
            fprintf (stderr, "TransMode::DctLlmInverse(): unsupported width: %d.\n", width);
            exit (EXIT_FAILURE);
        }
    }

    // Post-shift right by DCT8_2D_UP_SHIFT
	for (UInt s = 0; s < (width*height); s++) {
		Int sign = (tempArray[s] < 0 ? -1 : 1);
		pBlk[s] = sign * ((abs (tempArray[s]) + g_dctDeQuantRound) >> g_dctDeQuantBits);
    }

    //! clear memory !//
    delete[] tempArray;
}

/**
    Quantize transform coefficients (includes noramlization)

    @param pSrc array of transform coefficients
    @param pDes array of quantized transform coefficients
    @param width block width
    @param height block height
    @param k component index
    */
Void TransMode::QuantLlm (Int* pSrc, Int* pDes, UInt width, UInt height, UInt k) {
	Int qp = GetCurCompQpMod (m_qp, k);
    UInt qp_per = qp >> 3;
    UInt qp_rem = qp & 0x07;

    // get the forward quant table and mapping based on block component dimensions
    const UInt* quantTable = NULL;
    const UInt* mapping = NULL;
    if (width == 8 && height == 2) { // 8x2 block component
        quantTable = g_dctForwardQuant_8x2[qp_rem];
        mapping = g_dctQuantMapping_8x2;
    }
    else if (width == 4 && height == 2) { // 4x2 block component
        quantTable = g_dctForwardQuant_4x2[qp_rem];
        mapping = g_dctQuantMapping_4x2;
    }
    else if (width == 4 && height == 1) { // 4x1 block component
        quantTable = g_dctForwardQuant_4x1[qp_rem];
        mapping = g_dctQuantMapping_4x1;
    }
    else {
        fprintf (stderr, "TransMode::QuantLlm: unsupported width/height: %d/%d.\n", width, height);
        exit (EXIT_FAILURE);
    }

    // normalization and quantization
    for (UInt row = 0; row < height; row++) {
        for (UInt col = 0; col < width; col++) {
            UInt curQuant = quantTable[mapping[col]];
            UInt idx = (row * width) + col;
            Int sign = (pSrc[idx] < 0 ? -1 : 1);
            pDes[idx] = sign * ((curQuant * abs (pSrc[idx]) + (g_dctQuantDeadZone << qp_per)) >> (g_dctQuantBits + qp_per));
        }
    }
}

/**
    De-quantize transform coefficients (includes noramlization)

    @param pSrc array of quantized transform coefficients
    @param pDes array of reconstructed transform coefficients
    @param width block width
    @param height block height
    @param k component index
    */
Void TransMode::DeQuantLlm (Int* pSrc, Int* pDes, UInt width, UInt height, UInt k) {
	Int qp = GetCurCompQpMod (m_qp, k);
    UInt qp_per = qp >> 3;
    UInt qp_rem = qp & 0x07;

    // get the inverse quant table and mapping based on block component dimensions
    const UInt* quantTable = NULL;
    const UInt* mapping = NULL;
    if (width == 8 && height == 2) { // 8x2 block component
        quantTable = g_dctInverseQuant_8x2[qp_rem];
        mapping = g_dctQuantMapping_8x2;
    }
    else if (width == 4 && height == 2) { // 4x2 block component
        quantTable = g_dctInverseQuant_4x2[qp_rem];
        mapping = g_dctQuantMapping_4x2;
    }
    else if (width == 4 && height == 1) { // 4x1 block component
        quantTable = g_dctInverseQuant_4x1[qp_rem];
        mapping = g_dctQuantMapping_4x1;
    }
    else {
        fprintf (stderr, "TransMode::QuantLlm: unsupported width/height: %d/%d.\n", width, height);
        exit (EXIT_FAILURE);
    }

    // dequantization
    for (UInt row = 0; row < height; row++) {
        for (UInt col = 0; col < width; col++) {
            UInt curQuant = quantTable[mapping[col]];
            UInt idx = (row * width) + col;
            pDes[idx] = (pSrc[idx] * curQuant) << qp_per;
        }
    }
}

/**
    Forward 4-point DCT (SOIT implementation)

    @param x array of intra-predicted residuals (modififed in-place)
    */
Void TransMode::dct4_II_fw_LLM_int (Int *x) {
    UInt n = 4, n1 = 2;
    int *a = new int[n];

    // first stage
    for (UInt i = 0; i < n1; i++) {
        a[i] = x[i] + x[n - 1 - i];
        a[n - 1 - i] = x[i] - x[n - 1 - i];
    }

    // second stage (even part)
    x[0] = a[0] + a[1];
    x[2] = a[0] - a[1];

    // second stage (odd part)
    x[1] = (g_dctA4 * a[2] + g_dctB4 * a[3]) >> g_dctShift4;
    x[3] = (g_dctA4 * a[3] - g_dctB4 * a[2]) >> g_dctShift4;

    // free memory
    delete[] a;
}

/**
    Inverse 4-point DCT (SOIT implementation)

    @param x array of reconstructed transform coefficients (modififed in-place)
    */
Void TransMode::dct4_II_bw_LLM_int (Int *x) {
    int n = 4, n1 = 2;
    int *a = new int[n];

    // first stage (even part)
    a[0] = x[0] + x[2];
    a[1] = x[0] - x[2];

    // first stage (odd part)
    a[2] = (g_dctA4 * x[1] - g_dctB4 * x[3]) >> g_dctShift4;
    a[3] = (g_dctB4 * x[1] + g_dctA4 * x[3]) >> g_dctShift4;

    // second stage
    x[0] = a[0] + a[3];
    x[1] = a[1] + a[2];
    x[2] = a[1] - a[2];
    x[3] = a[0] - a[3];

    // free memory
    delete[] a;
}

/**
    Forward 8-point DCT (SOIT implementation)

    @param x array of intra-predicted residuals (modififed in-place)
    */
Void TransMode::dct8_II_fw_LLM_int (Int *x) {
    int n = 8, n1 = 4, n2 = 2;
    int *ea = new int[n1];
    int *eb = new int[n1];
    int *ec = new int[n1];
    int *da = new int[n1];
    int *db = new int[n1];
    int *dc = new int[n1];
    int *dd = new int[n1];

    // first stage (even part)
    ea[0] = x[0] + x[7];
    ea[1] = x[1] + x[6];
    ea[2] = x[2] + x[5];
    ea[3] = x[3] + x[4];

    // second stage (even part)
    eb[0] = ea[0] + ea[3];
    eb[1] = ea[1] + ea[2];
    eb[2] = ea[1] - ea[2];
    eb[3] = ea[0] - ea[3];

    // third stage (even part)
    ec[0] = eb[0] + eb[1];
    ec[1] = eb[0] - eb[1];
    ec[2] = (g_dctA8 * eb[2] + g_dctB8 * eb[3]) >> g_dctShift4;
    ec[3] = (g_dctA8 * eb[3] - g_dctB8 * eb[2]) >> g_dctShift4;

    // first stage (odd part)
    da[0] = x[3] - x[4];
    da[1] = x[2] - x[5];
    da[2] = x[1] - x[6];
    da[3] = x[0] - x[7];

    // second stage (odd part)
    db[0] = (g_dctE8 * da[0] + g_dctZ8 * da[3]) >> g_dctShift8;
    db[1] = (g_dctG8 * da[1] + g_dctD8 * da[2]) >> g_dctShift8;
    db[2] = (g_dctG8 * da[2] - g_dctD8 * da[1]) >> g_dctShift8;
    db[3] = (g_dctE8 * da[3] - g_dctZ8 * da[0]) >> g_dctShift8;

    // third stage (odd part)
    dc[0] = db[0] + db[2];
    dc[1] = db[3] - db[1];
    dc[2] = db[0] - db[2];
    dc[3] = db[1] + db[3];

    // 4th stage (odd part)
    dd[0] = dc[3] - dc[0];
    dd[1] = dc[1];
    dd[2] = dc[2];
    dd[3] = dc[0] + dc[3];

    // final stage (permutation)
    x[0] = ec[0];
    x[1] = dd[3];
    x[2] = ec[2];
    x[3] = dd[1];
    x[4] = ec[1];
    x[5] = dd[2];
    x[6] = ec[3];
    x[7] = dd[0];

    // free memory
    delete[] ea;
    delete[] eb;
    delete[] ec;
    delete[] da;
    delete[] db;
    delete[] dc;
    delete[] dd;
}

/**
    Inverse 8-point DCT (SOIT implementation)

    @param x array of reconstructed transform coefficients (modififed in-place)
    */
Void TransMode::dct8_II_bw_LLM_int (Int *x) {
    // assumes that the input vector has already been pre-scaled by appropriate factors
    int n = 8, n1 = 4;

    int *z = new int[n];
    int *ea = new int[n1];
    int *eb = new int[n1];
    int *da = new int[n1];
    int *db = new int[n1];
    int *dc = new int[n1];

    // input reordering
    z[0] = x[0];
    z[1] = x[4];
    z[2] = x[2];
    z[3] = x[6];
    z[4] = x[7];
    z[5] = x[3];
    z[6] = x[5];
    z[7] = x[1];

    // first stage, even part
    ea[0] = z[0] + z[1];
    ea[1] = z[0] - z[1];
    ea[2] = (g_dctA8 * z[2] - g_dctB8 * z[3]) >> g_dctShift4;
    ea[3] = (g_dctB8 * z[2] + g_dctA8 * z[3]) >> g_dctShift4;

    // second stage, even part
    eb[0] = ea[0] + ea[3];
    eb[1] = ea[1] + ea[2];
    eb[2] = ea[1] - ea[2];
    eb[3] = ea[0] - ea[3];

    // first stage, odd part
    da[0] = -z[4] + z[7];
    da[1] = z[5];
    da[2] = z[6];
    da[3] = z[4] + z[7];

    // second stage, odd part
    db[0] = da[0] + da[2];
    db[1] = da[3] - da[1];
    db[2] = da[0] - da[2];
    db[3] = da[1] + da[3];

    // third stage, odd part
    dc[0] = (g_dctE8 * db[0] - g_dctZ8 * db[3]) >> g_dctShift8;
    dc[1] = (g_dctG8 * db[1] - g_dctD8 * db[2]) >> g_dctShift8;
    dc[2] = (g_dctD8 * db[1] + g_dctG8 * db[2]) >> g_dctShift8;
    dc[3] = (g_dctZ8 * db[0] + g_dctE8 * db[3]) >> g_dctShift8;

    // final stage
    x[0] = eb[0] + dc[3];
    x[1] = eb[1] + dc[2];
    x[2] = eb[2] + dc[1];
    x[3] = eb[3] + dc[0];
    x[4] = eb[3] - dc[0];
    x[5] = eb[2] - dc[1];
    x[6] = eb[1] - dc[2];
    x[7] = eb[0] - dc[3];

    // free memory
    delete[] z;
    delete[] ea;
    delete[] eb;
    delete[] da;
    delete[] db;
    delete[] dc;
}