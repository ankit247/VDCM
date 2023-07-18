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

#include "BpSearchRange.h"
#include "TypeDef.h"
#include <cstring>

/**
    Constructor
    */
BpSearchRange::BpSearchRange (Int maxNumPos, Int numRow, ColorSpace srcCsc, ColorSpace modeCsc, ChromaFormat cf, Bool isPrevRecLine)
    : m_enable (true)
    , m_maxNumPositions (maxNumPos)
    , m_numRows (numRow)
    , m_srcCsc (srcCsc)
    , m_modeCsc (modeCsc)
    , m_chromaFormat (cf)
    , m_extraPixels (1)
    , m_isPrevRecLine (isPrevRecLine) {
    AllocateStorage ();
}

/**
    Destructor
    */
BpSearchRange::~BpSearchRange () {
    ClearStorage ();
}

/**
    Allocate storage for search range data structure
    */
Void BpSearchRange::AllocateStorage () {
    for (Int k = 0; k < g_numComps; k++) {
        m_data[k] = new Int*[m_numRows];
        for (UInt row = 0; row < m_numRows; row++) {
            //! one extra position needed for search range C !//
            //! this is for the overlap region with search range A !//
            UInt storageSize = 33 + m_extraPixels;
            m_data[k][row] = new Int[storageSize];
            std::memset (m_data[k][row], 0, storageSize * sizeof (Int));
        }
    }
}

/**
    Clear search range data structure
    */
Void BpSearchRange::ClearStorage () {
    for (UInt k = 0; k < g_numComps; k++) {
        for (UInt row = 0; row < m_numRows; row++) {
            delete[] m_data[k][row];
        }
        delete[] m_data[k];
    }
}

/**
    Write fixed constant value to given component in search range

    @param comp component index
    @param value value to write to buffer
    */
Void BpSearchRange::WriteBufferCompConstValue (Int comp, Int value) {
    Int numRows = (m_numRows >> g_compScaleY[comp][m_chromaFormat]);
    for (UInt row = 0; row < m_numRows; row++) {
        for (UInt pos = 0; pos < GetPxPerRow (comp); pos++) {
            m_data[comp][row][pos] = value;
        }
        UInt maxNumPos = (comp == 0 || m_chromaFormat == k444 ? 32 : 16) + m_extraPixels;
        if (GetPxPerRow (comp) < maxNumPos) {
            for (UInt pos = GetPxPerRow (comp); pos < maxNumPos; pos++) {
                m_data[comp][row][pos] = -1;
            }
        }
    }
}

/**
    Convert data structure from RGB to YCoCg
    */
Void BpSearchRange::DataBufferRgbToYCoCg () {
	Int Co, Cg, temp;
	Int *pR, *pG, *pB;
	for (UInt row = 0; row < m_numRows; row++) {
		for (UInt pos = 0; pos < GetPxPerRow (0); pos++) {
			pR = &m_data[0][row][pos];
			pG = &m_data[1][row][pos];
			pB = &m_data[2][row][pos];

			Co = *pR - *pB;
			temp = *pB + (Co >> 1);
			Cg = *pG - temp;

			*pR = temp + (Cg >> 1);
			*pG = Co;
			*pB = Cg;
		}
	}
}


/**
    Get the number of samples per row

    @return numPxPerRow number of pixels in current searchrange row
    */
UInt BpSearchRange::GetPxPerRow (Int comp) {
    return (m_numPositions >> g_compScaleX[m_chromaFormat][comp]) + m_extraPixels;
}