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

#include "PsnrParams.h"

using namespace std;

static void show_usage()
{
  // update later
  std::cerr << "diff_image_psnr.exe -origFile <input file name>" << std::endl;
}

Bool PsnrParams::parseCfg( Int argc, Char* argv[] )
{

  Bool origFound=false, recFound=false, diffFound=false, widthFound=false, heightFound=false;
  //string s;
  //std::stringstream ss;

  if (argc < 11) {
    std::cerr << "\nNumber of command line arguments is less than required!\n";
		show_usage();
		exit(EXIT_FAILURE);
  }

  // Set the defaults (moved to constructor)

  /*m_bitDepth        = 8;
  m_chromaFormat    = 2;    // Default: 2 - 4:4:4 ( 0 - 4:2:0, 1 - 422, 2 - 444 ) 
  m_isInterleaved   = true;
  m_scaleFactor     = 1;*/

  for (Int n=1; n < argc; n++) {
    std::string arg = argv[n];
    if ((arg == "-h") || (arg == "--help")) {
      show_usage();
      return 0;
    }
    else if (arg == "-origFile")
    {
      if ((n+1) < argc) // Make sure we aren't at the end of argv!
      {
        setOrigFile(argv[++n]);
        origFound = true;
      }
      else // An argument is required
      {
        std::cerr << "-inFile option requires one argument." << std::endl;
		    exit(EXIT_FAILURE);
      }  
    }
    else if (arg == "-diffFile")
    {
      if ((n+1) < argc) // Make sure we aren't at the end of argv!
      {
        setDiffFile(argv[++n]);
        diffFound = true;
      }
      else // An argument is required
      {
        std::cerr << "-diffFile option requires one argument." << std::endl;
		    exit(EXIT_FAILURE);
      }  
    }
    else if (arg == "-recFile")
    {
      if ((n+1) < argc) // Make sure we aren't at the end of argv!
      {
        setRecFile(argv[++n]);
        recFound = true;
      }
      else // An argument is required
      {
        std::cerr << "-recFile option requires one argument." << std::endl;
 		    exit(EXIT_FAILURE);
      }  
    }
    else if (arg == "-width")
    {
      if ((n+1) < argc) // Make sure we aren't at the end of argv!
      {
        m_srcWidth = atoi(argv[++n]);
        widthFound = true;
      }
      else // An argument is required
      {
        std::cerr << "-width option requires one argument." << std::endl;
 		    exit(EXIT_FAILURE);
      }  
    }
    else if (arg == "-height")
    {
      if ((n+1) < argc) // Make sure we aren't at the end of argv!
      {
        m_srcHeight = atoi(argv[++n]);
        heightFound = true;
      }
      else // An argument is required
      {
        std::cerr << "-height option requires one argument." << std::endl;
		    exit(EXIT_FAILURE);
      }  
    }
    else if (arg == "-bitDepth")
    {
      if ((n+1) < argc) // Make sure we aren't at the end of argv!
      {
        m_bitDepth = atoi(argv[++n]);
      }
      else // An argument is required
      {
        std::cerr << "-bitDepth option requires one argument." << std::endl;
		    exit(EXIT_FAILURE);
      }  
    }
    else if (arg == "-chromaFormat")
    {
      if ((n+1) < argc) // Make sure we aren't at the end of argv!
      {
        m_chromaFormat = atoi(argv[++n]);
      }
      else // An argument is required
      {
        std::cerr << "-chromaFormat option requires one argument." << std::endl;
		    exit(EXIT_FAILURE);
      }  
    }
    else if (arg == "-scaleFactor")
    {
      if ((n+1) < argc) // Make sure we aren't at the end of argv!
      {
        m_scaleFactor = atoi(argv[++n]);
      }
      else // An argument is required
      {
        std::cerr << "-scaleFactor option requires one argument." << std::endl;
		    exit(EXIT_FAILURE);
      }  
    }
    else
    {
      show_usage();
		  exit(EXIT_FAILURE);
    }
  }

  // Check validity of command-line arguments

  if ( !( origFound && diffFound && recFound && widthFound && heightFound ) )
  {
    show_usage();
		exit(EXIT_FAILURE);
  }

  return 0;
}
