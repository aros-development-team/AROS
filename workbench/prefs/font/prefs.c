/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/macros.h>
#include <dos/dos.h>
#include <exec/memory.h>
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
struct FontPrefs *fp_Original[FP_COUNT];
struct FontPrefs *fp_Temporary[FP_COUNT];    

/*** Prototypes *************************************************************/
/* struct FontPrefs handling ************************************************/
static void FontPrefs_Clear(struct FontPrefs *fp[FP_COUNT]);
static BOOL FontPrefs_Allocate(struct FontPrefs *fp[FP_COUNT]);
static void FontPrefs_Free(struct FontPrefs *fp[FP_COUNT]);
static void FontPrefs_Copy(struct FontPrefs *dst[FP_COUNT], struct FontPrefs *src[FP_COUNT]);

/* File IO (low-level) ******************************************************/
BOOL FP_Write(CONST_STRPTR filename, struct FontPrefs *fp[FP_COUNT]);
BOOL FP_Read(CONST_STRPTR filename, struct FontPrefs *fp[FP_COUNT]);


/*** Functions **************************************************************/
/* Setup ********************************************************************/
static void FontPrefs_Clear(struct FontPrefs *fp[FP_COUNT])
{
    memset(fp, 0, FP_COUNT * sizeof(void *));
}

static BOOL FontPrefs_Allocate(struct FontPrefs *fp[FP_COUNT])
{
    UBYTE i;
    
    for (i = 0; i < FP_COUNT; i++)
    {
        fp[i] = AllocMem(sizeof(struct FontPrefs), MEMF_ANY | MEMF_CLEAR);
        if (fp[i] == NULL) goto error;
    }
    
    return TRUE;
    
error:
    FontPrefs_Free(fp);

    return FALSE;
}

static void FontPrefs_Free(struct FontPrefs *fp[FP_COUNT])
{
    UBYTE i;
    
    for (i = 0; i < FP_COUNT; i++)
    {
        if (fp[i] != NULL) 
        {
            FreeMem(fp[i], sizeof(struct FontPrefs));
            fp[i] = NULL;
        }
    }
}

static void FontPrefs_Copy
(
    struct FontPrefs *dst[FP_COUNT], struct FontPrefs *src[FP_COUNT]
)
{
    UBYTE i;
    
    for (i = 0; i < FP_COUNT; i++)
    {
        memcpy(dst[i], src[i], sizeof(struct FontPrefs));
        dst[i]->fp_TextAttr.ta_Name = dst[i]->fp_Name;
    }
}

void FontPrefs_Default(struct FontPrefs *fp[FP_COUNT])
{
    UBYTE i;

    for (i = 0; i < FP_COUNT; i++)
    {
        fp[i]->fp_Type     = i;
        fp[i]->fp_FrontPen = 0; /* FIXME: Is this (really) default? Look it up! */
        fp[i]->fp_BackPen  = 0; /* FIXME: Is this (really) default? Look it up! */
        fp[i]->fp_DrawMode = 0; /* FIXME: Is this (really) default? Look it up! */
        
        fp[i]->fp_TextAttr.ta_YSize = 8; /* FIXME: Is this (really) default? Look it up! */
        fp[i]->fp_TextAttr.ta_Style = FS_NORMAL;
        fp[i]->fp_TextAttr.ta_Flags = FPB_DISKFONT; /* FIXME: Is this (really) default? Look it up! */
        
        fp[i]->fp_Name[0] = '\0';
        strlcat(fp[i]->fp_Name, "topaz.font", FONTNAMESIZE); /* FIXME: Is this (really) default? Check it up! */
        fp[i]->fp_TextAttr.ta_Name = fp[i]->fp_Name;
    }
}


BOOL FP_Initialize(void)
{
    iffHandle = NULL;   /* FIXME: ?? */
    
    FontPrefs_Clear(fp_Current);
    FontPrefs_Clear(fp_Original);
    FontPrefs_Clear(fp_Temporary);
    
    if (!FontPrefs_Allocate(fp_Current)) goto error;
    if (!FontPrefs_Allocate(fp_Original)) goto error;
    if (!FontPrefs_Allocate(fp_Temporary)) goto error;
    
    FontPrefs_Default(fp_Original);
    FontPrefs_Copy(fp_Current, fp_Original);
    
    return TRUE;
    
error:
    FP_Deinitialize();
    
    return FALSE;
}

void FP_Deinitialize(void)
{
    FontPrefs_Free(fp_Temporary);
    FontPrefs_Free(fp_Current);
    FontPrefs_Free(fp_Original);
    
    if(iffHandle != NULL) FreeIFF(iffHandle);
}

/* Utility ******************************************************************/
static void convertEndian(struct FontPrefs *fontPrefs)
{
    UBYTE a;

    for (a = 0; a <= 2; a++)
    {
        fontPrefs->fp_Reserved[a] = AROS_BE2LONG(fontPrefs->fp_Reserved[2]);
    }
    
    fontPrefs->fp_Reserved2 = AROS_BE2WORD(fontPrefs->fp_Reserved2);
    fontPrefs->fp_Type = AROS_BE2WORD(fontPrefs->fp_Type);
    fontPrefs->fp_TextAttr.ta_YSize = AROS_BE2WORD(fontPrefs->fp_TextAttr.ta_YSize);
}

/* File IO (high-level) *****************************************************/
BOOL FP_LoadFrom(CONST_STRPTR filename)
{
    if (FP_Read(filename, fp_Original))
    {
        FontPrefs_Copy(fp_Current, fp_Original);
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL FP_Load(void)
{
    return FP_LoadFrom(FP_PATH_ENV);
}

BOOL FP_Test(void)
{
    return FP_Use();
}

BOOL FP_Revert(void)
{
    FontPrefs_Copy(fp_Current, fp_Original);
    
    return FP_Use();
}

BOOL FP_SaveTo(CONST_STRPTR filename)
{
    return FP_Write(filename, fp_Current);
}

BOOL FP_Save(void)
{
    if (!FP_SaveTo(FP_PATH_ENV)) return FALSE;
    if (!FP_SaveTo(FP_PATH_ENVARC)) return FALSE;
    
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
BOOL FP_Write(CONST_STRPTR filename, struct FontPrefs *fp[FP_COUNT])
{
    BOOL              rc = TRUE;
    UBYTE             a = 0, error = 0;
    struct PrefHeader header; 
    
    memset(&header, 0, sizeof(struct PrefHeader));
    
    if ((iffHandle = AllocIFF()))
    {
        if ((iffHandle->iff_Stream = (IPTR) Open(filename, MODE_NEWFILE)))
        {
            InitIFFasDOS(iffHandle);
    
            if (!(error = OpenIFF(iffHandle, IFFF_WRITE))) /* NULL = successful! */
            {
                PushChunk(iffHandle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN);
    
                header.ph_Version = PHV_CURRENT;
                header.ph_Type    = 0;
    
                PushChunk(iffHandle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* IFFSIZE_UNKNOWN? */
    
                WriteChunkBytes(iffHandle, &header, sizeof(struct PrefHeader));
    
                PopChunk(iffHandle);
    
                for (a = 0; a < FP_COUNT; a++)
                {
                    error = PushChunk(iffHandle, ID_PREF, ID_FONT, sizeof(struct FontPrefs));
    
                    if (error != 0) // TODO: We need some error checking here!
                    {
                        printf("error: PushChunk() = %d ", error);
                    }
                    
                    convertEndian(fp[a]); // Convert to m68k endian
                    
                    error = WriteChunkBytes(iffHandle, fp[a], sizeof(struct FontPrefs));
                    error = PopChunk(iffHandle);
    
                    convertEndian(fp[a]); // Revert to initial endian
    
                    if (error != 0) // TODO: We need some error checking here!
                    {
                        printf("error: PopChunk() = %d ", error);
                    }
                }
    
                // Terminate the FORM
                PopChunk(iffHandle);
            }
            else
            {
                ShowError(_(MSG_CANT_OPEN_STREAM));
                rc = FALSE;
            }
        }
        else
        {
            // Unable to write - this is not run time critical; continue code flow
            ShowError(_(MSG_CANT_WRITE_PREFFILE));
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
	
	FreeIFF(iffHandle);
	iffHandle = NULL;
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        rc = FALSE;
    }

    return rc;
}

BOOL FP_Read(CONST_STRPTR filename, struct FontPrefs *fp[FP_COUNT])
{
    UBYTE a;
    LONG error;
    struct ContextNode *conNode;

    if (!(iffHandle = AllocIFF()))
    {
        ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        return(FALSE);
    }

    if ((iffHandle->iff_Stream = (IPTR) Open(filename, MODE_OLDFILE))) // FIXME: Whats up with the "IPTR"? Why not the usual "BPTR"?
    {
        InitIFFasDOS(iffHandle);
    
        if (!(error = OpenIFF(iffHandle, IFFF_READ))) // NULL = successful!
        {
            // FIXME: We want some sanity checking here!
            for (a = 0; a < FP_COUNT; a++)
            {
                if (0 <= (error = StopChunk(iffHandle, ID_PREF, ID_FONT)))
                {
                    if (0 <= (error = ParseIFF(iffHandle, IFFPARSE_SCAN)))
                    {
                        conNode = CurrentChunk(iffHandle);
                        
                        // Check what structure goes where!
                        error = ReadChunkBytes(iffHandle, fp[a], sizeof(struct FontPrefs));
                        
                        if (error < 0)
                        {
                            printf("Error: ReadChunkBytes() returned %ld!\n", error);
                        }
                        
                        fp[a]->fp_TextAttr.ta_Name = fp[a]->fp_Name;
                        
                        convertEndian(fp[a]);
                    }
                    else
                    {
                        printf("ParseIFF() failed, returncode %ld!\n", error);
                        break; /* FIXME: return error? */
                    }
                }
                else
                {
                    printf("StopChunk() failed, returncode %ld!\n", error);
                    /* FIXME: return error? */
                }
            }
    
            CloseIFF(iffHandle);
        }
        else
        {
            ShowError(_(MSG_CANT_OPEN_STREAM));
        }
        
        Close((BPTR) iffHandle->iff_Stream);
    }
    else
    {
        ShowError(_(MSG_CANT_READ_PREFFILE));
        CloseIFF(iffHandle);
    }

    FreeIFF(iffHandle);
    iffHandle = NULL;
    
    return TRUE;
}
