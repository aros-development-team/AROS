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

// Transformations stuff
// -----------------------------------------------------------------------

// Interface

cmsHTRANSFORM LCMSEXPORT cmsCreateTransform(cmsHPROFILE Input,
                                       DWORD InputFormat,
                                       cmsHPROFILE Output,
                                       DWORD OutputFormat,
                                       int Intent,
                                       DWORD dwFlags);

cmsHTRANSFORM LCMSEXPORT cmsCreateProofingTransform(cmsHPROFILE Input,
                                               DWORD InputFormat,
                                               cmsHPROFILE Output,
                                               DWORD OutputFormat,
                                               cmsHPROFILE Proofing,
                                               int Intent,
                                               int ProofingIntent,
                                               DWORD dwFlags);


void         LCMSEXPORT cmsDeleteTransform(cmsHTRANSFORM hTransform);

void         LCMSEXPORT cmsDoTransform(cmsHTRANSFORM Transform,
                                  LPVOID InputBuffer,
                                  LPVOID OutputBuffer, unsigned int Size);

void         LCMSEXPORT cmsGetAlarmCodes(int *r, int *g, int *b);
void         LCMSEXPORT cmsSetAlarmCodes(int r, int g, int b);
BOOL         LCMSEXPORT cmsIsIntentSupported(cmsHPROFILE hProfile,
                                                int Intent, int UsedDirection);

// -------------------------------------------------------------------------




// Alarm RGB codes

static WORD AlarmR = 0x7fff, AlarmG = 0xfffe, AlarmB = 0x7fff;

// Tag tables, soted by intents

static icTagSignature Device2PCS[] = {icSigAToB0Tag,       // Perceptual
                                      icSigAToB1Tag,       // Relative colorimetric
                                      icSigAToB2Tag,       // Saturation
                                      icSigAToB1Tag };     // Absolute colorimetric
                                                           // (Relative/WhitePoint)

static icTagSignature PCS2Device[] = {icSigBToA0Tag,       // Perceptual
                                      icSigBToA1Tag,       // Relative colorimetric
                                      icSigBToA2Tag,       // Saturation
                                      icSigBToA1Tag };     // Absolute colorimetric
                                                           // (Relative/WhitePoint)


static icTagSignature Preview[]    = {icSigPreview0Tag,
                                      icSigPreview1Tag,
                                      icSigPreview2Tag,
                                      icSigPreview1Tag };





// --------------------------------Stages--------------------------------------


// From Shaper-Matrix to PCS

static
void ShaperMatrixToPCS(struct _cmstransform_struct *p,
                     WORD In[3], WORD Out[3])
{
       cmsEvalMatShaper(p -> InMatShaper, In, Out);
}

// From LUT to PCS

static
void LUTtoPCS(struct _cmstransform_struct *p,
                     WORD In[], WORD Out[3])
{
       cmsEvalLUT(p -> Device2PCS, In, Out);
}


static
void PCStoShaperMatrix(struct _cmstransform_struct *p,
                     WORD In[3], WORD Out[3])
{
       cmsEvalMatShaper(p -> OutMatShaper, In, Out);
}

static
void PCStoLUT(struct _cmstransform_struct *p,
                     WORD In[3], WORD Out[])
{
       cmsEvalLUT(p -> PCS2Device, In, Out);
}




// ----------------------- TRANSFORMATIONS --------------------------



// Inlining some assignations

#define COPY_3CHANS(to, from) { to[0]=from[0]; to[1]=from[1]; to[2]=from[2]; }


// Null transformation, only hold channels

static
void NullXFORM(_LPcmsTRANSFORM p,
                     LPVOID in,
                     LPVOID out, unsigned int Size)
{
       register LPBYTE accum;
       register LPBYTE output;
       WORD wIn[MAXCHANNELS];
       register unsigned int i, n;


       accum  = (LPBYTE) in;
       output = (LPBYTE) out;
       n = Size;                    // Buffer len

       for (i=0; i < n; i++)
       {
       accum = p -> FromInput(p, wIn, accum);
       output = p -> ToOutput(p, wIn, output);
       }

}


static
void NormalXFORM(_LPcmsTRANSFORM p,
                     LPVOID in,
                     LPVOID out, unsigned int Size)
{
       register LPBYTE accum;
       register LPBYTE output;
       WORD wIn[MAXCHANNELS], wOut[MAXCHANNELS];
       WORD wStageABC[3], wPCS[3], wStageLMN[MAXCHANNELS];
       WORD wGamut[1];
       register unsigned int i, n;


       accum  = (LPBYTE) in;
       output = (LPBYTE) out;
       n = Size;                    // Buffer len

       for (i=0; i < n; i++)
       {

       accum = p -> FromInput(p, wIn, accum);

       p -> FromDevice(p, wIn, wStageABC);

       if (p -> Stage1) {

              p -> Stage1(wStageABC, wPCS, &p->m1, &p->of1);

              if (wPCS[0] == 0xFFFF &&
                  wPCS[1] == 0xFFFF &&
                  wPCS[2] == 0xFFFF) {

                     // White cutoff

                     output = p -> ToOutput((_LPcmsTRANSFORM) p,
                                   _cmsWhiteBySpace(cmsGetColorSpace(p -> OutputProfile)),
                                   output);
                     continue;
                     }
              }
       else
              COPY_3CHANS(wPCS, wStageABC);


       if (p->Gamut)
       {

       // Gamut check, enabled across CLUT

       cmsEvalLUT(p -> Gamut, wPCS, wGamut);
       
#ifdef DEBUG
         printf("Gamut %d\n", wGamut[0]);
#endif

       if (wGamut[0] >= 150) {              // 1a for roundoff
        
              
              wOut[0] = AlarmR;          // Gamut alarm
              wOut[1] = AlarmG;
              wOut[2] = AlarmB;
              wOut[3] = 0;

              output = p -> ToOutput((_LPcmsTRANSFORM)p, wOut, output);
              continue;
              }
       }

       if (p -> Preview)
       {
              WORD wPreview[3];    // PCS

              cmsEvalLUT(p -> Preview, wPCS, wPreview);
              COPY_3CHANS(wPCS, wPreview);
       }

       if (p -> Stage2) {
              p -> Stage2(wPCS, wStageLMN, &p->m2, &p->of2);
              if (wPCS[0] == 0xFFFF &&
                  wPCS[1] == 0xFFFF &&
                  wPCS[2] == 0xFFFF) {

                     // White cutoff

                     output = p -> ToOutput((_LPcmsTRANSFORM)p,
                                   _cmsWhiteBySpace(cmsGetColorSpace(p -> OutputProfile)),
                                   output);

                     continue;
                     }

              }
       else
              COPY_3CHANS(wStageLMN, wPCS);

       // Here wOut may come as MAXCHANNELS channels

       p -> ToDevice(p, wStageLMN, wOut);

       output = p -> ToOutput((_LPcmsTRANSFORM)p, wOut, output);
       }
}

// Using precalculated LUT


static
void PrecalculatedXFORM(_LPcmsTRANSFORM p,
                     LPVOID in,
                     LPVOID out, unsigned int Size)
{
       register LPBYTE accum;
       register LPBYTE output;
       WORD wIn[MAXCHANNELS], wOut[MAXCHANNELS];
       register unsigned int i, n;


       accum  = (LPBYTE) in;
       output = (LPBYTE) out;
       n = Size;                    // Buffer len

       for (i=0; i < n; i++)
       {
       accum = p -> FromInput(p, wIn, accum);
       cmsEvalLUT(p -> DeviceLink, wIn, wOut);
       output = p -> ToOutput(p, wOut, output);
       }
}


// Using smelted Matrix/Shaper

static
void MatrixShaperXFORM(_LPcmsTRANSFORM p,
                     LPVOID in,
                     LPVOID out, unsigned int Size)
{
       register LPBYTE accum;
       register LPBYTE output;
       WORD wIn[MAXCHANNELS], wOut[MAXCHANNELS];
       register unsigned int i, n;


       accum  = (LPBYTE) in;
       output = (LPBYTE) out;
       n = Size;                    // Buffer len

       for (i=0; i < n; i++)
       {
       accum = p -> FromInput(p, wIn, accum);
       cmsEvalMatShaper(p -> SmeltMatShaper, wIn, wOut);
       output = p -> ToOutput(p, wOut, output);
       }
}



// --------------------------------------------------------------------------
// Build a LUT based on shape-matrix method.

// Monochrome version

static
LPMATSHAPER cmsBuildGrayMatrixShaper(cmsHPROFILE hProfile, int Behaviour)
{
       cmsCIEXYZ Illuminant;
       LPGAMMATABLE GrayTRC, Shapes[3];
       int i;
       LPMATSHAPER MatShaper;

       GrayTRC = cmsReadICCGamma(hProfile, icSigGrayTRCTag);        // Y
       cmsTakeIluminant(&Illuminant, hProfile);

       Shapes[0] = cmsScaleGamma(GrayTRC, DOUBLE_TO_FIXED(Illuminant.X));
       Shapes[1] = cmsScaleGamma(GrayTRC, DOUBLE_TO_FIXED(Illuminant.Y));
       Shapes[2] = cmsScaleGamma(GrayTRC, DOUBLE_TO_FIXED(Illuminant.Z));

       cmsFreeGamma(GrayTRC);

       if (!Shapes[0] || !Shapes[1] || !Shapes[2])
              return NULL;

       MatShaper = cmsAllocMonoMatShaper(Shapes, Behaviour);

       for (i=0; i < 3; i++)
              cmsFreeGamma(Shapes[i]);

       return MatShaper;

}



// Input matrix, only in XYZ

static
BOOL cmsBuildInputMatrixShaper(_LPcmsTRANSFORM p, LPDWORD dwFlags)
{
       MAT3 DoubleMat;
       LPGAMMATABLE Shapes[3];
       int i;

       // Check if this is a grayscale profile. If so, build
       // appropiate conversion tables. The tables are the PCS
       // iluminant, scaled across GrayTRC

       if (cmsGetColorSpace(p -> InputProfile) == icSigGrayData)
       {
              *dwFlags |= cmsFLAGS_NOTPRECALC;
              p -> InMatShaper = cmsBuildGrayMatrixShaper(p->InputProfile, MATSHAPER_INPUT);
              return p -> InMatShaper != NULL;
       }

       if (!cmsReadICCMatrixRGB2XYZ(&DoubleMat, p -> InputProfile))
                     return FALSE;

       Shapes[0] = cmsReadICCGamma(p-> InputProfile, icSigRedTRCTag);
       Shapes[1] = cmsReadICCGamma(p-> InputProfile, icSigGreenTRCTag);
       Shapes[2] = cmsReadICCGamma(p-> InputProfile, icSigBlueTRCTag);

       if (!Shapes[0] || !Shapes[1] || !Shapes[2])
                     return FALSE;

       p -> InMatShaper = cmsAllocMatShaper(&DoubleMat, Shapes, MATSHAPER_INPUT);

       for (i=0; i < 3; i++)
              cmsFreeGamma(Shapes[i]);

       return p -> InMatShaper != NULL;
}




static
BOOL cmsBuildOutputMatrixShaper(_LPcmsTRANSFORM p, LPDWORD dwFlags)
{
       MAT3 DoubleMat, DoubleInv;
       LPGAMMATABLE Shapes[3], InverseShapes[3];
       int i;

       if (cmsGetColorSpace(p -> OutputProfile) == icSigGrayData)
       {
              *dwFlags |= cmsFLAGS_NOTPRECALC;
              p -> OutMatShaper = cmsBuildGrayMatrixShaper(p-> OutputProfile, MATSHAPER_OUTPUT);
              return p -> OutMatShaper != NULL;
       }


       if (!cmsReadICCMatrixRGB2XYZ(&DoubleMat, p -> OutputProfile))
                     return FALSE;

       if (MAT3inverse(&DoubleMat, &DoubleInv) < 0)
              return FALSE;

       Shapes[0] = cmsReadICCGamma(p-> OutputProfile, icSigRedTRCTag);
       Shapes[1] = cmsReadICCGamma(p-> OutputProfile, icSigGreenTRCTag);
       Shapes[2] = cmsReadICCGamma(p-> OutputProfile, icSigBlueTRCTag);


       InverseShapes[0] = cmsReverseGamma(256, Shapes[0]);
       InverseShapes[1] = cmsReverseGamma(256, Shapes[1]);
       InverseShapes[2] = cmsReverseGamma(256, Shapes[2]);

       for (i=0; i < 3; i++)
              cmsFreeGamma(Shapes[i]);

       p -> OutMatShaper = cmsAllocMatShaper(&DoubleInv, InverseShapes, MATSHAPER_OUTPUT);

       for (i=0; i < 3; i++)
              cmsFreeGamma(InverseShapes[i]);

       return p -> OutMatShaper != NULL;
}


// Builds an input matrix based on chromas, White point and
// gamma info


static
LPGAMMATABLE ComputeGammaTrans(_LPcmsTRANSFORM p, icTagSignature sig)
{
       LPGAMMATABLE GammaRes;
       LPGAMMATABLE InGamma, OutGamma;

       InGamma  = cmsReadICCGamma(p -> InputProfile,  sig);
       OutGamma = cmsReadICCGamma(p -> OutputProfile, sig);

       GammaRes = cmsJoinGamma(InGamma, OutGamma);

       cmsFreeGamma(OutGamma);
       cmsFreeGamma(InGamma);

       return GammaRes;
}


// This function builds a transform matrix chaining parameters

static
BOOL cmsBuildSmeltMatShaper(_LPcmsTRANSFORM p)
{
       MAT3 From, To, ToInv, Transfer;
       LPGAMMATABLE Shapes[3];
       int i;

		// VEC3 wp, wr, wf;

       if (!cmsReadICCMatrixRGB2XYZ(&From, p -> InputProfile))
                     return FALSE;


       if (!cmsReadICCMatrixRGB2XYZ(&To, p -> OutputProfile))
                     return FALSE;

	   

		
		// ++++++	
		//   VEC3init(&wp, 1., 1., 1.);
		//   MAT3eval(&wr, &From, &wp);
		// ++++++

	   
       // invert dest

	   
       if (MAT3inverse(&To, &ToInv) < 0)
                        return FALSE;

       // Multiply
        MAT3per(&Transfer, &ToInv, &From); 
	
		
		// +++++
		// MAT3eval(&wf, &ToInv, &wr);
		// MAT3eval(&wf, &Transfer, &wp);
		// ++++++

        // Ok, compute gamma trans

        Shapes[0] = ComputeGammaTrans(p, icSigRedTRCTag);
        Shapes[1] = ComputeGammaTrans(p, icSigGreenTRCTag);
        Shapes[2] = ComputeGammaTrans(p, icSigBlueTRCTag);


        p -> SmeltMatShaper = cmsAllocMatShaper(&Transfer, Shapes, MATSHAPER_ALLSMELTED);

        for (i=0; i < 3; i++)
              cmsFreeGamma(Shapes[i]);

        return p -> SmeltMatShaper != NULL;
}







// Conversion between PCS ------------------------------------------

// Identifies intent archieved by LUT, only absolute colorimetric ones
// are handled separatly

static
int GetPhase(cmsHPROFILE hProfile)
{
       icColorSpaceSignature PCS;
       icRenderingIntent Intent;

       PCS    = cmsGetPCS(hProfile);
       Intent = (icRenderingIntent) cmsTakeRenderingIntent(hProfile);

       switch (PCS) {
       case icSigXYZData:
                     if (Intent == icAbsoluteColorimetric)

                            return XYZAbs;
                     else
                            return XYZRel;

       case icSigLabData:

                     if (Intent == icAbsoluteColorimetric)

                            return LabAbs;
                     else
                            return LabRel;

       default:
                     cmsSignalError(LCMS_ERRC_ABORTED, "Invalid PCS");
       }

       return XYZRel;
}


static
void TakeConversionRoutines(_LPcmsTRANSFORM p)
{
       cmsCIEXYZ BlackPointIn, WhitePointIn, IlluminantIn;
       cmsCIEXYZ BlackPointOut, WhitePointOut, IlluminantOut;
       cmsCIEXYZ BlackPointProof, WhitePointProof, IlluminantProof;


       cmsTakeIluminant(&IlluminantIn,  p -> InputProfile);
       cmsTakeMediaWhitePoint(&WhitePointIn,  p -> InputProfile);
       cmsTakeMediaBlackPoint(&BlackPointIn,  p -> InputProfile);

       cmsTakeIluminant(&IlluminantOut,  p -> OutputProfile);
       cmsTakeMediaWhitePoint(&WhitePointOut,  p -> OutputProfile);
       cmsTakeMediaBlackPoint(&BlackPointOut,  p -> OutputProfile);


       if (p -> Preview == NULL)     // Non-proofing
       {
              cmsChooseCnvrt(p -> Intent == INTENT_ABSOLUTE_COLORIMETRIC,

                 p -> Phase1,
                             &BlackPointIn,
                             &WhitePointIn,
                             &IlluminantIn,

                 p -> Phase3,
                             &BlackPointOut,
                             &WhitePointOut,
                             &IlluminantOut,

                 &p->Stage1,
                 &p->m1, &p->of1);

       }
       else
       {

       cmsTakeIluminant(&IlluminantProof,        p -> PreviewProfile);
       cmsTakeMediaWhitePoint(&WhitePointProof,  p -> PreviewProfile);
       cmsTakeMediaBlackPoint(&BlackPointProof,  p -> PreviewProfile);

       cmsChooseCnvrt(p -> Intent == INTENT_ABSOLUTE_COLORIMETRIC,

                 p -> Phase1,
                             &BlackPointIn,
                             &WhitePointIn,
                             &IlluminantIn,

                 p -> Phase2,
                             &BlackPointProof,
                             &WhitePointProof,
                             &IlluminantProof,

                 &p->Stage1,
                 &p->m1, &p->of1);

       cmsChooseCnvrt(p -> ProofIntent == INTENT_ABSOLUTE_COLORIMETRIC,

                 p -> Phase2,
                             &BlackPointProof,
                             &WhitePointProof,
                             &IlluminantProof,

                 p -> Phase3,
                             &BlackPointOut,
                             &WhitePointOut,
                             &IlluminantOut,
                 &p->Stage2,
                 &p->m2, &p->of2);


       }

}


// Black, White compensation, just to obtain KCMS compatibility

static
void PatchLUT(LPLUT Grid, WORD At[], WORD Value[],
                     int nChannelsOut, int nChannelsIn)
{
       LPL16PARAMS p16  = &Grid -> CLut16params;
       double     px, py, pz, pw;
       int        x0, y0, z0, w0;
       int        i, index;

       px = ((double) At[0] * (p16->Domain)) / 65535.0;
       py = ((double) At[1] * (p16->Domain)) / 65535.0;
       pz = ((double) At[2] * (p16->Domain)) / 65535.0;
       pw = ((double) At[3] * (p16->Domain)) / 65535.0;

       x0 = (int) floor(px);
       y0 = (int) floor(py);
       z0 = (int) floor(pz);
       w0 = (int) floor(pw);

       if (nChannelsIn == 4)

              index = p16 -> opta4 * x0 +
                      p16 -> opta3 * y0 +
                      p16 -> opta2 * z0 +
                      p16 -> opta1 * w0;
       else
              index = p16 -> opta3 * x0 +
                      p16 -> opta2 * y0 +
                      p16 -> opta1 * z0;


       for (i=0; i < nChannelsOut; i++)
              Grid -> T[index + i] = Value[i];

}

// Replace endpoint of prelinearization curves, only
// if these does exist

	   
static
void PatchCurves(LPLUT Grid)
{
	unsigned int i;



	if (Grid ->wFlags & LUT_HASTL1) {

		for (i=0; i < Grid ->InputChan; i++) {

		Grid ->L1[i][Grid->In16params.Domain] = 0xFFFF;
		Grid ->L1[i][0] = 0;

		_cmsSmoothEndpoints(Grid->L1[i], Grid->InputEntries);					
		}
	}		

}



static
void WhiteBlackCompensation(_LPcmsTRANSFORM p)
{

       WORD *WhitePointIn, *WhitePointOut, *BlackPointIn, *BlackPointOut;
       int nOuts, nIns;


       if (!p -> DeviceLink) {

       cmsSignalError(LCMS_ERRC_WARNING,
              "Unable to do Black/White compensation, DeviceLink LUT is missing.");
       return;
       }



       if (!_cmsEndPointsBySpace(cmsGetColorSpace(p -> InputProfile),
                                   &WhitePointIn, &BlackPointIn, &nIns))
       {
              cmsSignalError(LCMS_ERRC_WARNING,
                     "Unable to do Black/White compensation, unsupported input space.");
              return;
       }


       if (!_cmsEndPointsBySpace(cmsGetColorSpace(p -> OutputProfile),
                                   &WhitePointOut, &BlackPointOut, &nOuts))
       {
              cmsSignalError(LCMS_ERRC_WARNING,
                     "Unable to do Black/White compensation, unsupported output space.");
              return;
       }



       PatchLUT(p -> DeviceLink, WhitePointIn, WhitePointOut, nOuts, nIns);
       PatchLUT(p -> DeviceLink, BlackPointIn, BlackPointOut, nOuts, nIns);
	   PatchCurves(p -> DeviceLink);

}



// Check colorspace

static
BOOL IsProperColorSpace(cmsHPROFILE hProfile, DWORD dwFormat)
{
       int Space = T_COLORSPACE(dwFormat);

       if (Space == PT_ANY) return TRUE;

       return (_cmsICCcolorSpace(Space) == cmsGetColorSpace(hProfile));
}

// Create a transform.

cmsHTRANSFORM LCMSEXPORT cmsCreateProofingTransform(cmsHPROFILE InputProfile,
                                               DWORD InputFormat,
                                               cmsHPROFILE OutputProfile,
                                               DWORD OutputFormat,
                                               cmsHPROFILE ProofingProfile,
                                               int nIntent,
                                               int ProofingIntent,
                                               DWORD dwFlags)

{
       _LPcmsTRANSFORM p;
       icTagSignature FromTag;
       icTagSignature ToTag;
       icTagSignature ProofTag;



       if (nIntent < 0 || nIntent > 3 ||
           ProofingIntent < 0 || ProofingIntent > 3)
       {
       cmsSignalError(LCMS_ERRC_ABORTED, "cmsCreateTransform: intent mismatch");
       return NULL;
       }

       // Allocate needed memory

       p = (_LPcmsTRANSFORM) malloc(sizeof(_cmsTRANSFORM));
       if (!p)
       {
              cmsSignalError(LCMS_ERRC_ABORTED, "cmsCreateTransform: malloc() failed");
              return NULL;
       }

       ZeroMemory(p, sizeof(_cmsTRANSFORM));

       // Initialize default methods

       p -> xform          = NormalXFORM;
       p -> Intent         = nIntent;
       p -> ProofIntent    = ProofingIntent;
       p -> DoGamutCheck   = FALSE;
       p -> InputProfile   = InputProfile;
       p -> OutputProfile  = OutputProfile;
       p -> PreviewProfile = ProofingProfile;
       p -> Preview        = NULL;
       p -> Gamut          = NULL;
       p -> DeviceLink     = NULL;
       p -> InMatShaper    = NULL;
       p -> OutMatShaper   = NULL;
       p -> SmeltMatShaper = NULL;

       p -> InputFormat     = InputFormat;
       p -> OutputFormat    = OutputFormat;

       p -> FromInput = _cmsIdentifyInputFormat(InputFormat);
       p -> ToOutput  = _cmsIdentifyOutputFormat(OutputFormat);

       if ((dwFlags & cmsFLAGS_NULLTRANSFORM) ||
                        ((InputProfile == NULL) &&
                                (OutputProfile == NULL))) {

              p -> xform = NullXFORM;
              return p;
              }


       //  Device link are means to store precalculated transform grids.

       if (cmsGetDeviceClass(InputProfile) == icSigLinkClass &&
              OutputProfile == NULL && ProofingProfile == NULL)
       {

              if (!IsProperColorSpace(InputProfile, InputFormat)) {
                    cmsSignalError(LCMS_ERRC_WARNING, "Device link is operating on wrong colorspace");
              }

              // Device link does only have AToB0Tag (ICC-Spec 1998/09)

              p->DeviceLink = cmsReadICCLut(InputProfile, icSigAToB0Tag);
              if (!p->DeviceLink) {
                     cmsSignalError(LCMS_ERRC_ABORTED, "Noncompliant device-link profile");
                     cmsDeleteTransform(p);
                     return NULL;
                     }

              p -> Phase1 = -1;
              p -> Phase2 = -1;
              p -> Phase3 = -1;
              p -> xform = PrecalculatedXFORM;

              // Precalculated device-link profile is ready

              return p;
       }


       if (OutputProfile == NULL) {
          free(p);
          cmsSignalError(LCMS_ERRC_ABORTED, "Output profile cannot be NULL!");
          return NULL;
       }

       if (InputProfile == NULL) {
          free(p);
          cmsSignalError(LCMS_ERRC_ABORTED, "Input profile cannot be NULL!");
          return NULL;
       }

       if (!IsProperColorSpace(InputProfile, InputFormat)) {
              cmsSignalError(LCMS_ERRC_WARNING, "Input profile is operating on wrong colorspace");
       }

       if (!IsProperColorSpace(OutputProfile, OutputFormat)) {
              cmsSignalError(LCMS_ERRC_WARNING, "Output profile is operating on wrong colorspace");
       }


       p -> Phase1 = GetPhase(InputProfile);
       p -> Phase2 = -1;
       p -> Phase3 = GetPhase(OutputProfile);

       // Try to locate a LUT

       FromTag  = Device2PCS[nIntent];
       ToTag    = PCS2Device[nIntent];


       if (!cmsIsTag(InputProfile, FromTag))
       {
              FromTag = Device2PCS[0];

              if (!cmsIsTag(InputProfile,  FromTag)) {
                            FromTag = (icTagSignature)0;
              }
       }

	   // Apr-15, 2002 - Too much profiles does have bogus content
       // on preview tag, so I do compute it by my own.

       if (ProofingProfile)
       {
           if (dwFlags & cmsFLAGS_SOFTPROOFING) {

			  p -> Preview = _cmsComputeSoftProofLUT(ProofingProfile, ProofingIntent);                  
			  if (p -> Preview == NULL) {
				  
				ProofTag = Preview[nIntent];

				if (!cmsIsTag(ProofingProfile,  ProofTag)) {

                    ProofTag = Preview[0];
                    if (!cmsIsTag(ProofingProfile,  ProofTag))
                                    ProofTag = (icTagSignature)0;
				}

				if (ProofTag) {

                     p -> Preview = cmsReadICCLut(ProofingProfile, ProofTag);
                     ToTag  = PCS2Device[ProofingIntent];
				}
				else
                     {
                     p -> Preview = NULL;
                     ProofingProfile = NULL;
                     cmsSignalError(LCMS_ERRC_WARNING, "Sorry, the proof profile has not previewing capabilities");
                     }
			  }

              // Preview available, link into render pipeline

              if (p -> Preview)
                        p -> Phase2 = GetPhase(ProofingProfile);

           }
            
           
           // Aug-31, 2001 - Too much profiles does have bogus content
           // on gamut tag, so I do compute it by my own.

           if (dwFlags & cmsFLAGS_GAMUTCHECK) {

                               
                     p -> Gamut = _cmsComputeGamutLUT(ProofingProfile);                  
                     if (p -> Gamut == NULL) {

                        // Profile goes only in one direction... try to see 
                        // if profile has the tag, and use it, no matter it
                         // could be bogus. This is the last chance!

                        if (cmsIsTag(ProofingProfile, icSigGamutTag)) {

                            p -> Gamut = cmsReadICCLut(ProofingProfile, icSigGamutTag);
                            }
                            else   {
                                        
                             // Nope, cannot be done.

                             cmsSignalError(LCMS_ERRC_WARNING, "Sorry, the proof profile has not gamut checking capabilities");
                             p -> Gamut = NULL;
                            }
                     }

              }
       }




       if (!cmsIsTag(OutputProfile,  ToTag))
       {
			  	  
              ToTag = PCS2Device[0];
              if (!cmsIsTag(OutputProfile,  ToTag))
                            ToTag = (icTagSignature)0;
       }


       if (dwFlags& cmsFLAGS_MATRIXINPUT)
              FromTag = (icTagSignature)0;

       if (dwFlags & cmsFLAGS_MATRIXOUTPUT)
              ToTag = (icTagSignature)0;

       if (dwFlags & cmsFLAGS_GAMUTCHECK)
              p -> DoGamutCheck = TRUE;

       // Smelted matrix/Shaper optimization

       if (FromTag == 0 && ToTag == 0 && !ProofingProfile
              && nIntent != INTENT_ABSOLUTE_COLORIMETRIC
              && (cmsGetColorSpace(p -> InputProfile) == icSigRgbData)
              && (cmsGetColorSpace(p -> OutputProfile) == icSigRgbData))
       {
              p -> xform = MatrixShaperXFORM;
              dwFlags |= cmsFLAGS_NOTPRECALC;
              if (!cmsBuildSmeltMatShaper(p))
              {
                     cmsSignalError(LCMS_ERRC_ABORTED, "unable to smelt shaper-matrix, required tags missing");
                     cmsDeleteTransform(p);
                     return NULL;
              }

              p -> Phase1 = p -> Phase3 = XYZRel;

       }
       else
              {
              if (FromTag != 0) {

                     p -> FromDevice = LUTtoPCS;
                     p -> Device2PCS = cmsReadICCLut(p -> InputProfile, FromTag);
                     if (!p -> Device2PCS) {
                            cmsSignalError(LCMS_ERRC_ABORTED, "profile is unsuitable for input");
                            cmsDeleteTransform(p);
                            return NULL;
                            }

                     }
              else
              {
                     p -> FromDevice = ShaperMatrixToPCS;
                     if (!cmsBuildInputMatrixShaper(p, &dwFlags)) {
                            cmsSignalError(LCMS_ERRC_ABORTED, "profile is unsuitable for input");
                            cmsDeleteTransform(p);
                            return NULL;
                            }
                     p -> Phase1 = XYZRel;

              }


              if (ToTag != 0) {

                     p -> ToDevice = PCStoLUT;
                     p -> PCS2Device = cmsReadICCLut(p -> OutputProfile, ToTag);
                     if (!p -> PCS2Device) {
                            cmsSignalError(LCMS_ERRC_ABORTED, "profile is unsuitable for output");
                            cmsDeleteTransform(p);
                            return NULL;
                            }

                     }
              else
              {
                     p -> ToDevice = PCStoShaperMatrix;
                     if (!cmsBuildOutputMatrixShaper(p, &dwFlags)) {
                            cmsSignalError(LCMS_ERRC_ABORTED, "profile is unsuitable for output");
                            cmsDeleteTransform(p);
                            return NULL;
                            }
                     p -> Phase3 = XYZRel;

                     // 3/9/2000 - Device link precalculations are not valid on
                     // gamut boundaries if matrix shaper is used on output.
                     // Inihibit precalculation in such cases.
                     // dwFlags |= cmsFLAGS_NOTPRECALC;
              }
       }


       TakeConversionRoutines(p);


       if (dwFlags & cmsFLAGS_WHITEBLACKCOMPENSATION)
                            dwFlags &= ~cmsFLAGS_NOTPRECALC;

       if (!(dwFlags & cmsFLAGS_NOTPRECALC))
       {

               if (!_cmsPrecalculateDeviceLink(p,
                                        (dwFlags & cmsFLAGS_HIGHRESPRECALC) ? 1 : 0)) {

                     cmsSignalError(LCMS_ERRC_ABORTED,
                                "Cannot precalculate %d->%d channels transform!",
                                T_CHANNELS(InputFormat), T_CHANNELS(OutputFormat));

                     cmsDeleteTransform(p);
                     return NULL;
               }


              p -> xform = PrecalculatedXFORM;

       }

       // Re-Identify formats

       p -> FromInput = _cmsIdentifyInputFormat(InputFormat);
       p -> ToOutput  = _cmsIdentifyOutputFormat(OutputFormat);

       if (dwFlags & cmsFLAGS_WHITEBLACKCOMPENSATION)
                                   WhiteBlackCompensation(p);

       return p;
}


// Wrapper por simpler non-proofing transforms.

cmsHTRANSFORM LCMSEXPORT cmsCreateTransform(cmsHPROFILE Input,
                                       DWORD InputFormat,
                                       cmsHPROFILE Output,
                                       DWORD OutputFormat,
                                       int Intent,
                                       DWORD dwFlags)

{
       return cmsCreateProofingTransform(Input, InputFormat,
                                         Output, OutputFormat,
                                         NULL,
                                         Intent, INTENT_ABSOLUTE_COLORIMETRIC,
                                         dwFlags);
}


// Profiles are *NOT* closed

void LCMSEXPORT cmsDeleteTransform(cmsHTRANSFORM hTransform)
{
       _LPcmsTRANSFORM p = (_LPcmsTRANSFORM) (LPSTR) hTransform;

       if (p -> Device2PCS)
              cmsFreeLUT(p -> Device2PCS);
       if (p -> PCS2Device)
              cmsFreeLUT(p -> PCS2Device);
       if (p -> Gamut)
              cmsFreeLUT(p -> Gamut);
       if (p -> Preview)
              cmsFreeLUT(p -> Preview);
       if (p -> DeviceLink)
              cmsFreeLUT(p -> DeviceLink);
       if (p -> InMatShaper)
              cmsFreeMatShaper(p -> InMatShaper);
       if (p -> OutMatShaper)
              cmsFreeMatShaper(p -> OutMatShaper);
       if (p -> SmeltMatShaper)
              cmsFreeMatShaper(p -> SmeltMatShaper);

       free((void *) p);
}



void LCMSEXPORT cmsDoTransform(cmsHTRANSFORM Transform,
                    LPVOID InputBuffer,
                    LPVOID OutputBuffer, unsigned int Size)

{

            _LPcmsTRANSFORM p = (_LPcmsTRANSFORM) (LPSTR) Transform;

            p -> StrideIn = p -> StrideOut = Size;

            p -> xform(p, InputBuffer, OutputBuffer, Size);

}


void LCMSEXPORT cmsSetAlarmCodes(int r, int g, int b)
{
       AlarmR = RGB_8_TO_16(r);
       AlarmG = RGB_8_TO_16(g);
       AlarmB = RGB_8_TO_16(b);
}

void LCMSEXPORT cmsGetAlarmCodes(int *r, int *g, int *b)
{
       *r = RGB_16_TO_8(AlarmR);
       *g = RGB_16_TO_8(AlarmG);
       *b = RGB_16_TO_8(AlarmB);
}

BOOL LCMSEXPORT cmsIsIntentSupported(cmsHPROFILE hProfile,
                                                int Intent, int UsedDirection)
{

     icTagSignature* TagTable;

     // Device link profiles only implements the intent in header

     if (cmsGetDeviceClass(hProfile) != icSigLinkClass) {

       switch (UsedDirection) {

       case LCMS_USED_AS_INPUT: TagTable = Device2PCS; break;
       case LCMS_USED_AS_OUTPUT:TagTable = PCS2Device; break;
       case LCMS_USED_AS_PROOF: TagTable = Preview; break;

       default:
        cmsSignalError(LCMS_ERRC_WARNING, "Unexpected direction (%d)", UsedDirection);
        return FALSE;
       }

       if (cmsIsTag(hProfile, TagTable[Intent])) return TRUE;
     }

     return (cmsTakeRenderingIntent(hProfile) == Intent);
}

// Multiple profile transform. 

static
int MultiprofileSampler(register WORD In[], register WORD Out[], register LPVOID Cargo)
{
    cmsHTRANSFORM* Transforms = (cmsHTRANSFORM*) Cargo;
    int i;
    
    cmsDoTransform(Transforms[0], In, Out, 1);

    for (i=1; Transforms[i]; i++)
        cmsDoTransform(Transforms[i], Out, Out, 1);


    
    return TRUE;
}


cmsHTRANSFORM LCMSEXPORT cmsCreateMultiprofileTransform(cmsHPROFILE hProfiles[],
                                                                int nProfiles,
                                                                DWORD InputFormat,
                                                                DWORD OutputFormat,
                                                                int Intent,
                                                                DWORD dwFlags)
{
    cmsHTRANSFORM Transforms[257];
    DWORD dwPrecalcFlags = (dwFlags|cmsFLAGS_NOTPRECALC);
    DWORD RawFormat, RawIdentity;
    cmsHPROFILE hLab, hXYZ, hProfile;   
    icColorSpaceSignature FirstColorSpace;
    LPLUT Grid;
    int nGridPoints, ChannelsIn, ChannelsOut, i;    
    _LPcmsTRANSFORM p;
    icColorSpaceSignature ColorSpace;
    icProfileClassSignature Class;

    if (nProfiles > 255) {
        cmsSignalError(LCMS_ERRC_ABORTED, "What are you trying to do with more that 255 profiles?!?, of course aborted");
        return NULL;
    }


    // Creates a phantom transform for latter filling
    p = cmsCreateTransform(NULL, InputFormat, NULL, OutputFormat, Intent, cmsFLAGS_NULLTRANSFORM);

    // If user wants null one, give it
    if (dwFlags & cmsFLAGS_NULLTRANSFORM) return (cmsHPROFILE) p;


    // We will need a 3DCLUT for device link
    Grid =  cmsAllocLUT();
    if (!Grid) return NULL;

    // This one is our PCS (Always Lab)
    hLab  = cmsCreateLabProfile(NULL);
    hXYZ  = cmsCreateXYZProfile();

    if (!hLab || !hXYZ) goto ErrorCleanup;

    // Take some info....
    FirstColorSpace = cmsGetColorSpace(hProfiles[0]);
    ChannelsIn     = _cmsChannelsOf(FirstColorSpace);
    ChannelsOut    = 3; // Will be updated on loop

    for (i=0; i < nProfiles; i++) {

        // Check colorspace, create transforms
        hProfile    = hProfiles[i];
        ColorSpace  = cmsGetColorSpace(hProfile);
        ChannelsOut = _cmsChannelsOf(ColorSpace);

        // Format: WORDs of current # of channels
        RawFormat    = BYTES_SH(2)|CHANNELS_SH(ChannelsOut);
        RawIdentity  = BYTES_SH(2)|CHANNELS_SH(3);

        if (ColorSpace == FirstColorSpace) {

            FirstColorSpace = cmsGetPCS(hProfile);
            Class           = cmsGetDeviceClass(hProfile);

            if (Class == icSigLinkClass) {

                    Transforms[i]  = cmsCreateTransform(hProfile, RawFormat, 
                                    NULL, RawIdentity, Intent, dwPrecalcFlags);         
            }

            else {

             Transforms[i]  = cmsCreateTransform(hProfile, RawFormat, 
                                    (FirstColorSpace == icSigLabData ? hLab : hXYZ),
                                    RawIdentity, Intent, dwPrecalcFlags);           
            }
        }
        else  // Can come from pcs?
        if (FirstColorSpace == icSigXYZData) {

            Transforms[i] = cmsCreateTransform(hXYZ, RawIdentity, hProfile, RawFormat, Intent, dwPrecalcFlags);
            FirstColorSpace = ColorSpace;
            
        }
        else
        if (FirstColorSpace == icSigLabData) {

            Transforms[i] = cmsCreateTransform(hLab, RawIdentity, hProfile, RawFormat, Intent, dwPrecalcFlags);
            FirstColorSpace = ColorSpace;
        } 
        else {
                cmsSignalError(LCMS_ERRC_ABORTED, "cmsCreateMultiprofileTransform: ColorSpace mismatch");
                goto ErrorCleanup;
        }

    }

    // Mark end 
    Transforms[i] = NULL;

    nGridPoints = 48; // This always comes with 48 points
    Grid = cmsAlloc3DGrid(Grid, nGridPoints, ChannelsIn, ChannelsOut);

    // Compute device link on 16-bit basis
                
    if (!cmsSample3DGrid(Grid, MultiprofileSampler, (LPVOID) Transforms, Grid -> wFlags)) {

                cmsFreeLUT(Grid);
                goto ErrorCleanup;
    }

    // All ok, store the newly created LUT
    
    p -> DeviceLink = Grid;
    p -> xform      = PrecalculatedXFORM;

    for (i=nProfiles-1; i >= 0; --i)
        cmsDeleteTransform(Transforms[i]);


    if (hLab) cmsCloseProfile(hLab);
    if (hXYZ) cmsCloseProfile(hXYZ);
    return (cmsHTRANSFORM) p;


ErrorCleanup:

    if (hLab) cmsCloseProfile(hLab);
    if (hXYZ) cmsCloseProfile(hXYZ);
    return NULL;
}
