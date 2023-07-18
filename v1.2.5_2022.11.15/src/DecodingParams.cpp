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

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <math.h>

#include "DecodingParams.h"
#include "Misc.h"

using namespace std;

Void DecodingParams::ShowDecoderUsage () {
    fprintf (stdout, "VDCM_Decoder.exe -bitstream <compressed bitstream file> -recFile <reconstructed image>\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "optional flags:\n");
    fprintf (stdout, "                 -dumpPpsBytes            : dump PPS byte table.\n");
    fprintf (stdout, "                 -dumpPpsFields           : dump PPS by field.\n");
    fprintf (stdout, "                 -dumpModes               : print out the coding mode distrubition.\n");
    fprintf (stdout, "                 -debugTracer             : write debug trace to file (slow! large file!)\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "DPX flags:\n");
    fprintf (stdout, "                 -forceDpxWriteLE         : write DPX file data using little endian byte order.\n");
    fprintf (stdout, "                 -forceDpxWriteBGR        : write DPX file data using BGR color order.\n");
	fprintf (stdout, "                 -forceDpxWriteDataOffset : write DPX data offset header for RGB source content.\n");
	fprintf (stdout, "                 -forceDpxWritePadLines   : pad each line to nearest doubleword.\n");
}

Bool DecodingParams::Init (Int argc, Char* argv[]) {
    Bool status = true;
    Bool isBitstreamFound = false;
    Bool isRecImageFound = false;
    for (Int n = 1; n < argc; n++) {
        string arg = argv[n];
        if ((arg == "-h") || (arg == "--help")) {
            ShowDecoderUsage ();
            status = false;
        }
        else if (arg == "-dumpPpsFields") {
            m_ppsDumpFields = true;
        }
        else if (arg == "-dumpPpsBytes") {
            m_ppsDumpBytes = true;
        }
        else if (arg == "-debugTracer") {
            m_debugTracer = true;
        }
        else if (arg == "-dumpModes") {
            m_dumpModes = true;
        }
        else if (arg == "-forceDpxWriteBGR") {
            m_dpxOptions->forceWriteBgrOrder = true;
        }
        else if (arg == "-forceDpxWriteLE") {
            m_dpxOptions->forceWriteLittleEndian = true;
        }
		else if (arg == "-forceDpxWriteDataOffset") {
			m_dpxOptions->forceWriteDataOffset = true;
		}
		else if (arg == "-forceDpxWritePadLines") {
			m_dpxOptions->forceWritePadLines = true;
		}
        else if (arg == "-bitstream") {
            if ((n + 1) < argc) {
                m_cmpFile = argv[++n];
                isBitstreamFound = true;
            }
            else {
                cerr << "-bitstream option requires one argument." << endl;
                status = false;
            }
        }

        else if (arg == "-recFile") {
            if ((n + 1) < argc) {
                m_recFile = argv[++n];
                isRecImageFound = true;
            }
            else {
                cerr << "-recFile option requires one argument." << endl;
                status = false;
            }
        }
        else {
            ShowDecoderUsage ();
            status = false;
        }
    }

    m_fileFormatDec = ParseFileFormat (m_recFile);

    //! bitstream must be specified !//
    if (!isBitstreamFound) {
        fprintf (stderr, "DecodingParams::Init(): must specify bitstream.\n");
        ShowDecoderUsage ();
        status = true;
    }

    if (!isRecImageFound) {
        if (isBitstreamFound) {
            //! if bitstream is found, dump PPS parameters !//
            fprintf (stdout, "DecodingParams::Init(): dumping PPS data only.  Specify -recFile to decode.\n");
            m_ppsDumpOnly = true;
            m_ppsDumpBytes = true;
            m_ppsDumpFields = true;
            status = true;
        }
        else {
            //! otherwise, show proper usage !//
            fprintf (stderr, "DecodingParams::Init(): must specify location for reconstructed image.\n");
            ShowDecoderUsage ();
            status = false;
        }
    }

    return status;
}