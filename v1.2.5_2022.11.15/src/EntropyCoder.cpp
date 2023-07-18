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

/* Copyright (c) 2015-2017 MediaTek Inc., entropy coding is modified and/or implemented by MediaTek Inc. based on EntropyCoder.cpp
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
\file       EntropyCoder.cpp
\brief      Entropy Coder
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <memory.h>
#include "EntropyCoder.h"
#include "Ssm.h"
#include <cstring>

extern Bool enEncTracer;
extern Bool enDecTracer;
extern FILE* encTracer;
extern FILE* decTracer;

/**
    Constructor

    @param width block width
    @param height block height
    @param chroma chroma-sampling format
    */
EntropyCoder::EntropyCoder (Int width, Int height, ChromaFormat chroma)
    : m_blkWidth (width)
    , m_blkHeight (height)
    , m_modeType (kEcTransform)
    , m_chromaFormat (chroma)
    , m_numEcGroups (4)
    , m_underflowPreventionMode (false)
    , m_rcStuffingBits (0) {
    m_numPx = (m_blkWidth * m_blkHeight);
    for (UInt k = 0; k < g_numComps; k++) {
        m_signBitValid[k] = new Bool[GetCompNumSamples (k)];
        m_signBit[k] = new Int[GetCompNumSamples (k)];
        memset (m_signBitValid[k], false, GetCompNumSamples (k) * sizeof (Bool));
        memset (m_signBit[k], 0, GetCompNumSamples (k) * sizeof (Int));
    }
    m_bitsPerGroup = new Int[m_numEcGroups];
    m_bitsPerComp = new Int[g_numComps];
    memset (m_bitsPerGroup, 0, m_numEcGroups * sizeof (Int));

    //! component 0 !//
    m_indexMappingTransform[0] = new Int[16];
    memcpy (m_indexMappingTransform[0], g_ecIndexMapping_Transform_8x2, 16 * sizeof (Int));
    m_indexMappingBp[0] = new Int[16];
    memcpy (m_indexMappingBp[0], g_ecIndexMapping_BP_8x2, 16 * sizeof (Int));

    switch (m_chromaFormat) {
    case k444:
        //! 4:4:4 components 1, 2 !//
        for (UInt k = 1; k < g_numComps; k++) {
            m_indexMappingTransform[k] = new Int[16];
            memcpy (m_indexMappingTransform[k], g_ecIndexMapping_Transform_8x2, 16 * sizeof (Int));
            m_indexMappingBp[k] = new Int[16];
            memcpy (m_indexMappingBp[k], g_ecIndexMapping_BP_8x2, 16 * sizeof (Int));
        }
        break;
    case k422:
        //! 4:2:2 chroma components !//
        for (UInt k = 1; k < g_numComps; k++) {
            m_indexMappingTransform[k] = new Int[8];
            memcpy (m_indexMappingTransform[k], g_ecIndexMapping_Transform_4x2, 8 * sizeof (Int));
            m_indexMappingBp[k] = new Int[8];
            memcpy (m_indexMappingBp[k], g_ecIndexMapping_BP_4x2, 8 * sizeof (Int));
        }
        break;
    case k420:
        //! 4:2:0 chroma components !//
        for (UInt k = 1; k < g_numComps; k++) {
            m_indexMappingTransform[k] = new Int[4];
            memcpy (m_indexMappingTransform[k], g_ecIndexMapping_Transform_4x1, 4 * sizeof (Int));
            m_indexMappingBp[k] = new Int[4];
            memcpy (m_indexMappingBp[k], g_ecIndexMapping_BP_4x1, 4 * sizeof (Int));
        }
        break;
    default:
        fprintf (stderr, "EntropyCoder::EntropyCoder(): unsupported chroma format: %d.\n", m_chromaFormat);
        exit (EXIT_FAILURE);
    }
}

/**
    Destructor
    */
EntropyCoder::~EntropyCoder () {
    if (m_bitsPerGroup) { delete[] m_bitsPerGroup; m_bitsPerGroup = NULL; }
    if (m_bitsPerComp) { delete[] m_bitsPerComp; m_bitsPerComp = NULL; }
    for (UInt k = 0; k < g_numComps; k++) {
        if (m_signBitValid[k]) { delete[] m_signBitValid[k]; m_signBitValid[k] = NULL; }
        if (m_signBit[k]) { delete[] m_signBit[k]; m_signBit[k] = NULL; }
        if (m_indexMappingTransform[k]) { delete[] m_indexMappingTransform[k]; m_indexMappingTransform[k] = NULL; }
        if (m_indexMappingBp[k]) { delete[] m_indexMappingBp[k]; m_indexMappingBp[k] = NULL; }
    }
}


/**
    Decode ECG prefix

    @param k component index
    @param ssmIdx substream index
    @param ssm substream multiplexer
    @return ECG bits required
    */
UInt EntropyCoder::DecodePrefix (Int k, UInt ssmIdx, SubStreamMux* ssm) {
    UInt prefix = 0;
    UInt uiBits = ssm->ReadFromFunnelShifter (ssmIdx, 1);
    while (uiBits & 1) {
        uiBits = ssm->ReadFromFunnelShifter (ssmIdx, 1);
        prefix++;
    }
    UInt bitsReq = (m_modeType == kEcTransform) ? GetBitsReqFromCodeWord (prefix, k) + 1 : prefix + 1;
    return bitsReq;
}

/**
    Convert VEC symbol to ECG samples (SM)

    @param src array of samples
    @param startInd starting index
    @param endInd ending index
    @param bitsReq bits required for ECG
    @param symbol VEC symbol
    */
Void EntropyCoder::VecCodeSymbolToSamplesSM (Int* src, Int startInd, Int endInd, Int bitsReq, Int symbol) {
    Int maxValue = (1 << bitsReq) - 1;
    Int mask = bitsReq == 1 ? 0x1 : 0x3;
    Int shift = 3 * bitsReq;
    for (Int i = startInd; i < endInd; i++) {
        src[i] = (symbol >> shift) & mask;
        shift -= bitsReq;
    }
}

/**
    Convert VEC symbol to ECG samples (2C)

    @param src array of samples
    @param startInd starting index
    @param endInd ending index
    @param bitsReq bits required for ECG
    @param symbol VEC symbol
    */
Void EntropyCoder::VecCodeSymbolToSamples2C (Int* src, Int startInd, Int endInd, Int bitsReq, Int symbol) {
    Int mask = bitsReq == 1 ? 0x1 : 0x3;
    Int thresh = 1 << (bitsReq - 1);
    Int offset = 1 << bitsReq;
    Int shift = 3 * bitsReq;
    for (Int i = startInd; i < endInd; i++) {
        Int field = (symbol >> shift) & mask;
        src[i] = (field < thresh ? field : field - offset);
        shift -= bitsReq;
    }
}

/**
    Convert samples in ECG to VEC symbol (2C)

    @param src array of samples
    @param startInd starting index
    @param endInd ending index
    @param bitsReq bits required for ECG
    @return VEC symbol for ECG
    */
Int EntropyCoder::VecCodeSamplesToSymbol2C (Int* src, Int startInd, Int endInd, Int bitsReq) {
    Int symbol = 0;
    Int mask = (bitsReq == 1 ? 0x1 : 0x3);
    for (Int i = startInd; i < endInd; i++) {
        symbol = ((symbol << bitsReq) | (src[i] & mask));
    }
    return symbol;
}

/**
    Convert samples in ECG to VEC symbol (SM)

    @param src array of samples
    @param startInd starting index
    @param endInd ending index
    @param bitsReq bits required for ECG
    @return VEC symbol for ECG
    */
Int EntropyCoder::VecCodeSamplesToSymbolSM (Int* src, Int startInd, Int endInd, Int bitsReq) {
    Int symbol = 0;
    Int mask = (bitsReq == 1 ? 0x1 : 0x3);
    for (Int i = startInd; i < endInd; i++) {
        symbol = ((symbol << bitsReq) | (abs (src[i]) & mask));
    }
    return symbol;
}

/**
    Encode VEC symbol (2C)

    @param ssm substream multiplexer
    @param symbol VEC symbol
    @param k component index
    @param bitsReq ECG bits required
    @param isFinal boolean flag is 0 for testing and 1 for final encode
    */
Void EntropyCoder::EncodeVecEcSymbol2C (SubStreamMux* ssm, Int symbol, Int k, Int bitsReq, Bool isFinal) {
    UInt curSsmIdx = (k + 1);
    Int vecCodeNumber;
    switch (bitsReq) {
    case 1:
        vecCodeNumber = (k == 0) ? g_vec_2c_bitsReq_1_luma[symbol] : g_vec_2c_bitsReq_1_chroma[symbol];
        break;
    case 2:
        vecCodeNumber = (k == 0) ? g_vec_2c_bitsReq_2_luma[symbol] : g_vec_2c_bitsReq_2_chroma[symbol];
        break;
    default:
        fprintf (stderr, "EntropyCoder::EncodeVecEcSymbol2C(): unsupported bitsReq (%d).\n", bitsReq);
        exit (EXIT_FAILURE);
    }

    // look up best gol-rice param
    UInt vecGrK = (k == 0) ? g_qcomVectorEcBestGr_Y[bitsReq - 1] : g_qcomVectorEcBestGr_C[bitsReq - 1];

    //gol-rice coding
    Int prefix = vecCodeNumber >> vecGrK;
    Int suffix = vecCodeNumber & ((1 << vecGrK) - 1);
    Int origPrefix = prefix;

    //! code the GR prefix !//
    for (; prefix > 0; prefix--) {
        if (isFinal) {
            ssm->GetCurSyntaxElement (curSsmIdx)->AddSyntaxWord (1, 1);
        }
        else {
            m_bitsPerComp[k] += 1;
        }
    }
    Int maxPrefix = ((1 << (bitsReq * 4)) - 1) >> vecGrK;
    if (origPrefix < maxPrefix) {
        if (isFinal) {
            ssm->GetCurSyntaxElement (curSsmIdx)->AddSyntaxWord (0, 1);
        }
        else {
            m_bitsPerComp[k] += 1;
        }
    }

    //! code the GR suffix !//
    if (isFinal) {
        ssm->GetCurSyntaxElement (curSsmIdx)->AddSyntaxWord (suffix, vecGrK);
    }
    else {
        m_bitsPerComp[k] += vecGrK;
    }
}

/**
    Calculate bitsReq for ECG (2C)

    @param src array of samples in ECG
    @param startInd starting index
    @param endInd ending index
    @return ECG bits required
    */
Int EntropyCoder::CalculateGroupSize2C (Int* src, Int startInd, Int endInd) {
    Int bitsReq = 0;
    for (Int i = startInd; i < endInd; i++) {
        bitsReq = Max (FindResidualSize2C (src[i]), bitsReq);
    }
    return bitsReq;
}

/**
    Calculate bitsReq for ECG (SM)

    @param src array of samples in ECG
    @param startInd starting index
    @param endInd ending index
    @return ECG bits required
    */
Int EntropyCoder::CalculateGroupSizeSM (Int* src, Int startInd, Int endInd) {
    Int bitsReq = 0;
    for (Int i = startInd; i < endInd; i++) {
        bitsReq = Max (FindResidualSizeSM (src[i]), bitsReq);
    }
    return bitsReq;
}

/**
    Determine unary prefix code from bitsReq

    @param bitsReq ECG bits required
    @param k component index
    @return codeword for unary prefix
    */
UInt EntropyCoder::GetCodeWordFromBitsReq (Int bitsReq, Int k) {
    UInt codeWord;

    if (k == 0) { // for Luma 
        if (bitsReq == 1)
            codeWord = 0;
        else if (bitsReq == 2)
            codeWord = 1;
        else if (bitsReq == 3)
            codeWord = 2;
        else if (bitsReq == 4)
            codeWord = 3;
        else if (bitsReq == 0)
            codeWord = 4;
        else
            codeWord = bitsReq;
    }
    else { // for Co and Cg
        if (bitsReq == 1)
            codeWord = 0;
        else if (bitsReq == 0)
            codeWord = 1;
        else
            codeWord = bitsReq;
    }
    return codeWord;
}

/**
    Determine bitsReq from unary prefix

    @param codeWord
    @param k component index
    @return ECG bits required
    */
UInt EntropyCoder::GetBitsReqFromCodeWord (UInt codeWord, Int k) {
    UInt bits;
    if (k == 0) // Luma
    {
        if (codeWord == 0)
            bits = 1; // 2
        else if (codeWord == 1)
            bits = 2; // 1
        else if (codeWord == 2)
            bits = 3;
        else if (codeWord == 3)
            bits = 4;
        else if (codeWord == 4)
            bits = 0;
        else
            bits = codeWord;
    }
    else // chroma
    {
        if (codeWord == 0)
            bits = 1;
        else if (codeWord == 1)
            bits = 0;
        else
            bits = codeWord;
    }
    return bits;
}

/**
    Determine bit width of value (2C)

    @param x input value
    @return number of bits
    */
Int EntropyCoder::FindResidualSize2C (Int x) {
    Int numBits;

    // Find the size in bits of x
    if (x == 0) numBits = 0;
    else if (x >= -1 && x <= 0) numBits = 1;
    else if (x >= -2 && x <= 1) numBits = 2;
    else if (x >= -4 && x <= 3) numBits = 3;
    else if (x >= -8 && x <= 7) numBits = 4;
    else if (x >= -16 && x <= 15) numBits = 5;
    else if (x >= -32 && x <= 31) numBits = 6;
    else if (x >= -64 && x <= 63) numBits = 7;
    else if (x >= -128 && x <= 127) numBits = 8;
    else if (x >= -256 && x <= 255) numBits = 9;
    else if (x >= -512 && x <= 511) numBits = 10;
    else if (x >= -1024 && x <= 1023) numBits = 11;
    else if (x >= -2048 && x <= 2047) numBits = 12;
    else if (x >= -4096 && x <= 4095) numBits = 13;
    else if (x >= -8192 && x <= 8191) numBits = 14;
    else if (x >= -16384 && x <= 16383) numBits = 15;
    else if (x >= -32768 && x <= 32767) numBits = 16;
    else if (x >= -65536 && x <= 65535) numBits = 17;
    else {
        fprintf (stderr, "EntropyCoder::FindResidualSize2C(): value out of range: %d, mode is %d.\n", x, m_modeType);
        exit (EXIT_FAILURE);
    }
    return numBits;
}

/**
    Determine bit width of value (SM)

    @param x input value
    @return number of bits
    */
Int EntropyCoder::FindResidualSizeSM (Int x) {
    Int numBits;
    Int absRes = abs (x);
    if (absRes == 0) numBits = 0;
    else if (absRes <= 1) numBits = 1;
    else if (absRes <= 3) numBits = 2;
    else if (absRes <= 7) numBits = 3;
    else if (absRes <= 15) numBits = 4;
    else if (absRes <= 31) numBits = 5;
    else if (absRes <= 63) numBits = 6;
    else if (absRes <= 127) numBits = 7;
    else if (absRes <= 255) numBits = 8;
    else if (absRes <= 511) numBits = 9;
    else if (absRes <= 1023) numBits = 10;
    else if (absRes <= 2047) numBits = 11;
    else if (absRes <= 4095) numBits = 12;
    else if (absRes <= 8191) numBits = 13;
    else if (absRes <= 16383) numBits = 14;
    else if (absRes <= 32767) numBits = 15;
    else if (absRes <= 65535) numBits = 16;
    else {
        fprintf (stderr, "EntropyCoder::FindResidualSizeSM(): value out of range: %d, mode is %d.\n", x, m_modeType);
        exit (EXIT_FAILURE);
    }
    return numBits;
}

/**
    Count number of significant coefficients in component

    @param coef array of coefficients
    @param uiNumofCoeff number of significant coefficients
    @param k component index
    */
Void EntropyCoder::CountNumSigCoeff (Int* coef, UInt& uiNumofCoeff, Int k) {
    for (UInt i = 0; i < GetCompNumSamples (k); i++) {
        if (coef[i] != 0) {
            uiNumofCoeff++;
            return;
        }
    }
}

/**
    Decode VEC symbol (2C)

    @param ssm substream multiplexer
    @param k component index
    @param bitsReq ECG bits required
    @param symbol VEC symbol
    @param mask bit mask
    */
Void EntropyCoder::DecodeVecEcSymbol2C (SubStreamMux* ssm, Int k, Int bitsReq, Int& symbol, Int& mask) {
    UInt curSsmIdx = (k + 1);
    UInt vecGrK = (k == 0) ? g_qcomVectorEcBestGr_Y[bitsReq - 1] : g_qcomVectorEcBestGr_C[bitsReq - 1];
    Int vecCodeNumber = 0;
    Int prefix = 0;
    Int suffix = 0;
    UInt value = 1;

    Int maxPrefix = ((1 << (bitsReq * 4)) - 1) >> vecGrK;
    while (value != 0) {
        value = ssm->ReadFromFunnelShifter (curSsmIdx, 1);
        prefix += value;
        if (prefix == maxPrefix) {
            break;
        }
    }
    if (vecGrK > 0) {
        value = ssm->ReadFromFunnelShifter (curSsmIdx, vecGrK);
        suffix = value;
    }
    vecCodeNumber = ((prefix << vecGrK) | suffix);

    if (bitsReq == 1) {
        symbol = (k == 0) ? g_vec_2c_bitsReq_1_luma_inv[vecCodeNumber] : g_vec_2c_bitsReq_1_chroma_inv[vecCodeNumber];
        mask = 1;
    }
    else if (bitsReq == 2) {
        symbol = (k == 0) ? g_vec_2c_bitsReq_2_luma_inv[vecCodeNumber] : g_vec_2c_bitsReq_2_chroma_inv[vecCodeNumber];
        mask = 3;
    }
}

/**
    Decode VEC symbol (SM)

    @param ssm substream multiplexer
    @param k component index
    @param bitsReq ECG bits required
    @param symbol VEC symbol
    @param mask bit mask
    */
Void EntropyCoder::DecodeVecEcSymbolSM (SubStreamMux* ssm, Int k, Int bitsReq, Int& symbol, Int& mask) {
    UInt curSsmIdx = (k + 1);
    UInt vecGrK = (k == 0) ? g_mtkQcomVectorEcBestGr_Y[bitsReq - 1] : g_mtkQcomVectorEcBestGr_C[bitsReq - 1];
    Int vecCodeNumber = 0;
    Int prefix = 0;
    Int suffix = 0;
    UInt value = 1;

    //! decode the GR prefix !//
    Int maxPrefix = ((1 << (bitsReq * 4)) - 1) >> vecGrK;
    while (value != 0) {
        value = ssm->ReadFromFunnelShifter (curSsmIdx, 1);
        prefix += value;
        if (prefix == maxPrefix) {
            break;
        }
    }

    //! decode the GR suffix !//
    value = ssm->ReadFromFunnelShifter (curSsmIdx, vecGrK);
    suffix = value;

    //! reconstruct the GR codeword !//
    vecCodeNumber = ((prefix << vecGrK) | suffix);

    //! map code number to symbol !//
    switch (bitsReq) {
    case 1:
        symbol = (k == 0) ? g_vec_sm_bitsReq_1_luma_inv[vecCodeNumber] : g_vec_sm_bitsReq_1_chroma_inv[vecCodeNumber];
        mask = 1;
        break;
    case 2:
        symbol = (k == 0) ? g_vec_sm_bitsReq_2_luma_inv[vecCodeNumber] : g_vec_sm_bitsReq_2_chroma_inv[vecCodeNumber];
        mask = 3;
        break;
    }
}

/**
    Encode VEC symbol (SM)

    @param ssm substream multiplexer
    @param symbol VEC symbol
    @param k component index
    @param bitsReq bits required for ECG
    @param isFinal boolean flag is 0 for testing and 1 for final encode
    */
Void EntropyCoder::EncodeVecEcSymbolSM (SubStreamMux* ssm, Int symbol, Int k, Int bitsReq, Bool isFinal) {
    Int vecCodeNumber;
    UInt curSubStream = (k + 1);
    
    //! look up vecCodeNumber !//
    switch (bitsReq) {
    case 1:
        vecCodeNumber = (k == 0) ? g_vec_sm_bitsReq_1_luma[symbol] : g_vec_sm_bitsReq_1_chroma[symbol];
        break;
    case 2:
        vecCodeNumber = (k == 0) ? g_vec_sm_bitsReq_2_luma[symbol] : g_vec_sm_bitsReq_2_chroma[symbol];
        break;
    default:
        fprintf (stderr, "EntropyCoder::EncodeVecEcSymbolSM(): unsupported bitsReq (%d).\n", bitsReq);
        exit (EXIT_FAILURE);
    }

    //! look up vecGrK !//
    UInt vecGrK = (k == 0) ? g_mtkQcomVectorEcBestGr_Y[bitsReq - 1] : g_mtkQcomVectorEcBestGr_C[bitsReq - 1];

    //gol-rice coding
    Int prefix = vecCodeNumber >> vecGrK;
    Int suffix = vecCodeNumber & ((1 << vecGrK) - 1);
    Int origPrefix = prefix;

    //! code the GR prefix !//
    for (; prefix > 0; prefix--) {
        if (isFinal) {
            ssm->GetCurSyntaxElement (curSubStream)->AddSyntaxWord (1, 1);
        }
        else {
            m_bitsPerComp[k] += 1;
        }
    }
    Int maxPrefix = ((1 << (bitsReq * 4)) - 1) >> vecGrK;
    if (origPrefix < maxPrefix) {
        if (isFinal) {
            ssm->GetCurSyntaxElement (curSubStream)->AddSyntaxWord (0, 1);
        }
        else {
            m_bitsPerComp[k] += 1;
        }
    }

    //! code the GR suffix !//
    if (isFinal) {
        ssm->GetCurSyntaxElement (curSubStream)->AddSyntaxWord ((UInt)suffix, vecGrK);
    }
    else {
        m_bitsPerComp[k] += vecGrK;
    }
}

/**
    Encode quantized transform coefficients

    @param src array of samples to encode
    @param k component index
    @param isFinal boolean flag is 0 for testing and 1 for final encode
    @param ssm substream multiplexer
    @return entropy coding rate
    */
Int EntropyCoder::EncodeTransformCoefficients (Int* src, Int k, Bool isFinal, SubStreamMux* ssm) {
    Int* compEcgCoeff = new Int[GetCompNumSamples (k)];
    for (UInt i = 0; i < GetCompNumSamples (k); i++) {
        compEcgCoeff[m_indexMappingTransform[k][i]] = src[i];
    }
    EncodeAllGroups (compEcgCoeff, k, ssm, isFinal);
    delete[] compEcgCoeff;
    return m_bitsPerComp[k];
}

/**
    Decode quantized residuals

    @param coeff
    @param k component index
    @param ssm substream multiplexer
    */
Void EntropyCoder::DecodeResiduals (Int* coeff, Int k, SubStreamMux* ssm) {
    Int* compEcgCoeff = new Int[GetCompNumSamples (k)];
    DecodeAllGroups (compEcgCoeff, k, ssm);
    for (UInt i = 0; i < GetCompNumSamples (k); i++) {
        coeff[i] = compEcgCoeff[m_indexMappingBp[k][i]];
    }
    delete[] compEcgCoeff;
}

/**
    Encode quantized residuals

    @param src array of samples to encode
    @param k component index
    @param isFinal boolean flag is 0 for testing and 1 for final encode
    @param ssm substream multiplexer
    @return entropy coding rate
    */
Int EntropyCoder::EncodeResiduals (Int* src, Int k, Bool isFinal, SubStreamMux* ssm) {
    Int* compEcgCoeff = new Int[GetCompNumSamples (k)];
    for (UInt i = 0; i < GetCompNumSamples (k); i++) {
        compEcgCoeff[m_indexMappingBp[k][i]] = src[i];
    }
    EncodeAllGroups (compEcgCoeff, k, ssm, isFinal);
    delete[] compEcgCoeff;
    return m_bitsPerComp[k];
}

/**
    Encode all ECG in component

    @param coeff array of samples to encode
    @param k component index
    @param ssm substream multiplexer
    @param isFinal boolean flag is 0 for testing and 1 for final encode
    */
Void EntropyCoder::EncodeAllGroups (Int* coeff, Int k, SubStreamMux* ssm, Bool isFinal) {
    memset (m_bitsPerGroup, 0, m_numEcGroups * sizeof (Int));
    m_bitsPerComp[k] = 0;
    UInt compNumSamples = GetCompNumSamples (k);
    UInt numBitsLastSigPos = ((compNumSamples == 16) ? 4 : ((compNumSamples == 8) ? 3 : 2));
    UInt curSubstream = (k + 1);
    Int signLastSigPos;

    //! reset info for sign bits !//
    m_compNumSignBits[k] = 0;
    ResetSignBits (k);
    
    //! check component skip !//
    UInt numSigSamples = 0;
    CountNumSigCoeff (coeff, numSigSamples, k);
    Bool isCompSkip = (numSigSamples == 0 && k > 0 ? true : false);

    //! signal component skip !//
    if (isCompSkip) {
        switch (m_modeType) {
        case kEcBp:
            if (isFinal) {
                ssm->GetCurSyntaxElement (curSubstream)->AddSyntaxWord (1, 1);
            }
            else {
                m_bitsPerComp[k] += 1;
            }
            break;
        case kEcTransform:
            if (isFinal) {
                if (isCompSkip) {
                    ssm->GetCurSyntaxElement (curSubstream)->AddSyntaxWord (1, 1);
                    ssm->GetCurSyntaxElement (curSubstream)->AddSyntaxWord (1, 1);
                }
                else {
                    ssm->GetCurSyntaxElement (curSubstream)->AddSyntaxWord (0, 1);
                }
            }
            else {
                m_bitsPerComp[k] += (isCompSkip ? 2 : 1);
            }
            break;
        }
    }
    else {
        if (k > 0) {
            if (isFinal) {
                ssm->GetCurSyntaxElement (curSubstream)->AddSyntaxWord (0, 1);
            }
            else {
                m_bitsPerComp[k] += 1;
            }
        }
    }

    //! compute lastSigPos for transform mode !//
    Int lastSigPos = -1;
    if (m_modeType == kEcTransform && !isCompSkip) {
        for (Int i = (compNumSamples - 1); i >= 0; i--) {
            if (abs (coeff[i]) != 0) {
                lastSigPos = i;
                break;
            }
        }

        //! lastSigPos=0 for luma component if all samples are zero !//
        lastSigPos = (k == 0 && lastSigPos == -1) ? 0 : lastSigPos;

        //! code lastSigPos !//
        if (isFinal) {
            ssm->GetCurSyntaxElement (curSubstream)->AddSyntaxWord ((UInt)lastSigPos, numBitsLastSigPos);
        }
        else {
            m_bitsPerComp[k] += numBitsLastSigPos;
        }

        //! store the sign bit for lastSigPos !//
        signLastSigPos = (coeff[lastSigPos] < 0 ? 1 : 0);

        //! adjust the last significant position coefficient value (unless luma and lastSigPos=0)!//
        //! x_new = (|x| - 1) * sign(x) !//
        if (!(lastSigPos == 0 && k == 0)) {
            if (coeff[lastSigPos] > 0) {
                coeff[lastSigPos] -= 1;
            }
            else {
                coeff[lastSigPos] += 1;
            }
        }
    }
    else {
        lastSigPos = compNumSamples - 1;
    }

    for (UInt ecgIdx = 0; ecgIdx < 4; ecgIdx++) {
        Bool ecgDataActive;
        Int ecgStart, ecgEnd;
        GetEcgInfo (ecgIdx, k, isCompSkip, lastSigPos, ecgDataActive, ecgStart, ecgEnd);
        UInt bitsStart = isFinal ? ssm->GetCurSyntaxElement (curSubstream)->GetNumBits () : m_bitsPerComp[k];

        //! ECG: data !//
        if (ecgDataActive) {
            EncodeOneGroup (coeff, ecgIdx, ecgStart, ecgEnd, k, ssm, isFinal);
        }

        //! ECG: padding in substreams 1-3 !//
        if (m_underflowPreventionMode && ecgIdx > 0) {
            if (isFinal) {
                ssm->GetCurSyntaxElement (curSubstream)->AddSyntaxWord (0, m_rcStuffingBits);
            }
            else {
                m_bitsPerComp[k] += m_rcStuffingBits;
            }
        }

        //! ECG: sign bits in ECG 3 !//
        if (ecgIdx == 3 && !isCompSkip) {
            if (isFinal) {
                for (UInt s = 0; s < compNumSamples; s++) {
                    if (m_signBitValid[k][s]) {
                        ssm->GetCurSyntaxElement (curSubstream)->AddSyntaxWord (m_signBit[k][s], 1);
                        m_bitsPerComp[k] += 1;
                    }
                }
            }
            else {
                m_bitsPerComp[k] += m_compNumSignBits[k];
            }

            //! signal signLastSigPos if coeff[lastSigPos] == 0 !//
            if (m_modeType == kEcTransform) {
                if (!(lastSigPos == 0 && k == 0)) {
                    if (coeff[lastSigPos] == 0) {
                        if (isFinal) {
                            ssm->GetCurSyntaxElement (curSubstream)->AddSyntaxWord ((UInt)signLastSigPos, 1);
                        }
                        else {
                            m_bitsPerComp[k] += 1;
                        }
                    }
                }
            }
        }
		UInt bitsEnd = isFinal ? ssm->GetCurSyntaxElement (curSubstream)->GetNumBits () : m_bitsPerComp[k];
        m_bitsPerGroup[ecgIdx] = (bitsEnd - bitsStart);
    }
}

/**
    Encode one ECG

    @param src array of samples to encode
    @param startInd starting index
    @param endInd ending index
    @param k component index
    @param ssm substream multiplexer
    @param isFinal boolean flag is 0 for testing and 1 for final encode
    */
Void EntropyCoder::EncodeOneGroup (Int* src, Int ecgIdx, Int ecgStart, Int ecgEnd, Int k, SubStreamMux* ssm, Bool isFinal) {
    Int bitsReq;
    Bool groupSkip;
    UInt curSubStream = (k + 1);

    //! group bit representation !//
    Bool useSignMag = (ecgIdx < 3 ? true : false);

    if (useSignMag) {
        bitsReq = CalculateGroupSizeSM (src, ecgStart, ecgEnd);
        groupSkip = EncodePrefix (bitsReq, k, ssm, isFinal);
        if (!groupSkip) {
            if (bitsReq <= g_mtkQcomVectorEcThreshold && m_modeType == kEcBp) {
                //! encode VEC ECG (SM) !//
                Int symbol = VecCodeSamplesToSymbolSM (src, ecgStart, ecgEnd, bitsReq);
                EncodeVecEcSymbolSM (ssm, symbol, k, bitsReq, isFinal);
            }
            else {
                //! encode CPEC ECG (SM) !//
                for (Int i = ecgStart; i < ecgEnd; i++) {
                    UInt mag = abs (src[i]);
                    if (isFinal) {
                        ssm->GetCurSyntaxElement (curSubStream)->AddSyntaxWord (mag, bitsReq);
                    }
                    else {
                        m_bitsPerComp[k] += bitsReq;
                    }
                }
            }

            //! keep track of sign bits for non-zero samples !//
            for (Int i = ecgStart; i < ecgEnd; i++) {
                Bool signalSignBit = (src[i] != 0);
                if (signalSignBit) {
                    m_compNumSignBits[k]++;
                    m_signBit[k][i] = (src[i] < 0 ? 1 : 0);
                    m_signBitValid[k][i] = true;
                }
            }
        }
    }
    else {
        bitsReq = CalculateGroupSize2C (src, ecgStart, ecgEnd);
        groupSkip = EncodePrefix (bitsReq, k, ssm, isFinal);
        if (!groupSkip) {
            if (bitsReq <= g_qcomVectorEcThreshold && m_modeType == kEcBp) {
                //! encode VEC ECG (2C) !//
                Int symbol = VecCodeSamplesToSymbol2C (src, ecgStart, ecgEnd, bitsReq);
                EncodeVecEcSymbol2C (ssm, symbol, k, bitsReq, isFinal);
            }
            else {
                //! encode CPEC ECG (2C) !//
                for (Int i = ecgStart; i < ecgEnd; i++) {
                    UInt value = src[i] < 0 ? (1 << bitsReq) + src[i] : src[i];
                    if (isFinal) {
                        ssm->GetCurSyntaxElement (curSubStream)->AddSyntaxWord (value, bitsReq);
                    }
                    else {
                        m_bitsPerComp[k] += bitsReq;
                    }
                }
            }
        }
    }
}

/**
    Encode ECG prefix

    @param bitsReq bits required for ECG
    @param k component index
    @param ssm substream multiplexer
    @param isFinal boolean flag is 0 for testing and 1 for final encode
    @return flag to indicate group skip (true if enabled)
    */
Bool EntropyCoder::EncodePrefix (Int bitsReq, Int k, SubStreamMux* ssm, Bool isFinal) {
    // Check skip mode of current group
    Bool groupSkip = (bitsReq == 0) ? true : false;
    UInt curSubStream = (k + 1);
    if (groupSkip) {
        if (isFinal) {
            ssm->GetCurSyntaxElement (curSubStream)->AddSyntaxWord (1, 1);
        }
        else {
            m_bitsPerComp[k] += 1;
        }
        return (true);
    }
    else {
        if (isFinal) {
            ssm->GetCurSyntaxElement (curSubStream)->AddSyntaxWord (0, 1);
        }
        else {
            m_bitsPerComp[k] += 1;
        }
        // directly code the (prefix value-1) without any prediction
        UInt codeWord = (m_modeType == kEcTransform) ? GetCodeWordFromBitsReq (bitsReq - 1, k) : bitsReq - 1;
        UnaryCoding (codeWord, k, ssm, isFinal);
        return (false);
    }
}

/**
    Encode value using unary code

    @param value value to encode
    @param k component index
    @param ssm substream multiplexer
    @param isFinal boolean flag is 0 for testing and 1 for final encode
    */
Void EntropyCoder::UnaryCoding (UInt value, Int k, SubStreamMux* ssm, Bool isFinal) {
    UInt curSubStream = (k + 1);
    while (value > 0) {
        if (isFinal) {
            ssm->GetCurSyntaxElement (curSubStream)->AddSyntaxWord (1, 1);
        }
        else {
            m_bitsPerComp[k] += 1;
        }
        value -= 1;
    }
    if (isFinal) {
        ssm->GetCurSyntaxElement (curSubStream)->AddSyntaxWord (0, 1);
    }
    else {
        m_bitsPerComp[k] += 1;
    }
}

/**
    Decode all ECG in given component

    @param coeff destination for decoded samples
    @param k component index
    @param ssm substream multiplexer
    */
Void EntropyCoder::DecodeAllGroups (Int* coeff, Int k, SubStreamMux* ssm) {
    UInt value = 0;
    UInt compNumSamples = GetCompNumSamples (k);
    UInt curSubstream = (k + 1);
    ResetSignBits (k);

    //! compSkip !//
    Bool isCompSkip = false;
    if (k > 0) {
        value = ssm->ReadFromFunnelShifter (curSubstream, 1);
        if (value == 1) {
            isCompSkip = true;
        }
    }
    if (isCompSkip) {
        //! read one more bit for 2-bit component skip flag !//
        if (m_modeType == kEcTransform) {
            value = ssm->ReadFromFunnelShifter (curSubstream, 1);
        }

        //! set all samples to zero !//
        for (UInt i = 0; i < compNumSamples; i++) {
            coeff[i] = 0;
        }
    }

    //! lastSigPos !//
    UInt lastSigPos;
    UInt numBitsLastSigPos = ((compNumSamples == 16) ? 4 : ((compNumSamples == 8) ? 3 : 2));
    if (m_modeType == kEcTransform && !isCompSkip) {
        lastSigPos = ssm->ReadFromFunnelShifter (curSubstream, numBitsLastSigPos);
    }
    else {
        lastSigPos = compNumSamples - 1;
    }

    //! process ECGs !//
    for (UInt ecgIdx = 0; ecgIdx < 4; ecgIdx++) {
        Bool ecgDataActive;
        Int ecgStart, ecgEnd;
        GetEcgInfo (ecgIdx, k, isCompSkip, lastSigPos, ecgDataActive, ecgStart, ecgEnd);

        //! ecg: data !//
        if (ecgDataActive) {
            DecodeOneGroup (coeff, ecgIdx, ecgStart, ecgEnd, k, ssm);
        }

        //! ecg: padding in substreams 1-3 !//
        if (m_underflowPreventionMode && ecgIdx >= 1) {
            ssm->ReadFromFunnelShifter (curSubstream, m_rcStuffingBits);
        }

		//! ecg: sign bits in substream 3 !//
		if (ecgIdx == 3 && !isCompSkip) {
			for (UInt s = 0; s < compNumSamples; s++) {
				if (m_signBitValid[k][s]) {
					value = ssm->ReadFromFunnelShifter (curSubstream, 1);
                    coeff[s] = (value == 1) ? -coeff[s] : coeff[s];
                }
            }

            //! signLastSigPos !//
            if (m_modeType == kEcTransform) {
                Int signSigPos;
                if (!(lastSigPos == 0 && k == 0)) {
                    if (coeff[lastSigPos] == 0) {
                        signSigPos = ssm->ReadFromFunnelShifter (curSubstream, 1);
                    }
                    else {
                        signSigPos = coeff[lastSigPos] < 0 ? 1 : 0;
                    }

                    //! modify coeff[lastSigPos] !//
                    coeff[lastSigPos] += (signSigPos == 1 ? -1 : 1);
                }
            }
        }
    }
}

/**
    Decode one ECG

    @param src destination for decoded samples
    @param startInd starting index
    @param endInd ending index
    @param k component index
    @param ssm substream multiplexer
    */
Void EntropyCoder::DecodeOneGroup (Int* src, Int ecgIdx, Int ecgStart, Int ecgEnd, Int k, SubStreamMux* ssm) {
    UInt curSsmIdx = (k + 1);
    
    //! parse group skip flag !//
    UInt value = ssm->ReadFromFunnelShifter (curSsmIdx, 1);
    if (value == 1) {
        //! group skip active !//
        for (Int i = ecgStart; i < ecgEnd; i++) {
            src[i] = 0;
        }
        return;
    }

    //! bitsReq !//
    UInt bitsReq = DecodePrefix (k, curSsmIdx, ssm);

    //! bit representation for group !//
    Bool useSignMag = (ecgIdx < 3 ? true : false);

    if (useSignMag) {
        if (bitsReq <= g_mtkQcomVectorEcThreshold && (m_modeType == kEcBp)) {
            //! decode VEC ECG (SM) !//
            Int symbol;
            Int mask;
            DecodeVecEcSymbolSM (ssm, k, bitsReq, symbol, mask);
            VecCodeSymbolToSamplesSM (src, ecgStart, ecgEnd, bitsReq, symbol);
        }
        else {
            //! decode CPEC ECG (SM) !//
            for (Int i = ecgStart; i < ecgEnd; i++) {
                value = ssm->ReadFromFunnelShifter (curSsmIdx, bitsReq);
                src[i] = value;
            }
        }

        //! set flag for valid sign bit !//
        for (Int i = ecgStart; i < ecgEnd; i++) {
            if (src[i] != 0) {
                m_signBitValid[k][i] = true;
            }
        }
    }
    else {
        if (bitsReq <= g_qcomVectorEcThreshold && m_modeType == kEcBp) {
            //! decode VEC ECG (2C) !//
            Int symbol;
            Int mask;
            DecodeVecEcSymbol2C (ssm, k, bitsReq, symbol, mask);
            VecCodeSymbolToSamples2C (src, ecgStart, ecgEnd, bitsReq, symbol);
        }
        else {
            //! decode CPEC ECG (2C) !//
            for (Int i = ecgStart; i < ecgEnd; i++) {
                value = ssm->ReadFromFunnelShifter (curSsmIdx, bitsReq);
                UInt th = (1 << (bitsReq - 1)) - 1;
                Int pos = (Int)value;
                Int neg = -1 * ((1 << bitsReq) - (Int)value);
                src[i] = (value > th) ? neg : pos;
            }
        }
    }
}

/**
    Decode transform coefficients for given component

    @param coeff destination for transform coefficients
    @param k component index
    @param ssm substream multiplexer
    */
Void EntropyCoder::DecodeTransformCoefficients (Int* coeff, Int k, SubStreamMux* ssm) {
    Int* compEcgCoeff = new Int[GetCompNumSamples (k)];
    memset (compEcgCoeff, 0, GetCompNumSamples (k) * sizeof (Int));
    DecodeAllGroups (compEcgCoeff, k, ssm);
    for (UInt i = 0; i < GetCompNumSamples (k); i++) {
        coeff[i] = compEcgCoeff[m_indexMappingTransform[k][i]];
    }
    delete[] compEcgCoeff;
}

/**
    Reset all sign bits for component

    @param k component index
    */
Void EntropyCoder::ResetSignBits (UInt k) {
    for (UInt i = 0; i < GetCompNumSamples (k); i++) {
        m_signBit[k][i] = LARGE_INT;
        m_signBitValid[k][i] = false;
    }
}

/**
	Get information for current entropy coding group (active, start, end)
	
	@param ecgIdx entropy coding group index
	@param k component index
	@param isCompSkip component skip flag
	@param lastSigPos last significant position (for transform mode)
	@param ecgDataActive (ref) flag if data portion of ECG is active
	@param curEcgStart (ref) current ECG starting sample index
	@param curEcgEnd (ref) current ECG ending sample index
*/
Void EntropyCoder::GetEcgInfo (UInt ecgIdx, UInt k, Bool isCompSkip, UInt lastSigPos, Bool &ecgDataActive, Int &curEcgStart, Int &curEcgEnd) {
    UInt maxChromaEcgIdx = (m_chromaFormat == k444 ? 4 : m_chromaFormat == k422 ? 2 : 1);
    if (isCompSkip) {
        ecgDataActive = false;
        curEcgStart = -1;
        curEcgEnd = -1;
        return;
    }

    switch (m_modeType) {
    case kModeTypeBP:
        if (k == 0 || ecgIdx < maxChromaEcgIdx) {
            ecgDataActive = true;
            curEcgStart = 4 * ecgIdx;
            curEcgEnd = curEcgStart + 4;
        }
        else {
            ecgDataActive = false;
            curEcgStart = -1;
            curEcgEnd = -1;
        }
        break;
    case kModeTypeTransform:
        if (m_chromaFormat == k444 || k == 0) {
            curEcgStart = g_ecTransformEcgStart_444[ecgIdx];
            curEcgEnd = curEcgStart + g_transformEcgMappingLastSigPos_444[lastSigPos][ecgIdx];
        }
        else {
            if (m_chromaFormat == k422) {
                curEcgStart = g_ecTransformEcgStart_422[ecgIdx];
                curEcgEnd = curEcgStart + g_transformEcgMappingLastSigPos_422[lastSigPos][ecgIdx];
            }
            else if (m_chromaFormat == k420) {
                curEcgStart = g_ecTransformEcgStart_420[ecgIdx];
                curEcgEnd = curEcgStart + g_transformEcgMappingLastSigPos_420[lastSigPos][ecgIdx];
            }
        }
        if (curEcgEnd > curEcgStart) {
            ecgDataActive = true;
        }
        else {
            ecgDataActive = false;
            curEcgStart = -1;
            curEcgEnd = -1;
        }
    }
}