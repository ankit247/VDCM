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

/* Copyright (c) 2015-2017 MediaTek Inc., RDO weighting is modified and/or implemented by MediaTek Inc. based on Mode.cpp
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
\file       Mode.cpp
\brief      Mode class
Parent class for individual modes
*/
#include <string.h>
#include "Mode.h"
#include "TypeDef.h"
#include "SyntaxElement.h"
#include "Ssm.h"
#include "Misc.h"

/**
    Mode constructor

    @param curMode
    @param inputCsc
    @param csc
    @param bpp
    @param bitDepth
    @param isEncoder
    @param chromaFormat
*/
Mode::Mode (ModeType curMode, ColorSpace inputCsc, ColorSpace csc, Int bpp, Int bitDepth, Bool isEncoder, ChromaFormat chromaFormat)
    : m_curBlockMode (curMode)
    , m_origSrcCsc (inputCsc)
    , m_csc (csc)
    , m_bpp (bpp)
    , m_chromaFormat (chromaFormat)
    , m_bitDepth (bitDepth)
    , m_isValid (true)
    , m_isFls (true)
    , m_isNextBlockFls (true)
    , m_isPartial (false)
    , m_isEncoder (isEncoder)
    , m_underflowPreventionMode (false)
{

}

/**
    Destructor
*/
Mode::~Mode () {
    for (Int c = 0; c < g_numComps; c++) {
        if (m_curBlk[c]) { xFree (m_curBlk[c]);              m_curBlk[c] = NULL; }
        if (m_predBlk[c]) { xFree (m_predBlk[c]);             m_predBlk[c] = NULL; }
        if (m_resBlk[c]) { xFree (m_resBlk[c]);              m_resBlk[c] = NULL; }
        if (m_qResBlk[c]) { xFree (m_qResBlk[c]);             m_qResBlk[c] = NULL; }
        if (m_deQuantResBlk[c]) { xFree (m_deQuantResBlk[c]);       m_deQuantResBlk[c] = NULL; }
        if (m_recBlk[c]) { xFree (m_recBlk[c]);              m_recBlk[c] = NULL; }
        if (m_neighborsAbove[c]) { xFree (m_neighborsAbove[c]);      m_neighborsAbove[c] = NULL; }
        if (m_neighborsLeft[c]) { xFree (m_neighborsLeft[c]);       m_neighborsLeft[c] = NULL; }
        if (m_neighborsLeftAbove[c]) { xFree (m_neighborsLeftAbove[c]);  m_neighborsLeftAbove[c] = NULL; }
    }
}

/**
    Allocate memory for buffers
*/
Void Mode::SetupBufferMemory () {
    for (Int c = 0; c < g_numComps; c++) {
        Int numPx = getWidth (c) * getHeight (c);
        Int neighborsAboveLen = m_neighborsAboveLen >> g_compScaleX[c][m_chromaFormat];
        Int height = getHeight (c);
        m_curBlk[c] = (Int*)xMalloc (Int, numPx);
        m_predBlk[c] = (Int*)xMalloc (Int, numPx);
        m_resBlk[c] = (Int*)xMalloc (Int, numPx);
        m_qResBlk[c] = (Int*)xMalloc (Int, numPx);
        m_deQuantResBlk[c] = (Int*)xMalloc (Int, numPx);
        m_recBlk[c] = (Int*)xMalloc (Int, numPx);
        m_neighborsAbove[c] = (Int*)xMalloc (Int, neighborsAboveLen);
        m_neighborsLeft[c] = (Int*)xMalloc (Int, m_neighborsLeftLen * height);
        m_neighborsLeftAbove[c] = (Int*)xMalloc (Int, m_neighborsAboveLeftLen);
    }
}

/**
    Compute SAD between two blocks

    @param blk0 buffer for first block data
    @param blk1 buffer for second block data
    @param length length of block data
    @return SAD
*/
Int Mode::CalcSad (Int* blk0, Int* blk1, Int length) {
    Int sad = 0;
    for (Int i = 0; i < length; i++) {
        sad += abs (blk0[i] - blk1[i]);
    }
    return sad;
}

/**
    Encode mode header

    @param isFinal flag to indicate final encode
    @return number of bits used to encode
*/
Int Mode::EncodeModeHeader (Bool isFinal) {
    UInt substreamId = 0;
    Int bits = 0;
    UInt modeSameBits = 1;
    UInt tempBits = g_modeHeaderLen[m_curBlockMode];
    UInt tempCode = g_modeHeaderCode[m_curBlockMode];
    Bool sameFlag = (m_prevBlockMode == m_curBlockMode ? true : false);
    if (sameFlag) {
        bits += 1;
        if (isFinal) {
            m_ssm->GetCurSyntaxElement (substreamId)->AddSyntaxWord (1, 1);
        }
    }
    else {
        bits += modeSameBits;
        bits += tempBits;
        if (isFinal) {
            m_ssm->GetCurSyntaxElement (substreamId)->AddSyntaxWord (0, modeSameBits);
            m_ssm->GetCurSyntaxElement (substreamId)->AddSyntaxWord (tempCode, tempBits);
        }
    }
    return bits;
}

/**
    Encode flatness info

    @param isFinal flag to indicate final encode
    @return number of bits needed to encode
*/
Int Mode::EncodeFlatness (Bool isFinal) {
    UInt substreamId = 0;
    UInt flatnessTypeBits = (m_flatnessType == kFlatnessTypeUndefined ? 0 : 2);
    UInt flatnessFlagBits = 1;
    if (isFinal) {
        if (m_flatnessType == kFlatnessTypeUndefined) {
            m_ssm->GetCurSyntaxElement (substreamId)->AddSyntaxWord (0, flatnessFlagBits);
        }
        else {
            UInt flatnessCode = (UInt)m_flatnessType;
            m_ssm->GetCurSyntaxElement (substreamId)->AddSyntaxWord (1, flatnessFlagBits);
            m_ssm->GetCurSyntaxElement (substreamId)->AddSyntaxWord (flatnessCode, flatnessTypeBits);
        }
    }
    return (flatnessFlagBits + flatnessTypeBits);
}

/**
    Encode MPP colorspace

    @param isFinal flag to indicate final encode
    @return number of bits needed to encode
*/
Int Mode::EncodeMPPCSpace (Bool isFinal) {
    Int CSpaceBits = 1;
    if (isFinal) {
        UInt cscFlag = (m_csc == kRgb ? 0 : 1);
        m_ssm->GetCurSyntaxElement (0)->AddSyntaxWord (cscFlag, 1);
    }
    return CSpaceBits;
}

/**
    Parse a fixed length code from given funnel shifter

    @param numBits number of bits to parse
    @param ssmIdx substream index
    @return parsed value
*/
Int Mode::DecodeFixedLengthCodeSsm (UInt numBits, UInt ssmIdx) {
    UInt value = m_ssm->ReadFromFunnelShifter (ssmIdx, numBits);
    return (Int)value;
}

/**
    Reset all buffers to zero
*/
Void Mode::ResetBuffers () {
    for (Int k = 0; k < g_numComps; k++) {
        Int numPx = getWidth (k) * getHeight (k);
        ::memset (m_curBlk[k], 0, numPx * sizeof (Int));
        ::memset (m_predBlk[k], 0, numPx * sizeof (Int));
        ::memset (m_resBlk[k], 0, numPx * sizeof (Int));
        ::memset (m_qResBlk[k], 0, numPx * sizeof (Int));
        ::memset (m_deQuantResBlk[k], 0, numPx * sizeof (Int));
        ::memset (m_recBlk[k], 0, numPx * sizeof (Int));
    }
}

/**
    Perform CSC for block neighbors (RGB -> YCoCg)
*/
Void Mode::NeighborsRgbToYCoCg () {
    assert (m_chromaFormat == k444 && m_csc != kYCbCr);
	Int *pR, *pG, *pB;

    // above
    pR = GetNeighborsAbove (0);
    pG = GetNeighborsAbove (1);
    pB = GetNeighborsAbove (2);
    for (Int x = 0; x < m_neighborsAboveLen; x++) {
        Int Co = pR[x] - pB[x];
        Int tmp = pB[x] + (Co >> 1);
        Int Cg = pG[x] - tmp;
        pR[x] = tmp + (Cg >> 1);	// Y
        pG[x] = Co;					// Co
        pB[x] = Cg;					// Cg
    }

    // left-above
    pR = GetNeighborsLeftAbove (0);
    pG = GetNeighborsLeftAbove (1);
    pB = GetNeighborsLeftAbove (2);
    for (Int x = 0; x < m_neighborsAboveLeftLen; x++) {
        Int Co = pR[x] - pB[x];
        Int tmp = pB[x] + (Co >> 1);
        Int Cg = pG[x] - tmp;
        pR[x] = tmp + (Cg >> 1);	// Y
        pG[x] = Co;					// Co
        pB[x] = Cg;					// Cg
    }
}

/**
    Perform CSC for current block (YCoCg -> RGB)
*/
Void Mode::CurBlockYCoCgToRgb () {
    // sanity check
    if (m_csc == kYCbCr) {
        fprintf (stderr, "Mode::CurBlockYCoCgToRgb(): should not get here for YCbCr source.\n");
        exit (EXIT_FAILURE);
    }
    Int Y, Co, Cg, temp, R, G, B;
    Int *pR = GetCurBlkBuf (0);
    Int *pG = GetCurBlkBuf (1);
    Int *pB = GetCurBlkBuf (2);
    Int numPx = m_blkWidth * m_blkHeight;
    for (Int x = 0; x < numPx; x++) {
        Y = pR[x];
        Co = pG[x];
        Cg = pB[x];
        temp = Y - (Cg >> 1);
        G = Cg + temp;
        B = temp - (Co >> 1);
        R = Co + B;
        pR[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, R);
        pG[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, G);
        pB[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, B);
    }
}

/**
    Perform CSC for current block (RGB -> YCoCg)
*/
Void Mode::CurBlockRgbToYCoCg () {
    // sanity check
    if (m_csc == kYCbCr) {
        fprintf (stderr, "Mode::CurBlockRgbToYCoCg(): should not get here for YCbCr source.\n");
        exit (EXIT_FAILURE);
    }
    Int *pR = GetCurBlkBuf (0);
    Int *pG = GetCurBlkBuf (1);
    Int *pB = GetCurBlkBuf (2);
    Int numPx = m_blkWidth * m_blkHeight;
    for (Int x = 0; x < numPx; x++) {
        Int Co = pR[x] - pB[x];
        Int tmp = pB[x] + (Co >> 1);
        Int Cg = pG[x] - tmp;
        pR[x] = tmp + (Cg >> 1);	// Y
        pG[x] = Co;					// Co
        pB[x] = Cg;					// Cg
    }
}

/**
    Perform CSC for reconstructed block (YCoCg -> RGB)
*/
Void Mode::RecBlockYCoCgToRgb () {
    assert (m_chromaFormat == k444 && m_csc != kYCbCr);
    Int temp, R, G, B;
    Int *pR = GetRecBlkBuf (0);
    Int *pG = GetRecBlkBuf (1);
    Int *pB = GetRecBlkBuf (2);
    Int numPx = m_blkWidth * m_blkHeight;
    for (Int x = 0; x < numPx; x++) {
        temp = pR[x] - (pB[x] >> 1);
        G = pB[x] + temp;
        B = temp - (pG[x] >> 1);
        R = pG[x] + B;
        pR[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, R);
        pG[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, G);
        pB[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, B);
    }
}

Void Mode::RecBlockRgbToYCoCg () {
	assert (m_chromaFormat == k444 && m_csc != kYCbCr);
	Int *comp0 = GetRecBlkBuf (0); // R
	Int *comp1 = GetRecBlkBuf (1); // G
	Int *comp2 = GetRecBlkBuf (2); // B
	Int numPx = m_blkWidth * m_blkHeight;
	for (Int x = 0; x < numPx; x++) {
		Int Co = comp0[x] - comp2[x];
		Int tmp = comp2[x] + (Co >> 1);
		Int Cg = comp1[x] - tmp;
		comp0[x] = tmp + (Cg >> 1); // Y
		comp1[x] = Co; // Co
		comp2[x] = Cg; // Cg
	}
}

/**
    Initialize current block data
*/
Void Mode::InitCurBlock () {
    for (Int k = 0; k < g_numComps; k++) {
        Int* pSrc = m_block->GetBufOrg (k);
        Int* pDst = GetCurBlkBuf (k);
        Int width = getWidth (k);
        Int height = getHeight (k);
        for (Int j = 0; j < height; j++) {
            for (Int i = 0; i < width; i++) {
                Int loc = j * width + i;
                pDst[loc] = pSrc[loc];
            }
        }
    }
    // switch is based on the current mode's desired colorspace
    switch (m_csc) {
    case kRgb:
        assert (m_origSrcCsc != kYCbCr);
        if (m_origSrcCsc == kYCoCg) {
            CurBlockYCoCgToRgb ();
        }
        break;
    case kYCoCg:
        assert (m_origSrcCsc != kYCbCr);
        if (m_origSrcCsc == kRgb) {
            CurBlockRgbToYCoCg ();
        }
        break;
    case kYCbCr:
        // no CSC is supported for YCbCr, handled natively
        assert (m_origSrcCsc == kYCbCr);
        break;
    default:
        fprintf (stderr, "Mode::InitCurBlock(): unsupported color space: %d.\n", m_csc);
        exit (EXIT_FAILURE);
    }
}

/**
    Initialize block neighbors
    */
Void Mode::InitNeighbors () {
    Int curPosX = m_posX;
    Int curPosY = m_posY;
    Bool resetLeft = (curPosX == 0 ? true : false);
    Bool resetAbove = GetIsFls ();

    // Initialize neighbors (left samples) for spatial prediction
    for (Int c = 0; c < g_numComps; c++) {
        Int curCompPosX = curPosX >> g_compScaleX[c][m_chromaFormat];
        Int curCompPosY = curPosY >> g_compScaleY[c][m_chromaFormat];
        Int *ptrN = GetNeighborsLeft (c);
        Int *ptrPel = m_pRecVideoFrame->getBuf (c);
        Int stride = m_pRecVideoFrame->getWidth (c);
        Int height = getHeight (c);
        ptrPel += (curCompPosY * stride + curCompPosX);
        if (resetLeft) {
			Int pred = (m_csc == kYCoCg && c > 0 ? 0 : 1 << (m_bitDepth - 1));
            for (Int i = 0; i < height * m_neighborsLeftLen; i++) {
                ptrN[i] = pred;
            }
        }
        else {
            for (Int j = 0; j < height; j++) {
                for (Int i = 0; i < m_neighborsLeftLen; i++) {
                    ptrN[j * m_neighborsLeftLen + i] = ptrPel[(j * stride) - m_neighborsLeftLen + i];
                }
            }
        }
    }

    // Initialize neighbors (above samples) for spatial prediction
    for (Int c = 0; c < g_numComps; c++) {
        Int neighborsAboveLen = m_neighborsAboveLen >> g_compScaleX[c][m_chromaFormat];
        Int curCompPosX = curPosX >> g_compScaleX[c][m_chromaFormat];
        Int curCompPosY = curPosY >> g_compScaleY[c][m_chromaFormat];
        Int *ptrN = GetNeighborsAbove (c);
        Int *ptrPel = m_pRecVideoFrame->getBuf (c); // top-left corner of image 
        Int stride = m_pRecVideoFrame->getWidth (c);
        ptrPel += (curCompPosY * stride + curCompPosX); // top-left corner in the current block
        Int width = getWidth (c);
        Bool lastBlock = (curCompPosX + width) >= stride ? true : false;
        int numPix = 0;
        if (lastBlock) {
            numPix = stride - curCompPosX;
        }

        if (resetAbove) {
            // first line in the slice
            Int pred = 1 << (m_bitDepth - 1);
            for (Int i = 0; i < neighborsAboveLen; i++) {
                ptrN[i] = pred;
            }
        }
        else if (lastBlock) {
            // for last block (also if the last block is partial)
            for (Int i = 0; i < neighborsAboveLen; i++) {
                if (i < numPix) {
                    ptrN[i] = *(ptrPel - stride + i);
                }
                else {
                    ptrN[i] = *(ptrPel - stride + numPix - 1); // out of boundary samples are assigned with the last value in the line 
                }
            }
        }
        else {
            // for all blocks in the slice
            for (Int i = 0; i < neighborsAboveLen; i++) {
                ptrN[i] = *(ptrPel - stride + i);
            }
        }
    }

    // Initialize neighbors (left-above samples) for spatial prediction
    for (Int c = 0; c < g_numComps; c++) {
        Int curCompPosX = curPosX >> g_compScaleX[c][m_chromaFormat];
        Int curCompPosY = curPosY >> g_compScaleY[c][m_chromaFormat];
        Int *ptrN = GetNeighborsLeftAbove (c);
        Int *ptrPel = m_pRecVideoFrame->getBuf (c);
        Int stride = m_pRecVideoFrame->getWidth (c);
        ptrPel += (curCompPosY * stride + curCompPosX);
        if (resetAbove || resetLeft) {
            for (Int i = 0; i < m_neighborsAboveLeftLen; i++) {
                ptrN[i] = 1 << (m_bitDepth - 1);
            }
        }
        else {
            for (Int i = 0; i < m_neighborsAboveLeftLen; i++) {
                ptrN[i] = *(ptrPel - stride - 1 - i);
            }
        }
    }

    switch (m_csc) {
    case kRgb:
        assert (m_origSrcCsc == kRgb);
        // nothing to do
        break;
    case kYCoCg:
        assert (m_origSrcCsc == kRgb);
        NeighborsRgbToYCoCg ();
        break;
    case kYCbCr:
        assert (m_origSrcCsc == kYCbCr);
        // nothing to do
        break;
    default:
        assert (false);
    }
}

/**
    Compute distortion for current block

    @return distortion
*/
Int Mode::ComputeCurBlockDistortion () {
    Int distortion = 0;
    for (Int k = 0; k < g_numComps; k++) {
        Int numPx = getWidth (k) * getHeight (k);
        Int compDistortion = 0;
        Int* pOrg = GetCurBlkBuf (k);
        Int* pRec = GetRecBlkBuf (k);
        Int* pRes = GetResBlkBuf(k);
        Int* pDeQuant = GetDeQuantBlkBuf(k);
        for (Int i = 0; i < numPx; i++) {
			if (m_curBlockMode == kModeTypeBP || m_curBlockMode == kModeTypeBPSkip)
                compDistortion += abs(pRes[i] - pDeQuant[i]);
            else
                compDistortion += abs(pOrg[i] - pRec[i]);
        }
        switch (m_csc) {
        case kRgb:
            distortion += compDistortion;
            break;
        case kYCoCg:
            distortion += ((g_ycocgWeights[k] * compDistortion + 128) >> 8);
            break;
        case kYCbCr:
            distortion += compDistortion;
            break;
        default:
            assert (false);
        }
    }
    return distortion;
}

/**
    Initialize (decoder)

    @param sliceY position of slice within picture (y)
    @param sliceX position of slice within picture (x)
    @param videoFrame VideoFrame object
    @param rc rate control object
    @param entropyCoder entropy coder object
    @param blkWidth block width
    @param blkHeight block height
    @param ssm substream multiplexer object
    @param pps PPS object
*/
Void Mode::InitDecoder (UInt sliceY, UInt sliceX, VideoFrame* videoFrame, RateControl* rc, EntropyCoder* entropyCoder, Int blkWidth, Int blkHeight, SubStreamMux* ssm, Pps* pps) {
    m_ssm = ssm;
    m_pps = pps;
    m_sliceStartY = sliceY;
    m_sliceStartX = sliceX;
    Int srcWidth = videoFrame->getWidth (0);
    Int srcHeight = videoFrame->getHeight (0);
    Int numPx = blkWidth * blkHeight;
    SetBlkSize (blkWidth, blkHeight);
    SetSrcSize (srcWidth, srcHeight);
    SetEntropyCoder (entropyCoder);
    m_pRecVideoFrame = videoFrame;
    m_rateControl = rc;
    Int nLeft = 2;
    Int nAbove = 2 * m_blkWidth;
    Int nLeftAbove = 4;
    SetNeighborsLen (nLeft, nAbove, nLeftAbove);
    SetupBufferMemory ();
    InitModeSpecific ();
}

/**
    Initialize (encoder)

    @param sliceY position of slice within picture (y)
    @param sliceX position of slice within picture (x)
    @param block source block object
    @param videoFrame VideoFrame object
    @param rc rate control object
    @param entropyCoder entropy coder object
    @param ssm substream multiplexer object
    @param pps PPS object
*/
Void Mode::Init (UInt sliceY, UInt sliceX, Block* block, VideoFrame* videoFrame, RateControl* rc, EntropyCoder* entropyCoder, SubStreamMux* ssm, Pps* pps) {
    m_ssm = ssm;
    m_pps = pps;
    m_sliceStartY = sliceY;
    m_sliceStartX = sliceX;
    Int width = block->GetWidth (0);
    Int height = block->GetHeight (0);
    Int srcWidth = videoFrame->getWidth (0);
    Int srcHeight = videoFrame->getHeight (0);
    SetBlkSize (width, height);
    SetSrcSize (srcWidth, srcHeight);
    SetBlock (block);
    SetEntropyCoder (entropyCoder);
    SetVideoFrame (videoFrame);
    SetRateControl (rc);
    Int nLeft = 2;
    Int nAbove = 2 * m_blkWidth;
    Int nLeftAbove = 4;
    SetNeighborsLen (nLeft, nAbove, nLeftAbove);
    SetupBufferMemory ();
    InitModeSpecific ();
}

/**
    Update mode info
        
    @param x current position (x)
    @param y current position (y)
    @param isFls flag to indicate FLS
    @param prevBlockMode previous block mode
    @param flatnessType flatness type
    @param maxBits max bits for current block
    @param lambdaBuffer BF lambda
*/
Void Mode::Update (Int x, Int y, Bool isFls, ModeType prevBlockMode, FlatnessType flatnessType, Int maxBits, Int lambdaBuffer) {
    SetPos (x, y);
    SetPrevBlockMode (prevBlockMode);
    SetFlatnessType (flatnessType);
    SetMaxBits (maxBits);
    SetLambdaBuffer (lambdaBuffer);
    SetIsFls (isFls);
    UpdateQp ();
}

/**
    Update mode info

    @param x current position (x)
    @param y current position (y)
    @param isFls flag to indicate FLS
    @param prevBlockMode previous block mode
*/
Void Mode::Update (Int x, Int y, Bool isFls, ModeType prevBlockMode) {
    // update for decoder side
    SetPos (x, y);
    SetPrevBlockMode (prevBlockMode);
    SetIsFls (isFls);
    UpdateQp ();
    if (m_chromaFormat == k422 || m_chromaFormat == k420) {
        m_csc = kYCbCr;
    }
}

/**
    Calculate RD cost for current block mode

    @return RD cost
*/
Int Mode::ComputeRdCost () {
    return ComputeRdCost (m_rate, m_distortion);
}

/**
    Calculate RD cost for given rate/distortion

    @param rate block mode rate
    @param distortion block mode distortion
    @return RD cost value
*/
Int Mode::ComputeRdCost (Int rate, Int distortion) {
    Int rdCost = LARGE_INT;
    if (rate == LARGE_INT || distortion == LARGE_INT || rate > m_maxBits) {
        m_rdCost = rdCost;
        return m_rdCost;
    }
	UInt lambda = ComputeBitrateLambda (rate, m_maxBits);
	lambda *= m_lambdaBuffer;
	Int64 tempResult = (Int64)lambda * (Int64)rate;
	Int64 scale = (Int64)(g_rc_bitrateLambdaBits + g_rc_BFLambdaPrecisionLut);
	rdCost = distortion + (Int)(tempResult >> scale);
    m_rdCost = rdCost;
    return m_rdCost;
}

/**
    Calculate bitrate lambda value

    @param bits bitrate for current block mode
    @param maxBits max bits/block
    @return bitrate lambda factor
*/
UInt Mode::ComputeBitrateLambda (UInt bits, UInt maxBits) {
    UInt x = g_rc_bitrateLambdaBits - g_rc_bitrateLambdaLutBits;
    UInt lutIndex = Clip3<UInt> (0, 63, (bits * m_pps->GetRcLambdaBitrateScale ()) >> x);
    Int idxMod = lutIndex & 0x03;
    Int x_0 = m_pps->GetLutLambdaBitrate (Clip3<Int> (0, 15, (lutIndex >> 2)));
    Int x_1 = m_pps->GetLutLambdaBitrate (Clip3<Int> (0, 15, (lutIndex >> 2) + 1));
    UInt lambda = 2 * (((4 - idxMod) * x_0 + idxMod * x_1 + 2) >> 2);
    return lambda;
}

/**
    Copy reconstructed block data to reconstructed slice buffer

    @param startCol upper-left position of block within slice (col)
    @param startRow upper-left position of block within slice (row)
*/
Void Mode::CopyToRecFrame (Int startCol, Int startRow) {
    for (Int c = 0; c < g_numComps; c++) {
        Int col = startCol >> g_compScaleX[c][m_chromaFormat];
        Int row = startRow >> g_compScaleY[c][m_chromaFormat];
        Int height = getHeight (c);
        Int width = getWidth (c);
        Int sliceStride = m_pRecVideoFrame->getWidth (c);
        Int* pBufRecDst = m_pRecVideoFrame->getBuf (c) + (row * sliceStride) + col;
        Int* pBufRecSrc = GetRecBlkBuf (c);
        for (Int j = 0; j < height; j++) {
            for (Int i = 0; i < width; i++) {
                Int loc = j * width + i;
                pBufRecDst[i] = pBufRecSrc[loc];
            }
            pBufRecDst += sliceStride;
        }
    }
}

Void Mode::CopyToYCoCgBuffer (Int col) {
	for (Int c = 0; c < g_numComps; c++) {
		Int height = getHeight (c);
		Int width = getWidth (c);
		Int sliceStride = m_recBufferYCoCg->getWidth (c);
		Int* pBufRecDst = m_recBufferYCoCg->getBuf (c) + col;
		Int* pBufRecSrc = GetRecBlkBuf (c);
		for (Int j = 0; j < height; j++) {
			for (Int i = 0; i < width; i++) {
				Int loc = j * width + i;
				pBufRecDst[i] = pBufRecSrc[loc];
			}
			pBufRecDst += sliceStride;
		}
	}
}

/**
    Check if all SE for the current block are at most maxSeSize

    @param isFinal flag to indicate final encode
    @param perSsmBits bits in each substream
    @return true if current block is valid (all SE <= maxSeSize)
*/
Bool Mode::CheckMaxSeSize (UInt *perSsmBits) {
    Bool isValid = true;
    for (UInt ss = 0; ss < m_ssm->GetNumSubStreams (); ss++) {
        if (perSsmBits[ss] > m_ssm->GetMaxSeSize ()) {
            isValid = false;
        }
    }
    return isValid;
}

/**
    Copy reconstructed slice to reconstructed frame

    @param startRow frame position (row) of upper-left slice pixel
    @param startCol frame position (col) of upper-left slice pixel
*/
Void Mode::CopyRecPxToFrame (Int startRow, Int startCol) {
    for (Int c = 0; c < g_numComps; c++) {
        Int col = startCol >> g_compScaleX[c][m_chromaFormat];
        Int row = startRow >> g_compScaleY[c][m_chromaFormat];
        Int height = getHeight (c);
        Int width = getWidth (c);
        Int frameStride = m_pRecVideoFrame->getWidth (c);
        Int* dst = m_pRecVideoFrame->getBuf (c) + (row * frameStride) + col;
        Int* src = GetRecBlkBuf (c);
        for (Int j = 0; j < height; j++) {
            for (Int i = 0; i < width; i++) {
                Int loc = j * width + i;
                dst[i] = src[loc];
            }
            dst += frameStride;
        }
    }
}

Int Mode::GetCurCompQpMod (Int masterQp, Int k) {
	Int tempQp = 0; 
	switch (m_csc) {
		case kRgb:
			tempQp = masterQp;
			break;
		case kYCbCr:
			if (masterQp < 16) {
				tempQp = masterQp;
			}
			else {
				tempQp = (k == 0 ? masterQp : g_qStepChroma[masterQp - 16]);
			}
			break;
		case kYCoCg:
			if (masterQp < 16) {
				tempQp = (k == 0 ? masterQp : masterQp + 8);
			}
			else {
				tempQp = (k == 0 ? masterQp : (k == 1) ? g_qStepCo[masterQp - 16] : g_qStepCg[masterQp - 16]);
			}
			break;
	}

	Int qpAdj = ((m_bitDepth - g_minBpc) << 3);
	Int modQp = Clip3<Int> (GetMinQp (m_pps), 72, tempQp) + qpAdj;
	return modQp;
}

/**
    Get QP for specified component

    @param masterQp QP for component 0
    @param k component index
    @return QP
*/
Int Mode::GetCurCompQp (Int masterQp, Int k) {
    Int minQp = GetMinQp (m_pps);
    Int maxQpFls = m_rateControl->GetMaxQp ();
    Int modQp = masterQp - g_rc_qpStepSizeOne;
    Int qp = 0;
    switch (m_csc) {
    case kYCoCg:
        if (modQp < 0) { //! bpc > 8 !//
            qp = masterQp + 8;
        }
        else {
            switch (k) {
            case 0:
                qp = masterQp;
                break;
            case 1:
                qp = g_qStepCo[modQp];
                break;
            case 2:
                qp = g_qStepCg[modQp];
                break;
            }
        }
        // clip QP between min/max
        if (k > 0 && GetIsFls () && masterQp <= maxQpFls)
            qp = Clip3<Int> (minQp, 72, qp);

        break;
    case kYCbCr:
        qp = (k == 0) ? masterQp : (modQp < 0 ? masterQp : g_qStepChroma[modQp] - 8);
        break;
    case kRgb:
        qp = masterQp;
        break;
    default:
        assert (false);
    }

    if (m_bitDepth > g_minBpc) {
        qp += ((m_bitDepth - g_minBpc) << 3);
    }
    return qp;
}