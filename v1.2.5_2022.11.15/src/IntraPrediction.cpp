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
\file       IntraPrediction.cpp
\brief      Intra Prediction
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <memory.h>
#include "IntraPrediction.h"
#include "Misc.h"
#include <cstring>

/**
    IntraPred constructor

    @param width block width
    @param height block height
    @param maxWidth block stride
    @param chromaformat chroma-sampling format
    */
IntraPred::IntraPred (Int width, Int height, Int bitDepth, ChromaFormat chromaformat, ColorSpace csc) {
    m_csc = csc;
    m_chromaFormat = chromaformat;
    m_blkHeight = height;
    m_blkWidth = width;
    m_blkStride = width;
    m_bitDepth = bitDepth;
    m_isFirstLineInSlice = false;

    for (Int c = 0; c < g_numComps; c++) {
        Int width = GetWidth (c);
        Int height = GetHeight (c);
        m_blkLeftNeigh[c] = (Int *)xMalloc (Int, height << 1); // The two columns to the left may be used for prediction
        m_blkAboveNeigh[c] = (Int *)xMalloc (Int, width << 1); // The rows to the above may be used for prediction
        m_blkLeftAboveNeigh[c] = (Int *)xMalloc (Int, height + 3); // The above left sample may be used for prediction
        m_blkDcPred[c] = (Int *)xMalloc (Int, width * height);
        m_blkVerPred[c] = (Int *)xMalloc (Int, width * height);
        m_blkVerLeftPred[c] = (Int *)xMalloc (Int, width * height);
        m_blkVerRightPred[c] = (Int *)xMalloc (Int, width * height);
        m_blkDiagLeftPred[c] = (Int *)xMalloc (Int, width * height);
        m_blkDiagRightPred[c] = (Int *)xMalloc (Int, width * height);
        m_blkHorLeftPred[c] = (Int*)xMalloc (Int, width * height);
        m_blkHorRightPred[c] = (Int*)xMalloc (Int, width * height);
        m_blkFblsPred[c] = (Int*)xMalloc (Int, width * height);
    }
    m_bestNModes = (Int*)xMalloc (Int, g_intra_numModesSecondPass);
}

/**
    Load spatial neighbors into IntraPred class from Mode class

    @param pSrcLeft neighbors left of current block
    @param pSrcAbove neighbors above current block
    @param pSrcLeftAbove neighbors above and left of current block
    @param comp component index
    */
Void IntraPred::InitNeighbours (Int* pSrcLeft, Int* pSrcAbove, Int* pSrcLeftAbove, Int comp) {
    // copy the left neighbours
    Int* ptrDes;
    ptrDes = GetBufNeighborsLeft (comp);
    memcpy (ptrDes, pSrcLeft, sizeof (Int)*(GetHeight (comp) << 1));

    // copy the above neighbours
    ptrDes = GetBufNeighborsAbove (comp);
    memcpy (ptrDes, pSrcAbove, sizeof (Int)*(GetWidth (comp) << 1));

    // copy the aboveLeft neighbours
    ptrDes = GetBufNeighborsLeftAbove (comp);
    memcpy (ptrDes, pSrcLeftAbove, 4 * sizeof (Int));
}

/**
    Clear buffers
    */
Void IntraPred::Destroy () {
    for (Int c = 0; c < g_numComps; c++) {
        if (m_blkLeftNeigh[c]) { xFree (m_blkLeftNeigh[c]); m_blkLeftNeigh[c] = NULL; }
        if (m_blkAboveNeigh[c]) { xFree (m_blkAboveNeigh[c]); m_blkAboveNeigh[c] = NULL; }
        if (m_blkLeftAboveNeigh[c]) { xFree (m_blkLeftAboveNeigh[c]); m_blkLeftAboveNeigh[c] = NULL; }
        if (m_blkDcPred[c]) { xFree (m_blkDcPred[c]); m_blkDcPred[c] = NULL; }
        if (m_blkVerPred[c]) { xFree (m_blkVerPred[c]); m_blkVerPred[c] = NULL; }
        if (m_blkVerLeftPred[c]) { xFree (m_blkVerLeftPred[c]); m_blkVerLeftPred[c] = NULL; }
        if (m_blkVerRightPred[c]) { xFree (m_blkVerRightPred[c]); m_blkVerRightPred[c] = NULL; }
        if (m_blkDiagLeftPred[c]) { xFree (m_blkDiagLeftPred[c]); m_blkDiagLeftPred[c] = NULL; }
        if (m_blkDiagRightPred[c]) { xFree (m_blkDiagRightPred[c]); m_blkDiagRightPred[c] = NULL; }
        if (m_blkHorLeftPred[c]) { xFree (m_blkHorLeftPred[c]); m_blkHorLeftPred[c] = NULL; }
        if (m_blkHorRightPred[c]) { xFree (m_blkHorRightPred[c]); m_blkHorRightPred[c] = NULL; }
        if (m_blkFblsPred[c]) {xFree(m_blkFblsPred[c]); m_blkFblsPred[c] = NULL;}
    }
    if (m_bestNModes) { xFree (m_bestNModes); m_bestNModes = NULL; }
}

/**
    Perform intra prediction

    @param predictor
    */
Void IntraPred::PerfPrediction (IntraPredictorType predictor) {
    switch (predictor) {
    case kIntraDc:
        IntraPredictionDc ();
        break;
    case kIntraVertical:
        IntraPredictionVertical ();
        break;
    case kIntraDiagonalRight:
        IntraPredictionDiagonalRight ();
        break;
    case kIntraDiagonalLeft:
        IntraPredictionDiagonalLeft ();
        break;
    case kIntraVerticalLeft:
        IntraPredictionVerticalLeft ();
        break;
    case kIntraVerticalRight:
        IntraPredictionVerticalRight ();
        break;
    case kIntraHorizontalRight:
        IntraPredictionHorizontalRight ();
        break;
    case kIntraHorizontalLeft:
        IntraPredictionHorizontalLeft ();
        break;
    case kIntraFbls:
        IntraPredictionFbls ();
        break;
    default:
        fprintf (stderr, "IntraPrediction::perfPrediction(): unsupported intra predictor: %d.\n", predictor);
        exit (EXIT_FAILURE);
    }
}

/**
    Determine best N predictors among all intra predictors

    @param pR pointer to component 0
    @param pG pointer to component 1
    @param pB pointer to component 2
    */
Void IntraPred::FindBestNPredictors (Int* pR, Int* pG, Int* pB) {
    Int sad[g_numComps];
    Int weightedSAD[g_intra_numTotalModes];
    Int* ptrSrc;
    Int* ptrOrg;

    //initiliaze every mode to -1
    for (Int i = 0; i < g_intra_numModesSecondPass; i++) {
        m_bestNModes[i] = -1;
    }

    for (Int predIndex = 0; predIndex < g_intra_numTotalModes; predIndex++) {
        weightedSAD[predIndex] = LARGE_INT;
    }

    for (Int predIndex = 0; predIndex < g_intra_numTotalModes; predIndex++) {
        for (Int c = 0; c < g_numComps; c++) {
            switch (predIndex) {
            case kIntraDc:
                ptrSrc = GetBufIntraDc (c);
                break;
            case kIntraVertical:
                ptrSrc = GetBufIntraVertical (c);
                break;
            case kIntraDiagonalRight:
                ptrSrc = GetBufIntraDiagonalRight (c);
                break;
            case kIntraDiagonalLeft:
                ptrSrc = GetBufIntraDiagonalLeft (c);
                break;
            case kIntraVerticalLeft:
                ptrSrc = GetBufIntraVerticalLeft (c);
                break;
            case kIntraVerticalRight:
                ptrSrc = GetBufIntraVerticalRight (c);
                break;
            case kIntraHorizontalRight:
                ptrSrc = GetBufIntraHorizontalRight (c);
                break;
            case kIntraHorizontalLeft:
                ptrSrc = GetBufIntraHorizontalLeft (c);
                break;
            default:
                printf ("Unsupported prediction index");
                exit (EXIT_FAILURE);
            }
            ptrOrg = (c == 0) ? pR : ((c == 1) ? pG : pB);
            sad[c] = CalSAD (ptrSrc, ptrOrg, c);
        }
        weightedSAD[predIndex] = sad[0] + sad[1] + sad[2];
    }

    // sort all the SAD and pick the minimum
    SortSAD (weightedSAD);
}

/**
    Compute difference between two buffers

    @param pSrc1 first buffer
    @param pSrc2 second buffer
    @param pDes destination buffer
    @param height block height
    @param width block width
    @param stride block stride
    */
Void IntraPred::CalDiff (Int* pSrc1, Int* pSrc2, Int* pDes, Int height, Int width, Int stride) {
    for (Int y = 0; y < height; y++) {
        for (Int x = 0; x < width; x++) {
            pDes[x] = (Int)(pSrc1[x] - pSrc2[x]);
        }
        pSrc1 += stride;
        pSrc2 += stride;
        pDes += stride;
    }
}

/**
    Compute the sum of absolute values of difference array (used for SAD)

    @param pSrc difference array
    @param height block height
    @param width block width
    @param stride block stride
    @return absolute sum of difference array
    */
Int IntraPred::CalAbsSum (Int* pSrc, Int height, Int width, Int stride) {
    Int distortion = 0;
    for (Int y = 0; y < height; y++) {
        for (Int x = 0; x < width; x++) {
            distortion += abs (pSrc[x]);
        }
        pSrc += stride;
    }
    return distortion;
}

/**
    Compute SAD between intra predicted block and source block

    @param recBlk buffer for predicted block
    @param pOrgBlk buffer for source block
    @param k component index
    @return SAD for current intra predictor
    */
Int IntraPred::CalSAD (Int* recBlk, Int* pOrgBlk, Int k) {
    Int stride = GetBlkStride (k);
    Int height = GetHeight (k);
    Int width = GetWidth (k);
    Int* pBlk = recBlk;
    Int* pDiff = new Int[height*width];
    CalDiff (pOrgBlk, pBlk, pDiff, height, width, stride);
    Int distortion = CalAbsSum (pDiff, height, width, stride);
    delete[] pDiff;
    return(distortion);
}

/**
    Sort intra predictors by SAD w.r.t. current block

    @param weightedSAD distortion for each intra predictor
    */
Void IntraPred::SortSAD (Int* weightedSAD) {
	Int idxArr[g_intra_numTotalModes];
	memset (idxArr, -1, g_intra_numTotalModes * sizeof (Int));

	Bool isCounted[g_intra_numTotalModes];
	memset (isCounted, false, g_intra_numTotalModes * sizeof (Bool));
	for (UInt i = 0; i < g_intra_numTotalModes; i++) {
		Int min = LARGE_INT;
		UInt pos = 0;
		for (UInt j = 0; j < g_intra_numTotalModes; j++) {
			if (!isCounted[j] && weightedSAD[j] < min) {
				pos = j;
				min = weightedSAD[j];
			}
		}
		idxArr[i] = pos;
		isCounted[pos] = true;
	}

    //! copy N best modes !//
    for (Int i = 0; i < g_intra_numModesSecondPass; i++) {
        m_bestNModes[i] = idxArr[i];
    }
}

/**
    Copy selected intra predictor to Mode predicted buffer

    @param ptrDes destination buffer
    @param predIndex intra predictor index
    @param k component index
    */
Void IntraPred::AssignPredValue (Int* ptrDes, IntraPredictorType predIndex, Int k) {
    if (predIndex == kIntraUndefined) {
        fprintf (stderr, "IntraPrediction::assignPredValue(): Intra predictor undefined.\n");
        exit (EXIT_FAILURE);
    }
    Int* ptrSrc;
    switch (predIndex) {
    case kIntraDc:
        ptrSrc = GetBufIntraDc (k);
        break;
    case kIntraVertical:
        ptrSrc = GetBufIntraVertical (k);
        break;
    case kIntraDiagonalRight:
        ptrSrc = GetBufIntraDiagonalRight (k);
        break;
    case kIntraDiagonalLeft:
        ptrSrc = GetBufIntraDiagonalLeft (k);
        break;
    case kIntraVerticalLeft:
        ptrSrc = GetBufIntraVerticalLeft (k);
        break;
    case kIntraVerticalRight:
        ptrSrc = GetBufIntraVerticalRight (k);
        break;
    case kIntraHorizontalRight:
        ptrSrc = GetBufIntraHorizontalRight (k);
        break;
    case kIntraHorizontalLeft:
        ptrSrc = GetBufIntraHorizontalLeft (k);
        break;
    case kIntraFbls:
        ptrSrc = GetBufIntraFbls (k);
        break;
    default:
        fprintf (stderr, "IntraPrediction::assignPredValue(): Unsupported prediction index: %d.\n", predIndex);
        exit (EXIT_FAILURE);
    }
    memcpy (ptrDes, ptrSrc, GetWidth (k) * GetHeight (k) * sizeof (Int));
}

/**
    Perform intra prediction: FBLS
    */
Void IntraPred::IntraPredictionFbls () {
    for (UInt k=0; k<g_numComps; k++) {
        Int* ptrDes = GetBufIntraFbls(k);
        Int meanValue = (m_csc == kYCoCg && k > 0 ? 0 : (1 << (m_bitDepth - 1)));
        for (Int x = 0; x < GetWidth (k) * GetHeight (k); x++) {
            ptrDes[x] = meanValue;
        }
    }
}

/**
    Perform intra prediction: DC
    */
Void IntraPred::IntraPredictionDc () {
    // compute the dc value from the above neighbors
    for (Int c = 0; c < g_numComps; c++) {
        Int* ptrleft = GetBufNeighborsLeft (c) + 1;
        Int* ptrAbove = GetBufNeighborsAbove (c);
        Int* ptrDes = GetBufIntraDc (c);
        Int sum = 0;
        Int meanValue = 0;

        //for non-first line in slice it is based on average of above pixels
        for (Int j = 0; j < GetWidth (c); j++) {
            sum += *ptrAbove;
            ptrAbove += 1;
        }

        if (GetWidth (c) == 8) {
            meanValue = sum >> 3;
        }
        else if (GetWidth (c) == 4) {
            meanValue = sum >> 2;
        }
        else {
            std::cout << "Unsupported transform width for DC Intra Prediction" << std::endl;
            exit (EXIT_FAILURE);
        }

        // set the mean value
        for (Int j = 0; j < GetWidth (c)*GetHeight (c); j++) {
            ptrDes[j] = meanValue;
        }
    }
}

/**
    Perform intra prediction:  Vertical
    */
Void IntraPred::IntraPredictionVertical () {
    for (Int c = 0; c < g_numComps; c++) {
        Int* ptrSrc = GetBufNeighborsAbove (c);
        Int* ptrDes = GetBufIntraVertical (c);
        for (Int i = 0; i < GetHeight (c); i++) {
            memcpy (ptrDes, ptrSrc, GetWidth (c) * sizeof (Int));
            ptrDes += GetWidth (c);
        }
    }
}

/**
    Perform intra prediction: Vertical-Left
    */
Void IntraPred::IntraPredictionVerticalLeft () {
    for (Int c = 0; c < g_numComps; c++) {
        Int width = GetWidth (c);
        Int height = GetHeight (c);
        Int* ptrDes = GetBufIntraVerticalLeft (c);
        Int* ptrSrcAbove = GetBufNeighborsAbove (c);
        Int* ptrInterp = new Int[width];

        if (m_chromaFormat == k444 || c == 0) {
            for (Int j = 0; j < height; j++) {
                for (Int w = 0; w < width; w++) {
                    if (j == 0) {
                        Int A, B;
                        A = ptrSrcAbove[w];
                        B = ptrSrcAbove[w + 1];
                        ptrInterp[w] = (A + B + 1) >> 1;
                    }
                    else {
                        Int A, B, C;
                        A = ptrSrcAbove[w];
                        B = ptrSrcAbove[w + 1];
                        C = ptrSrcAbove[w + 2];
                        ptrInterp[w] = (A + (B << 1) + C + 2) >> 2;
                    }
                }
                Int *ptrSrc;
                ptrSrc = &ptrInterp[0];
                memcpy (ptrDes, ptrSrc, width * sizeof (Int));
                ptrDes += width;
            }
        }
        else {
            Int AB = (ptrSrcAbove[0] + ptrSrcAbove[1] + 1) >> 1;
            Int BC = (ptrSrcAbove[1] + ptrSrcAbove[2] + 1) >> 1;
            Int CD = (ptrSrcAbove[2] + ptrSrcAbove[3] + 1) >> 1;
            Int DE = (ptrSrcAbove[3] + ptrSrcAbove[4] + 1) >> 1;
            switch (m_chromaFormat) {
                case k422:
                    ptrDes[0] = (ptrSrcAbove[0] + AB + 1) >> 1;
                    ptrDes[1] = (ptrSrcAbove[1] + BC + 1) >> 1;
                    ptrDes[2] = (ptrSrcAbove[2] + CD + 1) >> 1;
                    ptrDes[3] = (ptrSrcAbove[3] + DE + 1) >> 1;
                    ptrDes[4] = (ptrSrcAbove[0] + (AB << 1) + ptrSrcAbove[1] + 2) >> 2;
                    ptrDes[5] = (ptrSrcAbove[1] + (BC << 1) + ptrSrcAbove[2] + 2) >> 2;
                    ptrDes[6] = (ptrSrcAbove[2] + (CD << 1) + ptrSrcAbove[3] + 2) >> 2;
                    ptrDes[7] = (ptrSrcAbove[3] + (DE << 1) + ptrSrcAbove[4] + 2) >> 2;
                    break;
                case k420:
                    ptrDes[0] = AB;
                    ptrDes[1] = BC;
                    ptrDes[2] = CD;
                    ptrDes[3] = DE;
                    break;
            }
        }
        delete[] ptrInterp;
    }
}

/**
    Perform intra prediction: Vertical-Right
    */
Void IntraPred::IntraPredictionVerticalRight () {
    for (Int c = 0; c < g_numComps; c++) {
        Int width = GetWidth (c);
        Int height = GetHeight (c);
        Int* ptrDes = GetBufIntraVerticalRight (c);
        Int* ptrSrcLeftAbove = GetBufNeighborsLeftAbove (c);
        Int* ptrSrcAbove = GetBufNeighborsAbove (c);
        Int* ptrInterp = new Int[width];
        Int* ptrTempNeigh = new Int[width + 3];

        //! copy above-left neighbors !//
        Int numSamplesAboveLeft = 4;
        for (Int i = 0; i < 3; i++) {
            ptrTempNeigh[i] = ptrSrcLeftAbove[numSamplesAboveLeft - 2 - i];
        }

        //! copy above neighbors !//
        for (Int i = 0; i < width; i++) {
            ptrTempNeigh[3 + i] = ptrSrcAbove[i];
        }

        if (m_chromaFormat == k444 || c == 0) {
            for (Int j = 0; j < height; j++) {
                for (Int i = 0; i < width; i++) {
                    if (j == 0) {
                        Int A = ptrTempNeigh[i + height];
                        Int B = ptrTempNeigh[i + height + 1];
                        ptrInterp[i] = (A + B + 1) >> 1;
                    }
                    else {
                        Int A = ptrTempNeigh[i + height - 1];
                        Int B = ptrTempNeigh[i + height];
                        Int C = ptrTempNeigh[i + height + 1];
                        ptrInterp[i] = (A + (B << 1) + C + 2) >> 2;
                    }
                }

                Int *ptrSrc;
                ptrSrc = &ptrInterp[0];
                memcpy (ptrDes, ptrSrc, width * sizeof (Int));
                ptrDes += width;
            }
        }
        else {
            Int AB = (ptrTempNeigh[2] + ptrTempNeigh[3] + 1) >> 1;
            Int BC = (ptrTempNeigh[3] + ptrTempNeigh[4] + 1) >> 1;
            Int CD = (ptrTempNeigh[4] + ptrTempNeigh[5] + 1) >> 1;
            Int DE = (ptrTempNeigh[5] + ptrTempNeigh[6] + 1) >> 1;
            switch (m_chromaFormat) {
                case k422:
                    ptrDes[0] = (AB + ptrTempNeigh[3] + 1) >> 1;
                    ptrDes[1] = (BC + ptrTempNeigh[4] + 1) >> 1;
                    ptrDes[2] = (CD + ptrTempNeigh[5] + 1) >> 1;
                    ptrDes[3] = (DE + ptrTempNeigh[6] + 1) >> 1;
                    ptrDes[4] = (ptrTempNeigh[2] + (AB << 1) + ptrTempNeigh[3] + 2) >> 2;
                    ptrDes[5] = (ptrTempNeigh[3] + (BC << 1) + ptrTempNeigh[4] + 2) >> 2;
                    ptrDes[6] = (ptrTempNeigh[4] + (CD << 1) + ptrTempNeigh[5] + 2) >> 2;
                    ptrDes[7] = (ptrTempNeigh[5] + (DE << 1) + ptrTempNeigh[6] + 2) >> 2;
                    break;
                case k420:
                    ptrDes[0] = AB;
                    ptrDes[1] = BC;
                    ptrDes[2] = CD;
                    ptrDes[3] = DE;
                    break;
            }
        }
        delete[] ptrTempNeigh;
        delete[] ptrInterp;
    }
}

/**
    Perform intra prediction: Diagonal-Left
    */
Void IntraPred::IntraPredictionDiagonalLeft () {
    for (Int c = 0; c < g_numComps; c++) {
        Int width = GetWidth (c);
        Int height = GetHeight (c);
        Int* ptrDes = GetBufIntraDiagonalLeft (c);
        Int* ptrSrcAbove = GetBufNeighborsAbove (c);
        Int* ptrInterp = new Int[width + 1];

        if (m_chromaFormat == k444 || c == 0) {
            for (Int i = 0; i < width + 1; i++) {
                Int A = ptrSrcAbove[i];
                Int B = ptrSrcAbove[i + 1];
                Int C = ptrSrcAbove[i + 2];
                ptrInterp[i] = (A + (B << 1) + C + 2) >> 2;
            }
            for (Int j = 0; j < height; j++) {
                for (Int i = 0; i < width; i++) {
                    ptrDes[i + j * width] = ptrInterp[i + j];
                }
            }
        }
        else {
            Int AB = (ptrSrcAbove[0] + ptrSrcAbove[1] + 1) >> 1;
            Int BC = (ptrSrcAbove[1] + ptrSrcAbove[2] + 1) >> 1;
            Int CD = (ptrSrcAbove[2] + ptrSrcAbove[3] + 1) >> 1;
            Int DE = (ptrSrcAbove[3] + ptrSrcAbove[4] + 1) >> 1;
            Int EF = (ptrSrcAbove[4] + ptrSrcAbove[5] + 1) >> 1;
            switch (m_chromaFormat) {
                case k422:
                    ptrDes[0] = AB;
                    ptrDes[1] = BC;
                    ptrDes[2] = CD;
                    ptrDes[3] = DE;
                    ptrDes[4] = (AB + (ptrSrcAbove[1] << 1) + BC + 2) >> 2;
                    ptrDes[5] = (BC + (ptrSrcAbove[2] << 1) + CD + 2) >> 2;
                    ptrDes[6] = (CD + (ptrSrcAbove[3] << 1) + DE + 2) >> 2;
                    ptrDes[7] = (DE + (ptrSrcAbove[4] << 1) + EF + 2) >> 2;
                    break;
                case k420:
                    ptrDes[0] = (AB + (ptrSrcAbove[1] << 1) + BC + 2) >> 2;
                    ptrDes[1] = (BC + (ptrSrcAbove[2] << 1) + CD + 2) >> 2;
                    ptrDes[2] = (CD + (ptrSrcAbove[3] << 1) + DE + 2) >> 2;
                    ptrDes[3] = (DE + (ptrSrcAbove[4] << 1) + EF + 2) >> 2;
                    break;
            }
        }
        delete[] ptrInterp;
    }
}

/**
    Perform intra prediction: Diagonal-Right
    */
Void IntraPred::IntraPredictionDiagonalRight () {
    for (Int c = 0; c < g_numComps; c++) {
        Int width = GetWidth (c);
        Int height = GetHeight (c);
        Int* ptrDes = GetBufIntraDiagonalRight (c);
        Int* ptrSrcLeftAbove = GetBufNeighborsLeftAbove (c);
        Int* ptrSrcAbove = GetBufNeighborsAbove (c);
        Int* ptrInterp = new Int[width + height - 1];
        Int* ptrTempNeigh = new Int[width + 3];

        //! copy above left neighbors !//
        Int numSamplesAboveLeft = 4;
        for (Int i = 0; i < 3; i++) {
            ptrTempNeigh[i] = ptrSrcLeftAbove[numSamplesAboveLeft - 2 - i];
        }

        //! copy above neighbors !//
        for (Int i = 0; i < width; i++) {
            ptrTempNeigh[3 + i] = ptrSrcAbove[i];
        }

        //! interpolation at 45 degrees !//
        if (m_chromaFormat == k444 || c == 0) {
            for (Int i = 0; i < height + width - 1; i++) {
                Int A = ptrTempNeigh[i];
                Int B = ptrTempNeigh[i + 1];
                Int C = ptrTempNeigh[i + 2];
                ptrInterp[i] = (A + (B << 1) + C + 2) >> 2;
            }
            for (Int j = 0; j < height; j++) {
                Int *ptrSrc;
                ptrSrc = &ptrInterp[height - 1 - j];
                memcpy (ptrDes, ptrSrc, width * sizeof (Int));
                ptrDes += width;
            }
        }
        else {
            Int AB = (ptrTempNeigh[1] + ptrTempNeigh[2] + 1) >> 1;
            Int BC = (ptrTempNeigh[2] + ptrTempNeigh[3] + 1) >> 1;
            Int CD = (ptrTempNeigh[3] + ptrTempNeigh[4] + 1) >> 1;
            Int DE = (ptrTempNeigh[4] + ptrTempNeigh[5] + 1) >> 1;
            Int EF = (ptrTempNeigh[5] + ptrTempNeigh[6] + 1) >> 1;
            switch (m_chromaFormat) {
                case k422:
                    ptrDes[0] = BC;
                    ptrDes[1] = CD;
                    ptrDes[2] = DE;
                    ptrDes[3] = EF;
                    ptrDes[4] = (AB + (ptrTempNeigh[2] << 1) + BC + 2) >> 2;
                    ptrDes[5] = (BC + (ptrTempNeigh[3] << 1) + CD + 2) >> 2;
                    ptrDes[6] = (CD + (ptrTempNeigh[4] << 1) + DE + 2) >> 2;
                    ptrDes[7] = (DE + (ptrTempNeigh[5] << 1) + EF + 2) >> 2;
                    break;
                case k420:
                    ptrDes[0] = (AB + (ptrTempNeigh[2] << 1) + BC + 2) >> 2;
                    ptrDes[1] = (BC + (ptrTempNeigh[3] << 1) + CD + 2) >> 2;
                    ptrDes[2] = (CD + (ptrTempNeigh[4] << 1) + DE + 2) >> 2;
                    ptrDes[3] = (DE + (ptrTempNeigh[5] << 1) + EF + 2) >> 2;
                    break;
            }
        }
        delete[] ptrTempNeigh;
        delete[] ptrInterp;
    }
}

/**
    Perform intra prediction: Horizontal-Left
    */
Void IntraPred::IntraPredictionHorizontalLeft () {
    for (Int c = 0; c < g_numComps; c++) {
        Int width = GetWidth (c);
        Int height = GetHeight (c);
        Int* ptrDes = GetBufIntraHorizontalLeft (c);
        Int* ptrSrcAbove = GetBufNeighborsAbove (c);
        Int* ptrInterp = new Int[width];
        Int* ptrTempNeigh = new Int[width + 4];

        //! copy above neighbors !//
        for (Int i = 0; i < width; i++) {
            ptrTempNeigh[i] = ptrSrcAbove[i];
        }

        //! copy above-right neighbors !//
        Int numSamplesAboveRight = 4;
        for (Int i = 0; i < numSamplesAboveRight; i++) {
            ptrTempNeigh[width + i] = ptrSrcAbove[width + i];
        }

        if (m_chromaFormat == k444 || c == 0) {
            for (Int j = 0; j < height; j++) {
                for (Int i = 0; i < width; i++) {
                    Int A = (j == 0) ? ptrTempNeigh[i + 1] : ptrTempNeigh[i + 2];
                    Int B = (j == 0) ? ptrTempNeigh[i + 2] : ptrTempNeigh[i + 3];
                    Int C = (j == 0) ? ptrTempNeigh[i + 3] : ptrTempNeigh[i + 4];
                    ptrInterp[i] = (A + (B << 1) + C + 2) >> 2;
                }
                memcpy (ptrDes, ptrInterp, width * sizeof (Int));
                ptrDes += width;
            }
        }
        else {
            Int AB = (ptrTempNeigh[0] + ptrTempNeigh[1] + 1) >> 1;
            Int BC = (ptrTempNeigh[1] + ptrTempNeigh[2] + 1) >> 1;
            Int CD = (ptrTempNeigh[2] + ptrTempNeigh[3] + 1) >> 1;
            Int DE = (ptrTempNeigh[3] + ptrTempNeigh[4] + 1) >> 1;
            Int EF = (ptrTempNeigh[4] + ptrTempNeigh[5] + 1) >> 1;
            switch (m_chromaFormat) {
                case k422:
                    ptrDes[0] = (AB + (ptrTempNeigh[1] << 1) + BC + 2) >> 2;
                    ptrDes[1] = (BC + (ptrTempNeigh[2] << 1) + CD + 2) >> 2;
                    ptrDes[2] = (CD + (ptrTempNeigh[3] << 1) + DE + 2) >> 2;
                    ptrDes[3] = (DE + (ptrTempNeigh[4] << 1) + EF + 2) >> 2;
                    ptrDes[4] = (ptrTempNeigh[1] + (BC << 1) + ptrTempNeigh[2] + 2) >> 2;
                    ptrDes[5] = (ptrTempNeigh[2] + (CD << 1) + ptrTempNeigh[3] + 2) >> 2;
                    ptrDes[6] = (ptrTempNeigh[3] + (DE << 1) + ptrTempNeigh[4] + 2) >> 2;
                    ptrDes[7] = (ptrTempNeigh[4] + (EF << 1) + ptrTempNeigh[5] + 2) >> 2;
                    break;
                case k420:
                    ptrDes[0] = BC;
                    ptrDes[1] = CD;
                    ptrDes[2] = DE;
                    ptrDes[3] = EF;
                    break;
            }
        }
        delete[] ptrTempNeigh;
        delete[] ptrInterp;
    }
}

/**
    Perform intra prediction: Horizontal-Right
    */
Void IntraPred::IntraPredictionHorizontalRight () {
    for (Int c = 0; c < g_numComps; c++) {
        Int width = GetWidth (c);
        Int height = GetHeight (c);
        Int* ptrDes = GetBufIntraHorizontalRight (c);
        Int* ptrSrcLeftAbove = GetBufNeighborsLeftAbove (c);
        Int* ptrSrcAbove = GetBufNeighborsAbove (c);
        Int* ptrInterp = new Int[width];
        Int* ptrTempNeigh = new Int[width + 4];

        //! copy above-left neighbors !//
        Int numSamplesAboveLeft = 4;
        for (Int i = 0; i < 4; i++) {
            ptrTempNeigh[i] = ptrSrcLeftAbove[numSamplesAboveLeft - 1 - i];
        }

        //! copy above neighbors !//
        for (Int i = 0; i < width; i++) {
            ptrTempNeigh[numSamplesAboveLeft + i] = ptrSrcAbove[i];
        }

        if (m_chromaFormat == k444 || c == 0) {
            for (Int j = 0; j < height; j++) {
                for (Int i = 0; i < width; i++) {
                    Int A = (j == 0) ? ptrTempNeigh[i + 1] : ptrTempNeigh[i];
                    Int B = (j == 0) ? ptrTempNeigh[i + 2] : ptrTempNeigh[i + 1];
                    Int C = (j == 0) ? ptrTempNeigh[i + 3] : ptrTempNeigh[i + 2];
                    ptrInterp[i] = (A + (B << 1) + C + 2) >> 2;
                }
                memcpy (ptrDes, ptrInterp, width * sizeof (Int));
                ptrDes += width;
            }
        }
        else {
            Int AB = (ptrTempNeigh[2] + ptrTempNeigh[3] + 1) >> 1;
            Int BC = (ptrTempNeigh[3] + ptrTempNeigh[4] + 1) >> 1;
            Int CD = (ptrTempNeigh[4] + ptrTempNeigh[5] + 1) >> 1;
            Int DE = (ptrTempNeigh[5] + ptrTempNeigh[6] + 1) >> 1;
            Int EF = (ptrTempNeigh[6] + ptrTempNeigh[7] + 1) >> 1;
            switch (m_chromaFormat) {
                case k422:
                    ptrDes[0] = (AB + (ptrTempNeigh[3] << 1) + BC + 2) >> 2;
                    ptrDes[1] = (BC + (ptrTempNeigh[4] << 1) + CD + 2) >> 2;
                    ptrDes[2] = (CD + (ptrTempNeigh[5] << 1) + DE + 2) >> 2;
                    ptrDes[3] = (DE + (ptrTempNeigh[6] << 1) + EF + 2) >> 2;
                    ptrDes[4] = (ptrTempNeigh[2] + (AB << 1) + ptrTempNeigh[3] + 2) >> 2;
                    ptrDes[5] = (ptrTempNeigh[3] + (BC << 1) + ptrTempNeigh[4] + 2) >> 2;
                    ptrDes[6] = (ptrTempNeigh[4] + (CD << 1) + ptrTempNeigh[5] + 2) >> 2;
                    ptrDes[7] = (ptrTempNeigh[5] + (DE << 1) + ptrTempNeigh[6] + 2) >> 2;
                    break;
                case k420:
                    ptrDes[0] = AB;
                    ptrDes[1] = BC;
                    ptrDes[2] = CD;
                    ptrDes[3] = DE;
                    break;
            }
        }
        delete[] ptrTempNeigh;
        delete[] ptrInterp;
    }
}

Int* IntraPred::GetIntraBuffer (IntraPredictorType intra, UInt k) {
	switch (intra) {
		case kIntraDc:
			return m_blkDcPred[k];
		case kIntraVertical:
			return m_blkVerPred[k];
		case kIntraDiagonalRight:
			return m_blkDiagRightPred[k];
		case kIntraDiagonalLeft:
			return m_blkDiagLeftPred[k];
		case kIntraVerticalLeft:
			return m_blkVerLeftPred[k];
		case kIntraVerticalRight:
			return m_blkVerRightPred[k];
		case kIntraHorizontalRight:
			return m_blkHorRightPred[k];
		case kIntraHorizontalLeft:
			return m_blkHorLeftPred[k];
		case kIntraFbls:
			return m_blkFblsPred[k];
		default:
			fprintf (stderr, "IntraPrediction::GetIntraBuffer(): unsupported intra predictor: %d.\n", intra);
			exit (EXIT_FAILURE);
	}
}