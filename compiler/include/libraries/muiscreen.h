#ifndef MUISCREEN_H
#define MUISCREEN_H

/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>
#include <exec/lists.h>

#define PSD_INITIAL_NAME   "(unnamed)"
#define PSD_INITIAL_TITLE  "Zune Public Screen"
#define PSD_ID_MPUB        MAKE_ID('M','P','U','B')

#define PSD_NAME_FRONTMOST "«Frontmost»"

#define PSD_FILENAME_SAVE "envarc:Zune/PublicScreens.iff"
#define PSD_FILENAME_USE  "env:Zune/PublicScreens.iff"

#define PSD_MAXLEN_NAME         32
#define PSD_MAXLEN_TITLE       128
#define PSD_MAXLEN_FONT         48
#define PSD_MAXLEN_BACKGROUND  256
#define PSD_NUMCOLS              8
#define PSD_MAXSYSPENS          20
#define PSD_NUMSYSPENS          12
#define PSD_MAXMUIPENS          10
#define PSD_NUMMUIPENS  MPEN_COUNT

struct MUI_PubScreenDesc
{
    LONG  Version;

    char  Name[PSD_MAXLEN_NAME];
    char  Title[PSD_MAXLEN_TITLE];
    char  Font[PSD_MAXLEN_FONT];
    char  Background[PSD_MAXLEN_BACKGROUND];

    ULONG DisplayID;

    UWORD DisplayWidth;
    UWORD DisplayHeight;

    UBYTE DisplayDepth;
    UBYTE OverscanType;
    UBYTE AutoScroll;
    UBYTE NoDrag;
    UBYTE Exclusive;
    UBYTE Interleaved;
    UBYTE SysDefault;
    UBYTE Behind;
    UBYTE AutoClose;
    UBYTE CloseGadget;
    UBYTE DummyWasForeign;

    BYTE SystemPens[PSD_MAXSYSPENS];
    UBYTE Reserved[1+7*4-PSD_MAXSYSPENS];

    struct MUI_RGBcolor Palette[PSD_NUMCOLS];
    struct MUI_RGBcolor rsvd[PSD_MAXSYSPENS-PSD_NUMCOLS];

    struct MUI_PenSpec rsvd2[PSD_MAXMUIPENS];

    LONG Changed;
    APTR UserData;
};

struct MUIS_InfoClient
{
    struct MinNode node;
    struct Task *task;
    ULONG sigbit;
};

#endif
