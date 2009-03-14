/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <string.h>
#include <stdio.h>

#include "misc.h"
#include "locale.h"
#include "fpeditor.h"

/* Data is stored on disk in this format */
struct FileFontPrefs
{
    UBYTE   fp_Reserved[4 * 3];
    UBYTE   fp_Reserved2[2];
    UBYTE   fp_Type[2];
    UBYTE   fp_FrontPen;
    UBYTE   fp_BackPen;
    UBYTE   fp_Drawmode;
    UBYTE   fp_pad;
    UBYTE   fp_TextAttr_ta_Name[4];
    UBYTE   fp_TextAttr_ta_YSize[2];
    UBYTE   fp_TextAttr_ta_Style;
    UBYTE   fp_TextAttr_ta_Flags;
    BYTE    fp_Name[FONTNAMESIZE];
};

/*** Instance Data **********************************************************/
#define FP_COUNT (3)  /* Number of entries in fped_FontPrefs array */

struct FPEditor_DATA
{
    struct FontPrefs  fped_FontPrefs[FP_COUNT];
    Object           *fped_IconsString, 
                     *fped_ScreenString, 
                     *fped_SystemString;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct FPEditor_DATA *data = INST_DATA(CLASS, self)
#define FP(i) (&(data->fped_FontPrefs[(i)]))

/*** Utility Functions ******************************************************/
#if 0
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
#endif

STATIC VOID convertEndian(struct FontPrefs *fp)
{
    BYTE i;

    for (i = 0; i <= 2; i++)
    {
        fp->fp_Reserved[i] = AROS_BE2LONG(fp->fp_Reserved[i]);
    }
    
    fp->fp_Reserved2         = AROS_BE2WORD(fp->fp_Reserved2);
    fp->fp_Type              = AROS_BE2WORD(fp->fp_Type);
    fp->fp_TextAttr.ta_YSize = AROS_BE2WORD(fp->fp_TextAttr.ta_YSize);
}

VOID FileFontPrefs2FontPrefs(struct FileFontPrefs *ffp, struct FontPrefs *fp)
{
    /* Copy field by field to avoid any alignment problems whatsoever */
    CopyMem(&ffp->fp_Reserved, &fp->fp_Reserved, sizeof(fp->fp_Reserved));
    CopyMem(&ffp->fp_Reserved2, &fp->fp_Reserved2, sizeof(fp->fp_Reserved2));
    CopyMem(&ffp->fp_Type, &fp->fp_Type, sizeof(fp->fp_Type));
    fp->fp_FrontPen = ffp->fp_FrontPen;
    fp->fp_BackPen = ffp->fp_BackPen;
    fp->fp_DrawMode = ffp->fp_Drawmode;
    fp->fp_TextAttr.ta_Name = fp->fp_Name;
    CopyMem
    (
	&ffp->fp_TextAttr_ta_YSize,
	&fp->fp_TextAttr.ta_YSize,
	sizeof(fp->fp_TextAttr.ta_YSize)
    );
    fp->fp_TextAttr.ta_Style = ffp->fp_TextAttr_ta_Style;
    fp->fp_TextAttr.ta_Flags = ffp->fp_TextAttr_ta_Flags;
    CopyMem(&ffp->fp_Name, &fp->fp_Name, FONTNAMESIZE);
}

VOID FontPrefs2FileFontPrefs(struct FontPrefs *fp, struct FileFontPrefs *ffp)
{
    /* Copy field by field to avoid any alignment problems whatsoever */
    CopyMem(&fp->fp_Reserved, &ffp->fp_Reserved, sizeof(fp->fp_Reserved));
    CopyMem(&fp->fp_Reserved2, &ffp->fp_Reserved2, sizeof(fp->fp_Reserved2));
    CopyMem(&fp->fp_Type, &ffp->fp_Type, sizeof(fp->fp_Type));
    ffp->fp_FrontPen = fp->fp_FrontPen;
    ffp->fp_BackPen = fp->fp_BackPen;
    ffp->fp_Drawmode = fp->fp_DrawMode;
    /* fp->fp_TextAttr.ta_Name is not copied, it may have different sizes on
       different architectures and contains only a pointer, so I guess there's
       no need to write it on disk. */
    CopyMem
    (
	&fp->fp_TextAttr.ta_YSize,
	&ffp->fp_TextAttr_ta_YSize, 
	sizeof(fp->fp_TextAttr.ta_YSize)
    );
    ffp->fp_TextAttr_ta_Style = fp->fp_TextAttr.ta_Style;
    ffp->fp_TextAttr_ta_Flags = fp->fp_TextAttr.ta_Flags;
    CopyMem(&fp->fp_Name, &ffp->fp_Name, FONTNAMESIZE);
}

VOID FontPrefs2FontString
(
    STRPTR buffer, ULONG buffersize, struct FontPrefs *fp
)
{
    snprintf
    (
        buffer, buffersize, "%.*s/%d", 
        strlen(fp->fp_TextAttr.ta_Name) - 5 /* strlen(".font") */, 
        fp->fp_TextAttr.ta_Name, fp->fp_TextAttr.ta_YSize
    );
}

BOOL FontString2FontPrefs(struct FontPrefs *fp, CONST_STRPTR buffer)
{
    STRPTR separator    = PathPart((STRPTR) buffer);
    ULONG  nameLength   = separator - buffer;
    ULONG  suffixLength = 5; /* strlen(".font") */
    ULONG  size;
    
    if (nameLength + suffixLength >= FONTNAMESIZE)
    {
        /* Not enough space for the font name */
        return FALSE;
    }
    
    snprintf
    (
        fp->fp_Name, nameLength + suffixLength + 1, "%.*s.font", 
        (int) nameLength, buffer
    ); 
    fp->fp_TextAttr.ta_Name = fp->fp_Name;
    
    StrToLong(FilePart((STRPTR) buffer), &size);
    fp->fp_TextAttr.ta_YSize = size;

    return TRUE;
}

BOOL Gadgets2FontPrefs
(
    struct FPEditor_DATA *data
)
{
    STRPTR str = NULL;
    
    // FIXME: error checking
    GET(data->fped_IconsString, MUIA_String_Contents, &str);
    FontString2FontPrefs(FP(FP_WBFONT), str);
    FP(FP_WBFONT)->fp_Type = FP_WBFONT;
    
    GET(data->fped_SystemString, MUIA_String_Contents, &str);
    FontString2FontPrefs(FP(FP_SYSFONT), str);
    FP(FP_SYSFONT)->fp_Type = FP_SYSFONT;
    
    GET(data->fped_ScreenString, MUIA_String_Contents, &str);
    FontString2FontPrefs(FP(FP_SCREENFONT), str);
    FP(FP_SCREENFONT)->fp_Type = FP_SCREENFONT;
    
    return TRUE;
}

BOOL FontPrefs2Gadgets
(
    struct FPEditor_DATA *data
)
{
    TEXT buffer[FONTNAMESIZE + 8];
     
    // FIXME: error checking
    FontPrefs2FontString(buffer, FONTNAMESIZE + 8, FP(FP_WBFONT));
    NNSET(data->fped_IconsString, MUIA_String_Contents, (IPTR) buffer);
    
    FontPrefs2FontString(buffer, FONTNAMESIZE + 8, FP(FP_SYSFONT));
    NNSET(data->fped_SystemString, MUIA_String_Contents, (IPTR) buffer);
    
    FontPrefs2FontString(buffer, FONTNAMESIZE + 8, FP(FP_SCREENFONT));
    NNSET(data->fped_ScreenString, MUIA_String_Contents, (IPTR) buffer);
    
    return TRUE;
}

/*** Methods ****************************************************************/
Object *FPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *iconsString, *screenString, *systemString;
    
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_PrefsEditor_Name,        __(MSG_NAME),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/Font.prefs",
        
        Child, (IPTR) ColGroup(2),
            Child, (IPTR) Label2(_(MSG_ICONS)),
            Child, (IPTR) PopaslObject,
                MUIA_Popasl_Type,              ASL_FontRequest,
                ASLFO_MaxHeight,               100,
                MUIA_Popstring_String,  (IPTR) (iconsString = (Object *)StringObject, 
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                End),
                MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
            End,
            Child, (IPTR) Label2(_(MSG_SCREEN)),
            Child, (IPTR) PopaslObject,
                MUIA_Popasl_Type,              ASL_FontRequest,
                ASLFO_MaxHeight,               100,
                MUIA_Popstring_String,  (IPTR) (screenString = (Object *)StringObject, 
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                End),
                MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
            End,
            Child, (IPTR) Label2(_(MSG_SYSTEM)),
            Child, (IPTR) PopaslObject,
                MUIA_Popasl_Type,              ASL_FontRequest,
                ASLFO_FixedWidthOnly,          TRUE,
                ASLFO_MaxHeight,               100,
                MUIA_Popstring_String,  (IPTR) (systemString = (Object *)StringObject, 
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                End),
                MUIA_Popstring_Button,  (IPTR) PopButton(MUII_PopUp),
            End,
        End,
        
        TAG_DONE
    );
    
    if (self != NULL)
    {
        SETUP_INST_DATA;
        data->fped_IconsString  = iconsString;
        data->fped_ScreenString = screenString;
        data->fped_SystemString = systemString;
        
        /*-- Setup notifications -------------------------------------------*/
        DoMethod
        (
            iconsString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            screenString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            systemString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
    }
    
    return self;
}

IPTR FPEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    struct ContextNode  *context;
    struct IFFHandle    *handle;
    BOOL                 success = TRUE;
    LONG                 error;
    
    if (!(handle = AllocIFF()))
    {
        ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        return(FALSE);
    }
    
    handle->iff_Stream = (IPTR) message->fh;
    InitIFFasDOS(handle);

    if ((error = OpenIFF(handle, IFFF_READ)) == 0)
    {
        BYTE i;
        
        // FIXME: We want some sanity checking here!
        for (i = 0; i < FP_COUNT; i++)
        {
            if ((error = StopChunk(handle, ID_PREF, ID_FONT)) == 0)
            {
                if ((error = ParseIFF(handle, IFFPARSE_SCAN)) == 0)
                {
                    struct FileFontPrefs ffp;
                    context = CurrentChunk(handle);
                    
                    error = ReadChunkBytes
                    (
                        handle, &ffp, sizeof(struct FileFontPrefs)
                    );
                    
                    if (error < 0)
                    {
                        printf("Error: ReadChunkBytes() returned %ld!\n", error);
                    }
                    
                    FileFontPrefs2FontPrefs(&ffp, FP(i));
                    
                    convertEndian(FP(i));
                }
                else
                {
                    printf("ParseIFF() failed, returncode %ld!\n", error);
                    success = FALSE;
                    break;
                }
            }
            else
            {
                printf("StopChunk() failed, returncode %ld!\n", error);
                success = FALSE;
            }
        }

        CloseIFF(handle);
    }
    else
    {
        ShowError(_(MSG_CANT_OPEN_STREAM));
    }

    FreeIFF(handle);
    
    if (success) FontPrefs2Gadgets(data);
    
    return success;
}

IPTR FPEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    struct PrefHeader header; 
    struct IFFHandle *handle;
    BOOL              success = TRUE;
    LONG              error   = 0;
    
    Gadgets2FontPrefs(data);
    
    memset(&header, 0, sizeof(struct PrefHeader));
    
    if ((handle = AllocIFF()))
    {
        handle->iff_Stream = (IPTR) message->fh;
        
        InitIFFasDOS(handle);
        
        if (!(error = OpenIFF(handle, IFFF_WRITE))) /* NULL = successful! */
        {
            BYTE i;
            
            PushChunk(handle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */
            
            header.ph_Version = PHV_CURRENT;
            header.ph_Type    = 0;
            
            PushChunk(handle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */
            
            WriteChunkBytes(handle, &header, sizeof(struct PrefHeader));
            
            PopChunk(handle);
            
            for (i = 0; i < FP_COUNT; i++)
            {
                struct FileFontPrefs ffp;
                error = PushChunk(handle, ID_PREF, ID_FONT, sizeof(struct FileFontPrefs));
                
                if (error != 0) // TODO: We need some error checking here!
                {
                    printf("error: PushChunk() = %ld\n", error);
                }
                
                convertEndian(FP(i)); // Convert to m68k endian
                FontPrefs2FileFontPrefs(FP(i), &ffp);
                
                error = WriteChunkBytes(handle, &ffp, sizeof(struct FileFontPrefs));
                error = PopChunk(handle);
                
                convertEndian(FP(i)); // Revert to initial endian
                
                if (error != 0) // TODO: We need some error checking here!
                {
                    printf("error: PopChunk() = %ld\n", error);
                }
            }

            // Terminate the FORM
            PopChunk(handle);
        }
        else
        {
            ShowError(_(MSG_CANT_OPEN_STREAM));
            success = FALSE;
        }
        
        CloseIFF(handle);
	FreeIFF(handle);
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        success = FALSE;
    }

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_3
(
    FPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                    struct opSet *,
    MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *
);
