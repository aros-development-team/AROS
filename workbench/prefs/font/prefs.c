/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/macros.h>
#include <dos/dos.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>

#include <proto/iffparse.h>
#include <proto/dos.h>

#include <string.h>
#include <stdio.h>

#include "locale.h"
#include "misc.h"
#include "prefs.h"

#define DEBUG 1
#include <aros/debug.h>

/*** Variables **************************************************************/
struct IFFHandle *iffHandle;
struct FontPrefs *fp_Current[FP_COUNT];


/*** Functions **************************************************************/
/* Setup ********************************************************************/
void initDefaultPrefs(struct FontPrefs **fontPrefsPtr)
{
    UBYTE a;

    for (a = 0; a <= 2; a++)
    {
	fp_Current[a]->fp_Type = a;	/* Is this 0, 1, 2 or 1, 2, 3? Look it up! */
	fp_Current[a]->fp_FrontPen = 0;	/* Is this (really) default? Look it up! */
	fp_Current[a]->fp_BackPen = 0;	/* Is this (really) default? Look it up! */
	fp_Current[a]->fp_DrawMode = 0;	/* Is this (really) default? Look it up! */

	fp_Current[a]->fp_TextAttr.ta_YSize = 8; /* Is this (really) default? Look it up! */
	fp_Current[a]->fp_TextAttr.ta_Style = FS_NORMAL;
 	fp_Current[a]->fp_TextAttr.ta_Flags = FPB_DISKFONT; /* Is this (really) default? Look it up! */

	strcpy(fp_Current[a]->fp_Name, "topaz.font"); /* Is this (really) default? Check it up! */
	fp_Current[a]->fp_TextAttr.ta_Name = fp_Current[a]->fp_Name;
    }
}

BOOL Prefs_Initialize(void)
{
    UBYTE i;

    for (i = 0; i <= 2; i++)
    {
        fp_Current[i] = AllocMem
        (
            sizeof(struct FontPrefs), MEMF_ANY | MEMF_CLEAR
        );
        
        if (fp_Current[i] == NULL)
        {
	    return FALSE; /* FIXME: Some structures may have been allocated */
        }
    }
    
    initDefaultPrefs(fp_Current);
    
    return TRUE;
}

void Prefs_Deinitialize(void)
{
    UBYTE i;
    
    for (i = 0; i <= 2; i++)
    {
        if (fp_Current[i] != NULL) FreeMem(fp_Current[i], sizeof(struct FontPrefs));
    }
    
    if(iffHandle != NULL) FreeIFF(iffHandle);
}

/* Utility ******************************************************************/
void convertEndian(struct FontPrefs *fontPrefs)
{
    UBYTE a;

    for(a = 0; a <= 2; a++)
	fontPrefs->fp_Reserved[a] = AROS_BE2LONG(fontPrefs->fp_Reserved);

    fontPrefs->fp_Reserved2 = AROS_BE2WORD(fontPrefs->fp_Reserved2);
    fontPrefs->fp_Type = AROS_BE2WORD(fontPrefs->fp_Type);
    fontPrefs->fp_TextAttr.ta_YSize = AROS_BE2WORD(fontPrefs->fp_TextAttr.ta_YSize);
}

/* File IO (high-level) *****************************************************/
BOOL FP_Test(void)
{
    return FALSE;
}

BOOL FP_Revert(void)
{
    return FALSE;
}

BOOL FP_Save(void)
{
    if (!FP_Write(FP_PATH_ENV, fp_Current)) return FALSE;
    if (!FP_Write(FP_PATH_ENVARC, fp_Current)) return FALSE;
    
    return TRUE;
}

BOOL FP_Use(void)
{
    if (!FP_Write(FP_PATH_ENV, fp_Current)) return FALSE;

    return TRUE;
}

BOOL FP_Cancel(void)
{
    return FP_Revert();
}

/* File IO (low-level) ******************************************************/
BOOL FP_Write(CONST_STRPTR filename, struct FontPrefs **fontPrefs)
{
    BOOL              rc = TRUE;
    UBYTE             a = 0, b = 0;
    struct PrefHeader header; 
    
    memset(&header, 0, sizeof(struct PrefHeader));
    
    if ((iffHandle = AllocIFF()))
    {
	if ((iffHandle->iff_Stream = (IPTR) Open(filename, MODE_NEWFILE)))
	{
	    InitIFFasDOS(iffHandle); /* Can't fail? Look it up! */

	    if (!(b = OpenIFF(iffHandle, IFFF_WRITE))) /* NULL = successful! */
	    {
		PushChunk(iffHandle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN);

		header.ph_Version = PHV_CURRENT;
		header.ph_Type = NULL;

		PushChunk(iffHandle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* IFFSIZE_UNKNOWN? */

		WriteChunkBytes(iffHandle, &header, sizeof(struct PrefHeader));

		PopChunk(iffHandle);

		for (a = 0; a <= 2; a++)
		{
		    b = PushChunk(iffHandle, ID_PREF, ID_FONT, sizeof(struct FontPrefs));

		    if (b) // TODO: We need some error checking here!
                    {
                        printf("error: PushChunk() = %d ", b);
                    }
		    kprintf("fontPrefs = %d bytes struct FontPrefs = %d bytes\n", sizeof(fontPrefs), sizeof(struct FontPrefs));

		    convertEndian(fp_Current[a]); // Convert to m68k endian

		    b = WriteChunkBytes(iffHandle, fp_Current[a], sizeof(struct FontPrefs));

		    b = PopChunk(iffHandle);

		    convertEndian(fp_Current[a]); // Revert to initial endian

		    if (b) // TODO: We need some error checking here!
                    {
                        printf("error: PopChunk() = %d ", b);
                    }
                }

		// Terminate the FORM
		PopChunk(iffHandle);
	    }
	    else
	    {
		ShowError(MSG(MSG_CANT_OPEN_STREAM));
		rc = FALSE;
	    }
	}
	else
        {
	    // Unable to write - this is not run time critical; continue code flow
	    ShowError(MSG(MSG_CANT_WRITE_PREFFILE));
        }
        
	// CloseIFF() in iffparse.library 39 accepts NULL, but earlier versions doesn't
	if (iffHandle)
        {
	    CloseIFF(iffHandle);
        }
        
	if (iffHandle->iff_Stream) // File can't be closed prior to CloseIFF()!
        {
            Close((BPTR) iffHandle->iff_Stream); // Why isn't this stored in memory as a "BPTR"? Look up!
        }
    }
    else // AllocIFF()
    {
	// Do something more here - if IFF allocation has failed, something isn't right
        ShowError(MSG(MSG_CANT_ALLOCATE_IFFPTR));
	rc = FALSE;
    }

    kprintf("Finished writing IFF file\n");

    return rc;
}

BOOL FP_Read(CONST_STRPTR filename, struct FontPrefs **readFontPrefs)
{
    UBYTE a;
    LONG error;
    struct ContextNode *conNode;

    kprintf("reading %s preferences...\n", filename);

    if(!(iffHandle = AllocIFF()))
    {
        ShowError(MSG(MSG_CANT_ALLOCATE_IFFPTR));
	return(FALSE);
    }

    if ((iffHandle->iff_Stream = (IPTR) Open(filename, MODE_OLDFILE))) // Whats up with the "IPTR"? Why not the usual "BPTR"?
    {
	InitIFFasDOS(iffHandle); // No need to check for errors? RKRM:Libraries p. 781

	if (!(error = OpenIFF(iffHandle, IFFF_READ))) // NULL = successful!
	{
	    // TODO: We want some sanity checking here!
	    for (a = 0; a <= 2; a++)
	    {
		if (0 <= (error = StopChunk(iffHandle, ID_PREF, ID_FONT)))
		{
		    kprintf("StopChunk() returned %ld\n", error);

		    if (0 <= (error = ParseIFF(iffHandle, IFFPARSE_SCAN)))
		    {
			kprintf("ParseIFF returned %ld\n", error);

			conNode = CurrentChunk(iffHandle);

			// Check what structure goes where!
			error = ReadChunkBytes(iffHandle, readFontPrefs[a], sizeof(struct FontPrefs));

			if (error < 0)
                        {
			    printf("Error: ReadChunkBytes() returned %ld!\n", error);
                        }
                        
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
        {
            ShowError(MSG(MSG_CANT_OPEN_STREAM));
        }
        
	Close((BPTR) iffHandle->iff_Stream);

	kprintf("Closed IFF file for reading!\n");
    }
    else
    {
        ShowError(MSG(MSG_CANT_READ_PREFFILE));
	CloseIFF(iffHandle);
    }

    return TRUE;
}
