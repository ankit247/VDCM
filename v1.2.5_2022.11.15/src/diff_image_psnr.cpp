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

#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<iostream>
#include<fstream>
#include<string>

#include "psnrParams.h"
#include "VideoFrame.h"
#include "VideoIO.h"

using namespace std;

void PressEnterToContinue()
{
    std::cout << "Press ENTER to continue... " << flush;
    std::cin.ignore(std::numeric_limits <std::streamsize> ::max(), '\n');
}

int main(int argc, char **argv)
{
    double MSE[NUM_COMPS];
    double PSNR[NUM_COMPS];
    double meanPSNR_0;
    double meanPSNR_1;
    //double totalMSE;

    PsnrParams cp;
    if ( cp.parseCfg( argc, argv ) )
    {
        return 1;
    };

    Int height        = cp.getHeight();
    Int width         = cp.getWidth();
    Int bitDepth      = cp.getBitDepth();
    Int chromaFormat  = cp.getChromaFormat();
    Int scaleFactor   = cp.getScaleFactor();

    VideoFrame origVideoFrame;
    origVideoFrame.create( width, height, chromaFormat, 0, 0, true, bitDepth );

    VideoIO origFileVideoIO;
    origFileVideoIO.open( cp.getOrigFile(), true, bitDepth );     // read  mode
    origFileVideoIO.read( &origVideoFrame, cp.getIsInterleaved() );

    VideoFrame recVideoFrame;
    recVideoFrame.create( width, height, chromaFormat, 0, 0, true, bitDepth );

    VideoIO recFileVideoIO;
    recFileVideoIO.open( cp.getRecFile(), true, bitDepth );       // read  mode
    recFileVideoIO.read( &recVideoFrame, cp.getIsInterleaved() );

    VideoFrame diffVideoFrame;
    diffVideoFrame.create( width, height, chromaFormat, 0, 0, true, 8 );   // Difference video is always written to 8 bits

    VideoIO diffFileVideoIO;
    diffFileVideoIO.open( cp.getDiffFile(), false, 8 );            // write  mode

	
    Int maxVal = (1<<bitDepth) - 1;
    double meanMSE = 0.0F;
    for ( Int c = 0; c < NUM_COMPS; c++ )
    {
        Pel *ptrOrig  = origVideoFrame.getBuf(c);
        Pel *ptrRec   = recVideoFrame.getBuf(c);
        Pel *ptrDiff  = diffVideoFrame.getBuf(c);
        Int cWidth  = width >> compScaleX[c][chromaFormat];
        Int cHeight = height >> compScaleY[c][chromaFormat];

        double MSE_temp = 0.0F;
        Int diff;

        Int offset = 1 << (bitDepth-1);
        for ( Int i = 0; i < cHeight; i++ )
        {
            for ( Int j = 0; j < cWidth; j++ )
            {
                diff = (*(ptrOrig+j)) - (*(ptrRec+j));
                *(ptrDiff+j) = Clip3<Int>( 0, (1<<bitDepth)-1, (offset + ( scaleFactor * diff ) ) ); 
                diff = (diff < 0) ? (-diff) : diff;
                MSE_temp += (double) ( diff * diff );
            }
            ptrOrig += cWidth;
            ptrRec  += cWidth;
            ptrDiff += cWidth;
        }

        MSE_temp /= ( cWidth * cHeight );
        meanMSE += MSE_temp;
        MSE[c] = MSE_temp;
        PSNR[c] = 10.0F * log10( (double) (maxVal * maxVal) / MSE_temp );
    }
    //totalMSE = meanMSE;
    meanMSE /= NUM_COMPS;
    meanPSNR_0 = 10.0F * log10( (double) (maxVal * maxVal) / meanMSE );
    meanPSNR_1 = (PSNR[0] + PSNR[1] + PSNR[2]) / ((double)3);
    diffFileVideoIO.write( &diffVideoFrame, cp.getIsInterleaved() );
    printf("\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f", MSE[0], MSE[1], MSE[2], meanMSE, meanPSNR_0, meanPSNR_1);
#if _DEBUG
	// if in debug mode, user presses enter to continue (so they can see the PSNR results)
    //PressEnterToContinue();
#endif


	// PSNR for first line in slice
#if FIRSTLINE_STATSDUMP
	printf("\n");
	//Int maxVal = (1<<bitDepth) - 1;
	Int sliceHeight = 16;
	Int numSlices = ((height - 1) / sliceHeight) + 1; // Fractional slices are counted as full.
	Double *mseArray = new Double[numSlices*NUM_COMPS];
	::memset(mseArray, 0.0F, sizeof(Double)*(numSlices*NUM_COMPS));
	Int count = 0; 
	for ( Int c = 0; c < NUM_COMPS; c++ )
	{
		Pel *ptrOrig  = origVideoFrame.getBuf(c);
		Pel *ptrRec   = recVideoFrame.getBuf(c);
		// Pel *ptrDiff  = diffVideoFrame.getBuf(c);
		Int cWidth  = width >> compScaleX[c][chromaFormat];
		Int cHeight = height >> compScaleY[c][chromaFormat];

		//double MSE_temp = 0.0F;
		Int diff;

		Int offset = 1 << (bitDepth-1);
		for ( Int i = 0; i < cHeight; i++ )
		{
			for ( Int j = 0; j < cWidth; j++ )
			{
				diff = (*(ptrOrig+j)) - (*(ptrRec+j));
				//diff = Clip3<Int>( 0, (1<<bitDepth)-1, diff ); 
				diff = (diff < 0) ? (-diff) : diff;

				if( (i%sliceHeight) == 0)
				{
					mseArray[count] += (double) ( diff * diff );
				}
			}
			ptrOrig += cWidth;
			ptrRec  += cWidth;
			if( (i%sliceHeight) == 0)
			{
			   count += 1; 
			}
			//ptrDiff += cWidth;
		}

		//MSE_temp /= ( cWidth * cHeight );
		//meanMSE += MSE_temp;
		//MSE[c] = MSE_temp;
		//PSNR[c] = 10.0F * log10( (double) (maxVal * maxVal) / MSE_temp );
	}

	//compute the mean PSNR for each line
	Double meanPSNRFirstLine;
	for (Int i = 0; i < numSlices; i ++)
	{
		double MSE_temp = 0.0F;
		for ( Int c = 0; c < NUM_COMPS; c++ )
		{
			MSE_temp += mseArray[i + (c*numSlices)];
		}

		MSE_temp /= ( width  * NUM_COMPS );
		meanPSNRFirstLine = 10.0F * log10( (double) (maxVal * maxVal) / MSE_temp );
		printf("\n%.3f", meanPSNRFirstLine);
		//meanPSNR_1 = (PSNR[0] + PSNR[1] + PSNR[2]) / ((double)3);
		//diffFileVideoIO.write( &diffVideoFrame, cp.getIsInterleaved() );
	}
    
	delete [] mseArray; 
#endif

    // MSE in YCoCg domain
   // meanMSE = 0.0F;
    for ( Int c = 0; c < NUM_COMPS; c++ )
    {
        origVideoFrame.applyRgbToYCoCgTr ();
        recVideoFrame.applyRgbToYCoCgTr ();
        Pel *ptrOrig  = origVideoFrame.getBuf(c);
        Pel *ptrRec   = recVideoFrame.getBuf(c);
        Pel *ptrDiff  = diffVideoFrame.getBuf(c);
        bitDepth = origVideoFrame.getBitDepth(c);
        Int cWidth  = width >> compScaleX[c][chromaFormat];
        Int cHeight = height >> compScaleY[c][chromaFormat];

        double MSE_temp = 0.0F;
        Int diff;

        Int offset = 1 << (bitDepth-1);
        for ( Int i = 0; i < cHeight; i++ )
        {
            for ( Int j = 0; j < cWidth; j++ )
            {
                diff = (*(ptrOrig+j)) - (*(ptrRec+j));
                *(ptrDiff+j) = Clip3<Int>( 0, (1<<bitDepth)-1, (offset + ( scaleFactor * diff ) ) ); 
                diff = (diff < 0) ? (-diff) : diff;
                MSE_temp += (double) ( diff * diff );
            }
            ptrOrig += cWidth;
            ptrRec  += cWidth;
            ptrDiff += cWidth;
        }

        MSE_temp /= ( cWidth * cHeight );
      //  meanMSE += MSE_temp;
      //  MSE[c] = MSE_temp;
        maxVal = (1<<bitDepth) - 1;
        PSNR[c] = 10.0F * log10( (double) (maxVal * maxVal) / MSE_temp );
    }
    //totalMSE = meanMSE;
    //meanMSE /= NUM_COMPS;
    //meanPSNR_0 = 10.0F * log10( (double) (maxVal * maxVal) / meanMSE );
    //meanPSNR_1 = (PSNR[0] + PSNR[1] + PSNR[2]) / ((double)3);
    //diffFileVideoIO->write( diffVideoFrame, cp.getIsInterleaved() );
    //printf("\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n", MSE[0], MSE[1], MSE[2], meanMSE, meanPSNR_0, meanPSNR_1);
    printf("\t%.3f\t%0.3f\n", PSNR[0], (PSNR[1]+PSNR[2])/2);

    return 0;
}
