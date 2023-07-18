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

#ifndef __BP_SEARCH_RANGE_H__
#define __BP_SEARCH_RANGE_H__

#include "TypeDef.h"

enum BpPartitionType { k2x1, k2x2 };
enum BpPartitionHalf { kLower, kUpper };

class BpSearchRange {
protected:
    Bool m_enable;                  // may need to be able to disable a search range for some reason
    Bool m_isPrevRecLine;           // indicates if this search range is from the previous rec. line
    Int m_maxNumPositions;
    Int m_numPositions;             // this will match the # of possible vectors for the search range
    Int m_extraPixels;              // extra storage equal to (partition_width - 1)
    UInt m_numRows;                 // number of rows per "blockline"
    ColorSpace m_srcCsc;            // source colorspace
    ColorSpace m_modeCsc;           // BP mode operates in this colorspace
    ChromaFormat m_chromaFormat;    // source chroma format
    Int **m_data[g_numComps];       // buffer containing the search range pixel data

public:
    BpSearchRange (Int maxNumPos, Int numRow, ColorSpace srcCsc, ColorSpace modeCsc, ChromaFormat cf, Bool isPrevRecLine);
    ~BpSearchRange ();

    // methods
    Void AllocateStorage ();
    Void ClearStorage ();
    Void WriteBufferCompConstValue (Int comp, Int value);
    Void DataBufferRgbToYCoCg ();
	
    //Void DataBufferYCoCgToRgb ();

    // getters
    Bool GetIsPrevRecLine () { return m_isPrevRecLine; }
    ColorSpace GetSrcCsc () { return m_srcCsc; }
    ColorSpace GetModeCsc () { return m_modeCsc; }
    Int GetNumRows (Int comp) { return m_numRows >> g_compScaleY[comp][m_chromaFormat]; }
    Int GetNumPos () { return m_numPositions; }
    UInt GetPxPerRow (Int comp);
    Int* GetDataBuffer (Int comp, Int row) { return m_data[comp][row]; }
    Void SetNumPos (Int numPos) { m_numPositions = numPos; }
};

#endif // __BP_SEARCH_RANGE_H__