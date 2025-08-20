/*
 * otag.c - .otag files reading routines
 * Copyright © 1995-96 Michael Letowski
 */
 
 //#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <diskfont/diskfont.h>
#include <diskfont/diskfonttag.h>
#include <graphics/text.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <proto/utility.h>

#include "classbase.h"
#include "otag.h"

/*
 * Private constants and macros
 */
#define FTSUFFIX        ".font"

#define SUFFIX_LEN      5
#define MAX_NAME_LEN    32

/* Get size of FontContentHeader */
#define SizeOfFCH(fch)  (sizeof(struct FontContentsHeader) + \
                        (fch)->fch_NumEntries * sizeof(struct FontContents))
#define VarSizeOfFCH(x) (sizeof(struct FontContentsHeader) + \
                        (x) * sizeof(struct FontContents))

/*
 * Private functions
 */
STATIC struct FontContentsHeader *ReadOutlines(BPTR lock, STRPTR name);
STATIC LONG LocSeekFileSize(BPTR fh);

/*
 * Extended DiskFont functions
 */
struct FontContentsHeader *NewFC(BPTR lock, STRPTR name)
{
    struct FontContentsHeader *FCH, *MyFCH = NULL;
    ULONG Size;

    D(bug("[font.datatype] %s(0x%p, 0x%p)\n", __func__, lock, name));

    /* Use Diskfont to get contents */
    if ((FCH = NewFontContents(lock, name))) {
        D(bug("[font.datatype] %s: FCH @ 0x%p\n", __func__, FCH));

        /* Some entries exist */
        if (FCH->fch_NumEntries) {
            Size = SizeOfFCH(FCH);
            /* Allocate storage and copy */
            if ((MyFCH = AllocVec(Size, MEMF_CLEAR)))
                CopyMem(FCH, MyFCH, Size);
        } else if (FCH->fch_FileID == OFCH_ID) {
            /* This is outline font
             * Read tags and sizes
             */
            if ((MyFCH = ReadOutlines(lock, name)))
                MyFCH->fch_FileID = FCH->fch_FileID;
        }
        /* Free unused header */
        DisposeFontContents(FCH);
    }
    return(MyFCH);
} /* NewFC */

VOID DisposeFC(struct FontContentsHeader *fch)
{
    D(bug("[font.datatype] %s()\n", __func__));

    FreeVec(fch);
} /* DisposeFC */

/*
 * Private functions
 */
STATIC struct FontContentsHeader *ReadOutlines(BPTR lock, STRPTR name)
{
    char NewName[MAX_NAME_LEN];

    struct FontContentsHeader *FCH = NULL;
    struct TFontContents *TFC;
    struct TagItem *Tags;
    struct FileInfoBlock *FIB;
    BPTR FH, OldLock;
    UWORD *Sizes;
    STRPTR Suffix;
    IPTR Offset;
    ULONG I;
    LONG FileLen;

    D(bug("[font.datatype] %s()\n", __func__));

    OldLock = CurrentDir(lock);                                                 /* Change to FONTS: dir */

    /* Chage extension */
    memset(&NewName, 0, MAX_NAME_LEN);                                          /* Set to NULLs */
    strncpy(NewName, name, MAX_NAME_LEN - 1);                                   /* Copy name to buffer */
    if (!(Suffix = strstr(NewName, FTSUFFIX))) {
        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
        return NULL;
    }
    strncpy(Suffix, OTSUFFIX, SUFFIX_LEN);                                        /* Replace ".font" with ".otag" */

    /* Open .otag file and get its size */

    FH = Open(NewName, MODE_OLDFILE);
    if (FH) {
        D(bug("[font.datatype] %s: Opened '%s' @ 0x%p\n", __func__, NewName, FH));
        if ((FIB = AllocDosObject(DOS_FIB, NULL))) {
            if (ExamineFH(FH, FIB))                                                 /* Examine it */
                FileLen = FIB->fib_Size;                                            /* Get length of file */
            else
                FileLen = LocSeekFileSize(FH);                                  /* Get size by seeking */
            FreeDosObject(DOS_FIB, FIB);
        }
        else
            FileLen = LocSeekFileSize(FH);                                      /* No FIB? - seek */

        D(bug("[font.datatype] %s: FileLen = %u\n", __func__, FileLen));
        if (FileLen >= 0) {
            ULONG *tmptags;
            Tags = AllocVec(FileLen, MEMF_ANY);
            if (!Tags) {
                Close(FH);
                return NULL;
            }
            tmptags = (ULONG *)Tags;
            D(bug("[font.datatype] %s: Tag storage @  0x%p\n", __func__, Tags));

            if (Read(FH, Tags, FileLen) != FileLen) {
                FreeVec(Tags);
                Close(FH);
                return NULL;
            }

            D(bug("[font.datatype] %s: Tags read successfully\n", __func__));
#if __WORDSIZE == 64
            ULONG tagcnt = 0;
            while (tmptags[(tagcnt << 1)] != TAG_DONE)
                tagcnt++;
            D(bug("[font.datatype] %s: %u embedded 32bit tags\n", __func__, tagcnt));
            Tags = AllocVec(sizeof(struct TagItem) * tagcnt, MEMF_ANY);
#endif
#if !AROS_BIGENDIAN || __WORDSIZE == 64
            D(bug("[font.datatype] %s: converting tags...\n", __func__));
            if ((IPTR)Tags == (IPTR)tmptags)
                Tags = AllocVec(FileLen, MEMF_ANY);
            struct TagItem *NewTag = Tags;
            ULONG *srctag = tmptags;
            for (;; srctag += 2)
            {
                NewTag->ti_Tag = AROS_BE2LONG(srctag[0]);
                if (NewTag->ti_Tag == TAG_DONE)
                    break;
                NewTag->ti_Data = AROS_BE2LONG(srctag[1]);
                if (NewTag->ti_Tag & OT_Indirect) {
                    D(bug("[font.datatype] %s: adjusting OT_Indirect %u\n", __func__, NewTag->ti_Data));
                    NewTag->ti_Data = (IPTR)tmptags + NewTag->ti_Data;
                }
                D(bug("[font.datatype] %s: Tag %p : %p\n", __func__, NewTag->ti_Tag, NewTag->ti_Data));
                NewTag++;
            }
#endif
            /* Get size of new FCH and set up data */
            Offset = GetTagData(OT_AvailSizes, (IPTR)-1, Tags);
            if (Offset != (IPTR)-1) {
                UWORD szcount;
#if !AROS_BIGENDIAN || __WORDSIZE == 64
                Sizes = (UWORD *)Offset;
#else
                Sizes = (UWORD *)((IPTR)tmptags + Offset);                             /* Calculate position of data */
#endif
#if !AROS_BIGENDIAN
                szcount = AROS_BE2WORD(*Sizes);
#else
                szcount = *Sizes;
#endif
                if ((FCH = AllocVec(VarSizeOfFCH(szcount), MEMF_CLEAR))) {
                    UWORD ysize;
                    D(bug("[font.datatype] %s: FCH @ 0x%p, for %u entries.\n", __func__, FCH, szcount));
                    FCH->fch_NumEntries = szcount;
                    for (I = 0; I < szcount; I++) {
#if !AROS_BIGENDIAN
                        ysize = AROS_BE2WORD(Sizes[I+1]);
#else
                        ysize = Sizes[I+1];
#endif
                        TFC = &TFontContents(FCH)[I];
                        SNPrintf(TFC->tfc_FileName, MAXFONTPATH, "%s/%ld", name, ysize);
                        D(bug("[font.datatype] %s: '%s'\n", __func__, TFC->tfc_FileName));
                        TFC->tfc_TagCount = 1;                                      /* TAG_DONE */
                        TFC->tfc_YSize = ysize;
                        TFC->tfc_Style = FS_NORMAL;                                 /* Really ??? */
                        TFC->tfc_Flags = FPF_DISKFONT;                              /* Really ??? */
                    }
                }
                CurrentDir(OldLock);                                                /* Get back to old dir */
            }
            if ((IPTR)Tags != (IPTR)tmptags)
                FreeVec(tmptags);
            FreeVec(Tags);
        }
        Close(FH);
    }
    return(FCH);
} /* ReadOutlines */

STATIC LONG LocSeekFileSize(BPTR fh)
{
    LONG Size;

    D(bug("[font.datatype] %s()\n", __func__));

    if ((Size = Seek(fh, 0, OFFSET_END)) >= 0)
        Size = Seek(fh, 0, OFFSET_BEGINNING);

    return(Size);
} /* LocSeekFileSize */
