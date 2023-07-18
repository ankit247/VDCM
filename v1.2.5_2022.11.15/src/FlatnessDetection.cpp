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

/**
\file       FlatnessDetection.cpp
\brief      Flatness Detection
*/

#include "FlatnessDetection.h"
#include "TypeDef.h"
#include <iostream>
#include <cstring>

/**
    Reset complexity info for new line
*/
Void FlatnessDetection::PartialReset () {
    for (Int k = 0; k < g_numComps; k++) {
        m_complexity.prev[k] = 0;
        m_complexity.next[k] = 0;
    }
    m_maxCoeffVal = 0;
}

/**
    Reset all complexity info
*/
Void FlatnessDetection::Reset () {
    for (Int k = 0; k < g_numComps; k++) {
        m_complexity.prev[k] = 0;
        m_complexity.cur[k] = 0;
        m_complexity.next[k] = 0;
    }
    m_maxCoeffVal = 0;
}

/**
    Update complexity info buffers
*/
Void FlatnessDetection::ShiftComplexityValues () {
    for (Int i = 0; i < g_numComps; i++) {
        m_complexity.prev[i] = m_complexity.cur[i];
        m_complexity.cur[i] = m_complexity.next[i];
        m_complexity.next[i] = 0;
    }
    m_flatToComplex[0] = m_flatToComplex[1];
    m_flatToComplex[1] = m_flatToComplex[2];
    m_flatToComplex[2] = false;
}

/**
    Flatness type classification for current block
*/
Void FlatnessDetection::Classify () {
    Int complexityCumPrev = 0;
    Int complexityCumCur = 0;
    Int complexityCumNext = 0;

    // add the complexity value of all three channels
    for (Int i = 0; i < g_numComps; i++) {
        complexityCumNext += m_complexity.next[i];
        complexityCumCur += m_complexity.cur[i];
        complexityCumPrev += m_complexity.prev[i];
    }

    if (m_c == m_numBlksX - 1) {
        // for last block in slice, different rule is applied for type 2 flatness
        m_isPrevBlkComplex = (complexityCumCur <= 50);
    }
    else if (m_c == 0)  {
        // for first block in each line, different rule to check type 2 flatness
        m_isPrevBlkComplex = false;
        if (complexityCumCur <= 50) {
            m_isPrevBlkComplex = complexityCumPrev > 90;
        }
    }
    else {
        // two conditions are checked (i) next block is flat (ii) previous block is complex
        m_isPrevBlkComplex = false;
        m_isNextBlkFlat = false;

        // check if next block is flat
        Int Th = (m_maxCoeffVal >> 2) + (m_maxCoeffVal >> 3);
        Bool check1 = (complexityCumCur - complexityCumNext) > (m_maxCoeffVal >> 4) ? true : false;
        Bool check2 = complexityCumNext < Th ? true : false;
        if (check1 && check2) {
            m_isNextBlkFlat = true;
        }

        // check if previous block is complex
        switch (m_chromaFormat) {
        case k444:
        case k422:
            if (complexityCumNext <= 6) {
                // if next block is very flat then previous block complexity is not checked
                m_isPrevBlkComplex = true;
            }
            else if (complexityCumNext <= 25) {
                // if moderately flat then previous block complexity is checked
                m_isPrevBlkComplex = complexityCumPrev > (m_maxCoeffVal >> 1) + (m_maxCoeffVal >> 3);
            }
            break;
        case k420:
            if (complexityCumNext <= 20) {
                m_isPrevBlkComplex = complexityCumPrev > 40;
            }
            break;
        }
    }

    // check flat-to-complex flatness type
    if (m_c == m_numBlksX - 1 || m_c == 0) { 
        // flat-to-complex not valid for first/last block in slice
        m_flatToComplex[2] = false;
    }
    else {
        Bool check1 = (complexityCumPrev <= 3 && complexityCumNext >= 50);
        Bool check2 = (!m_flatToComplex[0] && !m_flatToComplex[1]);
        m_flatToComplex[2] = (check1 && check2);
    }

    // check classification for four flatness types
    Bool flatTypeA = (complexityCumCur <= 1);
    Bool flatTypeB = (complexityCumCur <= 3 && complexityCumCur > 1);
    Bool flatTypeC = (m_c == 0 || m_c == m_numBlksX - 1) ? m_isPrevBlkComplex : (m_isPrevBlkComplex && m_isNextBlkFlat);
    Bool flatTypeD = m_flatToComplex[1];

    // decide the current block flatness type
    m_curFlatnessType = kFlatnessTypeUndefined;
    if (flatTypeA) {
        m_curFlatnessType = kFlatnessTypeVeryFlat;
    }
    else if (flatTypeB) {
        m_curFlatnessType = kFlatnessTypeSomewhatFlat;
    }
    else if (flatTypeC) {
        m_curFlatnessType = kFlatnessTypeComplexToFlat;
    }
    else if (flatTypeD) {
        m_curFlatnessType = kFlatnessTypeFlatToComplex;
    }
}

/**
    Update position data for flatness class

    @param r current row index
    @param c current column index
    @param numPx num px in current block
    @param numBlksX num blocks per slice (x)
    @param numBlksY num blocks per slice (y)
*/
Void FlatnessDetection::UpdatePositionData (Int r, Int c, Int numBlksX, Int numBlksY) {
    m_r = r;
    m_c = c;
    m_numBlksX = numBlksX;
    m_numBlksY = numBlksY;
}

/**
    Initialize max coefficient values
*/
Void FlatnessDetection::InitMaxCoeffsVals () {
    m_maxCoeffVal = m_complexity.cur[0] + m_complexity.cur[1] + m_complexity.cur[2];
}

/**
    Update max coefficient values
*/
Void FlatnessDetection::CalculateMaxCoeffsVals () {
    Int sumCoeff = m_complexity.next[0] + m_complexity.next[1] + m_complexity.next[2];
    m_maxCoeffVal = Max (m_maxCoeffVal, sumCoeff);
}

/**
    Load current block data into flatness class
*/
Void FlatnessDetection::InitCurBlock (Block *block) {
    for (Int k = 0; k < g_numComps; k++) {
        Int numPixels = (m_blkWidth*m_blkHeight) >> (g_compScaleX[k][m_chromaFormat] + g_compScaleY[k][m_chromaFormat]);
        Int* pSrc = block->GetBufOrg (k);
        Int* pDes = GetCurBlockBuf (k);
        std::memcpy (pDes, pSrc, sizeof (Int)*numPixels);
    }
}

/**
    Load next block data into flatness class

    @param blockNext block data
*/
Void FlatnessDetection::InitNextBlock (Block *blockNext) {
    //Read the samples from the image
    Bool isLastBlkInLine = (m_c == (m_numBlksX - 1)) ? true : false;
    Int blkNextX = m_blkWidth * (isLastBlkInLine ? 0 : m_c + 1);
    Int blkNextY = m_blkHeight * (isLastBlkInLine ? m_r + 1 : m_r);
    if (!(isLastBlkInLine && m_r == m_numBlksY - 1)) {
        blockNext->Init (blkNextX, blkNextY, m_bitDepth, m_origSrcCsc);
    }

    //copy current block from block class to the buffers in the flatness class
    for (Int k = 0; k < g_numComps; k++) {
        Int numPixels = (m_blkWidth*m_blkHeight) >> (g_compScaleX[k][m_chromaFormat] + g_compScaleY[k][m_chromaFormat]);
        Int* pSrc = blockNext->GetBufOrg (k);
        Int* pDes = GetNextBlockBuf (k);
        std::memcpy (pDes, pSrc, sizeof (Int)*numPixels);
    }
}

/**
    RGB to YCoCg transform

    @param pR red component data
    @param pG green component data
    @param pB blue component data
*/
Void FlatnessDetection::ApplyRgbToYCoCgTr (Int* pR, Int* pG, Int* pB) {
    // This function is only called for chroma format 444, so all the components have the same size
    Int width = m_blkWidth;
    Int height = m_blkHeight;
    Int stride = m_blkStride;
    Int tmp, Co, Cg;
    for (Int y = 0; y < height; y++) {
        for (Int x = 0; x < width; x++) {
            Co = *(pR + x) - *(pB + x);
            tmp = *(pB + x) + (Co >> 1);
            Cg = *(pG + x) - tmp;
            *(pR + x) = tmp + (Cg >> 1);  // Y
            *(pG + x) = Co;             // Co
            *(pB + x) = Cg;             // Cg
        }
        pR += stride;
        pG += stride;
        pB += stride;
    }
}

/**
    Compute complexity for current and next blocks
*/
Void FlatnessDetection::CalcComplexity () {
    Bool isLastBlkInLine = (m_c == (m_numBlksX - 1)) ? true : false;
    ShiftComplexityValues ();

    // calculate complexity for current block
    if ((m_origSrcCsc == kRgb) && (m_flatnessCsc == kYCoCg) && (m_chromaFormat == k444)) {
        // if input is 4:4:4 RGB, convert to YCoCg
        ApplyRgbToYCoCgTr (m_curBlock[0], m_curBlock[1], m_curBlock[2]);
    }
    for (Int comp = 0; comp < g_numComps; comp++) {
        Int* coeff = new Int[GetWidth (comp)*GetHeight (comp)];

        switch (m_flatnessCsc) {
        case kRgb:
        case kYCbCr:
            m_transBitDepth = m_bitDepth;
            break;
        case kYCoCg:
            m_transBitDepth = m_bitDepth + (comp == 0 ? 0 : 1);
            break;
        default:
            fprintf (stderr, "FlatnessDetection::CalcComplexity(): unsupported CSC: %d.\n", m_flatnessCsc);
            exit (EXIT_FAILURE);
        }

        // perform HAD transform
        ForwardHadTrans (m_curBlock[comp], GetBlkStride (comp), coeff, comp);

        // calculate sum of HAD coefficients (exclude DC)
        m_complexity.cur[comp] = SumHadCoefficients (coeff, comp, GetTransformShift ());
        delete[] coeff;
    }

    // init max coefficient value
    if (m_c == 0) {
        InitMaxCoeffsVals ();
    }

    // calculate complexity for next block
    if (!(isLastBlkInLine && m_r == m_numBlksY - 1)) {
        if ((m_origSrcCsc == kRgb) && (m_flatnessCsc == kYCoCg) && m_chromaFormat == k444) {
            // if input is 4:4:4 RGB, convert to YCoCg
            ApplyRgbToYCoCgTr (m_nextBlock[0], m_nextBlock[1], m_nextBlock[2]);
        }

        for (Int comp = 0; comp < g_numComps; comp++) {
            Int* coeff = new Int[GetWidth (comp)*GetHeight (comp)];

            switch (m_flatnessCsc) {
            case kRgb:
            case kYCbCr:
                m_transBitDepth = m_bitDepth;
                break;
            case kYCoCg:
                m_transBitDepth = m_bitDepth + (comp == 0 ? 0 : 1);
                break;
            default:
                fprintf (stderr, "FlatnessDetection::CalcComplexity(): unsupported CSC: %d.\n", m_flatnessCsc);
                exit (EXIT_FAILURE);
            }

            // perform transform      
            ForwardHadTrans (m_nextBlock[comp], GetBlkStride (comp), coeff, comp);

            // calculate sum of HAD coefficients (exclude DC)
            m_complexity.next[comp] = SumHadCoefficients (coeff, comp, GetTransformShift ());

            delete[] coeff;
        }

        //calculate max value of complexity
        CalculateMaxCoeffsVals ();
    }
}

/**
    Compute sum of Hadamard coefficients

    @param coeff source buffer
    @param comp component index
    @param transShift transform shift
    @return sum of Hadamard coefficients
*/
Int FlatnessDetection::SumHadCoefficients (Int* coeff, Int comp, Int transShift) {
    Int shift, shiftFinal;
    Int offset, offsetFinal;
    Int width = GetWidth (comp);
    Int height = GetHeight (comp);

    // calculate normalization parameters
    if (width == 8 && height == 2) {
        shift = (g_transformMatrixShiftHad + g_transformMatrixShiftHaar) + 2 - transShift; // shift required for orthonormality
        offset = (1 << shift) >> 1;
        shiftFinal = 4; // log2(16);
        offsetFinal = (1 << shiftFinal) >> 1;
    }
    else if (width == 4 && height == 2) {
        shift = (g_transformMatrixShiftHad + g_transformMatrixShiftHaar) + 1 - transShift; // shift required for orthonormality
        offset = (1 << shift) >> 1;
        shiftFinal = 3; // log2(8);
        offsetFinal = (1 << shiftFinal) >> 1;
    }
    else if (width == 4 && height == 1) {
        shift = g_transformMatrixShiftHad + 1 - transShift; // shift required for orthonormality
        offset = (1 << shift) >> 1;
        shiftFinal = 2; // log2(4);
        offsetFinal = (1 << shiftFinal) >> 1;
    }

    //normalize transform Coefficients
    for (Int i = 0; i < width * height; i++) {
        coeff[i] = (coeff[i] + offset) >> shift;
    }

    //sum of transform coefficients 
    Int absHadCoeffs = 0;
    for (Int i = 0; i < width*height; i++) {
        if (i == 0) {
            continue; // exclude DC value
        }
        if (m_bitDepth > 8) {
            Int shift = (m_bitDepth - 8);
            Int offset = 1 << (shift - 1);
            absHadCoeffs += ((abs (coeff[i]) + offset) >> shift);
        }
        else {
            absHadCoeffs += abs (coeff[i]);
        }
    }

    if (width == 4 && height == 2) {
        absHadCoeffs = (absHadCoeffs * 362) >> 9;
    }

    //normalize (divide by number of samples in block)
    Int value = (absHadCoeffs + offsetFinal) >> shiftFinal;

    // for chroma channels complexity values are multiplied by 0.5 if it is performed in YCoCg space
    if (comp > 0 && (m_flatnessCsc == kYCoCg)) {
        value = ((value + 1) >> 1);
    }

    return value;
}

/**
    Forward Hadamard transform on current block

    @param pBlk source buffer
    @param stride block stride
    @param pCoeff destination buffer
    @param comp component index
*/
Void FlatnessDetection::ForwardHadTrans (Int* pBlk, Int stride, Int* pCoeff, Int comp) {
    Int width = GetWidth (comp);
    Int height = GetHeight (comp);
    Int *block = new Int[width * height];
    Int *coeff = new Int[width * height];

    Int shift_1st = 0;
    Int shift_2nd = 0;

    if (height == 2 && width == 8) {
        Int log2TransformSizeHad = 3; // log2(8) = 3
        Int log2TransformSizeHaar = 1; // log2(2) = 1
        Int bitDepth = g_transformMatrixShiftHad + log2TransformSizeHad + m_transBitDepth; // bit depth after stage 1
        shift_1st = Max (bitDepth - m_maxTrDynamicRange, 0); // negative value of shift is not allowed
        bitDepth = Min (m_maxTrDynamicRange, bitDepth); // clip the bit depth at stage 1 to m_maxTrDynamicRange
        shift_2nd = g_transformMatrixShiftHaar + log2TransformSizeHaar + bitDepth - m_maxTrDynamicRange;
    }
    else if (height == 2 && width == 4) {
        Int log2TransformSizeHad = 2; // log2(4) = 2
        Int log2TransformSizeHaar = 1; // log2(2) = 1
        Int bitDepth = g_transformMatrixShiftHad + log2TransformSizeHad + m_transBitDepth; // bit depth after stage 1
        shift_1st = Max (bitDepth - m_maxTrDynamicRange, 0); // negative value of shift is not allowed
        bitDepth = Min (m_maxTrDynamicRange, bitDepth); // clip the bit depth at stage 1 to m_maxTrDynamicRange
        shift_2nd = g_transformMatrixShiftHaar + log2TransformSizeHaar + bitDepth - m_maxTrDynamicRange;
    }
    else if (height == 1 && width == 4) {
        Int log2TransformSizeHad = 2; // log2(4) = 2
        Int bitDepth = g_transformMatrixShiftHad + log2TransformSizeHad + m_transBitDepth; // bit depth after stage 1
        shift_1st = Max (bitDepth - m_maxTrDynamicRange, 0); // negative value of shift is not allowed
        shift_2nd = 0;  // no transform is applied along cols
    }    
	
	m_transformShift = shift_1st + shift_2nd; // shifts required to maintain the dynamic range

    // Prepare the samples to be input to the forward transform module
    for (Int i = 0; i < height; i++) {
        for (Int j = 0; j < width; j++) {
            block[j + (i * width)] = (Int)*(pBlk + j + (i * stride));
        }
    }

    //**************** transform along the Rows **************************//
    if (width == 4) {
        CalHad4 (block, coeff, shift_1st, height);
    }
    else if (width == 8) {
        CalHad8 (block, coeff, shift_1st, height);
    }

    //**************** transform along the Cols **************************//
    if (height == 2) {
        Haar2 (coeff, block, shift_2nd, width);
    }
    else if (height == 1) {
        //just copy the coefficient values to the block array 
        std::memcpy (block, coeff, sizeof (Int)*width*height);
    }

    // ouput the coefficients
    for (Int j = 0; j < (height * width); j++) {
        pCoeff[j] = block[j];
    }

    delete[] block;
    delete[] coeff;
}

/**
    Compute the forward Haar transform

    @param src source buffer
    @param dst destination buffer
    @param shift final shift right
    @param line line index in current blockline
*/
Void FlatnessDetection::Haar2 (Int *src, Int *dst, Int shift, Int line) {
    Int add = 0;
    if (shift > 0) {
        add = 1 << (shift - 1);
    }
    for (Int j = 0; j < line; j++) {
        dst[0] = (src[0] + src[1] + add) >> shift;
        dst[line] = (src[0] - src[1] + add) >> shift;
        src += 2;
        dst++;
    }
}

/**
    Compute the 8-point forward Hadamard transform

    @param src source buffer
    @param dst destination buffer
    @param shiftRight final right shift on HAD coefficients
    @param line line index in current blockline
*/
Void FlatnessDetection::CalHad8 (Int* src, Int* dst, Int shiftRight, Int line) {
    Int s[4], d[4], e[4], f[4];

    // at the end values will be shifted for take care of normalization
    Int shiftLeft = g_transformMatrixShiftHad;

    Int add = 0;
    if (shiftRight > 0) {
        add = 1 << (shiftRight - 1);
    }

    for (Int j = 0; j < line; j++) {
        s[0] = src[0] + src[7];
        s[1] = src[1] + src[6];
        s[2] = src[2] + src[5];
        s[3] = src[3] + src[4];
        d[0] = src[0] - src[7];
        d[1] = src[1] - src[6];
        d[2] = src[2] - src[5];
        d[3] = src[3] - src[4];

        e[0] = s[0] + s[1];
        e[1] = s[2] + s[3];
        e[2] = d[0] + d[1];
        e[3] = d[2] + d[3];
        f[0] = s[0] - s[1];
        f[1] = s[2] - s[3];
        f[2] = d[0] - d[1];
        f[3] = d[2] - d[3];

        dst[0] = (e[0] + e[1]) << (shiftLeft);
        dst[line] = (f[2] + f[3]) << (shiftLeft);
        dst[2 * line] = (e[2] - e[3]) << (shiftLeft);
        dst[3 * line] = (f[0] - f[1]) << (shiftLeft);
        dst[4 * line] = (e[2] + e[3]) << (shiftLeft);
        dst[5 * line] = (f[0] + f[1]) << (shiftLeft);
        dst[6 * line] = (e[0] - e[1]) << (shiftLeft);
        dst[7 * line] = (f[2] - f[3]) << (shiftLeft);

        // shiftRight ensures final bitwidth is <= m_maxTrDynamicRange
        for (Int i = 0; i < 8; i++) {
            dst[i * line] = (dst[i * line] + add) >> shiftRight;
        }

        dst += 1;
        src += 8;
    }
}

/**
    Compute the 4-point forward Hadamard transform

    @param src source buffer
    @param dst destination buffer
    @param shiftRight final shift right on HAD coefficients
    @param line line index in current blockline
*/
Void FlatnessDetection::CalHad4 (Int* src, Int* dst, Int shiftRight, Int line) {
    Int s[2], d[2];

    // at the end values will be shifted for take care of normalization
    Int shiftLeft = g_transformMatrixShiftHad;

    Int add = 0;
    if (shiftRight > 0) {
        add = 1 << (shiftRight - 1);
    }

    for (Int j = 0; j < line; j++) {
        s[0] = src[0] + src[1];
        s[1] = src[2] + src[3];
        d[0] = src[0] - src[1];
        d[1] = src[2] - src[3];

        dst[0 * line] = (s[0] + s[1]) << shiftLeft;
        dst[1 * line] = (d[0] + d[1]) << shiftLeft;
        dst[2 * line] = (s[0] - s[1]) << shiftLeft;
        dst[3 * line] = (d[0] - d[1]) << shiftLeft;

        // shiftRight ensures final bitwidth is <=m_maxTrDynamicRange
        dst[0 * line] = (dst[0 * line] + add) >> shiftRight;
        dst[1 * line] = (dst[1 * line] + add) >> shiftRight;
        dst[2 * line] = (dst[2 * line] + add) >> shiftRight;
        dst[3 * line] = (dst[3 * line] + add) >> shiftRight;

        dst += 1;
        src += 4;
    }
}