/*
    Copyright (C) 1995-2018, The AROS Development Team. All rights reserved.

    ASL initialization code.
*/


#include <stddef.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <intuition/screens.h>
#include <graphics/modeid.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <prefs/prefhdr.h>

#include "asl_intern.h"
#include LC_LIBDEFS_FILE

#define CATCOMP_NUMBERS
#include "strings.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************************/

#define DEF_WIDTH   400
#define DEF_HEIGHT  500

/* Requester type specific default data */
const struct IntFileReq def_filereq =
{
    {
        ASL_FileRequest,
        NULL,                   /* Window               */
        NULL,                   /* Screen               */
        NULL,                   /* PubScreenName        */
        NULL,                   /* IntuiMsgFunc         */
        NULL,                   /* TextAttr             */
        NULL,                   /* Locale               */
        NULL,                   /* Catalog              */
        NULL,                   /* MemPool              */
        2048,                   /* MemPoolPuddle        */
        2048,                   /* MemPoolThresh        */
        MSG_FILEREQ_TITLE,      /* TitleID              */
        NULL,                   /* TitleText            */
        NULL,                   /* PositiveText         */
        NULL,                   /* NegativeText         */
        -1, -1,                 /* --> center on screen */
        DEF_WIDTH, DEF_HEIGHT,  /* Width/Height         */
        IF_POPTOFRONT
    },

    "",                         /* File                 */
    "",                         /* Drawer               */
    "#?",                       /* Pattern              */
    NULL,                       /* AcceptPattern        */ /* def. = "#?", but must be ParsePatternNoCase'ed */
    NULL,                       /* RejectPattern        */ /* def. = "~(#?)", but must be ParsePatternNoCase'ed */
    0,                          /* Flags1               */
    0,                          /* Flags2               */
    NULL,                       /* FilterFunc           */
    NULL,                       /* HookFunc             */
    NULL,                       /* GetSortBy            */
    NULL,                       /* GetSortOrder         */
    NULL,                       /* GetSortDrawers       */
    ASLFRSORTBY_Name,           /* SortBy               */
    ASLFRSORTORDER_Ascend,      /* SortOrder            */
    ASLFRSORTDRAWERS_First,     /* SortDrawers          */
    FALSE                       /* InitialShowVolumes   */
};

/*****************************************************************************************/

const struct IntSMReq def_smreq =
{
    {
        ASL_ScreenModeRequest,
        NULL,                           /* Window               */
        NULL,                           /* Screen               */
        NULL,                           /* PubScreenName        */
        NULL,                           /* IntuiMsgFunc         */
        NULL,                           /* TextAttr             */
        NULL,                           /* Locale               */
        NULL,                           /* Catalog              */
        NULL,                           /* MemPool              */
        2048,                           /* MemPoolPuddle        */
        2048,                           /* MemPoolThresh        */
        MSG_MODEREQ_TITLE,              /* TitleID              */
        NULL,                           /* TitleText            */
        NULL,                           /* PositiveText         */
        NULL,                           /* NegativeText         */
        -1, -1,                         /* --> center on screen */
        DEF_WIDTH, DEF_HEIGHT,          /* Width/Height         */
        IF_POPTOFRONT
    },

    NULL,                               /* CustomSMList         */
    NULL,                               /* FilterFunc           */
    0,                                  /* Flags                */
    LORES_KEY,                          /* DisplayID            */
    640,                                /* DisplayWidth         */
    200,                                /* DisplayHeight        */
    640,                                /* BitMapWidth          */
    200,                                /* BitMapHeight         */
    2,                                  /* DisplayDepth         */
    OSCAN_TEXT,                         /* OverscanType         */
    TRUE,                               /* AutoScroll           */
    DIPF_IS_WB,                         /* PropertyFlags        */
    DIPF_IS_WB,                         /* PropertyMask         */
    1,                                  /* MinDepth             */
    24,                                 /* MaxDepth             */
    16,                                 /* MinWidth             */
    16384,                              /* MaxWidth             */
    16,                                 /* MinHeight            */
    16384,                              /* MaxHeight            */
    20,                                 /* InfoLeftEdge         */
    20,                                 /* InfoTopEdge          */
    FALSE                               /* InfoOpened           */
};

/*****************************************************************************************/

const struct IntFontReq def_fontreq =
{
    {
        ASL_FontRequest,
        NULL,                           /* Window               */
        NULL,                           /* Screen               */
        NULL,                           /* PubScreenName        */
        NULL,                           /* IntuiMsgFunc         */
        NULL,                           /* TextAttr             */
        NULL,                           /* Locale               */
        NULL,                           /* Catalog              */
        NULL,                           /* MemPool              */
        2048,                           /* MemPoolPuddle        */
        2048,                           /* MemPoolThresh        */
        MSG_FONTREQ_TITLE,              /* TitleID              */
        NULL,                           /* TitleText            */
        NULL,                           /* PositiveText         */
        NULL,                           /* NegativeText         */
        -1, -1,                         /* --> center on screen */
        DEF_WIDTH, DEF_HEIGHT,          /* Width/Height         */
        IF_POPTOFRONT
    },
    {"topaz", 8, FS_NORMAL,FPF_ROMFONT},/* Default textattr     */
    1,                                  /* FrontPen             */
    0,                                  /* BackPen              */
    JAM1,                               /* DrawMode             */
    0,                                  /* Flags                */

    5,                                  /* Minheight            */
    24,                                 /* MaxHeight            */
    NULL,                               /* FilterFunc           */
    NULL,                               /* HookFunc             */
    32,                                 /* MaxFrontPen          */
    32,                                 /* MaxBackPen           */

    NULL,                               /* ModeList             */
    NULL,                               /* FrontPens            */
    NULL,                               /* BackPens             */

};

/* coolimages may fail to open */
LONG CoolImagesBase_version = -1;

/*****************************************************************************************/

VOID InitReqInfo(struct AslBase_intern *);
VOID LoadPrefs(struct AslBase_intern *);

/*****************************************************************************************/

static int InitBase(LIBBASETYPEPTR LIBBASE)
{
    D(bug("Inside InitBase of asl.library\n"));

    NEWLIST(&LIBBASE->ReqList);

    InitSemaphore(&LIBBASE->ReqListSem);

    InitReqInfo(LIBBASE);

    return 1;
}

/*****************************************************************************************/

ADD2INITLIB(InitBase, 0);

/*****************************************************************************************/

static int OpenBase(LIBBASETYPEPTR LIBBASE)
{
    if (!(LIBBASE->Prefs.ap_Reserved[0] & 0x1))
        LoadPrefs(LIBBASE);

    return 1;
}

/*****************************************************************************************/

ADD2OPENLIB(OpenBase, 0);

/*****************************************************************************************/

#include <string.h>
#include "filereqhooks.h"
#include "fontreqhooks.h"
#include "modereqhooks.h"

/*****************************************************************************************/

VOID InitReqInfo(struct AslBase_intern *AslBase)
{
    struct AslReqInfo *reqinfo;

    /* Set file requester info */

    reqinfo = &(ASLB(AslBase)->ReqInfo[ASL_FileRequest]);
    D(bug("AslBase: %p reqinfo: %p\n", AslBase, reqinfo));
    reqinfo->IntReqSize         = sizeof (struct IntFileReq);
    reqinfo->ReqSize            = sizeof (struct FileRequester);
    reqinfo->DefaultReq         = (struct IntFileReq *)&def_filereq;
    reqinfo->UserDataSize       = sizeof (struct FRUserData);

    bzero(&(reqinfo->ParseTagsHook), sizeof (struct Hook));
    bzero(&(reqinfo->GadgetryHook), sizeof (struct Hook));
    reqinfo->ParseTagsHook.h_Entry      = (void *)AROS_ASMSYMNAME(FRTagHook);
    reqinfo->GadgetryHook.h_Entry       = (void *)AROS_ASMSYMNAME(FRGadgetryHook);

    /* Set font requester info */

    reqinfo = &(ASLB(AslBase)->ReqInfo[ASL_FontRequest]);
    reqinfo->IntReqSize         = sizeof (struct IntFontReq);
    reqinfo->ReqSize            = sizeof (struct FontRequester);
    reqinfo->DefaultReq         = (struct IntFontReq *)&def_fontreq;
    reqinfo->UserDataSize       = sizeof (struct FOUserData);

    bzero(&(reqinfo->ParseTagsHook), sizeof (struct Hook));
    bzero(&(reqinfo->GadgetryHook), sizeof (struct Hook));
    reqinfo->ParseTagsHook.h_Entry      = (void *)AROS_ASMSYMNAME(FOTagHook);
    reqinfo->GadgetryHook.h_Entry       = (void *)AROS_ASMSYMNAME(FOGadgetryHook);

    /* Set screenmode requester info */

    reqinfo = &(ASLB(AslBase)->ReqInfo[ASL_ScreenModeRequest]);
    reqinfo->IntReqSize         = sizeof (struct IntSMReq);
    reqinfo->ReqSize            = sizeof (struct ScreenModeRequester);
    reqinfo->DefaultReq         = (struct IntSMReq *)&def_smreq;
    reqinfo->UserDataSize       = sizeof(struct SMUserData);

    bzero(&(reqinfo->ParseTagsHook), sizeof (struct Hook));
    bzero(&(reqinfo->GadgetryHook), sizeof (struct Hook));
    reqinfo->ParseTagsHook.h_Entry      = (void *)AROS_ASMSYMNAME(SMTagHook);
    reqinfo->GadgetryHook.h_Entry       = (void *)AROS_ASMSYMNAME(SMGadgetryHook);
}

/*****************************************************************************************/

VOID LoadPrefs(struct AslBase_intern *AslBase)
{
    struct Library *IFFParseBase;

    IFFParseBase = OpenLibrary("iffparse.library", 0);
    if (IFFParseBase)
    {
        struct IFFHandle *iff = AllocIFF();
        if (iff)
        {
            iff->iff_Stream = (IPTR)Open("ENV:Sys/Asl.prefs", MODE_OLDFILE);
            if (iff->iff_Stream)
            {
                InitIFFasDOS(iff);
                if (OpenIFF(iff, IFFF_READ) == 0)
                {
                    if (StopChunk(iff, ID_PREF, ID_ASL) == 0)
                    {
                        if (ParseIFF(iff, IFFPARSE_SCAN) == 0)
                        {
                            ReadChunkBytes(iff, &AslBase->Prefs, sizeof(struct AslPrefs));

                            AslBase->Prefs.ap_RelativeLeft  = AROS_BE2WORD(AslBase->Prefs.ap_RelativeLeft);
                            AslBase->Prefs.ap_RelativeTop   = AROS_BE2WORD(AslBase->Prefs.ap_RelativeTop);
                            AslBase->Prefs.ap_Reserved[0]   |= 0x1; /* loaded */
                        }
                    }

                    CloseIFF(iff);
                } /* if (OpenIFF(iff)) */
                Close((BPTR)iff->iff_Stream);

            } /* if (iff->iff_Stream) */
            FreeIFF(iff);

        } /* if (iff) */
        CloseLibrary(IFFParseBase);

    } /* if (IFFParseBase) */
}

/*****************************************************************************************/
