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

/* Copyright (c) 2015-2017 MediaTek Inc., BP-Skip mode is modified and/or implemented by MediaTek Inc. based on BpMode.cpp
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
\file       BpMode.cpp
\brief      Block Prediction Mode
*/

#include "BpMode.h"
#include "TypeDef.h"
#include "Misc.h"
#include <string.h>
#include <iostream>

extern Bool enEncTracer;
extern Bool enDecTracer;
extern FILE* encTracer;
extern FILE* decTracer;

/**
    BP mode constructor

    @param bpp compressed bitrate
    @param bitDepth bit depth
    @param isEncoder flag to indicate encoder or decoder
    @param chromaFormat chroma sampling format
    @param csc color space
    @param numPx num px in current block
    @param BPSkip flag to indicate BP-SKIP mode
    */
BpMode::BpMode (Int bpp, Int bitDepth, Bool isEncoder, ChromaFormat chromaFormat, ColorSpace csc, ColorSpace targetCsc, Bool isBpSkip)
    : Mode (isBpSkip ? kModeTypeBPSkip : kModeTypeBP, csc, targetCsc, bpp, bitDepth, isEncoder, chromaFormat)
    , m_bpskip (isBpSkip)
    , m_srSize (g_defaultBpSearchRange)
    , m_numSubBlocks (4)
    , m_partBitsPerLine (2) {
    m_bpv2x2 = new Int[m_numSubBlocks];
    m_use2x2 = new Bool[m_numSubBlocks];
    m_distortion2x2 = new Int[m_numSubBlocks];
    m_temp_bpv2x2 = new Int[m_numSubBlocks];
    m_temp_use2x2 = new Bool[m_numSubBlocks];
    m_distortion2x1 = new Int[2 * m_numSubBlocks];
    m_bpv2x1 = new Int[2 * m_numSubBlocks];
    m_temp_bpv2x1 = new Int[2 * m_numSubBlocks];
    m_mapSrId = new Int[g_defaultBpSearchRange];
    m_mapSrPos = new Int[g_defaultBpSearchRange];
}

/**
    Destructor
    */
BpMode::~BpMode () {
    if (m_bpv2x1) { delete[] m_bpv2x1; m_bpv2x1 = NULL; }
    if (m_bpv2x2) { delete[] m_bpv2x2; m_bpv2x2 = NULL; }
    if (m_use2x2) { delete[] m_use2x2; m_use2x2 = NULL; }
    if (m_distortion2x1) { delete[] m_distortion2x1; m_distortion2x1 = NULL; }
    if (m_distortion2x2) { delete[] m_distortion2x2; m_distortion2x2 = NULL; }
    if (m_temp_bpv2x2) { delete[] m_temp_bpv2x2; m_temp_bpv2x2 = NULL; }
    if (m_temp_bpv2x1) { delete[] m_temp_bpv2x1; m_temp_bpv2x1 = NULL; }
    if (m_temp_use2x2) { delete[] m_temp_use2x2; m_temp_use2x2 = NULL; }
    if (m_mapSrId) { delete[] m_mapSrId; m_mapSrId = NULL; }
    if (m_mapSrPos) { delete[] m_mapSrPos; m_mapSrPos = NULL; }
    for (UInt i = 0; i < m_bpSrStorage.size (); i++) {
        delete m_bpSrStorage.at (i);
    }
    m_bpSrStorage.clear ();
    DeleteBufferMemory ();
}

/**
    Clear all buffers
    */
Void BpMode::DeleteBufferMemory () {
    for (Int c = 0; c < g_numComps; c++) {
        if (m_qResBlkBuf2x1[c]) { delete[] m_qResBlkBuf2x1[c]; m_qResBlkBuf2x1[c] = NULL; }
        if (m_qResBlkBuf2x2[c]) { delete[] m_qResBlkBuf2x2[c]; m_qResBlkBuf2x2[c] = NULL; }
        if (m_iqResBlkBuf2x1[c]) { delete[] m_iqResBlkBuf2x1[c]; m_iqResBlkBuf2x1[c] = NULL; }
        if (m_iqResBlkBuf2x2[c]) { delete[] m_iqResBlkBuf2x2[c]; m_iqResBlkBuf2x2[c] = NULL; }
        if (m_resBlkBuf2x2[c]) { delete[] m_resBlkBuf2x2[c]; m_resBlkBuf2x2[c] = NULL; }
        if (m_resBlkBuf2x1[c]) { delete[] m_resBlkBuf2x1[c]; m_resBlkBuf2x1[c] = NULL; }
        if (m_recBlkBuf2x2[c]) { delete[] m_recBlkBuf2x2[c]; m_recBlkBuf2x2[c] = NULL; }
        if (m_recBlkBuf2x1[c]) { delete[] m_recBlkBuf2x1[c]; m_recBlkBuf2x1[c] = NULL; }
    }
}

/**
    Fetch data from another coding mode object
    */
Void BpMode::FetchDataFromReference () {
    FetchBpvs (m_bpModeRef);
}

/**
    Fetch BPVs from BP object for BP-SKIP
    */
Void BpMode::FetchBpvs (BpMode* bpModeRef) {
    for (UInt sb = 0; sb < m_numSubBlocks; sb++) {
        Int block_part_y = (sb >> m_partBitsPerLine);
        Int block_part_x = sb - (block_part_y << m_partBitsPerLine);
        Int p0 = (block_part_y << (m_partBitsPerLine + 1)) + block_part_x;
        Int p1 = p0 + (1 << m_partBitsPerLine);
        SetBpv2x2 (sb, bpModeRef->GetBpv2x2 (sb));
        SetBpv2x1 (p0, bpModeRef->GetBpv2x1 (p0));
        SetBpv2x1 (p1, bpModeRef->GetBpv2x1 (p1));
    }
}

/**
    Initialize BP mode
    */
Void BpMode::InitModeSpecific () {
    Int numPx = getWidth (0) * getHeight (0);
    m_partitionSize = 2;
    m_bpvNumBits = 6;
    for (Int c = 0; c < g_numComps; c++) {
        m_recBlkBuf2x1[c] = new Int[numPx];
        m_resBlkBuf2x1[c] = new Int[numPx];
        m_qResBlkBuf2x1[c] = new Int[numPx];
        m_qResBlkBuf2x2[c] = new Int[numPx];
        m_iqResBlkBuf2x1[c] = new Int[numPx];
        m_iqResBlkBuf2x2[c] = new Int[numPx];
        m_recBlkBuf2x2[c] = new Int[numPx];
        m_resBlkBuf2x2[c] = new Int[numPx];
    }

    BpSearchRange* srA = new BpSearchRange (g_defaultBpSearchRange, m_blkHeight, m_origSrcCsc, m_csc, m_chromaFormat, true);
    BpSearchRange* srB = new BpSearchRange (g_defaultBpSearchRange, m_blkHeight, m_origSrcCsc, m_csc, m_chromaFormat, true);
    BpSearchRange* srC = new BpSearchRange (g_defaultBpSearchRange, m_blkHeight, m_origSrcCsc, m_csc, m_chromaFormat, false);
    m_bpSrStorage.push_back (srA);
    m_bpSrStorage.push_back (srB);
    m_bpSrStorage.push_back (srC);
}

/**
    Test encode BP block
    */
Void BpMode::Test () {
    Int bits = 0;
    Int numPx = m_blkWidth * m_blkHeight;
    Int distortion = 0;
    Int rdCost = 0;
    UInt perSsmBits[4] = { 0 };
    m_ec->SetModeType (kEcBp);
    m_ec->SetUnderflowPreventionMode (m_underflowPreventionMode);
    bits += EncodeModeHeader (false);
    bits += EncodeFlatness (false);
    ResetBuffers ();
    InitCurBlock ();
    SetSrPositions ();
    InitSearchRange ();
    if (!m_bpskip) {
        BpvSearch ();               // perform BPV search
    }
    ComputeResidual ();             // compute BP prediction residual
    QuantizeResidual ();            // quantize residuals for both 2x1 and 2x2 partitions
    EncoderReconstruct ();          // reconstruct the block for both 2x1 and 2x2 partitions
	PartitionDecisionParallel (bits);
}

/**
    Encode BP block
    */
Void BpMode::Encode () {
    m_ec->SetModeType (kEcBp);
    m_ec->SetUnderflowPreventionMode (m_underflowPreventionMode);
    m_ssm->SetCurSyntaxElementMode (m_bpskip ? std::string ("BP-SKIP") : std::string ("BP"));
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
    EncodeBpv (true);

    if (!m_bpskip) {
        for (Int k = 0; k < g_numComps; k++) {
            m_ec->EncodeResiduals (GetQuantBlkBuf (k), k, true, m_ssm);
        }
    }
    for (UInt ss = 0; ss < m_ssm->GetNumSubStreams (); ss++) {
        postBits[ss] = m_ssm->GetCurSyntaxElement (ss)->GetNumBits ();
        postBitsSum += postBits[ss];
    }

    delete[] prevBits;
    delete[] postBits;

	if (m_csc == kYCoCg) {
		CopyToYCoCgBuffer (m_posX);
	}
	else {
		CopyToRecFrame (m_posX, m_posY);
	}

    //! debug tracer !//
    if (enEncTracer) {
        const char* curModeString = (m_bpskip ? "BP_SKIP" : "BP");
        fprintf (encTracer, "%s: colorSpace = %s\n", curModeString, g_colorSpaceNames[m_csc].c_str ());
        if (!m_bpskip) {
            debugTracerWriteArray3 (encTracer, curModeString, "QuantBlk", m_blkWidth * m_blkHeight, m_qResBlk, m_chromaFormat);
        }
        EncoderDebugBpv ();
        debugTracerWriteArray3 (encTracer, curModeString, "RecBlk", m_blkWidth * m_blkHeight, m_recBlk, m_chromaFormat);
    }
}

/**
    Swap BPV from next block to current block (for decoder)
    */
Void BpMode::BpvSwap () {
    // BPV table is all in substream 0
    for (UInt sb = 0; sb < m_numSubBlocks; sb++) {
        m_use2x2[sb] = m_temp_use2x2[sb];
    }
    // 2x2 partition for sub-block 0 in substream 0
    m_bpv2x2[0] = m_temp_bpv2x2[0];

    // 2x1 partitions for sub-block 0 in substream 0
    m_bpv2x1[0] = m_temp_bpv2x1[0];
    m_bpv2x1[m_numSubBlocks] = m_temp_bpv2x1[m_numSubBlocks];
}

/**
    Decode BP block
    */
Void BpMode::Decode () {
    SetSrPositions ();
    InitSearchRange ();
    m_ec->SetModeType (kEcBp);
    m_ec->SetUnderflowPreventionMode (m_underflowPreventionMode);
    DecodeBpvCurBlock ();
    if (!m_bpskip) {
        for (Int k = 0; k < g_numComps; k++) {
            Int* pQuant = GetQuantBlkBuf (k);
            m_ec->DecodeResiduals (pQuant, k, m_ssm);
        }
    }
    DecoderReconstruct ();

	//! copy to reconstructed frame buffer !//
	if (m_csc == kYCoCg) {
		CopyToYCoCgBuffer (m_posX);
	}
	else {
		CopyToRecFrame (m_posX, m_posY);
	}

    //! debug tracer !//
    if (enDecTracer) {
        const char* curModeString = (m_bpskip ? "BP_SKIP" : "BP");
        fprintf (decTracer, "%s: colorSpace = %s\n", curModeString, g_colorSpaceNames[m_csc].c_str ());
        if (!m_bpskip) {
            debugTracerWriteArray3 (decTracer, curModeString, "QuantBlk", m_blkWidth * m_blkHeight, m_qResBlk, m_chromaFormat);
        }
        DecoderDebugBpv ();
        debugTracerWriteArray3 (decTracer, curModeString, "RecBlk", m_blkWidth * m_blkHeight, m_recBlk, m_chromaFormat);
    }
}

/**
    Update QP
    */
Void BpMode::UpdateQp () {
    Int minQp = GetMinQp (m_pps);
    Int maxQp = m_rateControl->GetMaxQp ();
    Int offset;
    switch (m_chromaFormat) {
    case k444:
        offset = GetIsFls () ? BP_QP_OFFSET_FLS : BP_QP_OFFSET_NFLS;
        break;
    case k422:
    case k420:
        offset = 0;
        break;
    default:
        fprintf (stderr, "BpMode::UpdateQp(): unknown chroma format: %d.\n", m_chromaFormat);
        exit (EXIT_FAILURE);
    }
    SetQp (Clip3<Int> (minQp, maxQp, m_rateControl->GetQp () + offset));
}

/**
    Perform BPV search for 2x1 and 2x2 partitions
    */
Void BpMode::BpvSearch () {
    Int srId;
    Int srPos;

    //! BP search for 2x2 partitions !//
    for (UInt p = 0; p < m_numSubBlocks; p++) {
        Int tempSad[3] = { 0, 0, 0 };
        Int cumSad = 0;
        Int minSad = LARGE_INT;
        Int offset = -1;
        for (Int j = 0; j < GetSrSize (0); j++) {
            tempSad[0] = 0;
            tempSad[1] = 0;
            tempSad[2] = 0;
            Bool isCurBpvValid = true;
            for (Int k = 0; k < g_numComps; k++) {
                BpvMapping (j, k, srId, srPos);
                if (srId == -1 || srPos == -1) {
                    isCurBpvValid = false;
                    continue;
                }
                Int* pCur = GetCurBlkBuf (k) + GetPartitionStartOffset (p, k, k2x2);
                Int* pSrUpper = m_bpSrStorage[srId]->GetDataBuffer (k, GetPartitionSearchRange (p, k, k2x2, kUpper));
                Int* pSrLower = (srId == 0)
                    ? m_bpSrStorage[2]->GetDataBuffer (k, 0) + GetOverlapRegionOffset (k)
                    : m_bpSrStorage[srId]->GetDataBuffer (k, GetPartitionSearchRange (p, k, k2x2, kLower));
                tempSad[k] += CalcSad (pSrUpper + srPos, pCur, GetPartitionSize (k));
                if (k == 0 || m_chromaFormat == k444 || m_chromaFormat == k422) {
                    tempSad[k] += CalcSad (pSrLower + srPos, pCur + getWidth (k), GetPartitionSize (k));
                }
            }
            if (!isCurBpvValid) {
                continue;
            }
            switch (m_csc) {
            case kRgb:
            case kYCbCr:
                cumSad = tempSad[0] + tempSad[1] + tempSad[2];
                break;
            case kYCoCg:
                cumSad = tempSad[0] + ((tempSad[1] + tempSad[2] + 1) >> 1);
                break;
            default:
                assert (false);
            }
            if (cumSad < minSad) {
                offset = j;
                minSad = cumSad;
            }
        }
        assert (offset >= 0 && offset <= m_srSize);
        SetBpv2x2 (p, offset);
    }

    //! BP search for 2x1 partitions !//
    for (UInt p = 0; p < (2 * m_numSubBlocks); p++) {
        Int tempSad[3] = { 0, 0, 0 };
        Int cumSad = 0;
        Int minSad = LARGE_INT;
        Int offset = -1;
        for (Int j = 0; j < GetSrSize (0); j++) {
            tempSad[0] = 0;
            tempSad[1] = 0;
            tempSad[2] = 0;
            Bool isCurBpvValid = true;
            for (Int k = 0; k < g_numComps; k++) {
                BpvMapping (j, k, srId, srPos);
                if (srId == -1 || srPos == -1) {
                    isCurBpvValid = false;
                    continue;
                }
                Int* pCur = GetCurBlkBuf (k) + GetPartitionStartOffset (p, k, k2x1);
                Int curSrBuffer = (m_bpSrStorage[srId]->GetIsPrevRecLine ())
                    ? m_bpSrStorage[srId]->GetNumRows (k) - 1
                    : GetPartitionSearchRange (p, k, k2x1, kUpper);
                Int* pSr = (srId == 0 && p >= m_numSubBlocks)
                    ? m_bpSrStorage[2]->GetDataBuffer (k, 0) + GetOverlapRegionOffset (k)
                    : m_bpSrStorage[srId]->GetDataBuffer (k, curSrBuffer);
                if (!(m_chromaFormat == k420 && k > 0 && p >= m_numSubBlocks)) {
                    tempSad[k] += CalcSad (pSr + srPos, pCur, GetPartitionSize (k));
                }
            }

            if (!isCurBpvValid) {
                continue;
            }

            switch (m_csc) {
            case kRgb:
            case kYCbCr:
                cumSad = tempSad[0] + tempSad[1] + tempSad[2];
                break;
            case kYCoCg:
                cumSad = tempSad[0] + ((tempSad[1] + tempSad[2] + 1) >> 1);
                break;
            default:
                assert (false);
            }

            if (cumSad < minSad) {
                offset = j;
                minSad = cumSad;
            }
        }
        if (offset < 0 || offset >= m_srSize) {
            fprintf (stderr, "BpMode::BpvSearch(): invalid BPV index: %d.\n", offset);
            exit (EXIT_FAILURE);
        }
        m_bpv2x1[p] = offset;
    }
}

/**
    Compute residual using BP predictors
    */
Void BpMode::ComputeResidual () {
    for (Int k = 0; k < g_numComps; k++) {
        Int* ptrOrg = GetCurBlkBuf (k);
        Int* ptrRes2x1 = GetResBlkBuf2x1 (k);
        Int* ptrRes2x2 = GetResBlkBuf2x2 (k);

        // 2x1
        for (UInt p = 0; p < (2 * m_numSubBlocks); p++) {
            Int lumaBpv = GetBpv2x1 (p);
            Int srId = 0;
            Int srPos = 0;
            BpvMapping (lumaBpv, k, srId, srPos);
            Int curSrBuffer = (m_bpSrStorage[srId]->GetIsPrevRecLine ())
                ? m_bpSrStorage[srId]->GetNumRows (k) - 1
                : GetPartitionSearchRange (p, k, k2x1, kUpper);
            Int* pSr;
            if (srId == 0 && p >= m_numSubBlocks) {
                pSr = m_bpSrStorage[2]->GetDataBuffer (k, 0) + GetOverlapRegionOffset (k);
            }
            else {
                pSr = m_bpSrStorage[srId]->GetDataBuffer (k, curSrBuffer);
            }
            for (Int i = 0; i < GetPartitionSize (k); i++) {
                Int x = i + (p * GetPartitionSize (k));
                if (!(m_chromaFormat == k420 && k > 0 && p >= m_numSubBlocks)) {
                    ptrRes2x1[x] = ptrOrg[x] - pSr[srPos + i];
                }
            }
        }

        // 2x2
        for (UInt p = 0; p < m_numSubBlocks; p++) {
            Int lumaBpv = GetBpv2x2 (p);
            Int srId = 0;
            Int srPos = 0;
            BpvMapping (lumaBpv, k, srId, srPos);
            Int* pSrUpper = m_bpSrStorage[srId]->GetDataBuffer (k, GetPartitionSearchRange (p, k, k2x2, kUpper));
            Int* pSrLower = (srId == 0)
                ? m_bpSrStorage[2]->GetDataBuffer (k, 0) + GetOverlapRegionOffset (k)
                : m_bpSrStorage[srId]->GetDataBuffer (k, GetPartitionSearchRange (p, k, k2x2, kLower));

            for (Int i = 0; i < GetPartitionSize (k); i++) {
                Int x0 = GetPartitionStartOffset (p, k, k2x2) + i;
                Int x1 = x0 + getWidth (k);
                ptrRes2x2[x0] = ptrOrg[x0] - pSrUpper[srPos + i];
                if (!(m_chromaFormat == k420 && k > 0)) {
                    ptrRes2x2[x1] = ptrOrg[x1] - pSrLower[srPos + i];
                }
            }
        }
    }
}

/**
    Reconstruct (decoder side)
    */
Void BpMode::DecoderReconstruct () {
    for (Int k = 0; k < g_numComps; k++) {
        Int height = getHeight (k);
        Int width = getWidth (k);
        Int numPx = width * height;
		Int qp = GetCurCompQpMod (m_qp, k);
        Int qp_scale = qp >> 3;
        Int qp_rem = qp & 0x07;
        Int scale = g_bpInvQuantScales[qp_rem];
        Int shift = g_dequantShift - qp_scale;
        Int offset = 0;
        if (shift > 0) {
            offset = 1 << (shift - 1); // this offset is used for rounding
        }
        Int max = (1 << m_bitDepth) - 1;
		Int min = (m_csc == kYCoCg && k > 0 ? -(1 << m_bitDepth) : 0);
        Int *pRec = GetRecBlkBuf (k);
        Int *pQuant = GetQuantBlkBuf (k);

        for (UInt p = 0; p < m_numSubBlocks; p++) {
            if (m_use2x2[p]) { //! 2x2 partitions !//
                Int lumaBpv = GetBpv2x2 (p);
                Int srId = 0;
                Int srPos = 0;
                BpvMapping (lumaBpv, k, srId, srPos);
                Int* pSrUpper = m_bpSrStorage[srId]->GetDataBuffer (k, GetPartitionSearchRange (p, k, k2x2, kUpper));
                Int* pSrLower = (srId == 0)
                    ? m_bpSrStorage[2]->GetDataBuffer (k, 0) + GetOverlapRegionOffset (k)
                    : m_bpSrStorage[srId]->GetDataBuffer (k, GetPartitionSearchRange (p, k, k2x2, kLower));

                for (Int i = 0; i < GetPartitionSize (k); i++) {
                    Int x0 = GetPartitionStartOffset (p, k, k2x2) + i;
                    Int x1 = x0 + getWidth (k);

                    // first row (x0)
					Int iCoeffQClip = DequantSample (pQuant[x0], scale, offset, shift);
                    Int curPred = pSrUpper[srPos + i];
                    pRec[x0] = ReconstructSample (curPred, iCoeffQClip, m_bpskip, min, max);

                    // second row (x1)
					iCoeffQClip = DequantSample (pQuant[x1], scale, offset, shift);
                    if (!(m_chromaFormat == k420 && k > 0)) {
                        curPred = pSrLower[srPos + i];
                        pRec[x1] = ReconstructSample (curPred, iCoeffQClip, m_bpskip, min, max);
                    }
                }
            }
            else { //! 2x1 partitions !//
                Int block_part_y = (p >> m_partBitsPerLine);
                Int block_part_x = p - (block_part_y << m_partBitsPerLine);
                Int p0 = (block_part_y * getWidth (k)) + block_part_x;
                Int p1 = p0 + (1 << m_partBitsPerLine);
                Int lumaBpv_a = GetBpv2x1 (p0);
                Int lumaBpv_b = GetBpv2x1 (p1);
                Int srIdA = 0;
                Int srIdB = 0;
                Int srPosA = 0;
                Int srPosB = 0;
                BpvMapping (lumaBpv_a, k, srIdA, srPosA);
                BpvMapping (lumaBpv_b, k, srIdB, srPosB);
                Int curSrBufferA = (m_bpSrStorage[srIdA]->GetIsPrevRecLine ())
                    ? m_bpSrStorage[srIdA]->GetNumRows (k) - 1
                    : GetPartitionSearchRange (p0, k, k2x1, kUpper);
                Int* pSrA = m_bpSrStorage[srIdA]->GetDataBuffer (k, curSrBufferA);
                Int* pSrB = NULL;
                if (srIdB == 0) {
                    pSrB = m_bpSrStorage[2]->GetDataBuffer (k, 0) + GetOverlapRegionOffset (k);
                }
                else {
                    Int curSrBufferB = (m_bpSrStorage[srIdB]->GetIsPrevRecLine ())
                        ? m_bpSrStorage[srIdB]->GetNumRows (k) - 1
                        : GetPartitionSearchRange (p1, k, k2x1, kUpper);
                    pSrB = m_bpSrStorage[srIdB]->GetDataBuffer (k, curSrBufferB);
                }

                for (Int i = 0; i < GetPartitionSize (k); i++) {
                    Int x0 = GetPartitionStartOffset (p, k, k2x2) + i;
                    Int x1 = x0 + getWidth (k);

                    // first row (x0)
					Int iCoeffQClip = DequantSample (pQuant[x0], scale, offset, shift);
                    Int pMod = p % 4;
                    Int xMod = (pMod * GetPartitionSize (k)) + i;
                    Int curPred = pSrA[srPosA + i];
                    pRec[x0] = ReconstructSample (curPred, iCoeffQClip, m_bpskip, min, max);

                    // second row (x1)
					iCoeffQClip = DequantSample (pQuant[x1], scale, offset, shift);
                    pMod = p % 4;
                    xMod = (pMod * GetPartitionSize (k)) + i;
                    if (m_chromaFormat == k420 && k > 0) {
                        // no chroma samples in second row of current block for 4:2:0
                    }
                    else {
                        curPred = pSrB[srPosB + i];
                        pRec[x1] = ReconstructSample (curPred, iCoeffQClip, m_bpskip, min, max);
                    }
                }
            }
        }
    }
}

/**
    Encode BPV

    @param isFinal flag to indicate final encode
    @return bits used to encode BPV
    */
Int BpMode::EncodeBpv (Bool isFinal) {
    Int bits = 0;
    // BPV table signaled in substream 0
    for (UInt sb = 0; sb < m_numSubBlocks; sb++) {
        UInt ssmIdx = 0;
        bits += 1;
        if (isFinal) {
            m_ssm->GetCurSyntaxElement (ssmIdx)->AddSyntaxWord (m_use2x2[sb] ? 1 : 0, 1);
        }
    }
    // BPV signaled in substreams 0-3
    for (UInt sb = 0; sb < m_numSubBlocks; sb++) {
        UInt ssmIdx = sb;
        Int block_part_y = (sb >> m_partBitsPerLine);
        Int block_part_x = sb - (block_part_y << m_partBitsPerLine);
        Int p0 = (block_part_y << (m_partBitsPerLine + 1)) + block_part_x;
        Int p1 = p0 + (1 << m_partBitsPerLine);
        if (m_use2x2[sb] == true) {
            if (m_isFls) {
                if (sb == 0) {
                    bits += (m_bpvNumBits - 1);
                }
                if (isFinal) {
                    m_ssm->GetCurSyntaxElement (ssmIdx)->AddSyntaxWord (m_bpv2x2[sb] - 32, m_bpvNumBits - 1);
                }
            }
            else {
                if (sb == 0) {
                    bits += m_bpvNumBits;
                }
                if (isFinal) {
                    m_ssm->GetCurSyntaxElement (ssmIdx)->AddSyntaxWord (m_bpv2x2[sb], m_bpvNumBits);
                }
            }
        }
        else {
            if (m_isFls) {
                if (sb == 0) {
                    bits += (2 * (m_bpvNumBits - 1));
                }
                if (isFinal) {
                    m_ssm->GetCurSyntaxElement (ssmIdx)->AddSyntaxWord (m_bpv2x1[p0] - 32, m_bpvNumBits - 1);
                    m_ssm->GetCurSyntaxElement (ssmIdx)->AddSyntaxWord (m_bpv2x1[p1] - 32, m_bpvNumBits - 1);
                }
            }
            else {
                if (sb == 0) {
                    bits += (2 * m_bpvNumBits);
                }
                if (isFinal) {
                    m_ssm->GetCurSyntaxElement (ssmIdx)->AddSyntaxWord (m_bpv2x1[p0], m_bpvNumBits);
                    m_ssm->GetCurSyntaxElement (ssmIdx)->AddSyntaxWord (m_bpv2x1[p1], m_bpvNumBits);
                }
            }
        }
    }
    return bits;
}

/**
    Reconstruct (encoder-side)
    */
Void BpMode::EncoderReconstruct () {
    for (Int k = 0; k < g_numComps; k++) {
        Int height = getHeight (k);
        Int width = getWidth (k);
        Int numPx = width * height;
		Int qp = GetCurCompQpMod (m_qp, k);
        Int qp_scale = qp >> 3;   // 8 steps to double quantizer step-size;
        Int qp_rem = qp & 0x07;
        Int scale = g_bpInvQuantScales[qp_rem];
        Int shift = g_dequantShift - qp_scale;
        Int offset = 0;
        if (shift > 0) {
            offset = 1 << (shift - 1); // this offset is used for rounding
        }
        Int max = (1 << m_bitDepth) - 1;
		Int min = (m_csc == kYCoCg && k > 0 ? -(1 << m_bitDepth) : 0);
        Int *pQuant2x1 = GetQResBlkBuf2x1 (k);
        Int *pQuant2x2 = GetQResBlkBuf2x2 (k);
        Int *pRec2x1 = GetRecBlkBuf2x1 (k);
        Int *pRec2x2 = GetRecBlkBuf2x2 (k);
        Int *pIQuant2x1 = GetIQResBlkBuf2x1 (k);
        Int *pIQuant2x2 = GetIQResBlkBuf2x2 (k);

        // 2x2 
        for (UInt p = 0; p < m_numSubBlocks; p++) {
            Int lumaBpv = GetBpv2x2 (p);
            Int srId = 0;
            Int srPos = 0;
            BpvMapping (lumaBpv, k, srId, srPos);
            Int* pSrUpper = m_bpSrStorage[srId]->GetDataBuffer (k, GetPartitionSearchRange (p, k, k2x2, kUpper));
            Int* pSrLower = (srId == 0)
                ? m_bpSrStorage[2]->GetDataBuffer (k, 0) + GetOverlapRegionOffset (k)
                : m_bpSrStorage[srId]->GetDataBuffer (k, GetPartitionSearchRange (p, k, k2x2, kLower));

            for (Int i = 0; i < GetPartitionSize (k); i++) {
                Int x0 = GetPartitionStartOffset (p, k, k2x2) + i;
                Int x1 = x0 + getWidth (k);
				Int iCoeffQClip = DequantSample (pQuant2x2[x0], scale, offset, shift);

                Int pMod = p % 4;
                Int xMod = (pMod * GetPartitionSize (k)) + i;
                
                //! upper !//
                Int recon = iCoeffQClip + pSrUpper[srPos + i];
                pIQuant2x2[x0] = m_bpskip ? 0 : iCoeffQClip;
                pRec2x2[x0] = (m_bpskip ? pSrUpper[srPos + i] : Clip3<Int> (min, max, recon));

                //! lower !//
                if (!(m_chromaFormat == k420 && k > 0)) {
					iCoeffQClip = DequantSample (pQuant2x2[x1], scale, offset, shift);
                    pMod = p % 4;
                    xMod = (pMod * GetPartitionSize (k)) + i;
                    recon = iCoeffQClip + pSrLower[srPos + i];
                    pIQuant2x2[x1] = m_bpskip ? 0 : iCoeffQClip;
                    pRec2x2[x1] = (m_bpskip ? pSrLower[srPos + i] : Clip3<Int> (min, max, recon));
                }
            }
        }

        // 2x1
        for (UInt p = 0; p < (2 * m_numSubBlocks); p++) {
            Int lumaBpv = GetBpv2x1 (p);
            Int srId = 0;
            Int srPos = 0;
            BpvMapping (lumaBpv, k, srId, srPos);
            Int curSrBuffer = (m_bpSrStorage[srId]->GetIsPrevRecLine ())
                ? m_bpSrStorage[srId]->GetNumRows (k) - 1
                : GetPartitionSearchRange (p, k, k2x1, kUpper);

            Int* pSr;
            if (srId == 0 && p >= m_numSubBlocks) {
                pSr = m_bpSrStorage[2]->GetDataBuffer (k, 0) + GetOverlapRegionOffset (k);
            }
            else {
                pSr = m_bpSrStorage[srId]->GetDataBuffer (k, curSrBuffer);
            }

            for (Int i = 0; i < GetPartitionSize (k); i++) {
                Int x = (p * GetPartitionSize (k)) + i;
				Int iCoeffQClip = DequantSample (pQuant2x1[x], scale, offset, shift);
                Int pMod = p % 4;
                Int xMod = (pMod * GetPartitionSize (k)) + i;
                pIQuant2x1[x] = m_bpskip ? 0 : iCoeffQClip;
                if (!(m_chromaFormat == k420 && k > 0 && p >= m_numSubBlocks)) {
                    Int recon = iCoeffQClip + pSr[srPos + i];
                    pRec2x1[x] = (m_bpskip ? pSr[srPos + i] : Clip3<Int> (min, max, recon));
                }
            }
        }
    }
}

/**
    Quantize residuals for all 2x1 and 2x2 partitions
    */
Void BpMode::QuantizeResidual () {
    for (Int k = 0; k < g_numComps; k++) {
        Int numPx = getWidth (k) * getHeight (k);
        Int *pQuant2x1 = GetQResBlkBuf2x1 (k);
        Int *pQuant2x2 = GetQResBlkBuf2x2 (k);
        Int *pRes2x1 = GetResBlkBuf2x1 (k);
        Int *pRes2x2 = GetResBlkBuf2x2 (k);
		Int qp = GetCurCompQpMod (m_qp, k);
        Int qp_scale = qp >> 3;
        Int qp_rem = qp & 0x07;
        Int scale = g_bpQuantScales[qp_rem];
        for (Int j = 0; j < numPx; j++) {
            Int scalePredErr = pRes2x1[j];
            Int sign = (scalePredErr < 0 ? -1 : 1);
            pQuant2x1[j] = sign * ((scale * abs (scalePredErr) + (g_bpQuantDeadZone << qp_scale)) >> (g_bpQuantBits + qp_scale));

            scalePredErr = pRes2x2[j];
            sign = (scalePredErr < 0 ? -1 : 1);
            pQuant2x2[j] = sign * ((scale * abs (scalePredErr) + (g_bpQuantDeadZone << qp_scale)) >> (g_bpQuantBits + qp_scale));
        }
    }
}

/**
    Initialize the BP search range
    */
Void BpMode::InitSearchRange () {
    // set up storage for multiple search ranges
    // -------------+----------------+-------
    // ...        A | B                   ...
    // -------------+----------------+-------
    // ...        C | current block  |    ...
    // ...        C | current block  |    ...
    // -------------+----------------+-------
    InitSearchRangeA ();
    InitSearchRangeB ();
	switch (m_csc) {
		case kYCoCg:
			InitSearchRangeC_YCoCg ();
			break;
		case kYCbCr:
			InitSearchRangeC ();
			break;
	}
}

/**
    Initialize search range B (prev. rec. line, starting at current x position)
    */
Void BpMode::InitSearchRangeB () {
    Int curPosX = m_posX;
    Int curPosY = m_posY;
    Int blockPosX = m_posX;
    Int blockPosY = m_posY;
    Bool isAboveRightValid = true;
	Int halfDynRange = 1 << (m_bitDepth - 1);
	Int defVal[3] = { halfDynRange, halfDynRange, halfDynRange };
    BpSearchRange* curSr = m_bpSrStorage.at (1);
    for (Int k = 0; k < g_numComps; k++) {
        curSr->WriteBufferCompConstValue (k, defVal[k]);
        Int curCompPosX = curPosX >> g_compScaleX[k][m_chromaFormat];
        Int curCompPosY = curPosY >> g_compScaleY[k][m_chromaFormat];
        Int srcStride = m_pRecVideoFrame->getWidth (k);
        Int pxPerSrRow = curSr->GetPxPerRow (k);
        UInt posPrevRecLine = (curCompPosX + pxPerSrRow >= srcStride ? srcStride - curCompPosX : pxPerSrRow);
        UInt posNextPrevRecLine = (curCompPosX + pxPerSrRow >= srcStride ? pxPerSrRow - posPrevRecLine : 0);

        if (posPrevRecLine > 0) {
            Int srcRow = curCompPosY - 1;
            Int srcColStart = curCompPosX;
            Int srcColEnd = curCompPosX + posPrevRecLine - 1;
            Int numPos = (m_isFls ? 0 : srcColEnd - srcColStart + 1);
            if (numPos > 0) {
                for (Int row = 0; row < curSr->GetNumRows (k); row++) {
                    Int *pSrcData = m_pRecVideoFrame->getBuf (k) + (srcRow * srcStride + srcColStart);
                    Int *pDstData = curSr->GetDataBuffer (k, row);
                    ::memcpy (pDstData, pSrcData, numPos * sizeof (Int));
                }
            }
        }

        if (posNextPrevRecLine > 0) {
            Int srcRow = (m_chromaFormat == k420 && k > 0 ? curCompPosY : curCompPosY + 1);
            Int srcColStart = 0;
            Int srcColEnd = posNextPrevRecLine - 1;
            Int numPos = srcColEnd - srcColStart + 1;
            if (numPos > 0) {
                for (Int row = 0; row < curSr->GetNumRows (k); row++) {
                    Int *pSrcData = m_pRecVideoFrame->getBuf (k) + (srcRow * srcStride + srcColStart);
                    Int *pDstData = curSr->GetDataBuffer (k, row);
                    ::memcpy (pDstData + posPrevRecLine, pSrcData, numPos * sizeof (Int));
                }
            }
        }
    }

    if (curSr->GetSrcCsc () == kRgb && curSr->GetModeCsc () == kYCoCg) {
		//! CSC pixels from prev. rec. line buffer to YCoCg !//
        curSr->DataBufferRgbToYCoCg ();
    }
}

/**
	Initialize search range C from YCoCg reconstructed buffer 
*/
Void BpMode::InitSearchRangeC_YCoCg () {
	Int curPosX = m_posX;
	Int curPosY = m_posY;
	Int halfDynRange = 1 << (m_bitDepth - 1);
	Int defVal[3] = { halfDynRange, 0, 0 };
	BpSearchRange* curSr = m_bpSrStorage.at (2);
	Int stride = m_recBufferYCoCg->getWidth (0);
	Int pxPerSrRow = curSr->GetPxPerRow (0);
	Int posCurBlockline = std::min (pxPerSrRow, curPosX);
	Int posPrevBlockline = curPosY == 0 ? 0 : pxPerSrRow - posCurBlockline;

	for (Int k = 0; k < g_numComps; k++) {
		curSr->WriteBufferCompConstValue (k, defVal[k]);

		//! a !//
		if (posPrevBlockline > 0) {
			for (Int row = 0; row < curSr->GetNumRows (k); row++) {
				Int *src = m_recBufferYCoCg->getBuf (k) + (row * stride) + (stride - posPrevBlockline);
				Int *dst = curSr->GetDataBuffer (k, row);
				::memcpy (dst, src, posPrevBlockline * sizeof (Int));
				if (posCurBlockline == 0) {
					dst[pxPerSrRow] = dst[pxPerSrRow - 1];
				}
			}
		}

		//! b !//
		Int dstOffset = 0;
		if (curPosY == 0) {
			dstOffset = std::max (0, pxPerSrRow - posCurBlockline);
		}
		else {
			dstOffset = posPrevBlockline;
		}
		if (posCurBlockline > 0) {
			Int srcOffset = curPosX - posCurBlockline;
			for (Int row = 0; row < curSr->GetNumRows (k); row++) {
				Int *src = m_recBufferYCoCg->getBuf (k) + (row * stride) + srcOffset;
				Int *dst = curSr->GetDataBuffer (k, row);
				::memcpy (dst + dstOffset, src, posCurBlockline * sizeof (Int));
				dst[pxPerSrRow] = dst[pxPerSrRow - 1];
			}
		}
	}
}

/**
    Initialize search range C (current blockline)
    */
Void BpMode::InitSearchRangeC () {
    Int curPosX = m_posX;
    Int curPosY = m_posY;
    Int blockPosX = m_posX;
    Int blockPosY = m_posY;
	Int halfDynRange = 1 << (m_bitDepth - 1);
	Int defVal[3] = { halfDynRange, halfDynRange, halfDynRange };
    BpSearchRange* curSr = m_bpSrStorage.at (2);

    for (Int k = 0; k < g_numComps; k++) {
        curSr->WriteBufferCompConstValue (k, defVal[k]);
        Int curCompPosX = curPosX >> g_compScaleX[k][m_chromaFormat];
        Int curCompPosY = curPosY >> g_compScaleY[k][m_chromaFormat];
        Int srcStride = m_pRecVideoFrame->getWidth (k);
        Int pxPerSrRow = curSr->GetPxPerRow (k);

        UInt posPrevBlockline;
        UInt posCurrBlockline;
        if (curCompPosX == 0) {
            posPrevBlockline = pxPerSrRow;
            posCurrBlockline = 0;
        }
        else {
            posPrevBlockline = (curCompPosX < pxPerSrRow ? pxPerSrRow - curCompPosX : 0);
            posCurrBlockline = (curCompPosX < pxPerSrRow ? curCompPosX : pxPerSrRow);
        }
        if (posPrevBlockline > 0) {
            // previous blockline
            Int colStart = srcStride - posPrevBlockline;
            Int colEnd = srcStride - 1;
            Int numPos = (m_isFls ? 0 : colEnd - colStart + 1);
            if (numPos > 0) {
                for (Int row = 0; row < curSr->GetNumRows (k); row++) {
                    UInt rowShift = (m_chromaFormat == k420 && k > 0 ? 1 : 2);
                    Int *pSrcData = m_pRecVideoFrame->getBuf (k) + ((curCompPosY + row - rowShift) * srcStride + colStart);
                    Int *pDstData = curSr->GetDataBuffer (k, row);
                    ::memcpy (pDstData, pSrcData, numPos * sizeof (Int));
                    if (posCurrBlockline == 0) {
                        pDstData[pxPerSrRow] = pDstData[pxPerSrRow - 1];
                    }
                }
            }
        }

        if (posCurrBlockline > 0) {
            // current blockline
            Int colStart = std::max<Int> (0, curCompPosX - posCurrBlockline);
            Int colEnd = curCompPosX - 1;
            Int numPos = colEnd - colStart + 1;
            if (numPos > 0) {
                for (Int row = 0; row < curSr->GetNumRows (k); row++) {
                    Int *pSrcData = m_pRecVideoFrame->getBuf (k) + ((curCompPosY + row) * srcStride + colStart);
                    Int *pDstData = curSr->GetDataBuffer (k, row);
                    ::memcpy (pDstData + posPrevBlockline, pSrcData, numPos * sizeof (Int));
                    pDstData[pxPerSrRow] = pDstData[pxPerSrRow - 1];
                }
            }
        }
    }

    if (curSr->GetSrcCsc () == kRgb && curSr->GetModeCsc () == kYCoCg) {
		//! CSC pixels from prev. rec. line buffer to YCoCg !//
        curSr->DataBufferRgbToYCoCg ();
    }
}

/**
    Initialize search range A (prec. rec. line, above/left of current block)
    */
Void BpMode::InitSearchRangeA () {
    Int curPosX = m_posX;
    Int curPosY = m_posY;
    Int blockPosX = m_posX;
    Int blockPosY = m_posY;
	Int halfDynRange = 1 << (m_bitDepth - 1);
    Int defVal[3] = { halfDynRange, halfDynRange, halfDynRange };
    BpSearchRange* curSr = m_bpSrStorage.at (0);
    for (Int k = 0; k < g_numComps; k++) {
        curSr->WriteBufferCompConstValue (k, defVal[k]);
        Int curCompPosX = curPosX >> g_compScaleX[k][m_chromaFormat];
        Int curCompPosY = curPosY >> g_compScaleY[k][m_chromaFormat];
        Int srcStride = m_pRecVideoFrame->getWidth (k);
        Int pxPerSrRow = curSr->GetPxPerRow (k);
        UInt pxOverlap = (k == 0 || m_chromaFormat == k444 ? 8 : 4);
        UInt posPrevRecLine = (curCompPosX == 0 ? 1 : 1 + pxOverlap);
        UInt posPrevPrevRecLine = (curCompPosX == 0 ? pxOverlap : 0);
        if (posPrevPrevRecLine > 0) {
            Int srcRow = (m_chromaFormat == k420 && k > 0 ? curCompPosY - 2 : curCompPosY - 3);
            Int srcColStart = srcStride - posPrevPrevRecLine;
            Int srcColEnd = srcStride - 1;
            Int numPos = (m_posY <= 2 ? 0 : srcColEnd - srcColStart + 1);
            if (numPos > 0) {
                for (Int row = 0; row < curSr->GetNumRows (k); row++) {
                    Int *pSrcData = m_pRecVideoFrame->getBuf (k) + (srcRow * srcStride + srcColStart);
                    Int *pDstData = curSr->GetDataBuffer (k, row);
                    ::memcpy (pDstData, pSrcData, numPos * sizeof (Int));
                }
            }
        }
        if (posPrevRecLine > 0) {
            Int srcRow = curCompPosY - 1;
            Int srcColStart = curCompPosX - posPrevRecLine + 1;
            Int srcColEnd = curCompPosX;
            Int numPos = (m_isFls ? 0 : srcColEnd - srcColStart + 1);
            if (numPos > 0) {
                for (Int row = 0; row < curSr->GetNumRows (k); row++) {
                    Int *pSrcData = m_pRecVideoFrame->getBuf (k) + (srcRow * srcStride + srcColStart);
                    Int *pDstData = curSr->GetDataBuffer (k, row);
                    ::memcpy (pDstData + posPrevPrevRecLine, pSrcData, numPos * sizeof (Int));
                }
            }
        }
    }

    if (curSr->GetSrcCsc () == kRgb && curSr->GetModeCsc () == kYCoCg) {
		//! CSC pixels from prev. rec. line buffer to YCoCg !//
        curSr->DataBufferRgbToYCoCg ();
    }
}

/**
    Generate mapping between BPV and search range position

    @param posA number of positions within SR A
    @param posB number of positions within SR B
    @param posC number of positions within SR C
    */
Void BpMode::GenerateBpSrMappings (Int posA, Int posB, Int posC) {
    if ((posA + posB + posC) > g_defaultBpSearchRange) {
        fprintf (stderr, "BpMode::GenerateBpSrMappings(): too many search positions: %d.\n", posA + posB + posC);
        exit (EXIT_FAILURE);
    }
    // create the mappings
    Int curId = 0;
    Int idx = 0;
    Int curPos = 0;
    for (Int i = 0; i < posA; i++) {
        m_mapSrId[idx] = 0;
        m_mapSrPos[idx] = curPos;
        idx++;
        curPos++;
    }
    curPos = 0;
    for (Int i = 0; i < posB; i++) {
        m_mapSrId[idx] = 1;
        m_mapSrPos[idx] = curPos;
        idx++;
        curPos++;
    }

    curPos = 0;
    for (Int i = 0; i < posC; i++) {
        m_mapSrId[idx] = 2;
        m_mapSrPos[idx] = curPos;
        idx++;
        curPos++;
    }

    // set all invalid search range positions to -1
    while (idx < g_defaultBpSearchRange) {
        m_mapSrId[idx] = -1;
        m_mapSrPos[idx] = -1;
        idx++;
    }
}

/**
    Set the number of positions for each part of the search range
    */
Void BpMode::SetSrPositions () {
    Int numPosA = 8;
    Int numPosB = 24;
    Int numPosC = 32;
    m_bpSrStorage[0]->SetNumPos (numPosA);
    m_bpSrStorage[1]->SetNumPos (numPosB);
    m_bpSrStorage[2]->SetNumPos (numPosC);
    GenerateBpSrMappings (numPosA, numPosB, numPosC);
}

/**
    Get the search range for current partition

    @param partIdx partition index
    @param comp component index
    @param type partition type (2x2 or 2x1)
    @param half partition position within block (top or bottom)
    @return partition search range
    */
Int BpMode::GetPartitionSearchRange (Int partIdx, Int comp, BpPartitionType type, BpPartitionHalf half) {
    Int sr = 0;
    Int offsetY = 0;
    switch (type) {
    case k2x1:
        sr = (partIdx >> m_partBitsPerLine);
        break;
    case k2x2:
        offsetY = (partIdx >> m_partBitsPerLine);
        sr = (2 * offsetY + (half == kLower ? 1 : 0));
        break;
    default:
        assert (false);
    }
    return sr;
}

/**
    Get the starting position of the partition within block

    @param partIdx partition index
    @param comp component index
    @param type partition type (2x2 or 2x1)
    @return partition start offset
    */
Int BpMode::GetPartitionStartOffset (Int partIdx, Int comp, BpPartitionType type) {
    Int offset = 0;
    Int offsetY = 0;
    Int offsetX = 0;
    switch (type) {
    case k2x1:
        offsetY = partIdx >> m_partBitsPerLine;
        offsetX = partIdx - (offsetY << m_partBitsPerLine);
        offset = (getWidth (comp) * offsetY) + (offsetX * GetPartitionSize (comp));
        break;
    case k2x2:
        offsetY = partIdx >> m_partBitsPerLine;
        offsetX = partIdx - (offsetY << m_partBitsPerLine);
        offset = (2 * getWidth (comp) * offsetY) + (offsetX * GetPartitionSize (comp));
        break;
    default:
        assert (false);
    }
    return offset;
}

/**
    Add predictor to de-quantized value and clip

    @param predictor predicted value
    @param dequant de-quantized residual
    @param isSkip flag for BP-SKIP mode
    @param min min value
    @param max max value
    @return reconstructed sample
    */
Int BpMode::ReconstructSample (Int predictor, Int dequant, Bool isSkip, Int min, Int max) {
    return (isSkip ? predictor : Clip3<Int> (min, max, predictor + dequant));
}

/**
    Map BPV index to search range index and position

    @param lumaBpv BPV index
    @param comp component index
    @param srId (reference) search range ID
    @param srPos (reference) search range position
    */
Void BpMode::BpvMapping (Int lumaBpv, Int comp, Int &srId, Int &srPos) {
    if (lumaBpv < 0 || lumaBpv >= GetSrSize (0)) {
        fprintf (stderr, "BpMode::BpvMapping(): BPV is out of range: %d.\n", lumaBpv);
        exit (EXIT_FAILURE);
    }
    srId = m_mapSrId[lumaBpv];
    Int round = 0;
    if (m_chromaFormat != k444 && comp > 0) {
        round = (srId == 2 ? 1 : 0);
    }
    srPos = (m_mapSrPos[lumaBpv] + round) >> g_compScaleX[comp][m_chromaFormat];

    //! check validity of mapped search range position !//
    if (m_isFls && (srId == 0 || srId == 1)) {
        srId = -1;
        srPos = -1;
    }
}

/**
    Parse BPV for next block
    */
Void BpMode::DecodeBpvNextBlock () {
    UInt curSsmIdx = 0;
    Bool* use2x2 = m_temp_use2x2;
    Int* bpv2x2 = m_temp_bpv2x2;
    Int* bpv2x1 = m_temp_bpv2x1;
    for (UInt sb = 0; sb < m_numSubBlocks; sb++) {
        Int split = m_ssm->ReadFromFunnelShifter (curSsmIdx, 1);
        use2x2[sb] = (split > 0 ? true : false);
    }

    Int bitsPerBpv = (m_isNextBlockFls ? m_bpvNumBits - 1 : m_bpvNumBits);

    UInt sb = 0; // sub-block 0 only
    Int block_part_y = (sb >> m_partBitsPerLine);
    Int block_part_x = sb - (block_part_y << m_partBitsPerLine);
    Int p0 = (block_part_y << (m_partBitsPerLine + 1)) + block_part_x;
    Int p1 = p0 + (1 << m_partBitsPerLine);
    if (use2x2[sb]) {
        bpv2x2[sb] = m_ssm->ReadFromFunnelShifter (curSsmIdx, bitsPerBpv);
        if (m_isNextBlockFls) {
            bpv2x2[sb] += 32;
        }
    }
    else {
        bpv2x1[p0] = m_ssm->ReadFromFunnelShifter (curSsmIdx, bitsPerBpv);
        bpv2x1[p1] = m_ssm->ReadFromFunnelShifter (curSsmIdx, bitsPerBpv);
        if (m_isNextBlockFls) {
            bpv2x1[p0] += 32;
            bpv2x1[p1] += 32;
        }
    }
}

/**
    Parse BPV for current block
    */
Void BpMode::DecodeBpvCurBlock () {
    Int bitsPerBpv = (m_isFls ? m_bpvNumBits - 1 : m_bpvNumBits);
    for (UInt sb = 1; sb < 4; sb++) {
        UInt curSsmIdx = sb;
        Int block_part_y = (sb >> m_partBitsPerLine);
        Int block_part_x = sb - (block_part_y << m_partBitsPerLine);
        Int p0 = (block_part_y << (m_partBitsPerLine + 1)) + block_part_x;
        Int p1 = p0 + (1 << m_partBitsPerLine);
        if (m_use2x2[sb]) {
            m_bpv2x2[sb] = m_ssm->ReadFromFunnelShifter (curSsmIdx, bitsPerBpv);
            if (m_isFls) {
                m_bpv2x2[sb] += 32;
            }
        }
        else {
            m_bpv2x1[p0] = m_ssm->ReadFromFunnelShifter (curSsmIdx, bitsPerBpv);
            m_bpv2x1[p1] = m_ssm->ReadFromFunnelShifter (curSsmIdx, bitsPerBpv);
            if (m_isFls) {
                m_bpv2x1[p0] += 32;
                m_bpv2x1[p1] += 32;
            }
        }
    }
}

/**
    Dump BPV to debugTracer (encoder)
    */
Void BpMode::EncoderDebugBpv () {
    UInt numSubBlocks = 4;
    fprintf (encTracer, "BPV_2x2 : [");
    for (UInt sb = 0; sb < numSubBlocks; sb++) {
        if (m_use2x2[sb]) {
            fprintf (encTracer, "%02d", m_bpv2x2[sb]);
        }
        else {
            fprintf (encTracer, "xx");
        }
        if (sb < (numSubBlocks - 1)) {
            fprintf (encTracer, ", ");
        }
    }
    fprintf (encTracer, "]\n");

    for (UInt row = 0; row < 2; row++) {
        fprintf (encTracer, row == 0 ? "BPV_2x1a: [" : "BPV_2x1b: [");
        for (UInt sb = 0; sb < numSubBlocks; sb++) {
            m_use2x2[sb]
                ? fprintf (encTracer, "xx")
                : fprintf (encTracer, "%02d", m_bpv2x1[sb + (row * numSubBlocks)]);
            if (sb < (numSubBlocks - 1)) {
                fprintf (encTracer, ", ");
            }
        }
        fprintf (encTracer, "]\n");
    }
}

/**
    Dump BPV to debugTracer (decoder)
    */
Void BpMode::DecoderDebugBpv () {
    UInt numSubBlocks = 4;
    fprintf (decTracer, "BPV_2x2 : [");
    for (UInt sb = 0; sb < numSubBlocks; sb++) {
        m_use2x2[sb]
            ? fprintf (decTracer, "%02d", m_bpv2x2[sb])
            : fprintf (decTracer, "xx");
        if (sb < (numSubBlocks - 1)) {
            fprintf (decTracer, ", ");
        }
    }
    fprintf (decTracer, "]\n");

    for (UInt row = 0; row < 2; row++) {
        fprintf (decTracer, row == 0 ? "BPV_2x1a: [" : "BPV_2x1b: [");
        for (UInt sb = 0; sb < numSubBlocks; sb++) {
            m_use2x2[sb]
                ? fprintf (decTracer, "xx")
                : fprintf (decTracer, "%02d", m_bpv2x1[sb + (row * numSubBlocks)]);
            if (sb < (numSubBlocks - 1)) {
                fprintf (decTracer, ", ");
            }
        }
        fprintf (decTracer, "]\n");
    }
}

/**
    Search range C offset for overlap region of search range A

    @param k component index
    @return offset
    */
Int BpMode::GetOverlapRegionOffset (Int k) {
    Int posC = m_bpSrStorage[2]->GetNumPos () >> g_compScaleX[k][m_chromaFormat];
    Int posA = m_bpSrStorage[0]->GetNumPos () >> g_compScaleX[k][m_chromaFormat];
    Int offset = posC - posA + 1;
    return offset;
}

Void BpMode::PartitionDecisionParallel (UInt headerBits) {
	//! set up memory for per-partition RD cost !//
	UInt partitionRate[16];
	UInt partitionDistortion[16];
	UInt partitionRdCost[16];
	Bool partitionIsValid[16];
	Bool partitionIsBpSkip[16];
	memset (partitionRate, 0, 16 * sizeof (UInt));
	memset (partitionDistortion, 0, 16 * sizeof (UInt));
	memset (partitionRdCost, 0, 16 * sizeof (UInt));
	memset (partitionIsValid, true, 16 * sizeof (Bool));
	memset (partitionIsBpSkip, false, 16 * sizeof (Bool));

	//! set up a set of 16 temporary buffers for:
	// * residual
	// * quantized residual
	// * reconstructed residual
	Int** partitionRes[16];
	Int** partitionQuantRes[16];
	Int** partitionDequantRes[16];
	for (UInt pg = 0; pg < 16; pg++) {
		partitionRes[pg] = new Int*[g_numComps];
		partitionQuantRes[pg] = new Int*[g_numComps];
		partitionDequantRes[pg] = new Int*[g_numComps];
		for (UInt k = 0; k < g_numComps; k++) {
			UInt numSamples = getWidth (k) * getHeight (k);
			partitionRes[pg][k] = new Int[numSamples];
			partitionQuantRes[pg][k] = new Int[numSamples];
			partitionDequantRes[pg][k] = new Int[numSamples];
			memset (partitionRes[pg][k], 0, numSamples * sizeof (Int));
			memset (partitionQuantRes[pg][k], 0, numSamples * sizeof (Int));
			memset (partitionDequantRes[pg][k], 0, numSamples * sizeof (Int));
		}
	}
	
	for (UInt pg = 0; pg < 16; pg++) {
		//! selection using the binary mask !//
		Bool select2x2[4];
		select2x2[0] = (pg & 0x8) >> 3;
		select2x2[1] = (pg & 0x4) >> 2;
		select2x2[2] = (pg & 0x2) >> 1;
		select2x2[3] = (pg & 0x1);

		//! keep track of some information !//
		UInt perSsmBits[4] = { 0, 0, 0, 0 };

		//! generate the partition residual !//
		CopyPartitionGridData (select2x2, partitionRes[pg], m_resBlkBuf2x2, m_resBlkBuf2x1);

		//! generate the partition quant residual !//
		CopyPartitionGridData (select2x2, partitionQuantRes[pg], m_qResBlkBuf2x2, m_qResBlkBuf2x1);

		//! rec. residual
		CopyPartitionGridData (select2x2, partitionDequantRes[pg], m_iqResBlkBuf2x2, m_iqResBlkBuf2x1);

		//! check BP-SKIP !//
		Bool allowReplacement = (m_underflowPreventionMode ? false : true);
		if (!m_bpskip && allowReplacement) {
			Int coeffChecker = 0;
			for (Int k = 0; k < g_numComps; k++) {
				for (UInt s = 0; s < getWidth (k) * getHeight (k); s++) {
					coeffChecker |= partitionQuantRes[pg][k][s];
				}
			}
			if (coeffChecker == 0) {
				partitionIsBpSkip[pg] = true;
			}
		}

		//! rate !//
		Int bitsPerBpv = (m_isFls ? m_bpvNumBits - 1 : m_bpvNumBits);
		perSsmBits[0] = headerBits + 4 + (select2x2[0] ? bitsPerBpv : 2 * bitsPerBpv);
		partitionRate[pg] += perSsmBits[0];
		for (Int k = 0; k < g_numComps; k++) {
			Int ssmIdx = k + 1;
			perSsmBits[ssmIdx] += (select2x2[ssmIdx] ? bitsPerBpv : 2 * bitsPerBpv);
			if (!m_bpskip) {
				Int compEcRate = m_ec->EncodeResiduals (partitionQuantRes[pg][k], k, false, m_ssm);
				perSsmBits[ssmIdx] += compEcRate;
			}

			//! check if any ECG exceeds threshold !//
			if (!m_bpskip) {
				for (UInt grp = 0; grp < 4; grp++) {
					if (m_ec->GetBitsPerGroup (grp) > g_ec_groupSizeLimit) {
						partitionIsValid[pg] = false;
					}
				}
				if (perSsmBits[ssmIdx] > m_pps->GetSsmMaxSeSize ()) {
					partitionIsValid[pg] = false;
				}
			}

			//! add to total rate !//
			partitionRate[pg] += perSsmBits[ssmIdx];
		}

		//! distortion !//
		for (UInt k = 0; k < g_numComps; k++) {
			UInt perCompDistortion = 0;
			UInt compNumSamples = getWidth (k) * getHeight (k);
			for (UInt s = 0; s < compNumSamples; s++) {
				if (m_bpskip) {
					perCompDistortion += abs (partitionRes[pg][k][s]);
				}
				else {
					perCompDistortion += abs (partitionRes[pg][k][s] - partitionDequantRes[pg][k][s]);
				}
			}
			switch (m_csc) {
				case kRgb:
				case kYCbCr:
					partitionDistortion[pg] += perCompDistortion;
					break;
				case kYCoCg:
					partitionDistortion[pg] += ((g_ycocgWeights[k] * perCompDistortion + 128) >> 8);
					break;
			}
		}

		//! rd cost !//
		if (partitionIsValid[pg]) {
			partitionRdCost[pg] = ComputeRdCost (partitionRate[pg], partitionDistortion[pg]);
		}
		else {
			partitionRate[pg] = LARGE_INT;
			partitionDistortion[pg] = LARGE_INT;
			partitionRdCost[pg] = LARGE_INT;
		}
	}

	//! select the partition grid with minimum RD cost !//
	UInt bestGrid = 0;
	UInt minGridRdCost = LARGE_INT;
	UInt minGridRate = LARGE_INT;
	for (UInt grid = 0; grid < 16; grid++) {
		Bool a = (partitionRdCost[grid] < minGridRdCost);
		Bool b = (partitionRdCost[grid] == minGridRdCost) && (partitionRate[grid] < minGridRate);
		if (a || b) {
			minGridRdCost = partitionRdCost[grid];
			minGridRate = partitionRate[grid];
			bestGrid = grid;
		}
	}

	m_use2x2[0] = (bestGrid & 0x8) >> 3;
	m_use2x2[1] = (bestGrid & 0x4) >> 2;
	m_use2x2[2] = (bestGrid & 0x2) >> 1;
	m_use2x2[3] = (bestGrid & 0x1);

	//! copy best partition grid to final buffers !//
	CopyBufferToBuffer (partitionRes[bestGrid], m_resBlk);
	CopyBufferToBuffer (partitionQuantRes[bestGrid], m_qResBlk);
	CopyPartitionGridData (m_use2x2, m_recBlk, m_recBlkBuf2x2, m_recBlkBuf2x1);

	//! copy the final RDcost info !//
	m_rate = partitionRate[bestGrid];
	m_distortion = partitionDistortion[bestGrid];
	m_rdCost = partitionRdCost[bestGrid];
	m_bpskip_flag = partitionIsBpSkip[bestGrid] ? 1 : 0;
	m_rateWithoutPadding = m_rate;
	if (m_underflowPreventionMode && !m_bpskip) {
		m_rateWithoutPadding -= (9 * m_pps->GetRcStuffingBits ());
	}

	//! clear memory !//
	for (UInt p = 0; p < 16; p++) {
		for (UInt k = 0; k < g_numComps; k++) {
			delete[] partitionRes[p][k];
			delete[] partitionQuantRes[p][k];
			delete[] partitionDequantRes[p][k];
		}
		delete[] partitionRes[p];
		delete[] partitionQuantRes[p];
		delete[] partitionDequantRes[p];
	}
}

Void BpMode::CopyPartitionGridData (Bool* select2x2, Int** targetArray, Int** srcArray0, Int** srcArray1) {
	const Int mapping_444[16] = { 0, 1, 8, 9, 2, 3, 10, 11, 4, 5, 12, 13, 6, 7, 14, 15 };
	const Int mapping_422[8] = { 0, 4, 1, 5, 2, 6, 3, 7 };
	const Int mapping_420[4] = { 0, 1, 2, 3 };

	for (UInt p = 0; p < 4; p++) {
		for (UInt k = 0; k < g_numComps; k++) {
			UInt samplesPerPartition = 4 >> (g_compScaleX[k][m_chromaFormat] + g_compScaleY[k][m_chromaFormat]);
			for (UInt s = 0; s < samplesPerPartition; s++) {
				UInt sampleIdx = mapping_444[p * samplesPerPartition + s];
				if (k > 0 && m_chromaFormat == k422) {
					sampleIdx = mapping_422[p * samplesPerPartition + s];
				}
				else if (k > 0 && m_chromaFormat == k420) {
					sampleIdx = mapping_420[p * samplesPerPartition + s];
				}
				targetArray[k][sampleIdx] = select2x2[p] ? srcArray0[k][sampleIdx] : srcArray1[k][sampleIdx];
			}
		}
	}
}

Void BpMode::CopyBufferToBuffer (Int** src, Int** dst) {
	for (UInt k = 0; k < g_numComps; k++) {
		UInt compNumSamples = getWidth (k) * getHeight (k);
		for (UInt s = 0; s < compNumSamples; s++) {
			dst[k][s] = src[k][s];
		}
	}
}

Int BpMode::DequantSample (Int quant, Int scale, Int offset, Int shift) {
	Int sign = (quant < 0 ? -1 : 1);
	Int iCoeffQClip = (shift > 0)
		? sign * ((abs (quant) * scale + offset) >> shift)
		: (quant * scale) << abs (shift);
	return iCoeffQClip;
}