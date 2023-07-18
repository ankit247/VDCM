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

/* Copyright (c) 2015-2017 MediaTek Inc., MPP mode is modified and/or implemented by MediaTek Inc. based on Mode.h
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
\file       Mode.h
\brief      Mode class (header)
Parent class for individual modes
*/

#ifndef __MODE__
#define __MODE__

#include <assert.h>
#include "TypeDef.h"
#include "Block.h"
#include "RateControl.h"
#include "EntropyCoder.h"
#include "Ssm.h"
#include "Pps.h"

class Mode
{
protected:
    Bool m_isValid;
    Bool m_isFls;
    Bool m_isPartial;
    Bool m_isEncoder;
    Bool m_isNextBlockFls;
    Int m_sliceStartY;
    Int m_sliceStartX;
    ModeType m_curBlockMode;
    Int m_rate;
    Int m_bitDepth;
    Int m_blkStride;
    Int m_frameStride;
    Int m_distortion;
    Int m_rdCost;
    Int m_lambdaBuffer;
    Int m_maxBits;
    Int m_posX;
    Int m_posY;
    Int m_blkWidth;
    Int m_blkHeight;
    Int m_minBlkWidth;
    Int m_srcWidth;
    Int m_srcHeight;
    FlatnessType m_flatnessType;
    Int m_neighborsAboveLen;
    Int m_neighborsLeftLen;
    Int m_neighborsAboveLeftLen;
    Block* m_block;
    Int m_bpp;
    SubStreamMux* m_ssm;
    Int m_qp;
    ColorSpace m_csc;
    ColorSpace m_origSrcCsc;
    Int m_prevBlockMode;
    EntropyCoder* m_ec;
    VideoFrame *m_pRecVideoFrame;
	VideoFrame *m_recBufferYCoCg;
    RateControl *m_rateControl;
    ChromaFormat m_chromaFormat;
    Int* m_curBlk[g_numComps];
    Int* m_predBlk[g_numComps];
    Int* m_resBlk[g_numComps];
    Int* m_qResBlk[g_numComps];
    Int* m_deQuantResBlk[g_numComps];
    Int* m_recBlk[g_numComps];
    Int* m_neighborsAbove[g_numComps];
    Int* m_neighborsLeft[g_numComps];
    Int* m_neighborsLeftAbove[g_numComps];
    Int m_rateWithoutPadding;
    Pps* m_pps;
    Bool m_underflowPreventionMode;

public:
    Mode (ModeType cudMode, ColorSpace inputCsc, ColorSpace csc, Int bpp, Int bitDepth, Bool isEncoder, ChromaFormat chromaFormat);
    virtual ~Mode ();

    // abstract virtual routines (must be overloaded by derived class)
    virtual Void Test () = 0;
    virtual Void Encode () = 0;
    virtual Void Decode () = 0;
    virtual Void UpdateQp () = 0;

    // virtual routines
    virtual Void Reset () {}
    virtual Void InitModeSpecific () {}
    virtual Void FetchDataFromReference () {}

    // methods
    Void Init (UInt sliceY, UInt sliceX, Block* block, VideoFrame* videoFrame, RateControl* rc, EntropyCoder* entropyCoder, SubStreamMux* ssm, Pps* pps);
    Void InitDecoder (UInt sliceY, UInt sliceX, VideoFrame* videoFrame, RateControl* rc, EntropyCoder* entropyCoder, Int blkWidth, Int blkHeight, SubStreamMux* ssm, Pps* pps);
    Int DecodeFixedLengthCodeSsm (UInt numBits, UInt ssmIdx);
    UInt ComputeBitrateLambda (UInt bits, UInt maxBits);
    Void InitCurBlock ();
    Void InitNeighbors ();
    Int EncodeModeHeader (Bool isFinal);
    Int EncodeFlatness (Bool isFinal);
    Int EncodeMPPCSpace (Bool isFinal);
    Void SetupBufferMemory ();
    Int CalcSad (Int* blk0, Int* blk1, Int length);
    Void ResetBuffers ();
    Void CurBlockYCoCgToRgb ();
    Void CurBlockRgbToYCoCg ();
    Void RecBlockYCoCgToRgb ();
    Void NeighborsRgbToYCoCg ();
    Int ComputeCurBlockDistortion ();
    Int ComputeRdCost ();
    Int ComputeRdCost (Int rate, Int distortion);
    Void CopyToRecFrame (Int startCol, Int startRow);
    Bool CheckMaxSeSize (UInt *perSsmBits);
    Void CopyRecPxToFrame (Int row, Int col);
	Void CopyToYCoCgBuffer (Int col);
	Void RecBlockRgbToYCoCg ();
    Void Update (Int x, Int y, Bool isFls, ModeType prevBlockMode, FlatnessType flatnessType, Int maxBits, Int lambdaBuffer);
    Void Update (Int x, Int y, Bool isFls, ModeType prevBlockMode);
    Void SetNeighborsLen (Int left, Int above, Int leftAbove) {
        m_neighborsAboveLen = above;
        m_neighborsAboveLeftLen = leftAbove;
        m_neighborsLeftLen = left;
    }

    // setters
	Void SetRecBufferYCoCg (VideoFrame *buffer) { m_recBufferYCoCg = buffer; }
    Void SetRate (Int rate) { m_rate = rate; }
    Void SetDistortion (Int distortion) { m_distortion = distortion; }
    Void SetPos (Int x, Int y) { m_posX = x; m_posY = y; }
    Void SetBlkSize (Int width, Int height) { m_blkWidth = width; m_blkHeight = height; }
    Void SetSrcSize (Int width, Int height) { m_srcWidth = width; m_srcHeight = height; }
    Void SetIsFls (Bool isFls) { m_isFls = isFls; }
    Void SetBlock (Block* block) { m_block = block; }
    Void SetQp (Int qp) { m_qp = qp; }
    Void SetPrevBlockMode (ModeType mode) { m_prevBlockMode = mode; }
    Void SetFlatnessType (FlatnessType type) { m_flatnessType = type; }
    Void SetVideoFrame (VideoFrame* fr) { m_pRecVideoFrame = fr; }
    Void SetRateControl (RateControl* rc) { m_rateControl = rc; }
    Void SetMaxBits (Int maxBits) { m_maxBits = maxBits; }
    Void SetLambdaBuffer (Int lambda) { m_lambdaBuffer = lambda; }
    Void SetUnderflowMode (Bool enableUnderflowMode) { m_underflowPreventionMode = enableUnderflowMode; }
    Void SetEntropyCoder (EntropyCoder *ec) { m_ec = ec; }
    Void SetIsNextBlockFls (Bool is) { m_isNextBlockFls = is; }

    // getters
    Int GetCurCompQp (Int masterQp, Int k);
	Int GetCurCompQpMod (Int masterQp, Int k);
    Int* GetCurBlkBuf (Int comp) { return m_curBlk[comp]; }
    Int* GetPredBlkBuf (Int comp) { return m_predBlk[comp]; }
    Int* GetResBlkBuf (Int comp) { return m_resBlk[comp]; }
    Int* GetRecBlkBuf (Int comp) { return m_recBlk[comp]; }
    Int* GetQuantBlkBuf (Int comp) { return m_qResBlk[comp]; }
    Int* GetDeQuantBlkBuf (Int comp) { return m_deQuantResBlk[comp]; }
    Int* GetNeighborsAbove (Int comp) { return m_neighborsAbove[comp]; }
    Int* GetNeighborsLeft (Int comp) { return m_neighborsLeft[comp]; }
    Int* GetNeighborsLeftAbove (Int comp) { return m_neighborsLeftAbove[comp]; }
    UInt getWidth (const Int compID) { return m_blkWidth >> g_compScaleX[compID][m_chromaFormat]; }
    UInt getHeight (const Int compID) { return m_blkHeight >> g_compScaleY[compID][m_chromaFormat]; }
    UInt getSrcWidth (const Int compID) { return m_srcWidth >> g_compScaleX[compID][m_chromaFormat]; }
    UInt getSrcHeight (const Int compID) { return m_srcHeight >> g_compScaleY[compID][m_chromaFormat]; }
    UInt getPosX (const Int compID) { return m_posX >> g_compScaleX[compID][m_chromaFormat]; }
    UInt getPosY (const Int compID) { return m_posY >> g_compScaleY[compID][m_chromaFormat]; }
    Int GetRate () { return m_rate; }
    Int GetDistortion () { return m_distortion; }
    Int GetRdCost () { return m_rdCost; }
    Int GetQP () { return m_qp; }
    Bool GetIsFls () { return m_isFls; }
    ChromaFormat GetChromaFormat () { return m_chromaFormat; }
    ColorSpace GetCsc () { return m_csc; }
    Int GetRateWithoutPadding () { return m_rateWithoutPadding; }
};

#endif