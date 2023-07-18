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

/*
* The copyright in this software is being made available under the BSD
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

/**
\file       TransMode.h
\brief      Transform Mode (header)
*/

#ifndef __TRANSMODE__
#define __TRANSMODE__

#include "TypeDef.h"
#include "Mode.h"
#include "EntropyCoder.h"
#include "IntraPrediction.h"

class TransMode : public Mode {
protected:
    IntraPredictorType  m_bestIntraPredIdx;             // best Intra predictor index
    Int m_transBitDepth;                                // bit depth used in transform mode to account for intra prediction and colorspace conversion
    Int m_minBlkWidth;                                  // minimum transform size
    Int m_maxTrDynamicRange;                            // maximum dynamic range for transform mode
    IntraPred* m_intraPred;                             // object to IntraPrediction class
    IntraPredictorType m_nextBlockBestIntraPredIdx;     // next block best intra predictor (for decoder)
    Bool m_nextBlockIsFls;                              // is next block FLS (for decoder)
	Int*** m_intraBufferPred;
	Int*** m_intraBufferRes;
	Int*** m_intraBufferDctCoeff;
	Int*** m_intraBufferQuant;
	Int*** m_intraBufferDeQuant;
	Int*** m_intraBufferRec;

public:
    TransMode (Int bpp, Int bitDepth, Bool isEncoder, ChromaFormat chromaFormat, ColorSpace csc, ColorSpace targetCsc);
    ~TransMode ();

    // virtual overloads
    Void Test ();
    Void Encode ();
    Void Decode ();
    Void UpdateQp ();
    Void InitModeSpecific ();

    // methods
    Void CalculateResidue ();
    Void CalculateResidue (IntraPredictorType predictor);
    Void Reconstruct ();
    Void EncodeBestIntraPredictor ();
    Void DecodeBestIntraPredictor ();
    Void AddPredValue (Int* pRec, Int* pPred, UInt width, UInt height, UInt k);
    Void NextBlockIntraSwap ();
    Void SetNextBlockIsFls (Bool is) { m_nextBlockIsFls = is; }
    Void DctLlmForward (Int* pBlk, Int* pCoeff, UInt width, UInt height);
    Void DctLlmInverse (Int* pCoeff, Int* pBlk, UInt width, UInt height);
    Void QuantLlm (Int* pSrc, Int* pDes, UInt width, UInt height, UInt k);
    Void DeQuantLlm (Int* pSrc, Int* pDes, UInt width, UInt height, UInt k);
    Void dct8_II_fw_LLM_int (Int *x);
    Void dct8_II_bw_LLM_int (Int *x);
    Void dct4_II_fw_LLM_int (Int *x);
    Void dct4_II_bw_LLM_int (Int *x);
	Int* GetIntraBufferPred (UInt idx, UInt k) { return m_intraBufferPred[idx][k]; }
	Int* GetIntraBufferRes (UInt idx, UInt k) { return m_intraBufferRes[idx][k]; }
	Int* GetIntraBufferDctCoeff (UInt idx, UInt k) { return m_intraBufferDctCoeff[idx][k]; }
	Int* GetIntraBufferQuant (UInt idx, UInt k) { return m_intraBufferQuant[idx][k]; }
	Int* GetIntraBufferDeQuant (UInt idx, UInt k) { return m_intraBufferDeQuant[idx][k]; }
	Int* GetIntraBufferRec (UInt idx, UInt k) { return m_intraBufferRec[idx][k]; }
};

#endif // __TRANSMODE__