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


// ---------------------------------------------------------------------------------

// Quantize a value 0 <= i < MaxSamples

WORD _cmsQuantizeVal(double i, int MaxSamples)
{
       double x;

       x = ((double) i * 65535.) / (double) (MaxSamples - 1);

       return (WORD) floor(x + .5);
}


// pow() restricted to integer

static
int ipow(int base, int exp)
{
        int res = base;

        while (--exp)
               res *= base;

        return res;
}


// Given n, 0<=n<=clut^dim, returns the colorant.

static
int ComponentOf(int n, int clut, int nColorant)
{
        if (nColorant <= 0)
                return (n % clut);

        n /= ipow(clut, nColorant);

        return (n % clut);
}



// This routine does a sweep on whole input space, and calls its callback
// function on knots. returns TRUE if all ok, FALSE otherwise.

BOOL LCMSEXPORT cmsSample3DGrid(LPLUT Lut, _cmsSAMPLER Sampler, LPVOID Cargo, DWORD dwFlags)
{
   int i, t, nTotalPoints, Colorant, index;
   WORD In[MAXCHANNELS], Out[MAXCHANNELS];

   nTotalPoints = ipow(Lut->cLutPoints, Lut -> InputChan);

   index = 0;
   for (i = 0; i < nTotalPoints; i++) {

        for (t=0; t < (int) Lut -> InputChan; t++) {

                Colorant =  ComponentOf(i, Lut -> cLutPoints, (Lut -> InputChan - t  - 1 ));
                In[t]    = _cmsQuantizeVal(Colorant, Lut -> cLutPoints);
        }


        if (dwFlags & LUT_HASTL1) {

                 for (t=0; t < (int) Lut -> InputChan; t++)
                     In[t] = cmsReverseLinearInterpLUT16(In[t],
                                                Lut -> L1[t],
                                                &Lut -> In16params);
        }

        if (!Sampler(In, Out, Cargo))
                return FALSE;

                
        if (dwFlags & LUT_HASTL2) {
                for (t=0; t < (int) Lut -> OutputChan; t++)
                     Out[t] = cmsReverseLinearInterpLUT16(Out[t],
                                                   Lut -> L2[t],
                                                   &Lut -> Out16params);
                }

                
        for (t=0; t < (int) Lut -> OutputChan; t++)
                        Lut->T[index + t] = Out[t];

        index += Lut -> OutputChan;

    }

    return TRUE;
}



// Sampler implemented by another transform. This is a clean way to
// precalculate the devicelink 3D CLUT for almost any transform

static
int XFormSampler(register WORD In[], register WORD Out[], register LPVOID Cargo)
{
        cmsDoTransform((cmsHTRANSFORM) Cargo, In, Out, 1);
        return TRUE;
}


static
int NumOfGridPoints(int Precission, int ChannelsIn)
{

    int nGridPoints = 33;

    switch (Precission) {

       default: 
       case 0: 
           
            if (ChannelsIn > 4)
                nGridPoints = 6;
            else
            if (ChannelsIn > 3)
                nGridPoints = 17;
            else
                nGridPoints = 33;
            break;


       case 1:
            if (ChannelsIn > 4)
                nGridPoints = 7;
            else
            if (ChannelsIn > 3)
                nGridPoints = 33;
            else
                nGridPoints = 48;
            break;
       }

    return nGridPoints;
}



// This routine does compute the devicelink CLUT containing whole
// transform. Handles any channel number.

BOOL _cmsPrecalculateDeviceLink(_LPcmsTRANSFORM p, int Precission)
{
       LPLUT Grid;
       int nGridPoints;
       DWORD dwFormatIn, dwFormatOut;
       int ChannelsIn, ChannelsOut;
       
       ChannelsIn   = _cmsChannelsOf(cmsGetColorSpace(p->InputProfile));
       ChannelsOut  = _cmsChannelsOf(cmsGetColorSpace(p->OutputProfile));

       nGridPoints = NumOfGridPoints(Precission, ChannelsIn);          
       Grid =  cmsAllocLUT();
       if (!Grid) return FALSE;

       Grid = cmsAlloc3DGrid(Grid, nGridPoints, ChannelsIn, ChannelsOut);

       // Compute device link on 16-bit basis
       dwFormatIn   = (CHANNELS_SH(ChannelsIn)|BYTES_SH(2));
       dwFormatOut  = (CHANNELS_SH(ChannelsOut)|BYTES_SH(2));

       p -> FromInput = _cmsIdentifyInputFormat(dwFormatIn);
       p -> ToOutput  = _cmsIdentifyOutputFormat(dwFormatOut);


       // Fix gamut & gamma possible mismatches. 
       // Only operates on same colorspace
           
       if (cmsGetColorSpace(p->InputProfile) == cmsGetColorSpace(p->OutputProfile))
                    _cmsComputePrelinearizationTablesFromXFORM((cmsHTRANSFORM) p, Grid);
        
            
       // Attention to this typecast! we can take the luxury to
       // do this since cmsHTRANSFORM is only an alias to a pointer
       // to the transform struct.

       if (!cmsSample3DGrid(Grid, XFormSampler, (LPVOID) p, Grid -> wFlags)) {

                cmsFreeLUT(Grid);
                return FALSE;
       }

       // All ok, store the newly created LUT
       p -> DeviceLink = Grid;
       return TRUE;
}


static
LPLUT Create3x3EmptyLUT(void)
{
        LPLUT AToB0 = cmsAllocLUT();
        AToB0 -> InputChan = AToB0 -> OutputChan = 3;

        return AToB0;
}


// Creates a fake Lab identity.
cmsHPROFILE LCMSEXPORT cmsCreateLabProfile(LPcmsCIExyY WhitePoint)
{
        cmsHPROFILE hProfile;        
        LPLUT Lut;


        hProfile = cmsCreateRGBProfile(WhitePoint == NULL ? cmsD50_xyY() : WhitePoint, NULL, NULL);

        cmsSetDeviceClass(hProfile, icSigAbstractClass);
        cmsSetColorSpace(hProfile, icSigLabData);
        cmsSetPCS(hProfile,  icSigLabData);
      
       _cmsAddTextTag(hProfile, icSigProfileDescriptionTag, "Lab (built-in)");
       _cmsAddTextTag(hProfile, icSigDeviceMfgDescTag,      "(lcms internal)");
       _cmsAddTextTag(hProfile, icSigDeviceModelDescTag,    "(none)");


       // An empty LUTs is all we need
       Lut = Create3x3EmptyLUT();
       if (Lut == NULL) return NULL;

       _cmsAddLUTTag(hProfile, icSigAToB0Tag,    Lut);
       _cmsAddLUTTag(hProfile, icSigBToA0Tag,    Lut);
    

       cmsFreeLUT(Lut);

       return hProfile;
}


// Creates a fake XYZ identity
cmsHPROFILE LCMSEXPORT cmsCreateXYZProfile(void)
{
        cmsHPROFILE hProfile;       
        LPLUT Lut;

        hProfile = cmsCreateRGBProfile(cmsD50_xyY(), NULL, NULL);

        cmsSetDeviceClass(hProfile, icSigAbstractClass);
        cmsSetColorSpace(hProfile, icSigXYZData);
        cmsSetPCS(hProfile,  icSigXYZData);

       _cmsAddTextTag(hProfile, icSigProfileDescriptionTag, "lcms XYZ identity");
       _cmsAddTextTag(hProfile, icSigDeviceMfgDescTag,      "(lcms internal)");
       _cmsAddTextTag(hProfile, icSigDeviceModelDescTag,    "(none)");


       // An empty LUTs is all we need
       Lut = Create3x3EmptyLUT();
       if (Lut == NULL) return NULL;

       _cmsAddLUTTag(hProfile, icSigAToB0Tag,    Lut);
       _cmsAddLUTTag(hProfile, icSigBToA0Tag,    Lut);
       _cmsAddLUTTag(hProfile, icSigPreview0Tag, Lut);

       cmsFreeLUT(Lut);

       _cmsAddTextTag(hProfile, icSigProfileDescriptionTag, "XYZ (built-in)");
       return hProfile;
}



/*

If  R’sRGB,G’sRGB, B’sRGB < 0.04045

    R =  R’sRGB / 12.92
    G =  G’sRGB / 12.92
    B =  B’sRGB / 12.92

 

else if  R’sRGB,G’sRGB, B’sRGB >= 0.04045

    R = ((R’sRGB + 0.055) / 1.055)^2.4
    G = ((G’sRGB + 0.055) / 1.055)^2.4
    B = ((B’sRGB + 0.055) / 1.055)^2.4

  */

static
LPGAMMATABLE Build_sRGBGamma(void)
{
    double Parameters[5];

    Parameters[0] = 2.4;
    Parameters[1] = 1. / 1.055;
    Parameters[2] = 0.055 / 1.055;
    Parameters[3] = 1. / 12.92;
    Parameters[4] = 0.04045;    // d

    return cmsBuildParametricGamma(1024, 3, Parameters);
}

cmsHPROFILE LCMSEXPORT cmsCreate_sRGBProfile(void)
{
       cmsCIExyY       D65;
       cmsCIExyYTRIPLE Rec709Primaries = {
                                   {0.6400, 0.3300, 1.0},
                                   {0.3000, 0.6000, 1.0},
                                   {0.1500, 0.0600, 1.0}
                                   };
       LPGAMMATABLE Gamma22[3];
       cmsHPROFILE  hsRGB;
 
       cmsWhitePointFromTemp(6504, &D65);
       Gamma22[0] = Gamma22[1] = Gamma22[2] = Build_sRGBGamma();
           
       hsRGB = cmsCreateRGBProfile(&D65, &Rec709Primaries, Gamma22);
       cmsFreeGamma(Gamma22[0]);

       _cmsAddTextTag(hsRGB, icSigProfileDescriptionTag, "sRGB (built-in)");
       _cmsAddTextTag(hsRGB, icSigDeviceMfgDescTag,       "(lcms internal)");
       _cmsAddTextTag(hsRGB, icSigDeviceModelDescTag,    "(none)");

        
       return hsRGB;
}


