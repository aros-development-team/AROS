/*
 * prefs.c - prefrences handling for Font DataType class
 * Copyright © 1995-96 Michael Letowski
 */

//#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>

#include <stdlib.h>

#include "classbase.h"
#include "prefs.h"

/*
 * Private constants and macros
 */
#define BUF_SIZE                1024                                    /* Size of prefs file buffer */

#define MAX_SUB_CNT             3                                       /* Max number of sub-options */

#define PREFS_PATH1             "PROGDIR:Prefs/DataTypes/font.prefs"
#define PREFS_PATH2             "ENV:DataTypes/font.prefs"


/*
 * Private functions prototypes
 */
STATIC PBOOL ReadPrefs(struct PrefsHandle *ph,
                        struct Opts *opts, STRPTR name);
STATIC PBOOL ReadSubPrefs(struct RDArgs *rda,
                        STRPTR argStr, STRPTR argtmplt,
                        SIPTR fields[], LONG optCnt);
STATIC struct RDArgs *ReadArgsBuf(STRPTR argtmplt,
                        SIPTR *opts, struct RDArgs *rda);


/*
 * Public functions
 */
struct PrefsHandle *GetFontPrefs(struct Opts *opts)
{
    struct PrefsHandle *PH;
    PBOOL Res = FALSE;

    D(bug("[font.datatype] %s()\n", __func__));

    memset(opts, 0, sizeof(struct Opts));                               /* Set defaults */

    if ((PH = AllocMem(sizeof(struct PrefsHandle), MEMF_CLEAR))) {
        if ((PH->ph_RDA1 = AllocDosObject(DOS_RDARGS, NULL))) {
            if ((PH->ph_RDA2 = AllocDosObject(DOS_RDARGS, NULL))) {

                if (ThisProcessS->pr_HomeDir)                           /* PROGDIR: exists? */
                    Res = ReadPrefs(PH, opts, PREFS_PATH1);             /* Try first path */

                if (!Res)                                               /* Args not read so far */
                    Res = ReadPrefs(PH, opts, PREFS_PATH2);             /* Try second path */

                if (Res)
                    return(PH);
                FreeDosObject(DOS_RDARGS, PH->ph_RDA2);
            }
            FreeDosObject(DOS_RDARGS, PH->ph_RDA1);
        }
        FreeMem(PH, sizeof(struct PrefsHandle));
    }
    return(NULL);
} /* GetFontPrefs */

VOID FreeFontPrefs(struct PrefsHandle *ph)
{
    D(bug("[font.datatype] %s()\n", __func__));

    FreeArgs(ph->ph_Args);                                              /* Free args (may be NULL) */
    FreeDosObject(DOS_RDARGS, ph->ph_RDA2);                             /* Free 2nd RDA structure */
    FreeDosObject(DOS_RDARGS, ph->ph_RDA1);                             /* Free 1st RDA structure */
    FreeMem(ph, sizeof(struct PrefsHandle));
} /* FreeFontPrefs */

/*
 * Private functions
 */
STATIC PBOOL ReadPrefs(struct PrefsHandle *ph,
                        struct Opts *opts, STRPTR name)
{
    struct RDArgs *RDA1 = ph->ph_RDA1;
    struct RDArgs *RDA2 = ph->ph_RDA2;
    STRPTR Buf, Buffer;
    LONG Len, Size;
    BPTR FH;

    D(bug("[font.datatype] %s()\n", __func__));

    if ((FH = Open(name, MODE_OLDFILE))) {
        if ((Buffer = AllocMem(BUF_SIZE, MEMF_CLEAR))) {
            /* Consolidate multiple lines into a single buffer */
            Buf = Buffer;
            Size = BUF_SIZE - 1;
            while((Size > 0) && FGets(FH, Buf, Size)) {
                Len = strlen(Buf);
                if (Buf[Len-1] == '\n')
                    Buf[Len-1] = ' ';
                Buf += Len;
                Size -= Len;
            }

            /* Set up all RDA structures */
            RDA1->RDA_Source.CS_Buffer = Buffer;                        /* Set up buffer to read from */
            RDA2->RDA_Source.CS_Buffer = Buffer;                        /* Set up buffer to read from */

            /* Read args */
            if ((ph->ph_Args = ReadArgsBuf(TEMPLATE, (SIPTR *)opts, RDA1))) {
                opts->opt_DPIFlag =
                        ReadSubPrefs(RDA2, opts->opt_DPIStr, TEMPLATEDPI, &opts->opt_XDPI, 2);
                opts->opt_ForeFlag =
                        ReadSubPrefs(RDA2, opts->opt_ForeStr, TEMPLATECOL, opts->opt_ForeCol, 3);
                opts->opt_BackFlag =
                        ReadSubPrefs(RDA2, opts->opt_BackStr, TEMPLATECOL, opts->opt_BackCol, 3);
            }
            FreeMem(Buffer, BUF_SIZE);
        }
        Close(FH);
    }
    return (FH ? TRUE : FALSE);                                         /* True if prefs file exists */
} /* ReadPrefs */

STATIC PBOOL ReadSubPrefs(struct RDArgs *rda,
                            STRPTR argStr, STRPTR argtmplt,
                            SIPTR fields[], LONG optCnt)
{
    LONG *SubOpts[MAX_SUB_CNT];
    struct RDArgs *Args;
    LONG I;
    PBOOL Res = FALSE;

    D(bug("[font.datatype] %s()\n", __func__));

    if (argStr) {
        strcpy((char *)rda->RDA_Source.CS_Buffer, argStr);
        if ((Args = ReadArgsBuf(argtmplt, (SIPTR *)SubOpts, rda))) {
            Res = TRUE;
            for (I = 0;  I < optCnt; I++)
                if (SubOpts[I])
                    fields[I] = *SubOpts[I];
            FreeArgs(Args);
        }
    }
    return(Res);
} /* ReadSubPrefs */

STATIC struct RDArgs *ReadArgsBuf(STRPTR argtmplt,
                                    SIPTR *opts, struct RDArgs *rda)
{
    STRPTR Buffer = (char *)rda->RDA_Source.CS_Buffer;
    LONG Len;

    D(bug("[font.datatype] %s()\n", __func__));

    /* Prepare buffer */
    Len = strlen(Buffer);                               /* Get size of buffer */
    Buffer[Len] = '\n';                                 /* New line char */
    Buffer[Len + 1] = '\0';                             /* String terminator */

    /* Prepare RDArgs */
    rda->RDA_Source.CS_Length = Len + 1;                /* Set up string len */
    rda->RDA_Source.CS_CurChr = 0;                      /* Set up current char */
    rda->RDA_Buffer = NULL;                             /* As documented in AutoDocs */
    fset(rda->RDA_Flags, RDAF_NOPROMPT);                /* Disable prompts */
    return(ReadArgs(argtmplt, opts, rda));              /* Read arguments */
} /* ReadArgsBuf */
