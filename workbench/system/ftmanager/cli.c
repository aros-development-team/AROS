/*
    Copyright (C) 2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>

#include <aros/debug.h>

#include "globals.h"
#include "fontinfo_class.h"
#include "fontbitmap_class.h"

#define ARG_TEMPLATE "COPY/S,TTFFONT/A,CODEPAGE/K,TO/K,FONTDIR/K,OUTFONT/K"

enum
{
	ARG_COPY,
	ARG_TTFFONT,
	ARG_CODEPAGE,
	ARG_TO,
	ARG_FONTDIR,
	ARG_OUTFONT,
	ARG_COUNT
};

ULONG fiWriteFiles_real(FontInfoData *dat, STRPTR base, ULONG size);

static void usage(void) 
{
    BPTR out = ErrorOutput();
    char myname[256];
    APTR a[2];

    GetProgramName(myname, sizeof(myname));
    a[0]=(APTR) myname;
    a[1]=0;
    VFPrintf(out, "Usage: %s " ARG_TEMPLATE "\n", (APTR) a);

    VFPrintf(out, "\n", NULL);
    VFPrintf(out, "  COPY/S: copy ttf font file (default: no)\n", NULL);
    VFPrintf(out, "  TTFFONT/A: ttf font file to install\n", NULL);
    VFPrintf(out, "  CODEPAGE/A: codepage to install (not yet supported)\n", NULL);
    VFPrintf(out, "  TO/K: target directory for COPY operation (default: \"FONTS:TrueType\")\n", NULL);
    VFPrintf(out, "  FONTDIR/K: directory in which the .otag and .font files\n", NULL);
    VFPrintf(out, "             will be installed (default: \"FONTS:\")\n", NULL);
    VFPrintf(out, "  OUTFONT/K: baseame for the .otag and .font files\n", NULL);
    VFPrintf(out, "             (default: TTFFont filename without extension)\n", NULL);
}

/*****************************************************************************
 * get_basename
 *
 * If OUTFONT was supplied by command line, use outfont.
 * If not, build basename based on filename of TTF Font
 *****************************************************************************/
static void get_basename(STRPTR basename, STRPTR outfont, STRPTR ttf_name)
{
    /* get basename for .otag/.font files */
    if(outfont != NULL)
    {
        strncpy(basename, outfont, 255);
    }
    else
    {
        STRPTR b = FilePart(ttf_name);
        STRPTR dot = strrchr(b, '.');
        if(dot == NULL)
        {
            strncpy(basename, b, 256);
        }
        else
        {
            /* use TTFFont filename without extension */
            strncpy(basename, b, dot-b);
            basename[dot-b]=(char) 0;
        }
    }
    D(bug("basename: >%s<\n", basename));
}

/*****************************************************************************
 * copy_file(source, target)
 *
 * simple file copy
 *****************************************************************************/
static BOOL copy_file_handle(BPTR s, BPTR t)
{
    char buffer[BUFSIZ]; /* BUFSIZ is from stdio.h */
    LONG bytes;

    for(;;)
    {
        bytes = Read(s, buffer, sizeof(buffer) );
        if( bytes < 0)
        {
            /* error */
            return FALSE;
        }
        if( bytes == 0)
        {
            /* nothing more to read */
            return TRUE;
        }

        if(Write(t, buffer, bytes) == -1)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL copy_file(STRPTR source, STRPTR target)
{
    BPTR in, out;
    BOOL ret;

    if (!(in = Open(source, MODE_OLDFILE)))
    {
        printf("ERROR: Unable to open \"%s\"\n", source);
        return FALSE;
    }

    if (!(out = Open(target, MODE_NEWFILE)))
    {
        Close(in);
        printf("ERROR: Unable to open \"%s\"\n", target);
        return FALSE;
    }

    ret=copy_file_handle(in, out);
    if(!ret)
    {
        /* clean up eventually unfinished file */
        DeleteFile(target);
    }

    Close(in);
    Close(out);

    return ret;
}

/*****************************************************************************
 * ftmanager_cli
 *
 * FTmanager does not really distinguish between gui and functionality.
 * So create a faked struct FontInfoData usually used by the Zune GUI and
 * use it to call fiWriteFiles().
 *****************************************************************************/
int ftmanager_cli(void)
{
    struct RDArgs       *rdargs;
    struct FontInfoData *dat;
    BOOL                 copy = TRUE;
    TT_OS2              *os2;
    FT_Error             error;
    IPTR                 args[] = { (IPTR) FALSE,
                                    (IPTR) NULL,
                                    (IPTR) NULL,
                                    (IPTR) NULL,
                                    (IPTR) NULL,
                                    (IPTR) NULL,
                                    (IPTR) NULL };
    char basename[256];
    char ttf_source[256];

    rdargs = ReadArgs(ARG_TEMPLATE, args, NULL);

    if(rdargs == NULL)
    {
        usage();
        return RETURN_ERROR;
    }

    if(args[ARG_CODEPAGE] != 0) 
    {
        printf("WARNING: Option Codepage ignored (not implemented)\n");
    }

    if(args[ARG_COPY] == 0)
    {
        copy = FALSE;
    }

    D(bug("COPY    : %d\n", copy));
    D(bug("TTFFont : %s\n", args[ARG_TTFFONT]));
    D(bug("CODEPAGE: %s\n", args[ARG_CODEPAGE]));
    D(bug("TO      : %s\n", args[ARG_TO]));
    D(bug("FONTDIR : %s\n", args[ARG_FONTDIR]));
    D(bug("OUTFONT : %s\n", args[ARG_OUTFONT]));

    if(args[ARG_TO] != 0 && !copy)
    {
        printf("ERROR: TO parameter only allowed, if COPY is used\n");
        usage();
        return RETURN_ERROR;
    }

    dat=(struct FontInfoData *) AllocVec(sizeof(struct FontInfoData), MEMF_CLEAR);
    if(dat == NULL) 
    {
        printf("ERROR: Out of memory\n");
        return RETURN_ERROR;
    }


    /* copy must copy the .ttf file to the target directory and use the new
     * file in the .otag
     */
    if(!copy)
    {
        dat->Filename=(STRPTR)  args[ARG_TTFFONT];
    }
    else
    {
        if(args[ARG_TO] != 0)
        {
            strncpy(ttf_source, (STRPTR) args[ARG_TO], 255);
        }
        else
        {
            strcpy(ttf_source, "FONTS:TrueType");
        }
        D(bug("copy %s to %s\n", (STRPTR) args[ARG_TTFFONT], ttf_source));
        AddPart(ttf_source, FilePart((STRPTR) args[ARG_TTFFONT]), 255);

        if(copy_file((STRPTR) args[ARG_TTFFONT], ttf_source)) 
        {
            dat->Filename=ttf_source;
        }
        else
        {
            FreeVec(dat);
            return RETURN_ERROR;
        }
    }

    if (FT_Init_FreeType(&ftlibrary) != 0)
    {
        FreeVec(dat);
        printf("ERROR: Init FreeType library failed \n");
        return RETURN_ERROR;
    }

    get_basename(basename, (STRPTR) args[ARG_OUTFONT], dat->Filename);


    if(error=FT_New_Face(ftlibrary, dat->Filename, 0, &dat->Face)) 
    {
      printf("ERROR: opening \"%s\" failed (error code: %d)\n", dat->Filename, error);
      FT_Done_FreeType(ftlibrary);
      FreeVec(dat);
      return RETURN_ERROR;
    }

    /* fill in tags for .otag file */
    os2 = FT_Get_Sfnt_Table(dat->Face, ft_sfnt_os2);

    /* for all the magic here, please also see fontinfo_class.c */
    struct TagItem *tag = dat->OTags;

    tag->ti_Tag = OT_FileIdent;
    ++tag;

    tag->ti_Tag = OT_Engine;
    tag->ti_Data = (IPTR) "freetype2";
    ++tag;

    tag->ti_Tag = OT_Family;
    tag->ti_Data = (IPTR) dat->Face->family_name;
    D(bug("Family name: %s\n", dat->Face->family_name));
    ++tag;

    tag->ti_Tag = OT_YSizeFactor;
    tag->ti_Data = 1 | (1 << 16);
    ++tag;

    tag->ti_Tag = OT_SpaceWidth;
    tag->ti_Data = dat->Face->max_advance_width * 250.0 / 72.307;
    D(bug("Space width: %d\n", tag->ti_Data));
    ++tag;

    tag->ti_Tag = OT_IsFixed;
    tag->ti_Data = FT_IS_FIXED_WIDTH(dat->Face);
    D(bug("Fixed size: %d\n", tag->ti_Data));
    ++tag;

    tag->ti_Tag = OT_SerifFlag;
    tag->ti_Data = os2 && (unsigned) (((os2->sFamilyClass >> 8) & 0xff) - 1) < 5;
    D(bug("Serif: %d\n", tag->ti_Data));
    ++tag;

    /* use default for:
    tag->ti_Tag = OT_StemWeight;
    tag->ti_Tag = OT_SlantStyle;
    tag->ti_Tag = OT_HorizStyle;
    */

    tag->ti_Tag = OT_SpaceFactor;
    tag->ti_Data = 0x10000;
    ++tag;

    tag->ti_Tag = OT_InhibitAlgoStyle;
    tag->ti_Data = FSF_UNDERLINED | FSF_BOLD;
    ++tag;

    tag->ti_Tag = OT_SpecCount;
    tag->ti_Data = 4;
    ++tag;

    tag->ti_Tag = OT_Spec1_FontFile;
    tag->ti_Data = (IPTR)dat->Filename;
    ++tag;

    /* use default for:
    tag->ti_Tag = OT_Spec3_AFMFile;
    tag->ti_Tag = OT_Spec4_Metric;
    tag->ti_Tag = OT_Spec5_BBox;
    */

    /* TODO: CodePage
    tag->ti_Tag = OT_Spec2_CodePage;
    */

    tag->ti_Tag = TAG_END;

    dat->AvailSizes[0] = AROS_WORD2BE(2);   // <- number of entries...
    dat->AvailSizes[1] = AROS_WORD2BE(10);
    dat->AvailSizes[2] = AROS_WORD2BE(15);

    if(args[ARG_FONTDIR] == 0)
    {
        fiWriteFiles(dat, basename, "FONTS:", sizeof(ULONG) + (tag - dat->OTags + 2) * sizeof(UQUAD));
    }
    else
    {
        fiWriteFiles(dat, basename, (STRPTR) args[ARG_FONTDIR], sizeof(ULONG) + (tag - dat->OTags + 2) * sizeof(UQUAD));
    }

    FT_Done_Face(dat->Face);
    FreeVec(dat);
    FT_Done_FreeType(ftlibrary);

    return RETURN_OK;
}

