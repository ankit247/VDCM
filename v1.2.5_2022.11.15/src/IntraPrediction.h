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
    \file       IntraPrediction.h
    \brief      Spatial intra prediction class (header)
    */

#ifndef __INTRAPREDICTION__
#define __INTRAPREDICTION__

#include "TypeDef.h"

class IntraPred
{
protected:
    Int m_blkWidth;                                 // block width in pixels
    Int m_blkHeight;                                // block height in pixels
    Int m_bitDepth;
    Int m_blkStride;                                // Maximum block width in pixels. This will be same as m_blkWidth unless the block is on the right boundary.
    ChromaFormat m_chromaFormat;                    // current chroma subsampling format
    Bool m_isFirstLineInSlice;                      // flag to indicate block is within FLS
    Int* m_bestNModes;                              // array to store the most probable modes
    Int* m_blkLeftNeigh[g_numComps];                // buffers to store the left neighbours
    Int* m_blkAboveNeigh[g_numComps];               // buffers to store the above neighbours
    Int* m_blkLeftAboveNeigh[g_numComps];           // buffers to store the left-above neighbours
    Int* m_blkDcPred[g_numComps];                   // buffers for intra predictor: DC
    Int* m_blkVerPred[g_numComps];                  // buffers for intra predictor: Vertical
    Int* m_blkVerLeftPred[g_numComps];              // buffers for intra predictor: Vertical-Left
    Int* m_blkVerRightPred[g_numComps];             // buffers for intra predictor: Vertical-Right
    Int* m_blkDiagLeftPred[g_numComps];             // buffers for intra predictor: Diagonal-Left
    Int* m_blkDiagRightPred[g_numComps];            // buffers for intra predictor: Diagonal-Right
    Int* m_blkHorLeftPred[g_numComps];              // buffers for intra predictor: Horizontal-Left
    Int* m_blkHorRightPred[g_numComps];             // buffers for intra predictor: Horizontal-Right
    Int* m_blkFblsPred[g_numComps];                 // buffers for intra predictor: FBLS
    ColorSpace m_csc;

public:
    IntraPred (Int width, Int height, Int bitDepth, ChromaFormat m_chromaFormat, ColorSpace csc);
    ~IntraPred () {
        Destroy ();
    }

    // methods
    Void InitNeighbours (Int* pSrcLeft, Int* pSrcAbove, Int* pSrcLeftAbove, Int i);
    Void Destroy ();
    Void IntraPredictionDc ();
    Void IntraPredictionVertical ();
    Void IntraPredictionVerticalLeft ();
    Void IntraPredictionVerticalRight ();
    Void IntraPredictionDiagonalLeft ();
    Void IntraPredictionDiagonalRight ();
    Void IntraPredictionHorizontalLeft ();
    Void IntraPredictionHorizontalRight ();
    Void IntraPredictionFbls ();
    Void FindBestNPredictors (Int* pR, Int* pG, Int* pB);
    Void AssignPredValue (Int* ptrDes, IntraPredictorType predIndex, Int k);
    Void PerfPrediction (IntraPredictorType predictor);
    Int CalSAD (Int* recBlk, Int* pOrgBlk, Int k);
    Void SortSAD (Int* weightedSAD);
    Void CalDiff (Int* pSrc1, Int* pSrc2, Int* pDes, Int height, Int width, Int stride);
    Int CalAbsSum (Int* pSrc, Int height, Int width, Int stride);

    // setters
    Void SetFirstLineInSlice (Bool x) { m_isFirstLineInSlice = x; }

    // getters
    Int* GetBufNeighborsLeft (const Int k) { return m_blkLeftNeigh[k]; }
    Int* GetBufNeighborsAbove (const Int k) { return m_blkAboveNeigh[k]; }
    Int* GetBufNeighborsLeftAbove (const Int k) { return m_blkLeftAboveNeigh[k]; }
    Int* GetBufIntraDc (const Int k) { return m_blkDcPred[k]; }
    Int* GetBufIntraVertical (const Int k) { return m_blkVerPred[k]; }
    Int* GetBufIntraVerticalLeft (const Int k) { return m_blkVerLeftPred[k]; }
    Int* GetBufIntraVerticalRight (const Int k) { return m_blkVerRightPred[k]; }
    Int* GetBufIntraDiagonalLeft (const Int k) { return m_blkDiagLeftPred[k]; }
    Int* GetBufIntraDiagonalRight (const Int k) { return m_blkDiagRightPred[k]; }
    Int* GetBufIntraHorizontalLeft (const Int k) { return m_blkHorLeftPred[k]; }
    Int* GetBufIntraHorizontalRight (const Int k) { return m_blkHorRightPred[k]; }
    Int* GetBufIntraFbls (const Int k) { return m_blkFblsPred[k]; }
    Int GetWidth (const Int compID) { return m_blkWidth >> g_compScaleX[compID][m_chromaFormat]; }
    Int GetBlkStride (const Int compID) { return m_blkStride >> g_compScaleX[compID][m_chromaFormat]; }
    Int GetHeight (const Int compID) { return m_blkHeight >> g_compScaleY[compID][m_chromaFormat]; }
    Int* GetMPMBuffer () { return m_bestNModes; }
	Int* GetIntraBuffer (IntraPredictorType intra, UInt k);
};


#endif // __INTRAPREDICTION__
