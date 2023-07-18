/* Copyright (c) 2015-2019 Qualcomm Technologies, Inc. All Rights Reserved
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

#include <string.h>
#include <math.h>
#include <stdio.h>
#include "TypeDef.h"
#include "RateControl.h"
#include "EncTop.h"
#include "Pps.h"
#include "Misc.h"

/**
    Constructor

    @param chroma chroma sampling format
    @param ssm SSM object
    @param pps PPS object
    */
RateControl::RateControl (ChromaFormat chroma, SubStreamMux* ssm, Pps* pps)
    : m_blockMaxNumPixels (0)
    , m_curBlockNumPixels (0)
    , m_numBlocksCoded (0)
    , m_sliceBitsTotal (0)
    , m_sliceBitsCurrent (0)
    , m_slicePixelsRemaining (0)
    , m_slicePixelsTotal (0)
    , m_avgBlockBits (0)
    , m_rcBufferMaxSize (0)
    , m_bufferFullness (0)
    , m_rcFullness (0)
    , m_rcOffset (0)
    , m_isFirstBlockInSlice (true)
    , m_qpUpdateMode (0)
    , m_numBlocksInSlice (0)
    , m_qp (g_rcInitQp_444)
    , m_blockBits (0)
    , m_bitDepth (8)
    , m_flatnessQp (0)
    , m_isFirstSliceInFrame (true)
    , m_chromaFormat (chroma)
    , m_ssm (ssm)
    , m_pps (pps) {
    //
}

/**
    Destructor
    */
RateControl::~RateControl () {
    // stub
}

/**
    Initialize rate control for current slice

    @param bpp
    @param bitDepth
    @param srcWidth
    @param srcHeight
    @param blkWidth
    @param blkHeight
    @param sliceWidth
    @param sliceHeight
    @param padX
    @param padY
    @param startX
    @param startY
    */
Void RateControl::InitSlice (Int startX, Int startY ){
    m_blockMaxNumPixels = 16;
    m_curBlockNumPixels = m_blockMaxNumPixels;
    m_sliceStartX = startX;
    m_sliceStartY = startY;
    m_bpp = m_pps->GetBpp ();
    m_bitDepth = m_pps->GetBitDepth ();
    m_srcWidth = m_pps->GetSrcWidth ();
    m_srcHeight = m_pps->GetSrcHeight ();
    m_sliceWidth = m_pps->GetSliceWidth ();
    m_sliceHeight = m_pps->GetSliceHeight ();
    m_fracAccumAvgBlockBits = 0;
    m_bufferFracBitsAccum = 0;
    m_avgBlockBits = m_bpp;
    SetIsFirstSliceInFrame (startX == 0 && startY == 0);
    m_rcBufferInitSize = m_pps->GetRcBufferInitSize ();
    m_initTxDelay = m_pps->GetRcInitTxDelay ();
    m_rcBufferMaxSize = m_pps->GetRcBufferMaxSize ();
    m_bufferFullness = 0;
    switch (m_chromaFormat) {
    case k444:
        m_qp = g_rcInitQp_444;
        break;
    case k422:
        m_qp = g_rcInitQp_422;
        break;
    case k420:
        m_qp = g_rcInitQp_420;
        break;
    default:
        fprintf (stderr, "unsupported chroma format: %d.\n", m_chromaFormat);
        exit (EXIT_FAILURE);
    }

    m_blockBits = 0;
    m_sliceBitsCurrent = 0;
    m_slicePixelsRemaining = m_pps->GetSliceNumPx ();
    m_targetRateScale = m_pps->GetRcTargetRateScale ();
    m_targetRateThreshold = m_pps->GetRcTargetRateThreshold ();
	m_sliceBitsTotal = m_pps->GetSliceNumBits ();
    
    UInt muxWordSize = m_ssm->GetMuxWordSize ();
    UInt numSubstreams = m_ssm->GetNumSubStreams ();
    UInt maxSeSize = m_ssm->GetMaxSeSize ();

	m_numExtraMuxBits = m_pps->GetNumExtraMuxBits ();

    //! chunk byte-alignment !//
	m_chunkAdjustmentBits = m_pps->GetChunkAdjBits ();
	m_byteAlignBits = m_chunkAdjustmentBits * (m_sliceHeight >> 1);

    m_slicePixelsTotal = m_slicePixelsRemaining;
    m_numBlocksInSlice = m_slicePixelsRemaining / m_blockMaxNumPixels;
    m_numBlocksInLine = m_sliceWidth >> 3;
    m_numBlocksCoded = 0;
    m_numPixelsCoded = 0;

    m_isBufferTxStart = (m_bufferFullness == 0) ? false : true;
    m_rcOffset = 0;
    m_rcOffsetInit = m_initTxDelay * m_avgBlockBits;

    // set the QP increment table
    if (m_sliceHeight <= 32) {
        for (Int i = 0; i < 5; i++) {
            memcpy (m_QpIncrementTable[i], g_iQpIncrementSliceHeight16And32[i], sizeof (Int) * 6);
            memcpy (m_QpDecrementTable[i], g_iQpDecrementSliceHeight16And32[i], sizeof (Int) * 5);
        }
    }
    else { //set QP decrement table 	
        for (Int i = 0; i < 5; i++) {
            memcpy (m_QpIncrementTable[i], g_iQpIncrement[i], sizeof (Int) * 6);
            memcpy (m_QpDecrementTable[i], g_iQpDecrement[i], sizeof (Int) * 5);
        }
    }
    m_maxChunkSize = m_pps->GetChunkSize ();
    m_curChunkSize = 0;
    m_numChunkBlks = 0;
    m_chunkCounts = 0;
}

/**
    Check if coding bits will cause rate buffer to underflow/overflow

    @param modeBits number of bits to test against rate buffer state
    @return mode buffer check code (0: normal, 1: overflow, 2: underflow)
    */
Int RateControl::ModeBufferCheck (Int modeBits) {
    Int bitsTransmitted = (m_numBlocksCoded > m_initTxDelay ? m_avgBlockBits : 0);
    Int rcBufferSize = m_bufferFullness + modeBits - bitsTransmitted;
    Bool lastBlock = m_slicePixelsRemaining <= m_blockMaxNumPixels ? true : false;
    Int rcBufferMaxSize = (lastBlock ? m_rcBufferInitSize : m_rcBufferMaxSize - m_rcOffset - m_rcOffsetInit);
    UInt maxAdjBits = (GetChunkAdjBits () + 1) >> 1;
    if (rcBufferSize > rcBufferMaxSize) {
        return 1; // buffer overflow
    }
    else if (rcBufferSize < (m_avgBlockBits + (Int)maxAdjBits)) {
        return 2; // buffer underflow
    }
    else {
        return 0; // valid
    }
}

/**
    Update QP for current block

    @param prevBlockBits bits used to code previous block
    @param targetBits target bits
    @param isFls flag to indicate if block is in FLS
    @param flatnessType flatness type for current block
    */
Void RateControl::UpdateQp (Int prevBlockBits, Int targetBits, Bool isFls, FlatnessType flatnessType) {
    Int diffBits = prevBlockBits - targetBits;
    Int deltaQp = CalcDeltaQp (diffBits);
    Int maxQp = GetMaxQp ();
    Int minQp = GetMinQp (m_pps);
    Int minQpOffset = 8;
    if (m_bufferFullness <= (m_rcBufferInitSize >> 2) || m_rcFullness < 9830) {
        minQpOffset = 0;
    }

    // add deltaQp and clip
    m_qp = Clip3<Int> (minQp + minQpOffset, maxQp, m_qp + deltaQp);

    // flatness adjustment
    UInt shiftBits = g_rc_BFRangeBits - g_flatnessQpLutBits;
    switch (flatnessType) {
    case kFlatnessTypeVeryFlat:
        m_qp = Min (m_qp, (Int)m_pps->GetFlatnessQpVeryFlat (isFls));
        break;
    case kFlatnessTypeSomewhatFlat:
        m_qp = Min (m_qp, (Int)m_pps->GetFlatnessQpSomewhatFlat (isFls));
        break;
    case kFlatnessTypeComplexToFlat:
        m_qp = Min (m_qp, (Int)m_pps->GetLutFlatnessQp (m_rcFullness >> shiftBits));
        break;
    case kFlatnessTypeFlatToComplex:
        m_qp = Max (m_qp, (Int)m_pps->GetLutFlatnessQp (m_rcFullness >> shiftBits));
        break;
    }

    // for first line in slice and if buffer is less than some threshold
    // set the max.range of QP for the first line to be small
    if (isFls && m_rcFullness <= 62915) { // 96%
        m_qp = Min (m_qp, maxQp);
    }
}

/**
    Calculate target rate for the current block

    @param bitBudget target rate
    @param lastBlock flag to indicate if block is last block in slice
    @param isFls flag to indicate if block is in FLS
    */
Void RateControl::CalcTargetRate (Int &targetRate, Bool &lastBlock, Bool isFls) {
    UInt64 bitsRemaining = GetSliceBitsRemaining ();
    Int pxRemaining = m_slicePixelsRemaining;
    if (pxRemaining <= m_blockMaxNumPixels) {
        lastBlock = true;
    }

    //! update targetRateScale, targetRateThreshold !//
    if (pxRemaining < m_targetRateThreshold) {
        m_targetRateScale -= 1;
        m_targetRateThreshold = 1 << (m_targetRateScale - 1);
    }

    //! update baseTargetRate !//
    UInt64 pTemp = ((UInt64)pxRemaining) << g_rc_targetRateLutScaleBits;
    UInt64 pOffset = ((UInt64)1) << (m_targetRateScale - 1);
    UInt pMax = (1 << g_rc_targetRateLutScaleBits) - 1;
	UInt p = std::min<Int> (pMax, (UInt)((pTemp + pOffset) >> m_targetRateScale));
    UInt scale = m_targetRateScale + g_rc_targetRateLutScaleBits;
    UInt64 baseTagetRateTemp = (UInt64)bitsRemaining * 16 * g_targetRateInverseLut[p - 32];
    UInt64 offset = (UInt64)1 << (scale - 1);
    UInt baseTargetRate = (Int)((baseTagetRateTemp + offset) >> scale);

    //! add targetRateExtraFbls !//
    if (isFls) {
        baseTargetRate += (m_pps->GetRcTargetRateExtraFbls () * m_blockMaxNumPixels);
    }

    //! update targetRate !//
    Int targetRateShift = g_rc_BFRangeBits - g_rc_targetRateBits;
    Int clipMax = (1 << g_rc_targetRateBits) - 1;
    Int index = Clip3<Int> (0, clipMax, (m_rcFullness + (1 << (targetRateShift - 1))) >> targetRateShift);
    targetRate = baseTargetRate + m_pps->GetLutTargetRateDelta (index);
}

/**
    Get max QP for current block

    @return maxQp max QP for QP update
    */
Int RateControl::GetMaxQp () {
    Int maxQp = m_pps->GetLutMaxQp (m_rcFullness >> 13);
    if (m_rcFullness > 62259) {
        // buffer fullness >95%
        maxQp = 4 + m_pps->GetLutMaxQp (7);
    }
    return maxQp;
}

/**
    Calculate deltaQp

    @param diffBits difference between prevBlockRate and targetRate
    @return deltaQp
    */
Int RateControl::CalcDeltaQp (Int diffBits) {
    Int deltaQp;
    Int qpIndex = 0;

    // qpUpdateMode
    if (m_rcFullness >= 57672) { //88%
        m_qpUpdateMode = 2;
    }
    else if (m_rcFullness >= 49807) { //76%
        m_qpUpdateMode = 1;
    }

    if (m_rcFullness <= 7864) { //12%
        m_qpUpdateMode = 4;
    }
    else if (m_rcFullness <= 15729) { //24%
        m_qpUpdateMode = 3;
    }

    // qpIndex
    Int absDiff = abs (diffBits);
    if (diffBits > 0) {
        for (Int idx = 0; idx < 5; idx++) {
            if (absDiff < g_qpIndexThresholdPositive[m_chromaFormat][idx]) {
                break;
            }
            qpIndex++;
        }
    }
    else {
        for (Int idx = 0; idx < 4; idx++) {
            if (absDiff < g_qpIndexThresholdNegative[m_chromaFormat][idx]) {
                break;
            }
            qpIndex++;
        }
    }

    // deltaQp
    if (diffBits > 0) {
        deltaQp = 1 * m_QpIncrementTable[m_qpUpdateMode][qpIndex];
    }
    else {
        deltaQp = -1 * m_QpDecrementTable[m_qpUpdateMode][qpIndex];
    }

    return deltaQp;
}

/**
    Update bufferFullness

    @param bits bits used to code previous block
    */
Void RateControl::UpdateBufferFullness (Int bits) {
    m_blockBits = bits;
    if (!m_isFirstBlockInSlice) {
        m_numBlocksCoded += 1;
        m_numPixelsCoded += 16;
    }

    if (m_numPixelsCoded <= 16 * m_initTxDelay) {
        // bits accumulate in rate buffer during initial transmission delay
        m_bufferFullness += bits;
    }
    else {
        m_isBufferTxStart = true;
        m_bufferFullness += (bits - m_avgBlockBits);
        m_curChunkSize += m_avgBlockBits;
        m_numChunkBlks += 1;
    }

    //! chunk byte alignment and sanity check !//
    Bool isEvenChunk = ((m_chunkCounts % 2) == 0);
    Bool isSliceWidthMultipleOf16 = ((m_sliceWidth & 0xF) == 0);
	UInt chunkAdjBits = 0;
    UInt curChunkBits = 0;
    UInt nextChunkBits = 0;
	if ((16 * m_numChunkBlks) >= m_sliceWidth) {
        if (!isSliceWidthMultipleOf16) {
            chunkAdjBits = isEvenChunk ? ((m_chunkAdjustmentBits + 1) >> 1) : (m_chunkAdjustmentBits) >> 1;
            curChunkBits = isEvenChunk ? (m_avgBlockBits >> 1) : m_avgBlockBits;
            nextChunkBits = isEvenChunk ? ((m_avgBlockBits + 1) >> 1) : 0;
        }
        else {
            chunkAdjBits = (m_chunkAdjustmentBits) >> 1;
            curChunkBits = m_avgBlockBits;
            nextChunkBits = 0;
        }
        m_curChunkSize -= nextChunkBits;
        m_curChunkSize += chunkAdjBits;
        if (m_curChunkSize != (m_maxChunkSize << 3)) {
            fprintf (stderr, "RateControl::UpdateBufferFullness(): chunk size is not byte-aligned.\n");
            fprintf (stderr, "current chunk size (bits): %d.\n", m_curChunkSize);
            fprintf (stderr, "expected chunk size (bits): %d.\n", m_maxChunkSize << 3);
            exit (EXIT_FAILURE);
        }

        //! chunk byte-alignment adjustment bits !//
        m_bufferFullness -= chunkAdjBits;

        //! next chunk (used if slice width not divisible by 16 !//
        m_curChunkSize = nextChunkBits;
        m_numChunkBlks = (nextChunkBits > 0 ? 1 : 0);
        m_chunkCounts += 1;
    }

    m_sliceBitsCurrent += bits;
    if (m_sliceBitsCurrent > m_sliceBitsTotal) {
        fprintf (stderr, "RateControl::UpdateBufferFullness(): rate for current slice (%d) exceeds max (%I64d).\n",
            m_sliceBitsCurrent, m_sliceBitsTotal);
        exit (EXIT_FAILURE);
    }

    if (!m_isFirstBlockInSlice) {
        m_slicePixelsRemaining -= 16;
    }
}

/**
    Update offset parameters: rcOffset, rcOffsetInit
    */
Void RateControl::UpdateRcOffsets () {
    //! update rcOffsetInit !//
    if (m_numPixelsCoded <= m_initTxDelay * m_blockMaxNumPixels) {
        if (!m_isFirstBlockInSlice) {
            m_rcOffsetInit -= m_avgBlockBits;
        }
    }

    //! update rcOffset !//
    Int Th = m_numBlocksInSlice - (m_numBlocksInLine * m_pps->GetRcFullnessOffsetThreshold ());
    if (m_numBlocksCoded >= Th) {
        Int slope = m_pps->GetRcFullnessOffsetSlope ();
        m_bufferFracBitsAccum += slope & 0xffff;
        Int offset = (slope >> g_rc_bufferOffsetPrecision) + (m_bufferFracBitsAccum >> g_rc_bufferOffsetPrecision);
        m_bufferFracBitsAccum &= 0xffff;
        if (m_slicePixelsRemaining <= m_blockMaxNumPixels && m_slicePixelsRemaining > 0) {
            // special case for last block in slice
            if (m_bufferFracBitsAccum >= (1 << (g_rc_bufferOffsetPrecision - 1))) {
                offset += 1;
            }
        }
        m_rcOffset += offset;
    }
}

/**
    Update rcFullness
    */
Void RateControl::UpdateRcFullness () {
    if (m_bufferFullness > m_rcBufferMaxSize) {
        //! overflow !//
        fprintf (stderr, "RateControl::UpdateRcFullness (): bufferFullness overflow.\n");
        exit (EXIT_FAILURE);
    }
    else if (m_bufferFullness < 0) {
        //! underflow !//
        fprintf (stderr, "RateControl::UpdateRcFullness (): bufferFullness underflow.\n");
        exit (EXIT_FAILURE);
    }
    else {
        UpdateRcOffsets ();
        Int temp = m_pps->GetRcBufferFullnessScale () * (m_bufferFullness + m_rcOffset + m_rcOffsetInit);
        m_rcFullness = Clip3<Int> (0, (1 << g_rc_BFRangeBits) - 1, temp >> g_rc_BFScaleApprox);
    }
}

/**
    Update chunk adjustment bits for all remaining chunks in slice
    */
Void RateControl::UpdateBufferLastChunksInSlice() {
    UInt chunkAdjBits = 0;
	while (m_chunkCounts < m_sliceHeight) {
		if (m_chunkCounts % 2 == 0) {
            chunkAdjBits = (m_chunkAdjustmentBits + 1) >> 1;
		}
        else {
            chunkAdjBits = (m_chunkAdjustmentBits) >> 1;
        }
        m_bufferFullness -= chunkAdjBits;
		m_chunkCounts++;
	}
}

Bool RateControl::CheckUnderflowPrevention () {
    UInt maxAdjBits = (GetChunkAdjBits () + 1) >> 1;
    if (GetBufferFullness () < (Int)((2 * GetAvgBlockBits ()) + maxAdjBits)) {
        return true;
    }
    else {
        return false;
    }
}
