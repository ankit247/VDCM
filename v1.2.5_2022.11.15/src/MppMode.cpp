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

 /* Copyright (c) 2015-2017 MediaTek Inc., RDO weighting/MPP mode are modified and/or implemented by MediaTek Inc. based on MppMode.cpp
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
  \file       MppMode.cpp
  \brief      Midpoint Prediction Mode
  */

#include "MppMode.h"
#include "TypeDef.h"
#include "Misc.h"
#include <cstring>

extern Bool enEncTracer;
extern Bool enDecTracer;
extern FILE* encTracer;
extern FILE* decTracer;

/**
    MPP mode constructor

    @param bpp compressed bitrate
    @param bitDepth bit depth
    @param isEncoder flag to indicate encoder
    @param chromaFormat chroma sampling format
    @param csc color space
    @param colorSpaceQpThres QP threshold for MPP CSC
    */
MppMode::MppMode (Int bpp, Int bitDepth, Bool isEncoder, ChromaFormat chromaFormat, ColorSpace csc, Int colorSpaceQpThres)
    : Mode (kModeTypeMPP, csc, ((csc == kRgb) ? kRgb : kYCbCr), bpp, bitDepth, isEncoder, chromaFormat)
    , m_colorSpaceQpThres (colorSpaceQpThres)
    , m_curBlockStepSize (0)
    , m_nextBlockStepSize (0) {
    for (UInt k = 0; k < g_numComps; k++) {
        m_midpoint[k] = new Int[4];
    }
}

/**
    Destructor
    */
MppMode::~MppMode () {
    for (Int c = 0; c < g_numComps; c++) {
        if (m_midpoint[c]) { delete[] m_midpoint[c]; m_midpoint[c] = NULL; }
        if (m_recBlkBufA[c]) { delete[] m_recBlkBufA[c]; m_recBlkBufA[c] = NULL; }
        if (m_recBlkBufB[c]) { delete[] m_recBlkBufB[c]; m_recBlkBufB[c] = NULL; }
        if (m_recBlkBufC[c]) { delete[] m_recBlkBufC[c]; m_recBlkBufC[c] = NULL; }
        if (m_quantBlkBufA[c]) { delete[] m_quantBlkBufA[c]; m_quantBlkBufA[c] = NULL; }
        if (m_quantBlkBufB[c]) { delete[] m_quantBlkBufB[c]; m_quantBlkBufB[c] = NULL; }

        if (m_resBlkBufA[c]) { delete[] m_resBlkBufA[c]; m_resBlkBufA[c] = NULL; }
        if (m_resBlkBufB[c]) { delete[] m_resBlkBufB[c]; m_resBlkBufB[c] = NULL; }
    }
    ClearMppSuffixMapping ();
}

Void MppMode::InitBuffers () {
    for (UInt k = 0; k < g_numComps; k++) {
        UInt numPx = getWidth (k) * getHeight (k);
        m_resBlkBufA[k] = new Int[numPx];
        m_resBlkBufB[k] = new Int[numPx];
        m_quantBlkBufA[k] = new Int[numPx];
        m_quantBlkBufB[k] = new Int[numPx];
        m_recBlkBufA[k] = new Int[numPx];
        m_recBlkBufB[k] = new Int[numPx];
        m_recBlkBufC[k] = new Int[numPx];
    }
}

/**
    Initialize MPP mode
    */
Void MppMode::InitModeSpecific () {
    SetMppSuffixMapping ();
    InitBuffers ();
}

/**
    Get index based on compressed bitrate

    @return BPP index
    */
Int MppMode::GetBppIndex () {
    Int shift = 4;
    Int minBpp = 64;
    Int offset = (1 << shift) - 1;
    return Clip3<Int> (0, 7, (m_bpp - minBpp + offset) >> shift);
}

/**
    Update QP
    */
Void MppMode::UpdateQp () {
    Int minQp = GetMinQp (m_pps);
    Int offset, maxQp;
    Int bfIndex = m_rateControl->GetRcFullness () >> 13;
    switch (bfIndex) {
        case 7:
            // buffer fullness >= 87.5%
            maxQp = g_rc_mppMaxQp[m_chromaFormat][GetBppIndex ()];
            offset = 16;
            break;
        case 6:
            // buffer fullness in [75%, 87.5%)
            maxQp = g_rc_mppMaxQp[m_chromaFormat][GetBppIndex ()] - 8;
            offset = 8;
            break;
        default:
            // buffer fullness < 75%
            maxQp = m_rateControl->GetMaxQp ();
            offset = 0;
            break;
    }
    Int mppQp = Clip3<Int> (minQp, maxQp, m_rateControl->GetQp () + offset);
    Bool a = (m_chromaFormat == k444 && m_pps->GetBpp () > 192);
    Bool b = (m_chromaFormat == k420 || m_chromaFormat == k422);
    if (m_underflowPreventionMode && (a || b)) {
        mppQp = std::min<Int> (mppQp, m_pps->GetFlatnessQpVeryFlat (true));
    }
    m_qp = mppQp;
}

Void MppMode::Test () {
    Int modeDistortionRgb = 0;
    Int modeDistortionYCbCr = 0;
    Int modeDistortionYCoCg = 0;

    UInt ssmBitCounterA[4] = { 0 };
    UInt ssmBitCounterB[4] = { 0 };

    Int modeHeader = 0;
    modeHeader += EncodeModeHeader (false);
    modeHeader += EncodeFlatness (false);

    Bool testMultipleCsc = (m_origSrcCsc != kYCbCr);

    //* number of trials !//
    Int numTrials = 1;
    if (testMultipleCsc) {
        modeHeader += EncodeMPPCSpace (false);
        numTrials += 1;
    }
    modeHeader += EncodeStepSize (false);
    ssmBitCounterA[0] += modeHeader;
    ssmBitCounterB[0] += modeHeader;

    for (Int cscTrial = 0; cscTrial < numTrials; cscTrial++) {
        Bool isTrialValid = true;
        UInt* ssmBitCounter = NULL;
        //! determine colorspace for current trial !//
        if (testMultipleCsc) {
            m_csc = (cscTrial == 0 ? kRgb : kYCoCg);
            ssmBitCounter = (cscTrial == 0 ? ssmBitCounterA : ssmBitCounterB);
        }
        else {
            m_csc = m_origSrcCsc;
            ssmBitCounter = ssmBitCounterA;
        }

        //! initialize current block and midpoint values !//
        InitCurBlock ();
        ComputeMidpointUnified ();

        //! MPP !//
        for (UInt k = 0; k < g_numComps; k++) {
            Int curStepSize = EncMapStepSize (k);
            Int curBias = curStepSize == 0 ? 0 : (1 << (curStepSize - 1));
            Int bitDepth = (GetCsc () == kYCoCg && k > 0 ? m_bitDepth + 1 : m_bitDepth);
            Int clipMax = (1 << m_bitDepth) - 1;
            Int clipMin = (GetCsc () == kYCoCg && k > 0) ? -(1 << m_bitDepth) : 0;
            Int* pCur = GetCurBlkBuf (k);
            Int* pRes = (cscTrial == 0 ? m_resBlkBufA[k] : m_resBlkBufB[k]);
            Int* pRec = (cscTrial == 0 ? m_recBlkBufA[k] : m_recBlkBufB[k]);
            Int* pQuant = (cscTrial == 0 ? m_quantBlkBufA[k] : m_quantBlkBufB[k]);
            Int a = bitDepth - curStepSize - 1;
            Int minCode = -(1 << a);
            Int maxCode = (1 << a) - 1;
            Int curSample = 0;
            Int curSampleError = 0;
            UInt pxIdx = 0;
            Bool allowErrorDiffusion = (bitDepth - curStepSize <= g_mppErrorDiffusionThreshold);

            UInt numSubBlocks = 4;
            for (UInt subBlock = 0; subBlock < numSubBlocks; subBlock++) {
                Int mp = GetMidpoint (k, subBlock);
                Int error[2][2] = { 0 };
                UInt xOffset;
                if ((m_chromaFormat == k422 || m_chromaFormat == k420) && k > 0) {
                    xOffset = subBlock;
                }
                else {
                    xOffset = (subBlock << 1);
                }

                //! sub-block sample A !//
                pxIdx = xOffset;
                curSample = pCur[pxIdx];
                pRes[pxIdx] = curSample - mp;
                pQuant[pxIdx] = Clip3<Int> (minCode, maxCode, (pRes[pxIdx] > 0) ? (pRes[pxIdx] + curBias) >> curStepSize : -((curBias - pRes[pxIdx]) >> curStepSize));
                pRec[pxIdx] = Clip3<Int> (clipMin, clipMax, (pQuant[pxIdx] << curStepSize) + mp);

                //! add rate !//
                UInt ssmIdx = m_mppSuffixSsmMapping[k][pxIdx];
                ssmBitCounter[ssmIdx] += bitDepth - curStepSize;

                if (allowErrorDiffusion) {
                    curSampleError = curSample - pRec[pxIdx];
                    error[0][1] += ((curSampleError + 1) >> 1); // diffuse error to B
                    error[1][0] += ((curSampleError + 1) >> 1); // diffuse error to C
                }

                //! sub-block sample B !//
                if (m_chromaFormat == k444 || k == 0) {
                    pxIdx = xOffset + 1;
                    curSample = pCur[pxIdx];
                    pRes[pxIdx] = curSample + error[0][1] - mp;
                    pQuant[pxIdx] = Clip3<Int> (minCode, maxCode, (pRes[pxIdx] > 0) ? (pRes[pxIdx] + curBias) >> curStepSize : -((curBias - pRes[pxIdx]) >> curStepSize));
                    pRec[pxIdx] = Clip3<Int> (clipMin, clipMax, (pQuant[pxIdx] << curStepSize) + mp);

                    //! add rate !//
                    UInt ssmIdx = m_mppSuffixSsmMapping[k][pxIdx];
                    ssmBitCounter[ssmIdx] += bitDepth - curStepSize;

                    if (allowErrorDiffusion) {
                        curSampleError = curSample - pRec[pxIdx];
                        error[1][0] += ((curSampleError + 1) >> 1); // diffuse error to C
                        error[1][1] += ((curSampleError + 1) >> 1); // diffuse error to D
                    }
                }

                //! sub-block sample C !//
                if (m_chromaFormat == k444 || m_chromaFormat == k422 || k == 0) {
                    pxIdx = getWidth (k) + xOffset;
                    curSample = pCur[pxIdx];
                    pRes[pxIdx] = curSample + error[1][0] - mp;
                    pQuant[pxIdx] = Clip3<Int> (minCode, maxCode, (pRes[pxIdx] > 0) ? (pRes[pxIdx] + curBias) >> curStepSize : -((curBias - pRes[pxIdx]) >> curStepSize));
                    pRec[pxIdx] = Clip3<Int> (clipMin, clipMax, (pQuant[pxIdx] << curStepSize) + mp);

                    //! add rate !//
                    UInt ssmIdx = m_mppSuffixSsmMapping[k][pxIdx];
                    ssmBitCounter[ssmIdx] += bitDepth - curStepSize;

                    //! diffuse error
                    if (allowErrorDiffusion) {
                        curSampleError = curSample - pRec[pxIdx];
                        error[1][1] += ((curSampleError + 1) >> 1); // diffuse error to D
                    }
                }

                //! sub-block sample D !//
                if (m_chromaFormat == k444 || k == 0) {
                    pxIdx = getWidth (k) + xOffset + 1;
                    curSample = pCur[pxIdx];
                    pRes[pxIdx] = curSample + error[1][1] - mp;
                    pQuant[pxIdx] = Clip3<Int> (minCode, maxCode, (pRes[pxIdx] > 0) ? (pRes[pxIdx] + curBias) >> curStepSize : -((curBias - pRes[pxIdx]) >> curStepSize));
                    pRec[pxIdx] = Clip3<Int> (clipMin, clipMax, (pQuant[pxIdx] << curStepSize) + mp);

                    //! add rate !//
                    UInt ssmIdx = m_mppSuffixSsmMapping[k][pxIdx];
                    ssmBitCounter[ssmIdx] += bitDepth - curStepSize;
                }
            }
        }

        //! check syntax element size !//
        for (UInt ssm = 0; ssm < m_ssm->GetNumSubStreams (); ssm++) {
            if (ssmBitCounter[ssm] > m_ssm->GetMaxSeSize ()) {
                isTrialValid = false;
            }
        }

        if (m_rateControl->CheckUnderflowPrevention () && m_bpp > 192) {
            Int rate = ssmBitCounter[0] + ssmBitCounter[1] + ssmBitCounter[2] + ssmBitCounter[3];
            if (rate < m_rateControl->GetAvgBlockBits ()) {
                isTrialValid = false;
            }
        }

        //! distortion for current trial !//
        switch (m_csc) {
            case kYCoCg:
                CurBlockYCoCgToRgb ();
                RecTrialYCoCgToRgb ();
                m_csc = kRgb;
                modeDistortionYCoCg = isTrialValid ? ComputeCurTrialDistortion (2) : LARGE_INT;
                break;
            case kRgb:
                modeDistortionRgb = isTrialValid ? ComputeCurTrialDistortion (0) : LARGE_INT;
                break;
            case kYCbCr:
                modeDistortionYCbCr = isTrialValid ? ComputeCurTrialDistortion (cscTrial) : LARGE_INT;
                break;
        }
    }

    //! determine colorspace !//
    UInt modeRate;
    UInt modeDistortion;
    UInt bufferIdx = 0;
    if (testMultipleCsc) {
        if (modeDistortionRgb <= modeDistortionYCoCg) {
            modeRate = ssmBitCounterA[0] + ssmBitCounterA[1] + ssmBitCounterA[2] + ssmBitCounterA[3];
            modeDistortion = modeDistortionRgb;
            m_csc = kRgb;
        }
        else {
            modeRate = ssmBitCounterB[0] + ssmBitCounterB[1] + ssmBitCounterB[2] + ssmBitCounterB[3];
            modeDistortion = modeDistortionYCoCg;
            m_csc = kYCoCg;
            bufferIdx = 1;
        }
    }
    else {
        modeRate = ssmBitCounterA[0] + ssmBitCounterA[1] + ssmBitCounterA[2] + ssmBitCounterA[3];
        switch (m_csc) {
            case kRgb:
                modeDistortion = modeDistortionRgb;
                break;
            case kYCbCr:
                modeDistortion = modeDistortionYCbCr;
                break;
        }
    }

    //! copy buffers !//
    if (m_csc == kRgb) {
        RecTrialRgbToYCoCg ();
    }
    for (UInt k = 0; k < g_numComps; k++) {
        switch (bufferIdx) {
            case 0:
                if (m_csc == kRgb) {
                    memcpy (GetRecBlkBuf (k), m_recBlkBufC[k], getWidth (k) * getHeight (k) * sizeof (Int));
                }
                else {
                    memcpy (GetRecBlkBuf (k), m_recBlkBufA[k], getWidth (k) * getHeight (k) * sizeof (Int));
                }
                memcpy (GetResBlkBuf (k), m_resBlkBufA[k], getWidth (k) * getHeight (k) * sizeof (Int));
                memcpy (GetQuantBlkBuf (k), m_quantBlkBufA[k], getWidth (k) * getHeight (k) * sizeof (Int));
                break;
            case 1:
                memcpy (GetRecBlkBuf (k), m_recBlkBufB[k], getWidth (k) * getHeight (k) * sizeof (Int));
                memcpy (GetResBlkBuf (k), m_resBlkBufB[k], getWidth (k) * getHeight (k) * sizeof (Int));
                memcpy (GetQuantBlkBuf (k), m_quantBlkBufB[k], getWidth (k) * getHeight (k) * sizeof (Int));
                break;
        }
    }

    //! finish up !//
    SetRate (modeRate);
    SetDistortion (modeDistortion);
    ComputeRdCost ();
}

/**
    Encode MPP block
    */
Void MppMode::Encode () {
    m_ssm->SetCurSyntaxElementMode (std::string ("MPP"));
    EncodeModeHeader (true);
    EncodeFlatness (true);

    Bool testMultipleCsc = (m_origSrcCsc != kYCbCr);

    if (testMultipleCsc) {
        EncodeMPPCSpace (true);
    }

    EncodeStepSize (true);
    for (Int k = 0; k < g_numComps; k++) {
        Int curStepSize = EncMapStepSize (k);
        Int *ptrQuant = GetQuantBlkBuf (k);
        Int bitDepth = (m_csc == kYCoCg && k > 0 ? m_bitDepth + 1 : m_bitDepth);
        Int nBits = bitDepth - curStepSize;
        Int minCode = -(1 << (nBits - 1));
        for (UInt x = 0; x < getWidth (k) * getHeight (k); x++) {
            Int curSs = m_mppSuffixSsmMapping[k][x];
            assert (curSs >= 0);
            m_ssm->GetCurSyntaxElement (curSs)->AddSyntaxWord ((UInt)(ptrQuant[x] - minCode), nBits);
        }
    }

    //! modeRate !//
    UInt modeRateMpp = 0;
    for (UInt ss = 0; ss < m_ssm->GetNumSubStreams (); ss++) {
        modeRateMpp += m_ssm->GetCurSyntaxElement (ss)->GetNumBits ();
    }
    m_rateWithoutPadding = modeRateMpp;
    m_rate = modeRateMpp;

    //! copy to reconstructed frame buffer !//
    if (m_csc == kYCoCg || m_csc == kRgb) {
        CopyToYCoCgBuffer (m_posX);
    }
    else {
        CopyToRecFrame (m_posX, m_posY);
    }

    //! debug tracer !//
    if (enEncTracer) {
        char* curModeString = "MPP";
        fprintf (encTracer, "MPP: colorSpace = %s\n", g_colorSpaceNames[m_csc].c_str ());
        fprintf (encTracer, "MPP: stepSize = [%d, %d, %d]\n", EncMapStepSize (0), EncMapStepSize (1), EncMapStepSize (2));
        debugTracerWriteArray3 (encTracer, curModeString, "qResBlk", m_blkWidth * m_blkHeight, m_qResBlk, m_chromaFormat);
        debugTracerWriteArray3 (encTracer, curModeString, "RecBlk", m_blkWidth * m_blkHeight, m_recBlk, m_chromaFormat);
    }
}

/**
    Decode MPP block
    */
Void MppMode::Decode () {
    DecodeMppSuffixBits (false);
    ComputeMidpointUnified ();

    //! reconstruct !//
    for (Int k = 0; k < g_numComps; k++) {
        Int bitDepth = (GetCsc () == kYCoCg && k > 0 ? m_bitDepth + 1 : m_bitDepth);
        Int curStepSize = DecMapStepSize (k, false);
        Int bits = bitDepth - curStepSize;
        Int minCode = -(1 << (bits - 1));
        Int clipMax = (1 << m_bitDepth) - 1;
        Int clipMin = (GetCsc () == kYCoCg && k > 0) ? -(1 << m_bitDepth) : 0;
        Int *pRec = GetRecBlkBuf (k);
        Int* pQuant = GetQuantBlkBuf (k);
        for (UInt y = 0; y < getHeight (k); y++) {
            for (UInt x = 0; x < getWidth (k); x++) {
                Int idx = y * getWidth (k) + x;
                Int mp;
                if (m_chromaFormat == k444 || k == 0) {
                    mp = GetMidpoint (k, x >> 1);
                }
                else {
                    mp = GetMidpoint (k, x);
                }
                Int deQuant = pQuant[idx] << curStepSize;
                pRec[idx] = Clip3<Int> (clipMin, clipMax, mp + deQuant);
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
        char* curModeString = "MPP";
        fprintf (decTracer, "MPP: colorSpace = %s\n", g_colorSpaceNames[m_csc].c_str ());
        fprintf (decTracer, "MPP: stepSize = [%d, %d, %d]\n", DecMapStepSize (0, false), DecMapStepSize (1, false), DecMapStepSize (2, false));
        debugTracerWriteArray3 (decTracer, curModeString, "qResBlk", m_blkWidth * m_blkHeight, m_qResBlk, m_chromaFormat);
        debugTracerWriteArray3 (decTracer, curModeString, "RecBlk", m_blkWidth * m_blkHeight, m_recBlk, m_chromaFormat);
    }
}

/**
    Get step size for specified component

    @param k component index
    @return step size
    */
Int MppMode::GetStepSize (Int k) {
    Int curCompBitDepth = (m_csc == kYCoCg && k > 0 ? m_bitDepth + 1 : m_bitDepth);
    Int ssmMaxSeSize = m_pps->GetSsmMaxSeSize ();

    Int minStepSize = 0;
    if (m_pps->GetVersionMinor () >= 2) {
        minStepSize = m_pps->GetMppMinStepSize ();
    }
    else {
        switch (m_chromaFormat) {
            case k444:
                switch (m_bitDepth) {
                    case 8:
                        minStepSize = 0;
                        break;
                    case 10:
                        minStepSize = (ssmMaxSeSize == 128 ? 1 : 0);
                        break;
                    case 12:
                        minStepSize = (ssmMaxSeSize == 128 ? 3 : 1);
                        break;
                }
                break;
            case k422:
            case k420:
                minStepSize = 0;
                break;
        }
    }

    Int maxStepSize = curCompBitDepth - 1;
    Int qpOffsetRgb = 0;
    Int qpOffsetYCbCr = 0;
    Int curStepSize;
    Int curQp = m_qp;

    Int curStepSizeY;
    switch (m_csc) {
        case kRgb:
            curStepSize = ((curQp - g_rc_qpStepSizeOne) + qpOffsetRgb) >> 3;
            curStepSize += (m_bitDepth - g_minBpc);
            break;
        case kYCbCr:
            curStepSize = ((curQp - g_rc_qpStepSizeOne) + qpOffsetYCbCr) >> 3;
            curStepSize += (m_bitDepth - g_minBpc);
            break;
        case kYCoCg:
            curStepSizeY = ((curQp - g_rc_qpStepSizeOne) + qpOffsetRgb) >> 3;
            curStepSizeY += (m_bitDepth - g_minBpc);
            switch (k) {
                case 0:
                    curStepSize = curStepSizeY;
                    break;
                case 1:
                    curStepSize = g_mppStepSizeMapCo[curStepSizeY];
                    break;
                case 2:
                    curStepSize = g_mppStepSizeMapCg[curStepSizeY];
                    break;
            }
            break;
    }
    return Clip3<Int> (minStepSize, maxStepSize, curStepSize);
}

/**
Compute MPP midpoints
*/

Void MppMode::ComputeMidpointUnified () {
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

            //! convert mean from CSC->YCoCg !//
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
            Int curStepSize = m_isEncoder ? EncMapStepSize (k) : DecMapStepSize (k, false);
            Int curBias = curStepSize == 0 ? 0 : 1 << (curStepSize - 1);
            Int maxClip = std::min<Int> ((1 << m_bitDepth) - 1, middle + 2 * curBias);
            Int mp = Clip3<Int> (middle, maxClip, mean[k] + (2 * curBias));
            SetMidpoint (k, sb, mp);
        }
    }
}

/**
    Swap MPP step size from next block to current block (for decoder)
    */
Void MppMode::NextBlockStepSizeSwap () {
    m_curBlockStepSize = m_nextBlockStepSize;
}

/**
    Encode step size to bitstream

    @param isFinal flag to indicate final encode
    @return number of bits used to encoder
    */
UInt MppMode::EncodeStepSize (Bool isFinal) {
    UInt numBits = (m_bitDepth > 8 ? 4 : 3);
    if (isFinal) {
        UInt ssmIdx = 0;
        m_ssm->GetCurSyntaxElement (ssmIdx)->AddSyntaxWord (GetStepSize (0), numBits);
    }
    return numBits;
}

/**
    Parse MPP step size
    */
Void MppMode::ParseStepSize () {
    UInt ssmIdx = 0;
    UInt numBits = (m_bitDepth > 8 ? 4 : 3);
    m_nextBlockStepSize = m_ssm->ReadFromFunnelShifter (ssmIdx, numBits);
}

/**
    Map luma step size to component step size (for encoder)

    @param k component index
    @return step size
    */
Int MppMode::EncMapStepSize (Int k) {
    Int stepSize = 0;
    switch (m_csc) {
        case kRgb:
        case kYCbCr:
            stepSize = GetStepSize (0);
            break;
        case kYCoCg:
            switch (k) {
                case 0:
                    stepSize = GetStepSize (0);
                    break;
                case 1:
                    stepSize = g_mppStepSizeMapCo[GetStepSize (0)];
                    break;
                case 2:
                    stepSize = g_mppStepSizeMapCg[GetStepSize (0)];
                    break;
            }
            break;
    }
    return stepSize;
}

/**
    Map luma step size to component step size (for decoder)

    @param k component index
    @param isSsm0 flag to indicate substream 0
    @return step size
    */
Int MppMode::DecMapStepSize (Int k, Bool isSsm0) {
    Int stepSize = 0;
    ColorSpace curSuffixCsc = (isSsm0 ? m_nextBlockCsc : m_csc);
    switch (curSuffixCsc) {
        case kRgb:
        case kYCbCr:
            stepSize = isSsm0 ? m_nextBlockStepSize : m_curBlockStepSize;
            break;
        case kYCoCg:
            switch (k) {
                case 0:
                    stepSize = isSsm0 ? m_nextBlockStepSize : m_curBlockStepSize;
                    break;
                case 1:
                    stepSize = g_mppStepSizeMapCo[isSsm0 ? m_nextBlockStepSize : m_curBlockStepSize];
                    break;
                case 2:
                    stepSize = g_mppStepSizeMapCg[isSsm0 ? m_nextBlockStepSize : m_curBlockStepSize];
                    break;
            }
            break;
    }
    return stepSize;
}

/**
    Copy next block quantized residuals to current block (for decoder)
    */
Void MppMode::CopyCachedSuffixBits () {
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
    Allocate storage for next block quantized residuals (for decoder)
    */
Void MppMode::InitNextBlockCache () {
    Int numPx = (m_blkWidth * m_blkHeight);
    for (UInt c = 0; c < g_numComps; c++) {
        m_nextBlockQuantRes[c] = new Int[numPx];
        memset (m_nextBlockQuantRes[c], 0, numPx * sizeof (Int));
    }
}

/**
    Clear storage for next block quantized residuals (for decoder)
    */
Void MppMode::ClearNextBlockCache () {
    for (UInt c = 0; c < g_numComps; c++) {
        if (m_nextBlockQuantRes[c]) { delete[] m_nextBlockQuantRes[c];  m_nextBlockQuantRes[c] = NULL; }
    }
}

/**
    Load quantized residual substream mapping
    */
Void MppMode::SetMppSuffixMapping () {
    for (UInt k = 0; k < g_numComps; k++) {
        UInt numPx = getWidth (k) * getHeight (k);
        m_mppSuffixSsmMapping[k] = new Int[numPx]; // (Int*)xMalloc (Int, numPx);
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
    Clear quantized residual substream mapping
    */
Void MppMode::ClearMppSuffixMapping () {
    for (UInt k = 0; k < g_numComps; k++) {
        if (m_mppSuffixSsmMapping[k]) {
            delete[] m_mppSuffixSsmMapping[k];
            m_mppSuffixSsmMapping[k] = NULL;
        }
    }
}

/**
    Swap MPP colorspace flag from next block (for decoder)
    */
Void MppMode::CscBitSwap () {
    m_csc = m_nextBlockCsc;
}

/**
    Parse MPP colorspace
    */
Void MppMode::ParseCsc () {
    if (m_origSrcCsc == kYCbCr) {
        m_csc = kYCbCr;
        m_nextBlockCsc = kYCbCr;
        return;
    }
    UInt mppDecCsc = (UInt)m_ssm->ReadFromFunnelShifter (0, 1);
    m_nextBlockCsc = (mppDecCsc == 0 ? kRgb : kYCoCg);
}

/**
    Parse MPP quantized residuals

    @param isSsmIdx0 flag to indicate suffix bits are from substream 0
    */
Void MppMode::DecodeMppSuffixBits (Bool isSsmIdx0) {
    // read quantized residual from bitstream
    ColorSpace curSuffixCsc = (isSsmIdx0 ? m_nextBlockCsc : m_csc);
    for (UInt k = 0; k < g_numComps; k++) {
        Int bitDepth = (curSuffixCsc == kYCoCg && k > 0 ? m_bitDepth + 1 : m_bitDepth);
        Int *pQuant = GetQuantBlkBuf (k);
        Int *pNextBlockQuant = m_nextBlockQuantRes[k];
        UInt numPx = getWidth (k) * getHeight (k);
        if (isSsmIdx0) {
            Int curStepSize = DecMapStepSize (k, true);
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
            Int curStepSize = DecMapStepSize (k, false);
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

*/
Int MppMode::ComputeCurTrialDistortion (Int trial) {
    Int distortion = 0;
    for (Int k = 0; k < g_numComps; k++) {
        Int numPx = getWidth (k) * getHeight (k);
        Int compDistortion = 0;
        Int* pOrg = GetCurBlkBuf (k);
        Int* pRec;
        switch (trial) {
            case 0:
                pRec = m_recBlkBufA[k];
                break;
            case 1:
                pRec = m_recBlkBufB[k];
                break;
            case 2:
                pRec = m_recBlkBufC[k];
                break;
        }

        for (Int i = 0; i < numPx; i++) {
            compDistortion += abs (pOrg[i] - pRec[i]);
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

Void MppMode::RecTrialRgbToYCoCg () {
    assert (m_chromaFormat == k444 && m_csc != kYCbCr);
    Int *recBufSrc_r = m_recBlkBufA[0];
    Int *recBufSrc_g = m_recBlkBufA[1];
    Int *recBufSrc_b = m_recBlkBufA[2];
    Int *recBufDst_r = m_recBlkBufC[0];
    Int *recBufDst_g = m_recBlkBufC[1];
    Int *recBufDst_b = m_recBlkBufC[2];
    for (UInt x = 0; x < (getWidth (0) * getHeight (0)); x++) {
        Int Co = recBufSrc_r[x] - recBufSrc_b[x];
        Int tmp = recBufSrc_b[x] + (Co >> 1);
        Int Cg = recBufSrc_g[x] - tmp;
        recBufDst_r[x] = tmp + (Cg >> 1); // Y
        recBufDst_g[x] = Co; // Co
        recBufDst_b[x] = Cg; // Cg
    }
}

Void MppMode::RecTrialYCoCgToRgb () {
    assert (m_chromaFormat == k444 && m_csc != kYCbCr);
    Int *recBufSrc_r = m_recBlkBufB[0];
    Int *recBufSrc_g = m_recBlkBufB[1];
    Int *recBufSrc_b = m_recBlkBufB[2];
    Int *recBufDst_r = m_recBlkBufC[0];
    Int *recBufDst_g = m_recBlkBufC[1];
    Int *recBufDst_b = m_recBlkBufC[2];
    for (UInt x = 0; x < (getWidth (0) * getHeight (0)); x++) {
        Int temp = recBufSrc_r[x] - (recBufSrc_b[x] >> 1);
        Int G = recBufSrc_b[x] + temp;
        Int B = temp - (recBufSrc_g[x] >> 1);
        Int R = recBufSrc_g[x] + B;
        recBufDst_r[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, R);
        recBufDst_g[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, G);
        recBufDst_b[x] = Clip3<Int> (0, (1 << m_bitDepth) - 1, B);
    }
}
