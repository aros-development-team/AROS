#include <prefs/prefhdr.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <strings.h>
#include <stdio.h>

#include "prefs.h"
#include "misc.h"
#include "locale.h"

#define PREFS_PATH_ENVARC "ENVARC:SYS/font.prefs"
#define PREFS_PATH_ENV    "ENV:SYS/font.prefs"

/*** Utility Functions ******************************************************/
static VOID convertEndian(struct FontPrefs *fp)
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

static VOID FileFontPrefs2FontPrefs(struct FileFontPrefs *ffp, struct FontPrefs *fp)
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

static VOID FontPrefs2FileFontPrefs(struct FontPrefs *fp, struct FileFontPrefs *ffp)
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

static BOOL Prefs_Load(STRPTR from, struct FontPrefs fp[])
{
    BOOL retval = FALSE;

    BPTR fh = Open(from, MODE_OLDFILE);
    if (fh)
    {
        retval = Prefs_ImportFH(fh, fp);
        Close(fh);
    }
    return retval;
}

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save)
{
    BPTR fh;
    struct FontPrefs fp[FP_COUNT];

    if (from)
    {
        if (!Prefs_Load(from, fp))
        {
            ShowMessage("Can't read from input file");
            return FALSE;
        }
    }
    else
    {
        if (!Prefs_Load(PREFS_PATH_ENV, fp))
        {
            if (!Prefs_Load(PREFS_PATH_ENVARC, fp))
            {
                ShowMessage
                (
                    "Can't read from file " PREFS_PATH_ENVARC
                    ".\nUsing default values."
                );
                Prefs_Default(fp);
            }
        }
    }

    if (use || save)
    {
        fh = Open(PREFS_PATH_ENV, MODE_NEWFILE);
        if (fh)
        {
            Prefs_ExportFH(fh, fp);
            Close(fh);
        }
        else
        {
            ShowMessage("Cant' open " PREFS_PATH_ENV " for writing.");
        }
    }
    if (save)
    {
        fh = Open(PREFS_PATH_ENVARC, MODE_NEWFILE);
        if (fh)
        {
            Prefs_ExportFH(fh, fp);
            Close(fh);
        }
        else
        {
            ShowMessage("Cant' open " PREFS_PATH_ENVARC " for writing.");
        }
    }
    return TRUE;
}

BOOL Prefs_Default(struct FontPrefs fp[])
{
    UBYTE i;

    for (i = 0; i < FP_COUNT; i++)
    {
        fp[i].fp_Type     = i;
        fp[i].fp_FrontPen = 0; /* FIXME: Is this (really) default? Look it up! */
        fp[i].fp_BackPen  = 0; /* FIXME: Is this (really) default? Look it up! */
        fp[i].fp_DrawMode = 0; /* FIXME: Is this (really) default? Look it up! */

        fp[i].fp_TextAttr.ta_YSize = 8; /* FIXME: Is this (really) default? Look it up! */
        fp[i].fp_TextAttr.ta_Style = FS_NORMAL;
        fp[i].fp_TextAttr.ta_Flags = FPB_DISKFONT; /* FIXME: Is this (really) default? Look it up! */

        fp[i].fp_Name[0] = '\0';
        strlcat(fp[i].fp_Name, "topaz.font", FONTNAMESIZE); /* FIXME: Is this (really) default? Check it up! */
        fp[i].fp_TextAttr.ta_Name = fp[i].fp_Name;
    }
    return TRUE;
}


BOOL Prefs_ImportFH(BPTR fh, struct FontPrefs fp[])
{
    struct ContextNode  *context;
    struct IFFHandle    *handle;
    BOOL                 success = TRUE;
    LONG                 error;

    if (!(handle = AllocIFF()))
    {
        ShowMessage(_(MSG_CANT_ALLOCATE_IFFPTR));
        return(FALSE);
    }

    handle->iff_Stream = (IPTR) fh;
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
                        printf("Error: ReadChunkBytes() returned %d!\n", error);
                    }

                    FileFontPrefs2FontPrefs(&ffp, &fp[i]);

                    convertEndian(&fp[i]);
                }
                else
                {
                    printf("ParseIFF() failed, returncode %d!\n", error);
                    success = FALSE;
                    break;
                }
            }
            else
            {
                printf("StopChunk() failed, returncode %d!\n", error);
                success = FALSE;
            }
        }

        CloseIFF(handle);
    }
    else
    {
        ShowMessage(_(MSG_CANT_OPEN_STREAM));
    }

    FreeIFF(handle);

    return success;
}

BOOL Prefs_ExportFH(BPTR fh, struct FontPrefs fp[])
{
    struct PrefHeader header;
    struct IFFHandle *handle;
    BOOL              success = TRUE;
    LONG              error   = 0;

    memset(&header, 0, sizeof(struct PrefHeader));

    if ((handle = AllocIFF()))
    {
        handle->iff_Stream = (IPTR)fh;

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
                    printf("error: PushChunk() = %d\n", error);
                }

                convertEndian(&fp[i]); // Convert to m68k endian
                FontPrefs2FileFontPrefs(&fp[i], &ffp);

                error = WriteChunkBytes(handle, &ffp, sizeof(struct FileFontPrefs));
                error = PopChunk(handle);

                convertEndian(&fp[i]); // Revert to initial endian

                if (error != 0) // TODO: We need some error checking here!
                {
                    printf("error: PopChunk() = %d\n", error);
                }
            }

            // Terminate the FORM
            PopChunk(handle);
        }
        else
        {
            ShowMessage(_(MSG_CANT_OPEN_STREAM));
            success = FALSE;
        }

        CloseIFF(handle);
        FreeIFF(handle);
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        ShowMessage(_(MSG_CANT_ALLOCATE_IFFPTR));
        success = FALSE;
    }

    return success;
}

