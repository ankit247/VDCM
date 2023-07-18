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
\file       FlatnessDetection.h
\brief      Flatness Detection (header)
*/

#ifndef __FLATNESS__
#define __FLATNESS__

#include "TypeDef.h"
#include "Block.h"

class FlatnessDetection
{
protected:
    Int m_r;
    Int m_c;
    Int m_numBlksX;
    Int m_numBlksY;
    Int m_sliceStartRow;
    ColorSpace m_flatnessCsc;
    ColorSpace m_origSrcCsc;
    ChromaFormat m_chromaFormat;
    Int m_blkWidth;
    Int m_blkHeight;
    Int m_blkStride;
    Int m_bitDepth;
    Int m_numPx;
    Int m_maxCoeffVal;                      // max. complexity in a line 
    ComplexityStruct m_complexity;
    Bool m_isPrevBlkComplex;                // true if previous block is complex
    Bool m_isNextBlkFlat;                   // true if next block is flat
    Bool m_flatToComplex[3];
    Int* m_curBlock[g_numComps];
    Int* m_nextBlock[g_numComps];
    Int m_transformShift;		            // shifts required to maintain the dynamicrange after transform stage
    Int m_maxTrDynamicRange;                // maximum dynamic range for transform mode
    Int m_transBitDepth;                    // bit depth used in transform account for colorspace conversion
    FlatnessType m_curFlatnessType;         // flatness type for current block

public:
    FlatnessDetection (Int blkWidth, Int blkHeight, Int bitDepth, ColorSpace csc, ChromaFormat chromaFormat)
        : m_r (0)
        , m_c (0)
        , m_numBlksX (0)
        , m_numBlksY (0)
        , m_sliceStartRow (0)
        , m_numPx (0)
        , m_bitDepth (bitDepth)
        , m_isPrevBlkComplex (false)
        , m_isNextBlkFlat (false)
        , m_blkWidth (blkWidth)
        , m_blkHeight (blkHeight)
        , m_origSrcCsc (csc)
        , m_chromaFormat (chromaFormat)
        , m_blkStride (blkWidth)
    {
        Reset ();
        m_maxTrDynamicRange = (g_minBpc <= 10) ? g_maxTrDynamicRange[0] : g_maxTrDynamicRange[1];
        for (Int i = 0; i < 3; i++) {
            m_flatToComplex[i] = false;
        }
        for (Int k = 0; k < g_numComps; k++) {
            m_curBlock[k] = new Int[(m_blkWidth*m_blkHeight) >> (g_compScaleX[k][m_chromaFormat] + g_compScaleY[k][m_chromaFormat])];
            m_nextBlock[k] = new Int[(m_blkWidth*m_blkHeight) >> (g_compScaleX[k][m_chromaFormat] + g_compScaleY[k][m_chromaFormat])];
        }
    }
    ~FlatnessDetection () {
        for (Int k = 0; k < g_numComps; k++) {
            delete[] m_curBlock[k];
            delete[] m_nextBlock[k];
        }
    };

    // methods
    Void PartialReset ();
    Void Reset ();
    Void CalcComplexity ();
    Void InitCurBlock (Block *block);
    Void InitNextBlock (Block* blockNext);
    Void Classify ();
    Void UpdatePositionData (Int r, Int c, Int numBlksX, Int numBlksY);
    Void ShiftComplexityValues ();
    Void InitMaxCoeffsVals ();
    Void CalculateMaxCoeffsVals ();
    Int SumHadCoefficients (Int* coeff, Int comp, Int transShift);
    Void ApplyRgbToYCoCgTr (Int* pR, Int* pG, Int* pB);
    Void ForwardHadTrans (Int* pBlk, Int stride, Int* pCoeff, Int comp);
    Void Haar2 (Int *src, Int *dst, Int shift, Int line);
    Void CalHad8 (Int* src, Int* dst, Int shiftRight, Int line);
    Void CalHad4 (Int* src, Int* dst, Int shiftRight, Int line);

    // setters
    Void SetFlatnessDetectionColorSpace (ColorSpace x) { m_flatnessCsc = x; }
    
    // getters
    FlatnessType GetFlatnessType () { return m_curFlatnessType; }
    Int* GetCurBlockBuf (Int k) { return m_curBlock[k]; }
    Int* GetNextBlockBuf (Int k) { return m_nextBlock[k]; }
    Int GetWidth (const Int compID) { return m_blkWidth >> g_compScaleX[compID][m_chromaFormat]; }
    Int GetBlkStride (const Int compID) { return m_blkStride >> g_compScaleX[compID][m_chromaFormat]; }
    Int GetHeight (const Int compID) { return m_blkHeight >> g_compScaleY[compID][m_chromaFormat]; }
    Int GetTransformShift () { return m_transformShift; }
};

#endif // __FLATNESS__