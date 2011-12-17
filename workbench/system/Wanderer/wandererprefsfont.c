/*
     Copyright � 2002-2011, The AROS Development Team. All rights reserved.
     $Id$
*/
#include "portable_macros.h"
#ifdef __AROS__
#define DEBUG 0
#endif

#include <exec/types.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>

#ifdef __AROS__
#include <zune/customclasses.h>
#else
#include <zune_AROS/customclasses.h>
#endif

#include <dos/notify.h>

#ifdef __AROS__
#include <workbench/handler.h>
#else
#include <workbench_AROS/handler.h>
#endif

#ifndef PROTO_EXEC_H
#include <proto/exec.h>
#endif

#ifndef PROTO_DOS_H
#include <proto/dos.h>
#endif

#ifndef PROTO_INTUITION_H
#include <proto/intuition.h>
#endif

#ifndef PROTO_GRAPHICS_H
#include <proto/graphics.h>
#endif

#ifndef PROTO_UTILITY_H
#include <proto/utility.h>
#endif

#ifndef PROTO_KEYMAP_H
#include <proto/keymap.h>
#endif

#ifndef PROTO_LOCALE_H
#include <proto/locale.h>
#endif

#ifndef PROTO_LAYERS_H
#include <proto/layers.h>
#endif

#ifndef PROTO_DATATYPES_H
#include <proto/datatypes.h>
#endif

#ifdef __AROS__
#ifndef PROTO_ALIB_H
#include <proto/alib.h>
#endif
#endif

#ifndef PROTO_ASL_H
#include <proto/asl.h>
#endif

#ifndef PROTO_DISKFONT_H
#include <proto/diskfont.h>
#endif

#ifndef PROTO_IFFPARSE_H
#include <proto/iffparse.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __AROS__
#include <prefs/prefhdr.h>
#else
#include <prefs_AROS/prefhdr.h>
#endif

#include <prefs/font.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include "wandererprefsfont.h"

#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
    #define D(x) if (DEBUG) x
    #ifdef __amigaos4__
    #define bug DebugPrintF
    #else
    #define bug kprintf
    #endif
#else
    #define  D(...)
#endif
#endif

#define CHECK_PRHD_VERSION 1
#define CHECK_PRHD_SIZE    1

/** Data/Structs for font.prefs settings */

struct FilePrefHeader
{
        UBYTE ph_Version;
        UBYTE ph_Type;
        UBYTE ph_Flags[4];
};

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

LONG stopchunks[] =
{
        ID_PREF, ID_FONT
};

/** Data pertaining to wanderers internal prefs configured state */

struct IFFHandle *CreateIFF(STRPTR filename, LONG *stopchunks, LONG numstopchunks)
{
    struct IFFHandle *iff;

    D(bug("CreateIFF: filename = \"%s\"\n", filename));

    if ((iff = AllocIFF()))
    {
        D(bug("CreateIFF: AllocIFF okay.\n"));
        if ((iff->iff_Stream = (IPTR) Open(filename, MODE_OLDFILE)))
        {
            D(bug("CreateIFF: Open() okay.\n"));InitIFFasDOS(iff);

            if (OpenIFF(iff, IFFF_READ) == 0)
            {
                BOOL ok = FALSE;

                D(bug("CreateIFF: OpenIFF okay.\n"));

                if ((StopChunk(iff, ID_PREF, ID_PRHD) == 0)
                        && (StopChunks(iff, stopchunks, numstopchunks) == 0))
                {
                    D(bug("CreateIFF: StopChunk(PRHD) okay.\n"));

                    if (ParseIFF(iff, IFFPARSE_SCAN) == 0)
                    {
                        struct ContextNode *cn;

                        cn = CurrentChunk(iff);

                        D(bug("CreateIFF: ParseIFF okay. Chunk Type = %c%c%c%c  ChunkID = %c%c%c%c\n",
                                        cn->cn_Type >> 24,
                                        cn->cn_Type >> 16,
                                        cn->cn_Type >> 8,
                                        cn->cn_Type,
                                        cn->cn_ID >> 24,
                                        cn->cn_ID >> 16,
                                        cn->cn_ID >> 8,
                                        cn->cn_ID));

                        if ((cn->cn_ID == ID_PRHD)
#if CHECK_PRHD_SIZE
                                && (cn->cn_Size == sizeof(struct FilePrefHeader))
#endif
                                )
                        {
                            struct FilePrefHeader h;

                            D(bug("CreateIFF: PRHD chunk okay.\n"));

                            if (ReadChunkBytes(iff, &h, sizeof(h)) == sizeof(h))
                            {
                                D(bug("CreateIFF: Reading PRHD chunk okay.\n"));

#if CHECK_PRHD_VERSION
                                if (h.ph_Version == PHV_CURRENT)
                                {
                                    D(bug("CreateIFF: PrefHeader version is correct.\n"));
                                    ok = TRUE;
                                }
#else
                                ok = TRUE;
#endif

                            }

                        }

                    } /* if (ParseIFF(iff, IFFPARSE_SCAN) == 0) */

                } /* if ((StopChunk(iff, ID_PREF, ID_PRHD) == 0) && (StopChunks(... */

                if (!ok)
                {
                    CloseIFF(iff);
                    Close((BPTR)iff->iff_Stream);
                    FreeIFF(iff);
                    iff = NULL;
                }

            } /* if (OpenIFF(iff, IFFF_READ) == 0) */
            else
            {
                Close((BPTR)iff->iff_Stream);
                FreeIFF(iff);
                iff = NULL;
            }

        } /* if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE))) */
        else
        {
            FreeIFF(iff);
            iff = NULL;
        }

    } /* if ((iff = AllocIFF())) */

    return iff;
}


/*********************************************************************************************/

void KillIFF(struct IFFHandle *iff)
{
    if (iff)
    {
        CloseIFF(iff);
        Close((BPTR)iff->iff_Stream);
        FreeIFF(iff);
    }
}

/*********************************************************************************************/

void WandererPrefs_ReloadFontPrefs(CONST_STRPTR prefsfile, struct WandererFontPrefsData *wfpd)
{
    struct IFFHandle *iff = NULL;

    D(bug("[wanderer] In WandererPrefs_CheckFont()\n"));

    if ((iff = CreateIFF((STRPTR)prefsfile, stopchunks, 1)))
    {
        while (ParseIFF(iff, IFFPARSE_SCAN) == 0)
        {
            struct ContextNode *cn;
            struct FileFontPrefs fontprefs;

            cn = CurrentChunk(iff);

            D(bug("[wanderer] WandererPrefs_CheckFont: ParseIFF okay. Chunk Type = %c%c%c%c  ChunkID = %c%c%c%c\n",
                            cn->cn_Type >> 24,
                            cn->cn_Type >> 16,
                            cn->cn_Type >> 8,
                            cn->cn_Type,
                            cn->cn_ID >> 24,
                            cn->cn_ID >> 16,
                            cn->cn_ID >> 8,
                            cn->cn_ID));

            if ((cn->cn_ID == ID_FONT) && (cn->cn_Size == sizeof(fontprefs)))
            {
                D(bug("[wanderer] WandererPrefs_CheckFont: ID_FONT chunk with correct size found.\n"));

                if (ReadChunkBytes(iff, &fontprefs, sizeof(fontprefs))
                        == sizeof(fontprefs))
                {
                    UWORD type;

                    D(bug("[wanderer] WandererPrefs_CheckFont: Reading of ID_FONT chunk okay.\n"));

                    type = (fontprefs.fp_Type[0] << 8) + fontprefs.fp_Type[1];

#ifdef __AROS__
                    D(bug("[wanderer] WandererPrefs_CheckFont: Type = %d  Name = %s\n", type, fontprefs.fp_Name));
#endif

                    if (type == FP_WBFONT)
                    {
                        if (wfpd->wfpd_IconFont != NULL)
                        {
                            WandererPrefs_CloseOldIconFont(wfpd);
                            wfpd->wfpd_OldIconFont = wfpd->wfpd_IconFont;
                        }
                        wfpd->wfpd_IconFontTA.ta_Name = fontprefs.fp_Name;
                        wfpd->wfpd_IconFontTA.ta_YSize =
                                (fontprefs.fp_TextAttr_ta_YSize[0] << 8)
                                        + fontprefs.fp_TextAttr_ta_YSize[1];
                        wfpd->wfpd_IconFontTA.ta_Style =
                                fontprefs.fp_TextAttr_ta_Style;
                        wfpd->wfpd_IconFontTA.ta_Flags =
                                fontprefs.fp_TextAttr_ta_Flags;

                        wfpd->wfpd_IconFont = OpenDiskFont(&wfpd->wfpd_IconFontTA);
                        D(bug("[wanderer] WandererPrefs_CheckFont: Trying to use Font '%s' @ %x\n", wfpd->wfpd_IconFontTA.ta_Name, wfpd->wfpd_IconFont));
                    }

                } /* if (ReadChunkBytes(iff, &fontprefs, sizeof(fontprefs)) == sizeof(fontprefs)) */

            } /* if ((cn->cn_ID == ID_FONT) && (cn->cn_Size == sizeof(fontprefs))) */

        } /* while(ParseIFF(iff, IFFPARSE_SCAN) == 0) */

        KillIFF(iff);

    } /* if ((iff = CreateIFF(filename))) */
}

void WandererPrefs_CloseOldIconFont(struct WandererFontPrefsData *wfpd)
{
    if (wfpd->wfpd_OldIconFont)
    {
        CloseFont(wfpd->wfpd_OldIconFont);
        wfpd->wfpd_OldIconFont = NULL;
    }
}
