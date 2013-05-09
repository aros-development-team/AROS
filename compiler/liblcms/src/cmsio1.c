//
//  Little cms
//  Copyright (C) 1998-2002 Marti Maria
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
// 
// 1.09b


//     Input/Output


#include "lcms.h"

#include <stdlib.h>
#include <time.h>

cmsHPROFILE   LCMSEXPORT cmsOpenProfileFromFile(const char *ICCProfile, const char *sAccess);
cmsHPROFILE   LCMSEXPORT cmsOpenProfileFromMem(LPVOID MemPtr, DWORD dwSize);
cmsHPROFILE   LCMSEXPORT cmsCreateRGBProfile(LPcmsCIExyY WhitePoint,
                                        LPcmsCIExyYTRIPLE Primaries,
                                        LPGAMMATABLE TransferFunction[3]);

BOOL          LCMSEXPORT cmsCloseProfile(cmsHPROFILE hProfile);
LPLUT         LCMSEXPORT cmsReadICCLut(cmsHPROFILE hProfile, icTagSignature sig);
LPGAMMATABLE  cdecl      cmsReadICCGamma(cmsHPROFILE hProfile, icTagSignature sig);
BOOL          LCMSEXPORT cmsTakeMediaWhitePoint(LPcmsCIEXYZ Dest, cmsHPROFILE hProfile);
BOOL          LCMSEXPORT cmsTakeMediaBlackPoint(LPcmsCIEXYZ Dest, cmsHPROFILE hProfile);
BOOL          LCMSEXPORT cmsTakeIluminant(LPcmsCIEXYZ Dest, cmsHPROFILE hProfile);
BOOL          LCMSEXPORT cmsTakeColorants(LPcmsCIEXYZTRIPLE Dest, cmsHPROFILE hProfile);
BOOL          LCMSEXPORT cmsIsTag(cmsHPROFILE hProfile, icTagSignature sig);


const char*   LCMSEXPORT cmsTakeProductName(cmsHPROFILE hProfile);

icColorSpaceSignature   LCMSEXPORT cmsGetPCS(cmsHPROFILE hProfile);
icColorSpaceSignature   LCMSEXPORT cmsGetColorSpace(cmsHPROFILE hProfile);
icProfileClassSignature LCMSEXPORT cmsGetDeviceClass(cmsHPROFILE hProfile);

int                     cdecl cmsIsLinear(WORD Table[], int nEntries);

BOOL LCMSEXPORT _cmsAddLUTTag(cmsHPROFILE hProfile, icTagSignature sig, void* lut);
BOOL LCMSEXPORT _cmsAddGammaTag(cmsHPROFILE hProfile, icTagSignature sig, LPGAMMATABLE TransferFunction);
BOOL LCMSEXPORT _cmsAddChromaticityTag(cmsHPROFILE hProfile, icTagSignature sig, LPcmsCIExyYTRIPLE Chrm);




BOOL LCMSEXPORT cmsAddTag(cmsHPROFILE hProfile, icTagSignature sig, void* Tag);
BOOL LCMSEXPORT _cmsSaveProfile(cmsHPROFILE hProfile, const char* FileName);

// ------------------- implementation -----------------------------------



#define MAX_TABLE_TAG       50

#define ALIGNLONG(x) (((x)+3) & ~(3))         // Aligns to DWORD boundary

static size_t UsedSpace;

// This is the internal struct holding profile details.

typedef struct {

               FILE         *stream;       // Associated stream. If NULL,
                                           // tags are supposed to be in
                                           // memory rather than in a file.

               // Only most important items found in ICC profile

               icProfileClassSignature DeviceClass;
               icColorSpaceSignature   ColorSpace;
               icColorSpaceSignature   PCS;
               icRenderingIntent       RenderingIntent;
               icUInt32Number          flags;
               cmsCIEXYZ               Illuminant;

               // Dictionary

               icInt32Number   TagCount;
               icTagSignature  TagNames[MAX_TABLE_TAG];
               size_t          TagSizes[MAX_TABLE_TAG];
               size_t          TagOffsets[MAX_TABLE_TAG];
               LPVOID          TagPtrs[MAX_TABLE_TAG];


               char   PhysicalFile[MAX_PATH];
               BOOL   IsTemporary;
			   BOOL   IsWrite;

              } ICCPROFILE, FAR* LPICCPROFILE;



#ifdef __BEOS__
#		define USE_CUSTOM_SWAB	1
#endif


#ifdef USE_CUSTOM_SWAB

// Replacement to swab function, thanks to YNOP
// for providing the BeOS port
//
// from: @(#)swab.c       5.10 (Berkeley)  3/6/91

static
void xswab(const void *from, void *to, size_t len)
{
         register unsigned long temp;
         register int n;
         register char *fp, *tp;

         n = (len >> 1) + 1;
         fp = (char *)from;
         tp = (char *)to;
#define STEP    temp = *fp++,*tp++ = *fp++,*tp++ = temp
         /* round to multiple of 8 */
         while ((--n) & 07)
                 STEP;
         n >>= 3;
         while (--n >= 0) {

                 STEP; STEP; STEP; STEP;
                 STEP; STEP; STEP; STEP;
         }
#undef STEP
}
#else
#define xswab swab
#endif


//
//      Little-Endian to Big-Endian
//

#ifdef USE_BIG_ENDIAN
#define AdjustEndianess16(a)
#define AdjustEndianess32(a)
#define AdjustEndianessArray16(a, b)
#else

static
void AdjustEndianess16(LPBYTE pByte)
{
       BYTE tmp;

       tmp = pByte[0];
       pByte[0] = pByte[1];
       pByte[1] = tmp;
}

static
void AdjustEndianess32(LPBYTE pByte)
{
        BYTE temp1;
        BYTE temp2;

        temp1 = *pByte++;
        temp2 = *pByte++;
        *(pByte-1) = *pByte;
        *pByte++ = temp2;
        *(pByte-3) = *pByte;
        *pByte = temp1;
}


// swap bytes in a array of words

static
void AdjustEndianessArray16(LPWORD p, size_t num_words)
{
       xswab((char*) p, (char*)p, (int) num_words * sizeof(WORD));
}

#endif


// Fixed point conversion

static
double Convert8Fixed8(WORD fixed8)
{
       BYTE msb, lsb;

       lsb = (BYTE) (fixed8 & 0xff);
       msb = (BYTE) (((WORD) fixed8 >> 8) & 0xff);

       return (double) ((double) msb + ((double) lsb / 256.0));
}

static
double Convert15Fixed16(icS15Fixed16Number fix32)
{
    double floater, sign, mid, hack;
    int Whole, FracPart;


    AdjustEndianess32((LPBYTE) &fix32);

    sign  = (fix32 < 0 ? -1 : 1);
    fix32 = abs(fix32);

    Whole = LOWORD(fix32 >> 16);
    FracPart  = LOWORD(fix32 & 0x0000ffffL);

    hack    = 65536.0;
    mid     = (double) FracPart / hack;
    floater = (double) Whole + mid;

    return sign*floater;
}


// Read checking result

static
size_t SafeRead(void *buffer, size_t size, size_t count, FILE *stream)
{
	size_t nReaded = fread(buffer, size, count, stream);
	if (nReaded != count) {
			cmsSignalError(LCMS_ERRC_WARNING, "Read error. Got %d entries, table should be of %d entries", nReaded, count);			
	}

	return nReaded;
}


static
ICCPROFILE *ICCFileOpen(FILE *ICCfile)
{
       ICCPROFILE *Icc;
       icTag Tag;
       icHeader Header;
       icInt32Number TagCount, i;

       SafeRead(&Header, sizeof(icHeader), 1, ICCfile);

       // Convert endian

       AdjustEndianess32((LPBYTE) &Header.size);
       AdjustEndianess32((LPBYTE) &Header.cmmId);
       AdjustEndianess32((LPBYTE) &Header.version);
       AdjustEndianess32((LPBYTE) &Header.deviceClass);
       AdjustEndianess32((LPBYTE) &Header.colorSpace);
       AdjustEndianess32((LPBYTE) &Header.pcs);
       AdjustEndianess32((LPBYTE) &Header.magic);
       AdjustEndianess32((LPBYTE) &Header.flags);
       AdjustEndianess32((LPBYTE) &Header.renderingIntent);

       // Validate it

       if (Header.magic != icMagicNumber)
       {
              fclose(ICCfile);
              return NULL;
       }


       Icc = (ICCPROFILE*) malloc(sizeof(ICCPROFILE));
       if (!Icc)                          // can't allocate
       {
       fclose(ICCfile);
       return NULL;
       }

       ZeroMemory(Icc, sizeof(ICCPROFILE));

       Icc -> stream = ICCfile;

       if (SafeRead(&TagCount, sizeof(icInt32Number), 1, ICCfile) != 1)
                     goto ErrorCleanup;

       AdjustEndianess32((LPBYTE) &TagCount);

       Icc -> DeviceClass     = Header.deviceClass;
       Icc -> ColorSpace      = Header.colorSpace;
       Icc -> PCS             = Header.pcs;
       Icc -> RenderingIntent = (icRenderingIntent) Header.renderingIntent;
       Icc -> Illuminant.X    = Convert15Fixed16(Header.illuminant.X);
       Icc -> Illuminant.Y    = Convert15Fixed16(Header.illuminant.Y);
       Icc -> Illuminant.Z    = Convert15Fixed16(Header.illuminant.Z);


       // Read tag directory

       Icc -> TagCount   = TagCount;
       for (i=0; i < TagCount; i++)
       {
              SafeRead(&Tag, sizeof(icTag), 1, ICCfile);

              AdjustEndianess32((LPBYTE) &Tag.offset);
              AdjustEndianess32((LPBYTE) &Tag.size);
              AdjustEndianess32((LPBYTE) &Tag.sig);            // Signature

              Icc -> TagNames[i]   = Tag.sig;
              Icc -> TagOffsets[i] = Tag.offset;
              Icc -> TagSizes[i]   = Tag.size;

       }


       return Icc;


ErrorCleanup:

              fclose(ICCfile);
              free(Icc);
              return NULL;
}


// Auxiliary

static
icInt32Number  SearchTag(LPICCPROFILE Profile, icTagSignature sig)
{
       icInt32Number i;

       for (i=0; i < Profile -> TagCount; i++)
       {
              if (sig == Profile -> TagNames[i])
                            return i;
       }

       return -1;
}



// Search for a particular tag, replace if found or add if new

static
void InitTag(LPICCPROFILE Icc, icTagSignature sig, size_t size, const LPVOID Init)
{
       LPVOID Ptr;
       icInt32Number i;

       i = SearchTag(Icc, sig);
       if (i >=0) {

              if (Icc -> TagPtrs[i]) free(Icc -> TagPtrs[i]);
       }
       else  {
             i = Icc -> TagCount;
             Icc -> TagCount++;
             }


       Ptr = malloc(size);
       CopyMemory(Ptr, Init, size);

       Icc ->TagNames[i] = sig;
       Icc ->TagSizes[i] = size;
       Icc ->TagPtrs[i]  = Ptr;
}

// Duplicate a gamma table

static
LPGAMMATABLE DupGamma(LPGAMMATABLE In)
{
       LPGAMMATABLE Ptr;

       Ptr = cmsAllocGamma(In -> nEntries);
       CopyMemory(Ptr -> GammaTable,
                  In -> GammaTable,
                  In -> nEntries * sizeof(WORD));

       return Ptr;
}



static
int SizeOfGammaTab(LPGAMMATABLE In)
{
       return sizeof(GAMMATABLE) + (In -> nEntries - 1)*sizeof(WORD);
}


// This function creates a profile based on White point, primaries and
// transfer functions.

cmsHPROFILE LCMSEXPORT cmsCreateRGBProfile(LPcmsCIExyY WhitePoint,
                                LPcmsCIExyYTRIPLE Primaries,
                                LPGAMMATABLE TransferFunction[3])
{
       ICCPROFILE *Icc;
       cmsCIEXYZ tmp;
       MAT3 MColorants;
       cmsCIEXYZTRIPLE Colorants;
	   cmsCIExyY MaxWhite;
	  

       Icc = (ICCPROFILE *) malloc(sizeof(ICCPROFILE));
       if (!Icc)                          // can't allocate
       {
       return NULL;
       }

       ZeroMemory(Icc, sizeof(ICCPROFILE));

       Icc -> stream = NULL;
       Icc -> DeviceClass = icSigDisplayClass;      // icSigInputClass ?
       Icc -> ColorSpace = icSigRgbData;
       Icc -> PCS = icSigXYZData;
       Icc -> RenderingIntent = icPerceptual;    // icRelativeColorimetric ?
       Icc -> flags = 0;
       Icc -> Illuminant.X    = D50X;
       Icc -> Illuminant.Y    = D50Y;
       Icc -> Illuminant.Z    = D50Z;




       // I implement this profile as following tags:
       //
       //  1 icSigProfileDescriptionTag
       //  2 icSigMediaWhitePointTag
       //  3 icSigRedColorantTag
       //  4 icSigGreenColorantTag
       //  5 icSigBlueColorantTag
       //  6 icSigRedTRCTag
       //  7 icSigGreenTRCTag
       //  8 icSigBlueTRCTag

       // This conforms a standard RGB DisplayProfile as says ICC, and then I add

       // 9 icSigChromaticityTag

       // As addendum II

       Icc -> TagCount   = 0;

       // Fill-in the tags

       InitTag(Icc, icSigProfileDescriptionTag, 11, "(internal)");

       if (WhitePoint) {

       cmsxyY2XYZ(&tmp, WhitePoint);
       InitTag(Icc, icSigMediaWhitePointTag, sizeof(cmsCIEXYZ), &tmp);
       }

       if (WhitePoint && Primaries) {

		MaxWhite.x =  WhitePoint -> x;
		MaxWhite.y =  WhitePoint -> y;
		MaxWhite.Y =  1.0;

       if (!cmsBuildRGB2XYZtransferMatrix(&MColorants, &MaxWhite, Primaries))
       {
              free(Icc);
              return NULL;
       }  


	 

	   cmsAdaptMatrixToD50(&MColorants, &MaxWhite);

       Colorants.Red.X = MColorants.v[0].n[0];
       Colorants.Red.Y = MColorants.v[1].n[0];
       Colorants.Red.Z = MColorants.v[2].n[0];

       Colorants.Green.X = MColorants.v[0].n[1];
       Colorants.Green.Y = MColorants.v[1].n[1];
       Colorants.Green.Z = MColorants.v[2].n[1];

       Colorants.Blue.X = MColorants.v[0].n[2];
       Colorants.Blue.Y = MColorants.v[1].n[2];
       Colorants.Blue.Z = MColorants.v[2].n[2];

       InitTag(Icc, icSigRedColorantTag,     sizeof(cmsCIEXYZ), &Colorants.Red);
       InitTag(Icc, icSigBlueColorantTag,    sizeof(cmsCIEXYZ), &Colorants.Blue);
       InitTag(Icc, icSigGreenColorantTag,   sizeof(cmsCIEXYZ), &Colorants.Green);
       }


       if (TransferFunction) {

       // In case of gamma, we must dup' the table pointer

       InitTag(Icc, icSigRedTRCTag, SizeOfGammaTab(TransferFunction[0]), TransferFunction[0]);
       InitTag(Icc, icSigGreenTRCTag, SizeOfGammaTab(TransferFunction[1]), TransferFunction[1]);
       InitTag(Icc, icSigBlueTRCTag, SizeOfGammaTab(TransferFunction[2]), TransferFunction[2]);

       InitTag(Icc, icSigChromaticityTag, sizeof(cmsCIExyYTRIPLE), Primaries);
       }

       return (cmsHPROFILE) Icc;
}



// Create profile from ICC file

cmsHPROFILE LCMSEXPORT cmsOpenProfileFromFile(const char *lpFileName, const char *sAccess)
{
       LPICCPROFILE NewIcc;
       FILE *ICCfile;
	   cmsHPROFILE hEmpty;
	   
	   if (*sAccess == 'W' || *sAccess == 'w') {

		   hEmpty = cmsCreateLabProfile(NULL);
		   NewIcc = (LPICCPROFILE) (LPSTR) hEmpty;
		   NewIcc -> IsWrite = TRUE;
		   strncpy(NewIcc ->PhysicalFile, lpFileName, MAX_PATH-1);

		   return hEmpty;
	   }


       ICCfile = fopen(lpFileName, "rb");
       if (!ICCfile)
       {
              cmsSignalError(LCMS_ERRC_ABORTED, "File '%s' not found", lpFileName);
              return NULL;  
       }

       NewIcc = ICCFileOpen(ICCfile);
       if (!NewIcc)
       {
              cmsSignalError(LCMS_ERRC_ABORTED, "Bad file format: '%s'", lpFileName);
              return NULL;  // Urecoverable
       }

       strncpy(NewIcc -> PhysicalFile, lpFileName, MAX_PATH-1);
       NewIcc -> IsTemporary = FALSE;
	   NewIcc -> IsWrite     = FALSE;

       // in 32 bit, simply typecast
       return (cmsHPROFILE) (LPSTR) NewIcc;
}


// Both BC & MSVC seems to have problems with tmpfile()

#ifndef NON_WINDOWS
static
FILE *CreateTemporaryFile(char *StorePath)
{
   char TempPath[MAX_PATH];

   if (GetTempPath(MAX_PATH-1, TempPath) == 0) return NULL;
   if (GetTempFileName(TempPath, "$LC", 0, StorePath) == 0) return NULL;
   return fopen(StorePath, "w+b");

}

#else
static
FILE *CreateTemporaryFile(char *StorePath)
{
   *StorePath = '\0';
   return tmpfile();
}
#endif


// Open from mem

cmsHPROFILE LCMSEXPORT cmsOpenProfileFromMem(LPVOID MemPtr, DWORD dwSize)
{
       LPICCPROFILE NewIcc;
       FILE *TempFile;
       char TempFileLocation[MAX_PATH];


       TempFile = CreateTemporaryFile(TempFileLocation);
       if (TempFile == NULL)
       {
              cmsSignalError(LCMS_ERRC_ABORTED, "Unable to create tempfile");
              return NULL;  // Urecoverable
       }

       if (fwrite(MemPtr, 1, dwSize, TempFile) !=
              dwSize)
       {
              cmsSignalError(LCMS_ERRC_ABORTED, "fwrite to tempfile failed");
              fclose(TempFile);
              return NULL;  // Urecoverable

       };

       rewind(TempFile);
       NewIcc = ICCFileOpen(TempFile);
       if (!NewIcc)
       {
                     cmsSignalError(LCMS_ERRC_ABORTED, "Error in embbedded profile");
                     fclose(TempFile);
                     unlink(TempFileLocation);
                     return NULL;
       }


       strncpy(NewIcc -> PhysicalFile, TempFileLocation, MAX_PATH-1);
       NewIcc -> IsTemporary = TRUE;
       return (cmsHPROFILE) (LPSTR) NewIcc;
}




BOOL LCMSEXPORT cmsCloseProfile(cmsHPROFILE hProfile)
{
       LPICCPROFILE icco = (LPICCPROFILE) (LPSTR) hProfile;
       FILE *file;
	   BOOL rc = TRUE;

       if (!icco) return FALSE;


	   // Was open in write mode?	
	   if (icco ->IsWrite) {

		   icco ->IsWrite = FALSE;		// Assure no further writting
		   rc = _cmsSaveProfile(hProfile, icco ->PhysicalFile);		   
	   }


       file = icco -> stream;

       if (!file)
       {
              icInt32Number i;

              for (i=0; i < icco -> TagCount; i++)
              {
                  if (icco -> TagPtrs[i])
                            free(icco -> TagPtrs[i]);
              }

       }
       else   {
              fclose(file);
              if (icco -> IsTemporary &&
              	   icco -> PhysicalFile[0]) unlink(icco -> PhysicalFile);
              }

       free(icco);

       return rc;
}



// Is a table linear?

int cmsIsLinear(WORD Table[], int nEntries)
{
       register int i;
	   int diff;


       for (i=0; i < nEntries; i++) {

		   diff = abs((int) Table[i] - (int) _cmsQuantizeVal(i, nEntries));              
		   if (diff > 3)
                     return 0;

       }

       return 1;
}


static
unsigned int uipow(unsigned int a, unsigned int b) {
        unsigned int rv = 1;
        for (; b > 0; b--)
                rv *= a;
        return rv;
}


// Convert between notations.

#define TO16_TAB(x)      (WORD) (((x) << 8) | (x))


// LUT8 can only come in Lab space. There is a fatal flaw in
// converting from Lut8 to Lut16. Due to particular encoding 
// of Lab, different actions should be taken from input and 
// output Lab8 LUTS. For input, is as easy as applying a << 8,
// since numbers comes in fixed point. However, for output LUT
// things goes a bit more complex.... LUT 16 is supposed to
// have a domain of 0..ff00, so we should remap the LUT in order
// to get things working. Affected signatures are B2Axx tags, 
// preview and gamut.

// I do solve it by multiplying input matrix by:
//	
//	| 0xffff/0xff00   0    0  |
//  |       0         1    0  |
//  |       0         0    1  |
//
// The input values got then remapped to adequate domain

static
void FixLUT8(LPLUT Lut, icTagSignature sig, int nTabSize)
{	
	MAT3 Fixup, Original, Result;
	LPWORD PtrW;
	int i;

	switch (sig) {
 

       case icSigBToA0Tag:
       case icSigBToA1Tag:
       case icSigBToA2Tag:
       case icSigGamutTag:
       case icSigPreview0Tag:
       case icSigPreview1Tag:
       case icSigPreview2Tag: 
				
				VEC3init(&Fixup.v[0], (double) 0xFFFF/0xFF00, 0, 0);
				VEC3init(&Fixup.v[1], 0, 1, 0);
				VEC3init(&Fixup.v[2], 0, 0, 1);

				MAT3fromFix(&Original, &Lut->Matrix);				
				MAT3per(&Result, &Original, &Fixup);
				MAT3toFix(&Lut->Matrix, &Result);

				Lut -> wFlags |= LUT_HASMATRIX;
				break;

	   // For input, clear low part since this has to be
	   // Lab in fixed point

	   default:        

			    PtrW = Lut -> T;
			    for (i = 0; i < nTabSize; i++) {

							 *PtrW++ &= 0xFF00;
				}
	}
	
}

// The infamous LUT 8
static
void ReadLUT8(LPICCPROFILE Icc, LPLUT NewLUT, icTagSignature sig)
{
    icLut8 LUT8;
    LPBYTE Temp;
    size_t nTabSize;
    unsigned int i, j;
    unsigned int AllLinear;
    LPWORD PtrW;

       SafeRead(&LUT8, sizeof(icLut8) - SIZEOF_UINT8_ALIGNED, 1, Icc -> stream);
       
       NewLUT -> wFlags        = LUT_HASTL1|LUT_HASTL2|LUT_HAS3DGRID;
       NewLUT -> cLutPoints    = LUT8.clutPoints;
       NewLUT -> InputChan     = LUT8.inputChan;
       NewLUT -> OutputChan    = LUT8.outputChan;
       NewLUT -> InputEntries  = 256;
       NewLUT -> OutputEntries = 256;


       AdjustEndianess32((LPBYTE) &LUT8.e00);
       AdjustEndianess32((LPBYTE) &LUT8.e01);
       AdjustEndianess32((LPBYTE) &LUT8.e02);
       AdjustEndianess32((LPBYTE) &LUT8.e10);
       AdjustEndianess32((LPBYTE) &LUT8.e11);
       AdjustEndianess32((LPBYTE) &LUT8.e12);
       AdjustEndianess32((LPBYTE) &LUT8.e20);
       AdjustEndianess32((LPBYTE) &LUT8.e21);
       AdjustEndianess32((LPBYTE) &LUT8.e22);


       // Matrix handling

       NewLUT -> Matrix.v[0].n[0] = (Fixed32) LUT8.e00;
       NewLUT -> Matrix.v[0].n[1] = (Fixed32) LUT8.e01;
       NewLUT -> Matrix.v[0].n[2] = (Fixed32) LUT8.e02;
       NewLUT -> Matrix.v[1].n[0] = (Fixed32) LUT8.e10;
       NewLUT -> Matrix.v[1].n[1] = (Fixed32) LUT8.e11;
       NewLUT -> Matrix.v[1].n[2] = (Fixed32) LUT8.e12;
       NewLUT -> Matrix.v[2].n[0] = (Fixed32) LUT8.e20;
       NewLUT -> Matrix.v[2].n[1] = (Fixed32) LUT8.e21;
       NewLUT -> Matrix.v[2].n[2] = (Fixed32) LUT8.e22;


       // Only operates if not identity...

       if (!MAT3isIdentity(&NewLUT -> Matrix, 0.0001)) {

              NewLUT -> wFlags |= LUT_HASMATRIX;
       }


       // Copy input tables

       Temp = (LPBYTE) malloc(256);
       AllLinear = 0;
       for (i=0; i < NewLUT -> InputChan; i++) {

              PtrW = (LPWORD) malloc(sizeof(WORD) * 256);
              NewLUT -> L1[i] = PtrW;
              SafeRead(Temp, 1, 256, Icc -> stream);
              for (j=0; j < 256; j++)
                     PtrW[j] = TO16_TAB(Temp[j]);
                     AllLinear += cmsIsLinear(NewLUT -> L1[i], NewLUT -> InputEntries);
              }

       // Linear input, so ignore full step

       if (AllLinear == NewLUT -> InputChan) {

              NewLUT -> wFlags &= ~LUT_HASTL1;
       }

       free(Temp);

       // Copy 3D CLUT

       nTabSize = (NewLUT -> OutputChan * uipow(NewLUT->cLutPoints,
                                                NewLUT->InputChan));

       PtrW = (LPWORD) malloc(sizeof(WORD) * nTabSize);
       Temp = (LPBYTE) malloc(nTabSize);
       SafeRead(Temp, 1, nTabSize, Icc -> stream);

       NewLUT -> T = PtrW;
       for (i = 0; i < nTabSize; i++) {

                     *PtrW++ = TO16_TAB(Temp[i]);
       }
       free(Temp);


       // Copy output tables

       Temp = (LPBYTE) malloc(256);
       AllLinear = 0;
       for (i=0; i < NewLUT -> OutputChan; i++) {

              PtrW = (LPWORD) malloc(sizeof(WORD) * 256);
              NewLUT -> L2[i] = PtrW;
              SafeRead(Temp, 1, 256, Icc -> stream);              
			  for (j=0; j < 256; j++)
                     PtrW[j] = TO16_TAB(Temp[j]);
                     AllLinear += cmsIsLinear(NewLUT -> L2[i], 256);
              }

       // Linear input, so ignore full step

       if (AllLinear == NewLUT -> OutputChan) {

              NewLUT -> wFlags &= ~LUT_HASTL2;
       }


       free(Temp);

       cmsCalcL16Params(NewLUT -> InputEntries,  &NewLUT -> In16params);
       cmsCalcL16Params(NewLUT -> OutputEntries, &NewLUT -> Out16params);
       cmsCalcCLUT16Params(NewLUT -> cLutPoints,  NewLUT -> InputChan,
                                                  NewLUT -> OutputChan,
                                                  &NewLUT -> CLut16params);
	   // Fixup
	   FixLUT8(NewLUT, sig, nTabSize);

}




// Case LUT 16

static
void ReadLUT16(LPICCPROFILE Icc, LPLUT NewLUT)
{
    icLut16 LUT16;
    size_t nTabSize;
    unsigned int i;
    unsigned int AllLinear;
    LPWORD PtrW;


       SafeRead(&LUT16, sizeof(icLut16)- SIZEOF_UINT16_ALIGNED, 1, Icc -> stream);

       NewLUT -> wFlags        = LUT_HASTL1 | LUT_HASTL2 | LUT_HAS3DGRID;
       NewLUT -> cLutPoints    = LUT16.clutPoints;
       NewLUT -> InputChan     = LUT16.inputChan;
       NewLUT -> OutputChan    = LUT16.outputChan;

       AdjustEndianess16((LPBYTE) &LUT16.inputEnt);
       AdjustEndianess16((LPBYTE) &LUT16.outputEnt);

       NewLUT -> InputEntries  = LUT16.inputEnt;
       NewLUT -> OutputEntries = LUT16.outputEnt;


       // Matrix handling

       AdjustEndianess32((LPBYTE) &LUT16.e00);
       AdjustEndianess32((LPBYTE) &LUT16.e01);
       AdjustEndianess32((LPBYTE) &LUT16.e02);
       AdjustEndianess32((LPBYTE) &LUT16.e10);
       AdjustEndianess32((LPBYTE) &LUT16.e11);
       AdjustEndianess32((LPBYTE) &LUT16.e12);
       AdjustEndianess32((LPBYTE) &LUT16.e20);
       AdjustEndianess32((LPBYTE) &LUT16.e21);
       AdjustEndianess32((LPBYTE) &LUT16.e22);

       NewLUT -> Matrix.v[0].n[0] = (Fixed32) LUT16.e00;
       NewLUT -> Matrix.v[0].n[1] = (Fixed32) LUT16.e01;
       NewLUT -> Matrix.v[0].n[2] = (Fixed32) LUT16.e02;
       NewLUT -> Matrix.v[1].n[0] = (Fixed32) LUT16.e10;
       NewLUT -> Matrix.v[1].n[1] = (Fixed32) LUT16.e11;
       NewLUT -> Matrix.v[1].n[2] = (Fixed32) LUT16.e12;
       NewLUT -> Matrix.v[2].n[0] = (Fixed32) LUT16.e20;
       NewLUT -> Matrix.v[2].n[1] = (Fixed32) LUT16.e21;
       NewLUT -> Matrix.v[2].n[2] = (Fixed32) LUT16.e22;

       // Only operates if not identity...

       if (!MAT3isIdentity(&NewLUT -> Matrix, 0.0001)) {

              NewLUT -> wFlags |= LUT_HASMATRIX;
       }


       // Copy input tables

       AllLinear = 0;
       for (i=0; i < NewLUT -> InputChan; i++) {

              PtrW = (LPWORD) malloc(sizeof(WORD) * NewLUT -> InputEntries);
              NewLUT -> L1[i] = PtrW;
              SafeRead(PtrW, sizeof(WORD), NewLUT -> InputEntries, Icc -> stream);
              AdjustEndianessArray16(PtrW, NewLUT -> InputEntries);
              AllLinear += cmsIsLinear(NewLUT -> L1[i], NewLUT -> InputEntries);
              }

       // Linear input, so ignore full step

       if (AllLinear == NewLUT -> InputChan) {

              NewLUT -> wFlags &= ~LUT_HASTL1;
       }


       // Copy 3D CLUT

       nTabSize = (NewLUT -> OutputChan * uipow(NewLUT->cLutPoints,
                                                NewLUT->InputChan));
       PtrW = (LPWORD) malloc(sizeof(WORD) * nTabSize);

       NewLUT -> T = PtrW;
       SafeRead(PtrW, sizeof(WORD), nTabSize, Icc -> stream);
       AdjustEndianessArray16(NewLUT -> T, nTabSize);

       // Copy output tables

       AllLinear = 0;
       for (i=0; i < NewLUT -> OutputChan; i++) {

              PtrW = (LPWORD) malloc(sizeof(WORD) * NewLUT -> OutputEntries);
              NewLUT -> L2[i] = PtrW;
              SafeRead(PtrW, sizeof(WORD), NewLUT -> OutputEntries, Icc -> stream);
              AdjustEndianessArray16(PtrW, NewLUT -> OutputEntries);
              AllLinear += cmsIsLinear(NewLUT -> L2[i], NewLUT -> OutputEntries);
              }

       // Linear output, ignore step

       if (AllLinear == NewLUT -> OutputChan)
       {
              NewLUT -> wFlags &= ~LUT_HASTL2;
       }


       cmsCalcL16Params(NewLUT -> InputEntries,  &NewLUT -> In16params);
       cmsCalcL16Params(NewLUT -> OutputEntries, &NewLUT -> Out16params);
       cmsCalcCLUT16Params(NewLUT -> cLutPoints,  NewLUT -> InputChan,
                                                  NewLUT -> OutputChan,
                                                  &NewLUT -> CLut16params);
}



LPLUT LCMSEXPORT cmsReadICCLut(cmsHPROFILE hProfile, icTagSignature sig)
{

    LPICCPROFILE        Icc = (LPICCPROFILE) (LPSTR) hProfile;
    icTagBase           Base;
    int                 n;
    size_t              offset;
    LPLUT               NewLUT;

    n = SearchTag(Icc, sig);
    if (n < 0)
    {
    cmsSignalError(LCMS_ERRC_ABORTED, "Tag not found");
    return NULL;
    }


    // If is in memory, the LUT is already there, so throw a copy
    if (!Icc -> stream) {

            LPVOID Block = malloc(Icc -> TagSizes[n]);
            if (!Block) return NULL;

            CopyMemory(Block, Icc -> TagPtrs[n], Icc -> TagSizes[n]);
            return (LPLUT) Block;
    }

    offset = Icc -> TagOffsets[n];


    if (fseek(Icc -> stream, offset, SEEK_SET))
    {
       cmsSignalError(LCMS_ERRC_ABORTED, "seek error; probably corrupted file");
       return NULL;
    }

    SafeRead(&Base, sizeof(icTagBase), 1, Icc -> stream);
    AdjustEndianess32((LPBYTE) &Base.sig);

    NewLUT = cmsAllocLUT();
    if (!NewLUT)
    {
       cmsSignalError(LCMS_ERRC_ABORTED, "cmsAllocLUT() failed");
       return NULL;
    }


    switch (Base.sig) {

    case icSigLut8Type: ReadLUT8(Icc, NewLUT, sig); break;
    case icSigLut16Type: ReadLUT16(Icc, NewLUT); break;

    default:  cmsSignalError(LCMS_ERRC_ABORTED, "Bad tag signature %lx found.", Base.sig);
              cmsFreeLUT(NewLUT);
              return NULL;
    }

    return NewLUT;
}


// Take an ASCII item

static
int ReadICCAscii(cmsHPROFILE hProfile, icTagSignature sig, char *Name)
{
    LPICCPROFILE        Icc = (LPICCPROFILE) (LPSTR) hProfile;
    icTagBase           Base;
    size_t              offset, size;
    int                 n;

    n = SearchTag(Icc, sig);
    if (n < 0)
    {
    cmsSignalError(LCMS_ERRC_ABORTED, "Tag not found");
    return -1;
    }


    if (!Icc -> stream)
    {
    CopyMemory(Name, Icc -> TagPtrs[n],
                     Icc -> TagSizes[n]);

    return Icc -> TagSizes[n];
    }

    offset = Icc -> TagOffsets[n];
    size   = Icc -> TagSizes[n];

    fseek(Icc -> stream, offset, SEEK_SET);
    SafeRead(&Base, sizeof(icTagBase), 1, Icc -> stream);
    AdjustEndianess32((LPBYTE) &Base.sig);

    switch (Base.sig)
    {
    case icSigTextDescriptionType: {

           icUInt32Number  AsciiCount;

           SafeRead(&AsciiCount, sizeof(icUInt32Number), 1, Icc -> stream);
           AdjustEndianess32((LPBYTE) &AsciiCount);
           SafeRead(Name, 1, AsciiCount, Icc -> stream);
           size = AsciiCount;
           }
           break;


    case icSigCopyrightTag:   // Broken profiles from agfa does store copyright info in such type
    case icSigTextType:

           size -= sizeof(icTagBase);
           SafeRead(Name, 1, size, Icc -> stream);
           break;

    default:
              cmsSignalError(LCMS_ERRC_ABORTED, "Bad tag signature %lx found.", Base.sig);
              return -1;
    }

    return size;
}



// Take an XYZ item

static
int ReadICCXYZ(cmsHPROFILE hProfile, icTagSignature sig, LPcmsCIEXYZ Value, BOOL lIsFatal)
{
    LPICCPROFILE        Icc = (LPICCPROFILE) (LPSTR) hProfile;
    icTagBase           Base;
    size_t              offset;
    int                 n;
    icXYZNumber         XYZ;

    n = SearchTag(Icc, sig);
    if (n < 0)
    {
    // Tag not found
    return -1;
    }


    if (!Icc -> stream)
    {
    CopyMemory(Value, Icc -> TagPtrs[n],
                      Icc -> TagSizes[n]);

    return Icc -> TagSizes[n];
    }

    offset = Icc -> TagOffsets[n];

    fseek(Icc -> stream, offset, SEEK_SET);
    SafeRead(&Base, 1, sizeof(icTagBase), Icc -> stream);
    AdjustEndianess32((LPBYTE) &Base.sig);

    switch (Base.sig)
    {
   		
    case 0x7c3b10cL:	// Some apple broken embedded profiles does not have correct type	
    case icSigXYZType:

           SafeRead(&XYZ, sizeof(icXYZNumber), 1, Icc -> stream);
           Value -> X = Convert15Fixed16(XYZ.X);
           Value -> Y = Convert15Fixed16(XYZ.Y);
           Value -> Z = Convert15Fixed16(XYZ.Z);
           break;

	// Aug/21-2001 - arrg, Monaco 2 does have WRONG values.

    default:
		   if (lIsFatal)
				cmsSignalError(LCMS_ERRC_ABORTED, "Bad tag signature %lx found.", Base.sig);
           return -1;
    }

    return 1;
}

// Jun-21-2000: Some profiles (those that comes with W2K) comes
// with the media white (media black?) x 100. Add a sanity check

static
void NormalizeXYZ(LPcmsCIEXYZ Dest)
{
    while (Dest -> X > 2. &&
           Dest -> Y > 2. &&
           Dest -> Z > 2.) {

               Dest -> X /= 10.;
               Dest -> Y /= 10.;
               Dest -> Z /= 10.;
       }
}


// White point must be in XYZ to avoid extra calculation on
// absolute intents

BOOL LCMSEXPORT cmsTakeMediaWhitePoint(LPcmsCIEXYZ Dest, cmsHPROFILE hProfile)
{
       if (ReadICCXYZ(hProfile, icSigMediaWhitePointTag, Dest, FALSE) < 0)
       {
              Dest->X = D50X;   // Default to D50
              Dest->Y = D50Y;
              Dest->Z = D50Z;

              return FALSE;
       }

       NormalizeXYZ(Dest);
       return TRUE;
}


BOOL LCMSEXPORT cmsTakeMediaBlackPoint(LPcmsCIEXYZ Dest, cmsHPROFILE hProfile)
{
       if (ReadICCXYZ(hProfile, icSigMediaBlackPointTag, Dest, FALSE) < 0)
       {
              Dest->X = 0.0;
              Dest->Y = 0.0;
              Dest->Z = 0.0;
              return FALSE;
       }


       NormalizeXYZ(Dest);
       return TRUE;
}

BOOL  LCMSEXPORT cmsTakeIluminant(LPcmsCIEXYZ Dest, cmsHPROFILE hProfile)
{
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;

       Dest -> X = Icc -> Illuminant.X;
       Dest -> Y = Icc -> Illuminant.Y;
       Dest -> Z = Icc -> Illuminant.Z;

       NormalizeXYZ(Dest);
       return TRUE;
}

int LCMSEXPORT cmsTakeRenderingIntent(cmsHPROFILE hProfile)
{
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;

       return Icc -> RenderingIntent;
}


// Primaries are to be in xyY notation

BOOL LCMSEXPORT cmsTakeColorants(LPcmsCIEXYZTRIPLE Dest, cmsHPROFILE hProfile)
{

       if (ReadICCXYZ(hProfile, icSigRedColorantTag, &Dest -> Red, TRUE) < 0) return FALSE;
       if (ReadICCXYZ(hProfile, icSigGreenColorantTag, &Dest -> Green, TRUE) < 0) return FALSE;
       if (ReadICCXYZ(hProfile, icSigBlueColorantTag, &Dest -> Blue, TRUE) < 0) return FALSE;


       return TRUE;

}


BOOL cdecl cmsReadICCMatrixRGB2XYZ(LPMAT3 r, cmsHPROFILE hProfile)
{
       cmsCIEXYZTRIPLE Primaries;

       if (!cmsTakeColorants(&Primaries, hProfile)) return FALSE;

       VEC3init(&r -> v[0], Primaries.Red.X, Primaries.Green.X,  Primaries.Blue.X);
       VEC3init(&r -> v[1], Primaries.Red.Y, Primaries.Green.Y,  Primaries.Blue.Y);
       VEC3init(&r -> v[2], Primaries.Red.Z, Primaries.Green.Z,  Primaries.Blue.Z);

       return TRUE;

}


LPGAMMATABLE cmsReadICCGamma(cmsHPROFILE hProfile, icTagSignature sig)
{
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;
       LPGAMMATABLE NewGamma;
       icUInt32Number Count;
       icTagBase      Base;
       size_t         offset;
       int            n;


       n = SearchTag(Icc, sig);
       if (n < 0)
       {
       cmsSignalError(LCMS_ERRC_ABORTED, "Tag not found");
       return NULL;
       }


       if (!Icc -> stream)
       {
       return DupGamma(Icc -> TagPtrs[n]);
       }

       offset = Icc -> TagOffsets[n];

       fseek(Icc -> stream, offset, SEEK_SET);
       SafeRead(&Base, 1, sizeof(icTagBase), Icc -> stream);
       AdjustEndianess32((LPBYTE) &Base.sig);


       switch (Base.sig) {


	   case 0x9478ee00L:    // Monaco 2 profiler is BROKEN!
       case icSigCurveType:

           SafeRead(&Count, sizeof(icUInt32Number), 1, Icc -> stream);
           AdjustEndianess32((LPBYTE) &Count);


           switch (Count) {

           case 0:   // Linear.

                     NewGamma = cmsAllocGamma(2);
                     if (!NewGamma) return NULL;
                     NewGamma -> GammaTable[0] = 0;
                     NewGamma -> GammaTable[1] = 0xFFFF;
                     return NewGamma;

           case 1:  {
                     WORD SingleGammaFixed;

                     SafeRead(&SingleGammaFixed, sizeof(WORD), 1, Icc -> stream);
                     AdjustEndianess16((LPBYTE) &SingleGammaFixed);
                     return cmsBuildGamma(4096, Convert8Fixed8(SingleGammaFixed));
                     }

           default: { // Curve
					
                     NewGamma = cmsAllocGamma(Count);
                     if (!NewGamma) return NULL;

                     SafeRead(NewGamma -> GammaTable, sizeof(WORD), Count, Icc -> stream);
					 
                     AdjustEndianessArray16(NewGamma -> GammaTable, Count);

                     return NewGamma;
					}
              }
		      break;

	
	   // Parametric curves
	   case icSigParametricCurveType: {
           
		   int ParamsByType[] = { 1, 3, 4, 5, 7 };
		   double Params[10];
		   icS15Fixed16Number Num;
		   icUInt32Number Reserved;
		   icUInt16Number Reserved2;
		   icUInt16Number   Type;
		   int i;

		   SafeRead(&Reserved, sizeof(icUInt32Number), 1, Icc -> stream);
		   SafeRead(&Type, sizeof(icUInt16Number), 1, Icc -> stream);
		   SafeRead(&Reserved2, sizeof(icUInt16Number), 1, Icc -> stream);
		   
		   AdjustEndianess16((LPBYTE) &Type);
		   if (Type > 5) {

				cmsSignalError(LCMS_ERRC_ABORTED, "Unknown parametric curve type '%d' found.", Type);
				return NULL;
		   }
		
		  ZeroMemory(Params, 10* sizeof(double));
		  n = ParamsByType[Type];

		  for (i=0; i < n; i++) {
				SafeRead(&Num, sizeof(icS15Fixed16Number), 1, Icc -> stream);
				Params[i] = Convert15Fixed16(Num);
		  }


		   NewGamma = cmsBuildParametricGamma(4096, Type, Params);
		   return NewGamma;
		  }
	

       default:
              cmsSignalError(LCMS_ERRC_ABORTED, "Bad tag signature '%lx' found.", Base.sig);
              return NULL;
       }

       // It would never reach here

       // return NULL;
}



icColorSpaceSignature LCMSEXPORT cmsGetPCS(cmsHPROFILE hProfile)
{
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;

       return Icc -> PCS;
}


void LCMSEXPORT cmsSetPCS(cmsHPROFILE hProfile, icColorSpaceSignature pcs)
{
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;

       Icc -> PCS = pcs;
}



icColorSpaceSignature LCMSEXPORT cmsGetColorSpace(cmsHPROFILE hProfile)
{
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;

       return Icc -> ColorSpace;
}



void LCMSEXPORT cmsSetColorSpace(cmsHPROFILE hProfile, icColorSpaceSignature sig)
{
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;

       Icc -> ColorSpace = sig;
}

// Check existance

BOOL LCMSEXPORT cmsIsTag(cmsHPROFILE hProfile, icTagSignature sig)
{
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;

       return SearchTag(Icc, sig) >= 0;

}

icProfileClassSignature LCMSEXPORT cmsGetDeviceClass(cmsHPROFILE hProfile)
{
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;

       return Icc -> DeviceClass;
}

void LCMSEXPORT cmsSetDeviceClass(cmsHPROFILE hProfile, icProfileClassSignature sig)
{
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;
       Icc -> DeviceClass = sig;
}



// We compute name with model - manufacturer

const char*  LCMSEXPORT cmsTakeProductName(cmsHPROFILE hProfile)
{
       static char Name[2048];
       char Manufacturer[512], Model[512];

       Name[0] = '\0';
       Manufacturer[0] = Model[0] = '\0';

       if (cmsIsTag(hProfile, icSigDeviceMfgDescTag))
       {
       ReadICCAscii(hProfile, icSigDeviceMfgDescTag, Manufacturer);
       }

       if (cmsIsTag(hProfile, icSigDeviceModelDescTag))
       {
       ReadICCAscii(hProfile, icSigDeviceModelDescTag, Model);
       }

       if (!Manufacturer[0] && !Model[0])
       {
              if (cmsIsTag(hProfile, icSigProfileDescriptionTag))
              {
              ReadICCAscii(hProfile, icSigProfileDescriptionTag, Name);
              return Name;
              }
              else return "{no name}";
       }


       if (!Manufacturer[0] || strncmp(Model, Manufacturer, 8) == 0 ||
							   strlen(Model) > 30)
              strcpy(Name, Model);
       else
              sprintf(Name, "%s - %s", Model, Manufacturer);

       return Name;

}


// We compute desc with manufacturer - model

const char*  LCMSEXPORT cmsTakeProductDesc(cmsHPROFILE hProfile)
{
       static char Name[2048];

       if (cmsIsTag(hProfile, icSigProfileDescriptionTag))
              {
              ReadICCAscii(hProfile, icSigProfileDescriptionTag, Name);
              }
       else return cmsTakeProductName(hProfile);

       if (strncmp(Name, "Copyrig", 7) == 0)
              return cmsTakeProductName(hProfile);

       return Name;
}


const char*  LCMSEXPORT cmsTakeProductInfo(cmsHPROFILE hProfile)
{
       // Simply typecast
       LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;

       static char Info[4096];

       Info[0] = '\0';

       if (cmsIsTag(hProfile, icSigProfileDescriptionTag))
       {
       char Desc[1024];

       ReadICCAscii(hProfile, icSigProfileDescriptionTag, Desc);
       strcat(Info, Desc);
       strcat(Info, "\r\n\r\n");
       }


       if (cmsIsTag(hProfile, icSigCopyrightTag))
       {
       char Copyright[2048];

       ReadICCAscii(hProfile, icSigCopyrightTag, Copyright);
       strcat(Info, Copyright);
       strcat(Info, "\r\n\r\n");
       }



// KODAK private tag... But very useful

#define K007         (icTagSignature)0x4B303037

       // MonCal

       if (cmsIsTag(hProfile, K007))
       {
       char MonCal[1024];

       ReadICCAscii(hProfile, K007, MonCal);
       strcat(Info, MonCal);
       strcat(Info, "\r\n\r\n");
       }
       else
       {
       cmsCIEXYZ WhitePt;
	   char WhiteStr[1024];

       cmsTakeMediaWhitePoint(&WhitePt, hProfile);
       _cmsIdentifyWhitePoint(WhiteStr, &WhitePt);
	   strcat(WhiteStr, "\r\n\r\n");
	   strcat(Info, WhiteStr);
       }


       if (Icc -> stream) {
              // strcat(Info, "Physical file :");
              strcat(Info, Icc -> PhysicalFile);
       }
       return Info;
}

// Extract the target data as a big string. Does not signal if tag is not present.

BOOL LCMSEXPORT cmsTakeCharTargetData(cmsHPROFILE hProfile, char** Data, size_t* len)
{
	LPICCPROFILE  Icc = (LPICCPROFILE) (LPSTR) hProfile;
	int n;

	*Data = NULL;
	*len  = 0;

    n = SearchTag(Icc, icSigCharTargetTag);
    if (n < 0) return FALSE;				

    *len =  Icc -> TagSizes[n];
	*Data = (char*) malloc(*len + 1);

	if (!*Data) {

		cmsSignalError(-1, "Out of memory allocating CharTarget space!");
		return FALSE;
	}

	if (ReadICCAscii(hProfile, icSigCharTargetTag, *Data) < 0) 
		return FALSE;

	(*Data)[*len] = 0;	// Force a zero marker. Shouldn't be needed, but is 
						// here for simplify things.

	return TRUE;    
}


// Write profile ------------------------------------------------------------

// Transports to properly encoded values - note that icc profiles does use
// big endian notation.

static
icInt32Number TransportValue32(icInt32Number Value)
{
       icInt32Number Temp = Value;

       AdjustEndianess32((LPBYTE) &Temp);
       return Temp;
}

static
WORD TransportValue16(WORD Value)
{
       WORD Temp = Value;

       AdjustEndianess16((LPBYTE) &Temp);
       return Temp;
}


// Writes data to stream, also keeps used space for further reference

static
BOOL DoWrite(FILE *OutStream, size_t size, void *Ptr)
{
	   if (size == 0) return TRUE;
	   
	   UsedSpace += size;

       if (OutStream == NULL) {

              return TRUE;
       }

       return (fwrite(Ptr, size, 1, OutStream) == 1);
}


static
BOOL SaveWordsTable(FILE* OutStream, int nEntries, LPWORD Tab)
{
   size_t nTabSize = sizeof(WORD) * nEntries;
   LPWORD PtrW = (LPWORD) malloc(nTabSize);

   if (!PtrW) return FALSE;
   CopyMemory(PtrW, Tab, nTabSize);
   AdjustEndianessArray16(PtrW, nEntries);
   DoWrite(OutStream, nTabSize, PtrW);
   free(PtrW);
   
   return TRUE;
}

// Encodes now to date/time field

static
void EncodeDateTime(icDateTimeNumber* DateTime)
{
       time_t timer;
       struct tm *tmstr;

       time(&timer);
       tmstr = localtime(&timer);

       DateTime -> year    = TransportValue16((WORD) (tmstr -> tm_year + 1900));
       DateTime -> month   = TransportValue16((WORD) (tmstr -> tm_mon + 1));
       DateTime -> day     = TransportValue16((WORD) tmstr -> tm_mday);
       DateTime -> hours   = TransportValue16((WORD) tmstr -> tm_hour);
       DateTime -> minutes = TransportValue16((WORD) tmstr -> tm_min);
       DateTime -> seconds = TransportValue16((WORD) tmstr -> tm_sec);

}



// Saves profile header

static
BOOL SaveHeader(FILE *OutStream, LPICCPROFILE Icc)
{
  icHeader Header;

       Header.size        = TransportValue32(UsedSpace);
       Header.cmmId       = TransportValue32(lcmsSignature);
       Header.version     = TransportValue32(0x02300000);
       Header.deviceClass = TransportValue32(Icc -> DeviceClass);
       Header.colorSpace  = TransportValue32(Icc -> ColorSpace);
       Header.pcs         = TransportValue32(Icc -> PCS);

       EncodeDateTime(&Header.date);

       Header.magic       = TransportValue32(icMagicNumber);
       Header.platform    = (icPlatformSignature)TransportValue32(icSigMicrosoft);  // Sorry, I must put something here

       Header.flags        = TransportValue32(Icc -> flags);
       Header.manufacturer = TransportValue32(lcmsSignature);
       Header.model        = TransportValue32(0);
       Header.attributes[0]= TransportValue32(0);              // Reflective, Glossy
       Header.attributes[1]= TransportValue32(0);

       Header.renderingIntent = TransportValue32(Icc -> RenderingIntent);

       // Illuminant is D50

       Header.illuminant.X = TransportValue32(DOUBLE_TO_FIXED(Icc -> Illuminant.X));
       Header.illuminant.Y = TransportValue32(DOUBLE_TO_FIXED(Icc -> Illuminant.Y));
       Header.illuminant.Z = TransportValue32(DOUBLE_TO_FIXED(Icc -> Illuminant.Z));

       Header.creator      = TransportValue32(lcmsSignature);

       ZeroMemory(&Header.reserved, sizeof(Header.reserved));


       UsedSpace = 0; // Mark as begin-of-file

       return DoWrite(OutStream, sizeof(icHeader), &Header);
}



// Setup base marker

static
BOOL SetupBase(FILE *OutStream, icTagTypeSignature sig)
{
    icTagBase  Base;

    Base.sig = TransportValue32(sig);
    ZeroMemory(&Base.reserved, sizeof(Base.reserved));
    return DoWrite(OutStream, sizeof(icTagBase), &Base);
}


// Store an XYZ tag

static
BOOL SaveXYZNumber(FILE *OutStream, LPcmsCIEXYZ Value)
{

    icXYZNumber XYZ;

    if (!SetupBase(OutStream, icSigXYZType)) return FALSE;

    XYZ.X = TransportValue32(DOUBLE_TO_FIXED(Value -> X));
    XYZ.Y = TransportValue32(DOUBLE_TO_FIXED(Value -> Y));
    XYZ.Z = TransportValue32(DOUBLE_TO_FIXED(Value -> Z));


    return DoWrite(OutStream, sizeof(icXYZNumber), &XYZ);
}



// Store a curve type.

static
BOOL SaveGamma(FILE *OutStream, LPGAMMATABLE Gamma)
{
    icInt32Number Count;
    int i;


    if (!SetupBase(OutStream, icSigCurveType)) return FALSE;

    Count = TransportValue32(Gamma->nEntries);

    if (!DoWrite(OutStream, sizeof(icInt32Number), &Count)) return FALSE;

    for (i=0; i < Gamma->nEntries; i++)
    {
    WORD Val = TransportValue16(Gamma->GammaTable[i]);

    if (!DoWrite(OutStream, sizeof(WORD), &Val))
              return FALSE;
    }

    return TRUE;
}




// Save an DESC Tag 

static
BOOL SaveDescription(FILE *OutStream, const char *Text)
{

    size_t len, Count, TotalSize, AlignedSize, FillerSize;
    char Filler[80];

    len = strlen(Text) + 1;

    TotalSize = sizeof(icTagBase) + sizeof(size_t) + 71 + len;
    AlignedSize = ALIGNLONG(TotalSize);
    FillerSize  = AlignedSize - TotalSize;

    if (!SetupBase(OutStream, icSigTextDescriptionType)) return FALSE;

    Count = TransportValue32(len);
    if (!DoWrite(OutStream, sizeof(size_t), &Count)) return FALSE;
    if (!DoWrite(OutStream, len, (LPVOID)Text)) return FALSE;

    ZeroMemory(Filler, 80);
    if (!DoWrite(OutStream, FillerSize, Filler)) return FALSE;

    return DoWrite(OutStream, 71, Filler);
}

// Save an ASCII Tag 

static
BOOL SaveText(FILE *OutStream, const char *Text)
{
    size_t len = strlen(Text) + 1;

    if (!SetupBase(OutStream, icSigTextType)) return FALSE;
    if (!DoWrite(OutStream, len, (LPVOID) Text)) return FALSE;
    return TRUE;
}


// Save one of these new chromaticity values

static
BOOL SaveOneChromaticity(FILE *OutStream, double x, double y)
{
       Fixed32 xf, yf;

       xf = TransportValue32(DOUBLE_TO_FIXED(x));
       yf = TransportValue32(DOUBLE_TO_FIXED(y));

       if (!DoWrite(OutStream, sizeof(Fixed32), &xf)) return FALSE;
       if (!DoWrite(OutStream, sizeof(Fixed32), &yf)) return FALSE;

       return TRUE;
}


// New tag added in Addendum II of old spec.

static
BOOL SaveChromaticities(FILE *OutStream, LPcmsCIExyYTRIPLE chrm)
{
       WORD nChans, Table;
	   DWORD Reserved = 0;

       if (!SetupBase(OutStream, icSigChromaticityType)) return FALSE;

	   if (!DoWrite(OutStream, sizeof(DWORD) , &Reserved)) return FALSE;
       nChans = TransportValue16(3);
       if (!DoWrite(OutStream, sizeof(WORD) , &nChans)) return FALSE;
       Table =  TransportValue16(0);
       if (!DoWrite(OutStream, sizeof(WORD) , &Table)) return FALSE;

       if (!SaveOneChromaticity(OutStream, chrm -> Red.x, chrm -> Red.y)) return FALSE;
       if (!SaveOneChromaticity(OutStream, chrm -> Green.x, chrm -> Green.y)) return FALSE;
       if (!SaveOneChromaticity(OutStream, chrm -> Blue.x, chrm -> Blue.y)) return FALSE;

       return TRUE;
}


// Does serialization of LUT and writes to disk. I'm always using LUT16 type
// because it seems to me (only a feeling, not a proven fact) that the
// interpolation makes more accurate a table of 16 bps that same with double
// of nodes on LUT8. Anyway, this should be regarded more carefully

static
BOOL SaveLUT(FILE* OutStream, const LPLUT NewLUT)
{
       icLut16 LUT16;
       unsigned int i;
       size_t nTabSize;
       WORD NullTbl[2] = { 0, 0xFFFFU};


       if (!SetupBase(OutStream, icSigLut16Type)) return FALSE;

       LUT16.clutPoints = (icUInt8Number) NewLUT -> cLutPoints;
       LUT16.inputChan  = (icUInt8Number) NewLUT -> InputChan;
       LUT16.outputChan = (icUInt8Number) NewLUT -> OutputChan;

       LUT16.inputEnt   = TransportValue16((WORD) ((NewLUT -> wFlags & LUT_HASTL1) ? NewLUT -> InputEntries  : 2));
       LUT16.outputEnt  = TransportValue16((WORD) ((NewLUT -> wFlags & LUT_HASTL2) ? NewLUT -> OutputEntries : 2));

       if (NewLUT -> wFlags & LUT_HASMATRIX) {

       LUT16.e00 = TransportValue32(NewLUT -> Matrix.v[0].n[0]);
       LUT16.e01 = TransportValue32(NewLUT -> Matrix.v[0].n[1]);
       LUT16.e02 = TransportValue32(NewLUT -> Matrix.v[0].n[2]);
       LUT16.e10 = TransportValue32(NewLUT -> Matrix.v[1].n[0]);
       LUT16.e11 = TransportValue32(NewLUT -> Matrix.v[1].n[1]);
       LUT16.e12 = TransportValue32(NewLUT -> Matrix.v[1].n[2]);
       LUT16.e20 = TransportValue32(NewLUT -> Matrix.v[2].n[0]);
       LUT16.e21 = TransportValue32(NewLUT -> Matrix.v[2].n[1]);
       LUT16.e22 = TransportValue32(NewLUT -> Matrix.v[2].n[2]);
       }
       else {

       LUT16.e00 = TransportValue32(DOUBLE_TO_FIXED(1));
       LUT16.e01 = TransportValue32(DOUBLE_TO_FIXED(0));
       LUT16.e02 = TransportValue32(DOUBLE_TO_FIXED(0));
       LUT16.e10 = TransportValue32(DOUBLE_TO_FIXED(0));
       LUT16.e11 = TransportValue32(DOUBLE_TO_FIXED(1));
       LUT16.e12 = TransportValue32(DOUBLE_TO_FIXED(0));
       LUT16.e20 = TransportValue32(DOUBLE_TO_FIXED(0));
       LUT16.e21 = TransportValue32(DOUBLE_TO_FIXED(0));
       LUT16.e22 = TransportValue32(DOUBLE_TO_FIXED(1));
       }


       // Save header

       DoWrite(OutStream,  sizeof(icLut16)- SIZEOF_UINT16_ALIGNED, &LUT16);

       // The prelinearization table

       for (i=0; i < NewLUT -> InputChan; i++) {

        if (NewLUT -> wFlags & LUT_HASTL1) {

               if (!SaveWordsTable(OutStream, NewLUT -> InputEntries, NewLUT -> L1[i])) return FALSE;

        }
        else DoWrite(OutStream, sizeof(WORD)* 2, NullTbl);
       }


       nTabSize = (NewLUT -> OutputChan * uipow(NewLUT->cLutPoints,
                                                 NewLUT->InputChan));
       // The 3D CLUT.

       if (!SaveWordsTable(OutStream, nTabSize, NewLUT -> T)) return FALSE;

       // The postlinearization table

       for (i=0; i < NewLUT -> OutputChan; i++) {

        if (NewLUT -> wFlags & LUT_HASTL2) {

                if (!SaveWordsTable(OutStream, NewLUT -> OutputEntries, NewLUT -> L2[i])) return FALSE;
        }
        else DoWrite(OutStream, sizeof(WORD)* 2, NullTbl);

       }

        return TRUE;
}

// Saves Tag directory

static
BOOL SaveTagDirectory(FILE *OutStream, LPICCPROFILE Icc)
{
       icInt32Number i;
       icTag Tag;
       icInt32Number TagCount = TransportValue32(Icc -> TagCount);

       if (!DoWrite(OutStream, sizeof(icInt32Number) , &TagCount)) return FALSE;

       for (i=0; i < Icc -> TagCount; i++) {

          Tag.sig    = (icTagSignature)TransportValue32(Icc -> TagNames[i]);
          Tag.offset = TransportValue32(Icc -> TagOffsets[i]);
          Tag.size   = TransportValue32(Icc -> TagSizes[i]);

          if (!DoWrite(OutStream, sizeof(icTag), &Tag)) return FALSE;
       }

       return TRUE;
}


// Dump tag contents

static
BOOL SaveTags(FILE *OutStream, LPICCPROFILE Icc)
{

    LPBYTE Data;
    icInt32Number i;
    size_t Begin;
	size_t AlignedSpace, FillerSize;


    for (i=0; i < Icc -> TagCount; i++) {

		
        // Align to DWORD boundary, following new spec.
		
		AlignedSpace = ALIGNLONG(UsedSpace);
        FillerSize  = AlignedSpace - UsedSpace;
		if (FillerSize > 0)  {
			
			BYTE Filler[20];

			ZeroMemory(Filler, 16);
		    if (!DoWrite(OutStream, FillerSize, Filler)) return FALSE;
		}
		
		
       Icc -> TagOffsets[i] = Begin = UsedSpace;
       Data = (LPBYTE) Icc -> TagPtrs[i];
       if (!Data)
              continue;

       switch (Icc -> TagNames[i]) {

       case icSigProfileDescriptionTag: 
	   case icSigDeviceMfgDescTag:
       case icSigDeviceModelDescTag:
              if (!SaveDescription(OutStream, (const char *) Data)) return FALSE;
              break;

       case icSigRedColorantTag:
       case icSigGreenColorantTag:
       case icSigBlueColorantTag:
       case icSigMediaWhitePointTag:
       case icSigMediaBlackPointTag:		   
               if (!SaveXYZNumber(OutStream, (LPcmsCIEXYZ) Data)) return FALSE;
               break;


       case icSigRedTRCTag:
       case icSigGreenTRCTag:
       case icSigBlueTRCTag:
       case icSigGrayTRCTag:
               if (!SaveGamma(OutStream, (LPGAMMATABLE) Data)) return FALSE;
               break;

	   case icSigCharTargetTag:
       case icSigCopyrightTag:      
              if (!SaveText(OutStream, (const char *) Data)) return FALSE;
              break;

       case icSigChromaticityTag:
              if (!SaveChromaticities(OutStream, (LPcmsCIExyYTRIPLE) Data)) return FALSE;
              break;

       // Save LUT 

       case icSigAToB0Tag:
       case icSigAToB1Tag:
       case icSigAToB2Tag:
       case icSigBToA0Tag:
       case icSigBToA1Tag:
       case icSigBToA2Tag:
       case icSigGamutTag:
       case icSigPreview0Tag:
       case icSigPreview1Tag:
       case icSigPreview2Tag:
              if (!SaveLUT(OutStream, (LPLUT) Data)) return FALSE;
              break;


       default:
              return FALSE;
       }

       Icc -> TagSizes[i] = (UsedSpace - Begin);
       }

		

       return TRUE;
}

BOOL LCMSEXPORT _cmsAddXYZTag(cmsHPROFILE hProfile, icTagSignature sig, const LPcmsCIEXYZ XYZ)
{
       LPICCPROFILE Icc = (LPICCPROFILE) (LPSTR) hProfile;
       
       InitTag(Icc, sig, sizeof(cmsCIEXYZ), XYZ);
       return TRUE;
}


BOOL LCMSEXPORT _cmsAddTextTag(cmsHPROFILE hProfile, icTagSignature sig, const char* Text)
{
       LPICCPROFILE Icc = (LPICCPROFILE) (LPSTR) hProfile;

       InitTag(Icc, sig, strlen(Text)+1, (LPVOID) Text);
       return TRUE;
}


BOOL LCMSEXPORT _cmsAddLUTTag(cmsHPROFILE hProfile, icTagSignature sig, void* lut)
{
       LPICCPROFILE Icc = (LPICCPROFILE) (LPSTR) hProfile;

       InitTag(Icc, sig, sizeof(LUT), lut);
       return TRUE;
}


BOOL LCMSEXPORT _cmsAddGammaTag(cmsHPROFILE hProfile, icTagSignature sig, LPGAMMATABLE TransferFunction)
{
	LPICCPROFILE Icc = (LPICCPROFILE) (LPSTR) hProfile;

	InitTag(Icc, sig, SizeOfGammaTab(TransferFunction), TransferFunction);
	return TRUE;
}


BOOL LCMSEXPORT _cmsAddChromaticityTag(cmsHPROFILE hProfile, icTagSignature sig, LPcmsCIExyYTRIPLE Chrm)
{
	LPICCPROFILE Icc = (LPICCPROFILE) (LPSTR) hProfile;

	InitTag(Icc, sig, sizeof(cmsCIExyYTRIPLE), Chrm);
	return TRUE;
}


// Add tags to profile structure

BOOL LCMSEXPORT cmsAddTag(cmsHPROFILE hProfile, icTagSignature sig, void* Tag)
{
   switch (sig) {

	   case icSigCharTargetTag:
       case icSigCopyrightTag:             
       case icSigProfileDescriptionTag: 
	   case icSigDeviceMfgDescTag:
       case icSigDeviceModelDescTag:
              return _cmsAddTextTag(hProfile, sig, (const char*) Tag);

       case icSigRedColorantTag:
       case icSigGreenColorantTag:
       case icSigBlueColorantTag:
       case icSigMediaWhitePointTag:
       case icSigMediaBlackPointTag:		   
			  return _cmsAddXYZTag(hProfile, sig, (const LPcmsCIEXYZ) Tag);
               

       case icSigRedTRCTag:
       case icSigGreenTRCTag:
       case icSigBlueTRCTag:
       case icSigGrayTRCTag:
              return _cmsAddGammaTag(hProfile, sig, (LPGAMMATABLE) Tag);
	                 
       case icSigAToB0Tag:
       case icSigAToB1Tag:
       case icSigAToB2Tag:
       case icSigBToA0Tag:
       case icSigBToA1Tag:
       case icSigBToA2Tag:
       case icSigGamutTag:
       case icSigPreview0Tag:
       case icSigPreview1Tag:
       case icSigPreview2Tag:
              return _cmsAddLUTTag(hProfile, sig, Tag);
              

	   case icSigChromaticityTag:
			  return _cmsAddChromaticityTag(hProfile, sig, (LPcmsCIExyYTRIPLE) Tag);              
        
	   default:
			cmsSignalError(-1, "cmsAddTag: Tag '%x' is unsupported", sig);
			return FALSE;
   }

}

// Low-level save
BOOL LCMSEXPORT _cmsSaveProfile(cmsHPROFILE hProfile, const char* FileName)
{
       FILE *OutStream;
       LPICCPROFILE Icc = (LPICCPROFILE) (LPSTR) hProfile;

	  
       // Pass #1 does compute offsets

       if (!SaveHeader(NULL, Icc)) return FALSE;
       if (!SaveTagDirectory(NULL, Icc)) return FALSE;
       if (!SaveTags(NULL, Icc)) return FALSE;
       OutStream = fopen(FileName, "wb");
       if (!OutStream) return FALSE;

       // Pass #2 does save to file

       if (!SaveHeader(OutStream, Icc)) goto CleanUp;
       if (!SaveTagDirectory(OutStream, Icc)) goto CleanUp;
       if (!SaveTags(OutStream, Icc)) goto CleanUp;

       return (fclose(OutStream) == 0);

   CleanUp:

       fclose(OutStream);
       unlink(FileName);
       return FALSE;
}

