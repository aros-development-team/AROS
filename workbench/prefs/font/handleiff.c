#include <stdio.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>

extern struct Library *IFFParseBase;
extern void quitApp(UBYTE *, UBYTE);
struct IFFHandle *iffHandle;

void writeIFF(UBYTE *fileName, struct FontPrefs **fontPrefs)
{
 struct PrefHeader prefHeader; /* Allocate this memory using AllocMem()!*/
 UBYTE a = 0, b = 0;

 if(iffHandle = AllocIFF())
 {
  if(iffHandle->iff_Stream = (IPTR)Open(fileName, MODE_NEWFILE))
  {
   InitIFFasDOS(iffHandle); /* Can't fail? Look it up! */

   if(!(b = OpenIFF(iffHandle, IFFF_WRITE))) /* NULL = successful! */
   {
    PushChunk(iffHandle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN);

    prefHeader.ph_Version = PHV_CURRENT;
    prefHeader.ph_Type = NULL;
    prefHeader.ph_Flags = 0; /* Set to 0; see <prefs/prefhdr.h> */

    PushChunk(iffHandle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* IFFSIZE_UNKNOWN? */
    WriteChunkBytes(iffHandle, &prefHeader, sizeof(struct PrefHeader));
    PopChunk(iffHandle);

    for(a = 0; a <= 2; a++)
    {
     //printf("\nCreating ID_FONT chunk... a = %d\n", a);

     b = PushChunk(iffHandle, ID_PREF, ID_FONT, sizeof(struct FontPrefs));
     if(b)
      printf("error: PushChunk() = %d ", b);

     printf("fontPrefs = %d bytes struct FontPrefs = %d bytes\n", sizeof(fontPrefs), sizeof(struct FontPrefs));
     b = WriteChunkBytes(iffHandle, fontPrefs[a], sizeof(struct FontPrefs));
     //if(b)
      //printf("WriteChunkyBytes(fontPrefs) = %d ", b);

     b = PopChunk(iffHandle);
     if(b)
      printf("error: PopChunk() = %d ", b);

     //printf("\ndone! a = %d", a);
    }

    // Terminate the FORM
    PopChunk(iffHandle);
   }
   else
    printf("Unable to open IFF stream (OpenIFF() returned %d)!\n", b);
  }
  else
   printf("Unable to open preferences!\n");

  CloseIFF(iffHandle);

  if((BPTR)iffHandle->iff_Stream) // File can't be closed prior to CloseIFF()!
   Close(iffHandle->iff_Stream);
 }
 else
  printf("Can't allocate IFF handle!\n");

 printf("Finished writing IFF file\n");
}

void readIFF(UBYTE *fileName, struct FontPrefs **readFontPrefs)
{
 UBYTE a, b;
 struct ContextNode *conNode;

 if(!(iffHandle = AllocIFF()))
  quitApp("Unable to allocate IFF handle!", 20);

 if(iffHandle->iff_Stream = (IPTR)Open(fileName, MODE_OLDFILE)) // Whats up with the "IPTR"? Why not the usual "BPTR"?
 {
  InitIFFasDOS(iffHandle); // No need to check for errors? RKRM:Libraries p. 781

  if(!(b = OpenIFF(iffHandle, IFFF_READ))) // NULL = successful!
  {
   // We want some sanity checking here!
   for(a = 0; a <= 2; a++)
   {
    printf("Reading chunk %d...\n", a);
    b = StopChunk(iffHandle, ID_PREF, ID_FONT);
    printf("StopChunk returned %d. sizeof(FontPrefs) = %d\n", b, sizeof(struct FontPrefs));

    ParseIFF(iffHandle, IFFPARSE_SCAN);
    conNode = CurrentChunk(iffHandle);
    // printf("chunk size = %ld\n", conNode->cn_Size);

    // Check what structure goes where!
    b = ReadChunkBytes(iffHandle, readFontPrefs[a], sizeof(struct FontPrefs));
    // printf("Read %d bytes\n", b);
    readFontPrefs[a]->fp_TextAttr.ta_Name = readFontPrefs[a]->fp_Name;
    // printf("readFontPrefs->fp_Name = >%s<\n", readFontPrefs[a]->fp_Name);
   }
  }

  CloseIFF(iffHandle);

  Close((BPTR)iffHandle->iff_Stream);
  printf("Finished reading IFF file\n");
 }
 else
 {
  printf("Can't open preferences file!\n");
  CloseIFF(iffHandle);
 }
}
