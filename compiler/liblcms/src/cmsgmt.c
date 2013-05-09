//
//  Little cms
//  Copyright (C) 1998-2000 Marti Maria
//
// THIS SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
// EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
// WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
// IN NO EVENT SHALL MARTI MARIA BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
// INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
// OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
// LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
// OF THIS SOFTWARE.
//
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "lcms.h"

// #define DEBUG 1

/*
Gamut check by default is a catching of 0xFFFF/0xFFFF/0xFFFF PCS values, used
internally by lcms to hold invalid values. Matrix LUT's, operates in a way that
unencodeable values are marked as this combination, if PCS is XYZ, this is a very
high value since encoding is a 1.15 fixed point, something like 1.9997, 1.9997, 1.9997
not a very common color after all. Lab PCS is not to be a problem, since L>100 are truely
undefined. There is a posibility than ICC comitee defines L>100 as a valid means
to use highlights, then it will be lost.
*/


BOOL _cmsEndPointsBySpace(icColorSpaceSignature Space, WORD **White, WORD **Black,
                            int *nOutputs)
{
       // Only most common spaces

       static WORD RGBblack[4]  = { 0, 0, 0 };
       static WORD RGBwhite[4]  = { 0xffff, 0xffff, 0xffff };
       static WORD CMYKblack[4] = { 0, 0, 0, 0xffff };
       static WORD CMYKwhite[4] = { 0, 0, 0, 0 };
       static WORD LABblack[4]  = { 0, 0, 0 };
       static WORD LABwhite[4]  = { 0xFF00, 0x8000, 0x8000 };


       switch (Space) {

       case icSigRgbData:  *White = RGBwhite;
                           *Black = RGBblack;
                           *nOutputs = 3;
                           return TRUE;

       case icSigLabData:  *White = LABwhite;
                           *Black = LABblack;
                           *nOutputs = 3;
                           return TRUE;

       case icSigCmykData: *White = CMYKwhite;
                           *Black = CMYKblack;
                           *nOutputs = 4;
                           return TRUE;

       default:;
       }

  return FALSE;
}


WORD *_cmsWhiteBySpace(icColorSpaceSignature Space)
{
       WORD *White= NULL, *Black = NULL;
       int Dummy;
       static WORD Default[MAXCHANNELS];

       if (_cmsEndPointsBySpace(Space, &White, &Black, &Dummy))
              return White;

       return Default;

}


WORD Clamp_XYZ(int in)
{
       if (in < 0) return 0;
       if (in > 0xFFFF) return 0xFFFFU;   // Including marker
       return (WORD) in;
}

WORD Clamp_RGB(int in)
{
       if (in < 0) return 0;
       if (in > 0xFFFF) return 0xFFFFU;   // Including marker
       return (WORD) in;
}


WORD Clamp_L(Fixed32 in)
{
       if (in == 0xFFFF) return 0xFFFFU;  // Marker

       if (in > 0xFF00) return 0xFF00U;             // L* = 100.0
       return (WORD) in;
}


#define ENCODE_AB(x) (WORD) (((x) + 128.0) * 256.0 + 0.5)

WORD Clamp_ab(Fixed32 in)
{
       if (in == 0xFFFF) return 0xFFFFU;            // Marker

       if (in < 0) return ENCODE_AB(-128.0);         // 0;         // Max negative number
       if (in > 0xFFFF) return ENCODE_AB(+127.9961); // 0xFFFF;    // Max positive number
       return (WORD) in;
}



// Returns dE on two Lab values

double LCMSEXPORT cmsDeltaE(LPcmsCIELab Lab1, LPcmsCIELab Lab2)
{
        double dL, da, db;
        
        if (Lab1 -> L < 0 ||
            Lab2 -> L < 0) return 65536.;

        if (Lab1 -> a < -200 || Lab1 -> a > 200) return 65536.;
        if (Lab1 -> b < -200 || Lab1 -> b > 200) return 65536.;

        if (Lab2 -> a < -200 || Lab2 -> a > 200) return 65536.;
        if (Lab2 -> b < -200 || Lab2 -> b > 200) return 65536.;

		if (Lab1 ->L == 0 && Lab2 ->L == 0) return 0;
		
        dL = fabs(Lab1 -> L - Lab2 -> L);
        da = fabs(Lab1 -> a - Lab2 -> a);
        db = fabs(Lab1 -> b - Lab2 -> b);

        return pow(dL*dL + da * da + db * db, 0.5);

}


// Square
static
double Sqr(double v)
{
	return v *  v; 
}

// Return the CIE94 Delta E 
double LCMSEXPORT cmsCIE94DeltaE(LPcmsCIELab Lab1, LPcmsCIELab Lab2)
{
	cmsCIELCh LCh1, LCh2;
	double dE, dL, dC, dh, dhsq;
	double c12, sc, sh;

	if (Lab1 ->L == 0 && Lab2 ->L == 0) return 0;

	dL = fabs(Lab1 ->L - Lab2 ->L);

	cmsLab2LCh(&LCh1, Lab1);
	cmsLab2LCh(&LCh2, Lab2);

	dC  = fabs(LCh1.C - LCh2.C);
	dE  = cmsDeltaE(Lab1, Lab2);
	
	dhsq = Sqr(dE) - Sqr(dL) - Sqr(dC);
	if (dhsq < 0)
		dh = 0;
	else
		dh = pow(dhsq, 0.5);

	c12 = sqrt(LCh1.C * LCh2.C);

	sc = 1.0 + (0.048 * c12);
	sh = 1.0 + (0.014 * c12);
			
	return sqrt(Sqr(dL)  + Sqr(dC) / Sqr(sc) + Sqr(dh) / Sqr(sh));
}


// Auxiliary

static
double ComputeLBFD(LPcmsCIELab Lab)
{
  double yt;



  if (Lab->L > 7.996969)
		yt = (Sqr((Lab->L+16)/116)*((Lab->L+16)/116))*100;
  else
		yt = 100 * (Lab->L / 903.3);

  return (54.6 * (LOGE * (log(yt + 1.5))) - 9.6);
}



// bfd - gets BFD(1:1) difference between Lab1, Lab2
double LCMSEXPORT cmsBFDdeltaE(LPcmsCIELab Lab1, LPcmsCIELab Lab2)
{
  double lbfd1,lbfd2,AveC,Aveh,dE,deltaL,
  deltaC,deltah,dc,t,g,dh,rh,rc,rt,bfd;
  cmsCIELCh LCh1, LCh2;


  if (Lab1 ->L == 0 && Lab2 ->L == 0) return 0;

  lbfd1 = ComputeLBFD(Lab1);
  lbfd2 = ComputeLBFD(Lab2);
  deltaL = lbfd2 - lbfd1;

  cmsLab2LCh(&LCh1, Lab1);
  cmsLab2LCh(&LCh2, Lab2);

  deltaC = LCh2.C - LCh1.C;
  AveC = (LCh1.C+LCh2.C)/2;
  Aveh = (LCh1.h+LCh2.h)/2;

  dE = cmsDeltaE(Lab1, Lab2);

  if (Sqr(dE)>(Sqr(Lab2->L-Lab1->L)+Sqr(deltaC)))
    deltah = sqrt(Sqr(dE)-Sqr(Lab2->L-Lab1->L)-Sqr(deltaC));
  else
    deltah =0;

  
  dc   = 0.035 * AveC / (1 + 0.00365 * AveC)+0.521;
  g    = sqrt(Sqr(Sqr(AveC))/(Sqr(Sqr(AveC))+14000));
  t    = 0.627+(0.055*cos((Aveh-254)/(180/M_PI))-       
		 0.040*cos((2*Aveh-136)/(180/M_PI))+
		 0.070*cos((3*Aveh-31)/(180/M_PI))+
		 0.049*cos((4*Aveh+114)/(180/M_PI))-
		 0.015*cos((5*Aveh-103)/(180/M_PI)));

  dh    = dc*(g*t+1-g);
  rh    = -0.260*cos((Aveh-308)/(180/M_PI))-
         0.379*cos((2*Aveh-160)/(180/M_PI))-
         0.636*cos((3*Aveh+254)/(180/M_PI))+
         0.226*cos((4*Aveh+140)/(180/M_PI))-
         0.194*cos((5*Aveh+280)/(180/M_PI));
  
	rc = sqrt((AveC*AveC*AveC*AveC*AveC*AveC)/((AveC*AveC*AveC*AveC*AveC*AveC)+70000000));
	rt = rh*rc;
  
	bfd = sqrt(Sqr(deltaL)+Sqr(deltaC/dc)+Sqr(deltah/dh)+(rt*(deltaC/dc)*(deltah/dh)));

	return bfd;
}


//  cmc - CMC(1:1) difference between Lab1, Lab2
double LCMSEXPORT cmsCMCdeltaE(LPcmsCIELab Lab1, LPcmsCIELab Lab2)
{
  double dE,dL,dC,dh,sl,sc,sh,t,f,cmc;
  cmsCIELCh LCh1, LCh2;

  if (Lab1 ->L == 0 && Lab2 ->L == 0) return 0;

  cmsLab2LCh(&LCh1, Lab1);
  cmsLab2LCh(&LCh2, Lab2);

  
  dL = Lab2->L-Lab1->L;
  dC = LCh2.C-LCh1.C;

  dE = cmsDeltaE(Lab1, Lab2);
  if (Sqr(dE)>(Sqr(dL)+Sqr(dC))) 
			dh = sqrt(Sqr(dE)-Sqr(dL)-Sqr(dC));
  else
			dh =0;

  if ((LCh1.h > 164) && (LCh1.h<345)) 
	  t = 0.56 + fabs(0.2 * cos(((LCh1.h + 168)/(180/M_PI))));
  else 
	  t = 0.36 + fabs(0.4 * cos(((LCh1.h + 35 )/(180/M_PI))));

   sc  = 0.0638   * LCh1.C / (1 + 0.0131  * LCh1.C) + 0.638;
   sl  = 0.040975 * Lab1->L /(1 + 0.01765 * Lab1->L);
   
   if (Lab1->L<16)
		 sl = 0.511; 

   f   = sqrt((LCh1.C * LCh1.C * LCh1.C * LCh1.C)/((LCh1.C * LCh1.C * LCh1.C * LCh1.C)+1900));
   sh  = sc*(t*f+1-f);
   cmc = sqrt(Sqr(dL/sl)+Sqr(dC/sc)+Sqr(dh/sh));

   return cmc;
}

   
	

// Carefully,  clamp on CIELab space.

void LCMSEXPORT cmsClampLab(LPcmsCIELab Lab, double amax, double amin,
                                   double bmax, double bmin)
{

            // Whole Luma surface to zero

	    if (Lab -> L < 0) {

                Lab-> L = Lab->a = Lab-> b = 0.0;
                return;
            }

            // Clamp white, DISCARD HIGHLIGHTS. This is done
            // in such way because icc spec doesn't allow the
            // use of L>100 as a highlight means.

            if (Lab->L > 100)
                        Lab -> L = 100;

            // Check out gamut prism, on a, b faces

            if (Lab -> a < amin || Lab->a > amax||
                Lab -> b < bmin || Lab->b > bmax) {

                 cmsCIELCh LCh;
                 double h, slope;

                 // Falls outside a, b limits. Transports to LCh space,
                 // and then do the clipping


                 if (Lab -> a == 0.0) { // Is hue exactly 90?

                        // atan will not work, so clamp here
                        Lab -> b = Lab->b < 0 ? bmin : bmax;
                        return;
                 }

                 cmsLab2LCh(&LCh, Lab);

                 slope = Lab -> b / Lab -> a;
                 h = LCh.h;

                 // There are 4 zones

                 if ((h >= 0. && h < 45.) ||
                     (h >= 315 && h <= 360.)) {

                     // clip by amax
                     Lab -> a = amax;
                     Lab -> b = amax * slope;
                 }
                 else
                 if (h >= 45. && h < 135)
                 {
                        // clip by bmax
                        Lab -> b = bmax;
                        Lab -> a = bmax / slope;
                 }
                 else
                 if (h >= 135 && h < 225) {
                        // clip by amin
                        Lab -> a = amin;
                        Lab -> b = amin * slope;

                 }
                 else
                 if (h >= 225 && h < 315) {
                        // clip by bmin
                        Lab -> b = bmin;
                        Lab -> a = bmin / slope;
                 }
                 else
                        cmsSignalError(-1, "Invalid angle");

        }
}

// Several utilities -------------------------------------------------------

// Translate from our colorspace to ICC representation

icColorSpaceSignature LCMSEXPORT _cmsICCcolorSpace(int OurNotation)
{
       switch (OurNotation) {

       case 1:
       case PT_GRAY: return  icSigGrayData;

       case 2:
       case PT_RGB:  return  icSigRgbData;

       case PT_CMY:  return  icSigCmyData;
       case PT_CMYK: return  icSigCmykData;
       case PT_YCbCr:return  icSigYCbCrData;
       case PT_YUV:  return  icSigLuvData;
       case PT_XYZ:  return  icSigXYZData;
       case PT_Lab:  return  icSigLabData;
       case PT_YUVK: return  icSigLuvKData;
       case PT_HSV:  return  icSigHsvData;
       case PT_HLS:  return  icSigHlsData;
       case PT_Yxy:  return  icSigYxyData;
       case PT_HiFi: return  icSigHexachromeData;

       default:  return icMaxEnumData;
       }
}

int LCMSEXPORT _cmsChannelsOf(icColorSpaceSignature ColorSpace)
{

	switch (ColorSpace) {

	case icSigGrayData: return 1;

	case icSig2colorData:  return 2;
		
	case icSigXYZData:
    case icSigLabData:
    case icSigLuvData:
    case icSigYCbCrData:
    case icSigYxyData:
    case icSigRgbData:   
    case icSigHsvData:
    case icSigHlsData:
	case icSigCmyData: 
	case icSig3colorData:  return 3;

		
			
	case icSigLuvKData:
    case icSigCmykData:
    case icSig4colorData:  return 4;


    case icSig5colorData:  return 5;  

	case icSigHexachromeData:	
    case icSig6colorData:  return 6;
		
    case icSig7colorData:  return  7;
    case icSig8colorData:  return  8;
    case icSig9colorData:  return  9;
    case icSig10colorData: return 10;
    case icSig11colorData: return 11;
    case icSig12colorData: return 12;
    case icSig13colorData: return 13;
    case icSig14colorData: return 14;
    case icSig15colorData: return 15;

	default: return 3;
	}

}



// Used by gamut & softproofing

typedef struct {

	cmsHTRANSFORM hForward, hReverse;

	} GAMUTCHAIN,FAR* LPGAMUTCHAIN;

// This sampler does compute gamut boundaries by comparing original
// values with a transform going back and forth. There is a thereshold
// to safeguard against roundoff errors

static
int GamutSampler(register WORD In[], register WORD Out[], register LPVOID Cargo)
{
		LPGAMUTCHAIN t = (LPGAMUTCHAIN) Cargo;
		WORD Proof[MAXCHANNELS], Check[MAXCHANNELS];
		cmsCIELab LabIn, LabOut;		
		double dE;
		
		// Assume in-gamut by default.
		dE = 0.;

		// converts from pcs to colorant. This always
		// does return in-gamut values, 
        cmsDoTransform(t -> hForward, In, Proof, 1);

		// Now, do the inverse, from colorant to pcs.
		cmsDoTransform(t -> hReverse, Proof, Check, 1);

		// Does the transform returns out-of-gamut?
		if (Check[0] == 0xFFFF) 
						dE = 0xF000;	// Out of gamut!
		else {

			// Transport encoded values
			cmsLabEncoded2Float(&LabIn, In);
			cmsLabEncoded2Float(&LabOut, Check);
		
			// Take difference
			dE = cmsDeltaE(&LabIn, &LabOut);


			// Let small errors to pass trought
			if (dE < 2.) 
					dE = 0;
		
		}

		// Ok, let's store the dE
		
		Out[0] = (WORD) dE;
		return TRUE;
}






// Does compute a gamut LUT going back and forth across 
// pcs -> relativ. colorimetric intent -> pcs
// the dE obtained is then annotated on the LUT.
// values truely out of gamut, are clipped to dE = 0xFFFE
// and values changed are supposed to be handled by
// any gamut remapping, so, are out of gamut as well.
//
// **WARNING: This algorithm does assume that gamut
// remapping algorithms does NOT move in-gamut colors,
// of course, many perceptual and saturation intents does
// not work in such way, but relativ. ones should.


LPLUT _cmsComputeGamutLUT(cmsHPROFILE hProfile)
{

	cmsHPROFILE hLab;
	LPLUT Gamut;
	DWORD dwFormat;
	GAMUTCHAIN Chain;
	int nErrState;
		

	ZeroMemory(&Chain, sizeof(GAMUTCHAIN));

	hLab = cmsCreateLabProfile(NULL);

	// ONLY 4 channels	
	dwFormat = (CHANNELS_SH(4)|BYTES_SH(2));
	
	// Safeguard against early abortion
	nErrState = cmsErrorAction(LCMS_ERROR_IGNORE);

	// Does create the first step
	Chain.hForward = cmsCreateTransform(hLab, TYPE_Lab_16, 
										hProfile, dwFormat, 
										INTENT_RELATIVE_COLORIMETRIC, 
										cmsFLAGS_NOTPRECALC);

	// Does create the last step
	Chain.hReverse = cmsCreateTransform(hProfile, dwFormat, 
										hLab, TYPE_Lab_16, 										
										INTENT_RELATIVE_COLORIMETRIC, 
										cmsFLAGS_NOTPRECALC);

	// Restores error handler previous state
	cmsErrorAction(nErrState);

	// All ok?
	if (Chain.hForward && Chain.hReverse) {
			
	// Go on, try to compute gamut LUT from PCS.
	// This consist on a single channel containing 
	// dE when doing a transform back and forth on
	// the colorimetric intent. This table will
	// take 42 points to give some accurancy, 
	// 47 * 47 * 47 * 2 = 203K

	Gamut = cmsAllocLUT();
	Gamut = cmsAlloc3DGrid(Gamut, 42, 3, 1);

	cmsSample3DGrid(Gamut, GamutSampler, (LPVOID) &Chain, 0);
	}
	else 
		Gamut = NULL;	// Didn't work...

	// Free all needed stuff.
	if (Chain.hForward) cmsDeleteTransform(Chain.hForward);
	if (Chain.hReverse) cmsDeleteTransform(Chain.hReverse);

	cmsCloseProfile(hLab);


	// And return computed hull
	return Gamut;
}

// SoftProofing. Convert from Lab to device, then back to Lab, 
// any gamut remapping is applied

static
int SoftProofSampler(register WORD In[], register WORD Out[], register LPVOID Cargo)
{
		LPGAMUTCHAIN t = (LPGAMUTCHAIN) Cargo;
		WORD Colorant[MAXCHANNELS];
				
		// From pcs to colorant
        cmsDoTransform(t -> hForward, In, Colorant, 1);

		// Now, do the inverse, from colorant to pcs.
		cmsDoTransform(t -> hReverse, Colorant, Out, 1);
				
		return TRUE;
}

// Does return Softproofing LUT on desired intent

LPLUT _cmsComputeSoftProofLUT(cmsHPROFILE hProfile, int nIntent)
{
	cmsHPROFILE hLab;
	LPLUT SoftProof;
	DWORD dwFormat;
	GAMUTCHAIN Chain;
	int nErrState;
		

	ZeroMemory(&Chain, sizeof(GAMUTCHAIN));

	hLab = cmsCreateLabProfile(NULL);

	// ONLY 4 channels	
	dwFormat = (CHANNELS_SH(4)|BYTES_SH(2));
	
	// Safeguard against early abortion
	nErrState = cmsErrorAction(LCMS_ERROR_IGNORE);

	// Does create the first step
	Chain.hForward = cmsCreateTransform(hLab, TYPE_Lab_16, 
										hProfile, dwFormat, 
										nIntent,
										cmsFLAGS_NOTPRECALC);

	// Does create the last step
	Chain.hReverse = cmsCreateTransform(hProfile, dwFormat, 
										hLab, TYPE_Lab_16, 										
										nIntent, 
										cmsFLAGS_NOTPRECALC);

	// Restores error handler previous state
	cmsErrorAction(nErrState);

	// All ok?
	if (Chain.hForward && Chain.hReverse) {
				
	// This is Lab -> Lab, so 33 point should hold anything
	SoftProof = cmsAllocLUT();
	SoftProof = cmsAlloc3DGrid(SoftProof, 33, 3, 3);

	cmsSample3DGrid(SoftProof, SoftProofSampler, (LPVOID) &Chain, 0);
	}
	else 
		SoftProof = NULL;	// Didn't work...

	// Free all needed stuff.
	if (Chain.hForward) cmsDeleteTransform(Chain.hForward);
	if (Chain.hReverse) cmsDeleteTransform(Chain.hReverse);

	cmsCloseProfile(hLab);

	return SoftProof;
}






#ifdef DEBUG
static
void ASAVE(LPGAMMATABLE p, const char* dump)
{
	FILE* f;
	int i;

        f = fopen(dump, "wt");
        if (!f)
                return;

        if (p) {

	for (i=0; i < p -> nEntries; i++)
		fprintf(f, "%g\n", (double) p -> GammaTable[i]);
        }

	fclose(f);
}
#endif


// Fixes the gamma balancing of transform. Thanks to Mike Chaney
// for pointing this subtle bug.

#define PRELINEARIZATION_POINTS	256

void _cmsComputePrelinearizationTablesFromXFORM(cmsHTRANSFORM h, LPLUT Grid)
{
	LPGAMMATABLE Trans[MAXCHANNELS];
	unsigned int t, i, v;
	WORD In[MAXCHANNELS], Out[MAXCHANNELS];
	BOOL lIsSuitable;
	
	for (t = 0; t < Grid -> InputChan; t++) 
			Trans[t] = cmsAllocGamma(PRELINEARIZATION_POINTS);

	for (i=0; i < PRELINEARIZATION_POINTS; i++) {

				v = _cmsQuantizeVal(i, PRELINEARIZATION_POINTS);

				for (t=0; t < Grid -> InputChan; t++)
						In[t] = (WORD) v;

				cmsDoTransform(h, In, Out, 1);

				for (t=0; t < Grid -> InputChan; t++)
						Trans[t] ->GammaTable[i] = Out[t];

	}
	
	// Smooth and check transfer curves
	lIsSuitable = TRUE;
	for (t=0; (lIsSuitable && (t < Grid->InputChan)); t++) {
	
		if (!cmsSmoothGamma(Trans[t], PRELINEARIZATION_POINTS))
					lIsSuitable = FALSE;

		// Exclude if already linear
		if (cmsIsLinear(Trans[t]->GammaTable, PRELINEARIZATION_POINTS))
					lIsSuitable = FALSE;
	}

	if (lIsSuitable) {

		cmsAllocLinearTable(Grid, Trans, 1);
	}
	

#ifdef DEBUG	
	if (lIsSuitable) {
			ASAVE(Trans[0], "\\gammar.txt");
			ASAVE(Trans[1], "\\gammag.txt");
			ASAVE(Trans[2], "\\gammab.txt");
	}
#endif

	for (t = 0; t < Grid ->InputChan; t++) 
						cmsFreeGamma(Trans[t]);

}

