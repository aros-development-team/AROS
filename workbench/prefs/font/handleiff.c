#include <proto/dos.h>
#include <proto/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>

#include <aros/macros.h>

#include <stdio.h>

#define CATCOMP_NUMBERS
#include "fontprefs_strings.h"

/* This structure is AROS custom. Apperantly, Linux GCC aligns everything
   larger than one byte in four byte boundaries - with the sole expection
   of bytes themselves. Therefor, to maintain compatibility we define our
   AROS Preferences headers byte by byte to keep it six (6) bytes instead
   of eight (8) which would be the case if Linux GCC called the shots.
   Thank you Georg Steger for pointing this out!
*/
struct FilePrefHeader
{
 UBYTE ph_Version;
 UBYTE ph_Type;
 UBYTE ph_Flags[4];
};

//extern struct Library *IFFParseBase; // Unnecessary, already declared in <proto/iffparse.h>
extern struct Catalog *catalogPtr;
extern STRPTR getCatalog(struct Catalog *, ULONG);
extern void quitApp(UBYTE *, UBYTE);
struct IFFHandle *iffHandle;

void convertEndian(struct FontPrefs *fontPrefs)
{
 UBYTE a;

 for(a = 0; a <= 2; a++)
  fontPrefs->fp_Reserved[a] = AROS_BE2LONG(fontPrefs->fp_Reserved);

 fontPrefs->fp_Reserved2 = AROS_BE2WORD(fontPrefs->fp_Reserved2);
 fontPrefs->fp_Type = AROS_BE2WORD(fontPrefs->fp_Type);
 fontPrefs->fp_TextAttr.ta_YSize = AROS_BE2WORD(fontPrefs->fp_TextAttr.ta_YSize);
}

void writeIFF(UBYTE *fileName, struct FontPrefs **fontPrefs)
{
 //struct PrefHeader prefHeader; /* Allocate this memory using AllocMem()!*/
 struct FilePrefHeader prefHeader; // Allocate this memory using AllocMem() instead!
 UBYTE a = 0, b = 0;

 if((iffHandle = AllocIFF()))
 {
  if((iffHandle->iff_Stream = (IPTR)Open(fileName, MODE_NEWFILE)))
  {
   InitIFFasDOS(iffHandle); /* Can't fail? Look it up! */

   if(!(b = OpenIFF(iffHandle, IFFF_WRITE))) /* NULL = successful! */
   {
    PushChunk(iffHandle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN);

    prefHeader.ph_Version = PHV_CURRENT;
    prefHeader.ph_Type = NULL;
    for(a = 0; a <= 3; a++)
     prefHeader.ph_Flags[a] = 0; /* Set to 0; see <prefs/prefhdr.h> */

    PushChunk(iffHandle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* IFFSIZE_UNKNOWN? */
    WriteChunkBytes(iffHandle, &prefHeader, sizeof(struct FilePrefHeader));
    PopChunk(iffHandle);

    for(a = 0; a <= 2; a++)
    {
     b = PushChunk(iffHandle, ID_PREF, ID_FONT, sizeof(struct FontPrefs));
     if(b)
      printf("error: PushChunk() = %d ", b);

     printf("fontPrefs = %d bytes struct FontPrefs = %d bytes\n", sizeof(fontPrefs), sizeof(struct FontPrefs));
     convertEndian(fontPrefs[a]);
     b = WriteChunkBytes(iffHandle, fontPrefs[a], sizeof(struct FontPrefs));
     //if(b)
      //printf("WriteChunkyBytes(fontPrefs) = %d ", b);

     b = PopChunk(iffHandle);
     convertEndian(fontPrefs[a]);
     if(b)
      printf("error: PopChunk() = %d ", b);

     //printf("\ndone! a = %d", a);
    }

    // Terminate the FORM
    PopChunk(iffHandle);
   }
   else
    //printf("Unable to open IFF stream (OpenIFF() returned %d)!\n", b);
    printf("%s\n", getCatalog(catalogPtr, MSG_CANT_OPEN_STREAM));
  }
  else
   //printf("Unable to open preferences!\n");
   printf("%s\n", getCatalog(catalogPtr, MSG_CANT_WRITE_PREFFILE));

  CloseIFF(iffHandle);

  if((BPTR)iffHandle->iff_Stream) // File can't be closed prior to CloseIFF()!
   Close((BPTR)iffHandle->iff_Stream); // Why isn't this stored in memory as a "BPTR"? Look up!
 }
 else
  //printf("Can't allocate IFF handle!\n");
  // Do something more here - if IFF allocation has failed, something isn't right
  printf("%s\n", getCatalog(catalogPtr, MSG_CANT_ALLOCATE_IFFPTR));

 printf("Finished writing IFF file\n");
}

void readIFF(UBYTE *fileName, struct FontPrefs **readFontPrefs)
{
 UBYTE a;
 LONG error;
 struct ContextNode *conNode;

 printf("reading %s preferences...\n", fileName);

 if(!(iffHandle = AllocIFF()))
  quitApp(getCatalog(catalogPtr, MSG_CANT_ALLOCATE_IFFPTR), 20);

 if((iffHandle->iff_Stream = (IPTR)Open(fileName, MODE_OLDFILE))) // Whats up with the "IPTR"? Why not the usual "BPTR"?
 {
  InitIFFasDOS(iffHandle); // No need to check for errors? RKRM:Libraries p. 781

  if(!(error = OpenIFF(iffHandle, IFFF_READ))) // NULL = successful!
  {
   // We want some sanity checking here!
   for(a = 0; a <= 2; a++)
   {
    if(0 <= (error = StopChunk(iffHandle, ID_PREF, ID_FONT)))
    {
     /*printf("StopChunk returned %d. sizeof(FontPrefs) = %d\n", b, sizeof(struct FontPrefs));*/
     printf("StopChunk() returned %ld\n", error);

     if(0 <= (error = ParseIFF(iffHandle, IFFPARSE_SCAN)))
     {
      printf("ParseIFF returned %ld\n", error);
      conNode = CurrentChunk(iffHandle);
      // printf("chunk size = %ld\n", conNode->cn_Size);
      // Check what structure goes where!
      error = ReadChunkBytes(iffHandle, readFontPrefs[a], sizeof(struct FontPrefs));
      //printf("Read %d bytes\n", b);

      if(error < 0)
      printf("Error: ReadChunkBytes() returned %ld!\n", error);

      readFontPrefs[a]->fp_TextAttr.ta_Name = readFontPrefs[a]->fp_Name;
      printf("readFontPrefs->YSize = >%d<\n", readFontPrefs[a]->fp_TextAttr.ta_YSize);
      convertEndian(readFontPrefs[a]);
     }
     else
     {
      printf("ParseIFF() failed!\n");
      printf("ParseIFF() returned %ld\n", error);
      a = 3; // Bail out!
     }
    }
    else
    {
     printf("StopChunk() failed!\n");
     printf("StopChunk() returned %ld\n", error);
    }
   }

   CloseIFF(iffHandle);
  }
  else
   //printf("OpenIFF() failed!\n");
   printf("%s\n", getCatalog(catalogPtr, MSG_CANT_OPEN_STREAM));

  Close((BPTR)iffHandle->iff_Stream);
  printf("Finished reading IFF file\n");
 }
 else
 {
  //printf("Can't open preferences file!\n");
  printf("%s\n", getCatalog(catalogPtr, MSG_CANT_READ_PREFFILE));
  CloseIFF(iffHandle);
 }
}
