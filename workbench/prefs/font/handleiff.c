/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "global.h"

#include <proto/iffparse.h>
#include <prefs/prefhdr.h>

#include <aros/macros.h>

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

BOOL writeIFF(UBYTE *fileName, struct FontPrefs **fontPrefs)
{
    //struct PrefHeader prefHeader; // Allocate this memory using AllocMem()!
    BOOL errorCode = TRUE;
    struct FilePrefHeader prefHeader; // TODO: Allocate this memory using AllocMem() instead!
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

		    if(b) // TODO: We need some error checking here!
			printf("error: PushChunk() = %d ", b);

		    kprintf("fontPrefs = %d bytes struct FontPrefs = %d bytes\n", sizeof(fontPrefs), sizeof(struct FontPrefs));

		    convertEndian(fontPrefs[a]); // Convert to m68k endian

		    b = WriteChunkBytes(iffHandle, fontPrefs[a], sizeof(struct FontPrefs));

		    b = PopChunk(iffHandle);

		    convertEndian(fontPrefs[a]); // Revert to initial endian

		    if(b) // TODO: We need some error checking here!
			printf("error: PopChunk() = %d ", b);
		}

		// Terminate the FORM
		PopChunk(iffHandle);
	    }
	    else
	    {
		displayError(getCatalog(catalogPtr, MSG_CANT_OPEN_STREAM));
		errorCode = FALSE;
	    }
	}
	else
	    // Unable to write - this is not run time critical; continue code flow
	    displayError(getCatalog(catalogPtr, MSG_CANT_WRITE_PREFFILE));

	// CloseIFF() in iffparse.library 39 accepts NULL, but earlier versions doesn't
	if(iffHandle)
	    CloseIFF(iffHandle);

	if((BPTR)iffHandle->iff_Stream) // File can't be closed prior to CloseIFF()!
	    Close((BPTR)iffHandle->iff_Stream); // Why isn't this stored in memory as a "BPTR"? Look up!
    }
    else // AllocIFF()
    {
	// Do something more here - if IFF allocation has failed, something isn't right
	displayError(getCatalog(catalogPtr, MSG_CANT_ALLOCATE_IFFPTR));
	errorCode = FALSE;
    }

    kprintf("Finished writing IFF file\n");

    return(errorCode);
}

BOOL readIFF(UBYTE *fileName, struct FontPrefs **readFontPrefs)
{
    UBYTE a;
    LONG error;
    struct ContextNode *conNode;

    kprintf("reading %s preferences...\n", fileName);

    if(!(iffHandle = AllocIFF()))
    {
	displayError(getCatalog(catalogPtr, MSG_CANT_ALLOCATE_IFFPTR));
	return(FALSE);
    }

    if((iffHandle->iff_Stream = (IPTR)Open(fileName, MODE_OLDFILE))) // Whats up with the "IPTR"? Why not the usual "BPTR"?
    {
	InitIFFasDOS(iffHandle); // No need to check for errors? RKRM:Libraries p. 781

	if(!(error = OpenIFF(iffHandle, IFFF_READ))) // NULL = successful!
	{
	    // TODO: We want some sanity checking here!
	    for(a = 0; a <= 2; a++)
	    {
		if(0 <= (error = StopChunk(iffHandle, ID_PREF, ID_FONT)))
		{
		    kprintf("StopChunk() returned %ld\n", error);

		    if(0 <= (error = ParseIFF(iffHandle, IFFPARSE_SCAN)))
		    {
			kprintf("ParseIFF returned %ld\n", error);

			conNode = CurrentChunk(iffHandle);

			// Check what structure goes where!
			error = ReadChunkBytes(iffHandle, readFontPrefs[a], sizeof(struct FontPrefs));

			if(error < 0)
			    printf("Error: ReadChunkBytes() returned %ld!\n", error);

			readFontPrefs[a]->fp_TextAttr.ta_Name = readFontPrefs[a]->fp_Name;

			kprintf("readFontPrefs->YSize = >%d< ", readFontPrefs[a]->fp_TextAttr.ta_YSize);

			convertEndian(readFontPrefs[a]);

			kprintf("Converted = >%d<\n", readFontPrefs[a]->fp_TextAttr.ta_YSize);
		    }
		    else
		    {
			printf("ParseIFF() failed, returncode %ld!\n", error);
			a = 3; // Bail out!
		    }
		}
		else
		    printf("StopChunk() failed, returncode %ld!\n", error);
	    }

	    CloseIFF(iffHandle);
	}
	else
	    displayError(getCatalog(catalogPtr, MSG_CANT_OPEN_STREAM));

	Close((BPTR)iffHandle->iff_Stream);

	kprintf("Closed IFF file for reading!\n");
    }
    else
    {
	displayError(getCatalog(catalogPtr, MSG_CANT_READ_PREFFILE));
	CloseIFF(iffHandle);
    }

    return(TRUE);
}
