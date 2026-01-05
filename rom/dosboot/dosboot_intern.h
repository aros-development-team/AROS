#ifndef DOSBOOT_INTERN_H
#define DOSBOOT_INTERN_H

/*
   Copyright © 1995-2026, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Internal definitions for dosboot
   Lang: english
*/

#include <dos/dosextens.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <graphics/gfxbase.h>

#include "bootflags.h"

#define BUFSIZE 100

struct BootConfig
{
    /* default hidds used in bootmenu and for fallback mode */
    STRPTR gfxlib;
    STRPTR gfxhidd;
    BOOL bootmode;
};

struct DOSBootBase
{
    struct Node           db_Node;              /* Node for linking into the list                          */
    ULONG                 db_BootFlags;         /* Bootup flags (identical to IntExpansionBase->BootFlags) */

    struct BootNode      *db_BootNode;          /* Device to boot up from                                  */

    struct GfxBase       *bm_GfxBase;           /* Library bases                                           */
    struct IntuitionBase *bm_IntuitionBase;
    struct GadToolsBase  *bm_GadToolsBase;
    APTR                  bm_VisualInfo;
    struct ExpansionBase *bm_ExpansionBase;
    struct Screen        *bm_Screen;            /* Screen                                                  */
    struct Window        *bm_Window;            /* Window and gadgets                                      */
    APTR                  bm_BootNode;          /* Boot node selected in the menu                          */

    struct BootConfig     bm_BootConfig;        /* Current HIDD configuration                              */
    APTR                  animData;	            /* Animation stuff                                         */
    ULONG                 delayTicks;           /* Delay period. Can be adjusted by animation code         */
    WORD                  bottomY;

    WORD                  devicesCount;
    BOOL                 *devicesEnabled;
    UWORD                *blank_pointer;
    struct List           bootList;
    struct List           devicesList;
};

void InitBootConfig(struct BootConfig *bootcfg);
LONG dosboot_BootStrap(struct DOSBootBase *DOSBootBase);
void dosboot_BootScan(struct DOSBootBase *DOSBootBase);

struct Screen *NoBootMediaScreen(struct DOSBootBase *DOSBootBase);
struct Screen *OpenBootScreen(struct DOSBootBase *DOSBootBase);
void CloseBootScreen(struct Screen *scr, struct DOSBootBase *DOSBootBase);

APTR anim_Init(struct Screen *scr, struct DOSBootBase *DOSBootBase);
void anim_Stop(struct DOSBootBase *DOSBootBase);
void anim_Animate(struct Screen *scr, struct DOSBootBase *DOSBootBase);

#define IntuitionBase DOSBootBase->bm_IntuitionBase
#define GadToolsBase DOSBootBase->bm_GadToolsBase
#define GfxBase DOSBootBase->bm_GfxBase

/* Check to see if the bootnode is bootable */
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>

static inline BOOL IsBootableNode(struct BootNode *bootNode)
{
    return ((bootNode->bn_Node.ln_Type == NT_BOOTNODE) &&
            (bootNode->bn_Node.ln_Pri > -128)) ? TRUE : FALSE;
}


#endif /* DOSBOOT_INTERN_H */
