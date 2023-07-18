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

/* Copyright (c) 2015-2017 MediaTek Inc., entropy coding is modified and/or implemented by MediaTek Inc. based on EntropyCoder.h
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
\file       EntropyCoder.h
\brief      Entropy Encoder and Decoder (header)
*/

#ifndef __ENTROPYCODER__
#define __ENTROPYCODER__

#include <string.h>
#include "TypeDef.h"
#include "Ssm.h"

class EntropyCoder
{
protected:
    EcType m_modeType;
    UInt m_blkWidth;
    UInt m_blkHeight;
    UInt m_numPx;
    ChromaFormat m_chromaFormat;
    Int* m_bitsPerGroup;
    Int* m_bitsPerComp;
    Bool* m_signBitValid[g_numComps];
    Int* m_signBit[g_numComps];
    Int m_numEcGroups;
    Int m_posX;
    Int m_posY;
    Bool m_underflowPreventionMode;
    Int m_rcStuffingBits;
    Int* m_indexMappingTransform[g_numComps];
    Int* m_indexMappingBp[g_numComps];
    Int m_compNumSignBits[g_numComps];

public:
    EntropyCoder (Int width, Int height, ChromaFormat chroma);
    ~EntropyCoder ();

    //! Encoder !//
    Int EncodeResiduals (Int* src, Int k, Bool isFinal, SubStreamMux* ssm);
    Int EncodeTransformCoefficients (Int* src, Int k, Bool isFinal, SubStreamMux* ssm);
    Void EncodeAllGroups (Int* coeff, Int k, SubStreamMux* ssm, Bool isFinal);
    Void EncodeOneGroup (Int* src, Int ecgIdx, Int ecgStart, Int ecgEnd, Int k, SubStreamMux* ssm, Bool isFinal);
    Bool EncodePrefix (Int bitsReq, Int k, SubStreamMux* ssm, Bool isFinal);
    Void EncodeVecEcSymbol2C (SubStreamMux* ssm, Int symbol, Int k, Int bitsReq, Bool isFinal);
    Void EncodeVecEcSymbolSM (SubStreamMux* ssm, Int symbol, Int k, Int bitsReq, Bool isFinal);
    Void UnaryCoding (UInt value, Int k, SubStreamMux* ssm, Bool isFinal);
    UInt GetCodeWordFromBitsReq (Int bitsReq, Int k);
    Void CountNumSigCoeff (Int* coef, UInt& uiNumofCoeff, Int k);

    //! Decoder !//
    Void DecodeResiduals (Int* coeff, Int k, SubStreamMux* ssm);
    Void DecodeTransformCoefficients (Int* coeff, Int k, SubStreamMux* ssm);
    Void DecodeAllGroups (Int* coeff, Int k, SubStreamMux* ssm);
    Void DecodeOneGroup (Int* src, Int ecgIdx, Int ecgStart, Int ecgEnd, Int k, SubStreamMux* ssm);
    UInt DecodePrefix (Int k, UInt ssmIdx, SubStreamMux* ssm);
    Void DecodeVecEcSymbol2C (SubStreamMux* ssm, Int k, Int bitsReq, Int &symbol, Int &mask);
    Void DecodeVecEcSymbolSM (SubStreamMux* ssm, Int k, Int bitsReq, Int &symbol, Int &mask);
    UInt GetBitsReqFromCodeWord (UInt codeWord, Int k);

    //! VEC !//
    Int VecCodeSamplesToSymbol2C (Int* src, Int startInd, Int endInd, Int bitsReq);
    Int VecCodeSamplesToSymbolSM (Int* src, Int startInd, Int endInd, Int bitsReq);
    Void VecCodeSymbolToSamples2C (Int* src, Int startInd, Int endInd, Int bitsReq, Int symbol);
    Void VecCodeSymbolToSamplesSM (Int* src, Int startInd, Int endInd, Int bitsReq, Int symbol);

    //! 2C representation !//
    Int FindResidualSize2C (Int x);
    Int CalculateGroupSize2C (Int* src, Int startInd, Int endInd);

    //! SM representation !//
    Int FindResidualSizeSM (Int x);
    Int CalculateGroupSizeSM (Int* src, Int startInd, Int endInd);
    Void ResetSignBits (UInt k);
    Void EncodeSignBits (UInt k);
    Void ParseSignBits (UInt k);
    
    //! setters !//
    Void SetModeType (EcType x) { m_modeType = x; }
    Void SetPos (Int x, Int y) { m_posX = x, m_posY = y; }
    Void SetUnderflowPreventionMode (Bool x) { m_underflowPreventionMode = x; }
    Void SetRcStuffingBits (Int x) { m_rcStuffingBits = x; }

    //! getters !//
    Int GetBitsPerGroup (Int group) { return m_bitsPerGroup[group]; }
    Int GetWidth (const Int k) { return m_blkWidth >> g_compScaleX[k][m_chromaFormat]; }
    Int GetHeight (const Int k) { return m_blkHeight >> g_compScaleY[k][m_chromaFormat]; }
    UInt GetCompNumSamples (const Int k) { return GetWidth (k) * GetHeight (k); }
    Void GetEcgInfo (UInt ecgOrderIdx, UInt k, Bool isCompSkip, UInt lastSigPos, Bool &ecgDataActive, Int &curEcgStart, Int &curEcgEnd);
};

#endif // __ENTROPYCODER__