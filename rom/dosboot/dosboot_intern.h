#ifndef DOSBOOT_INTERN_H
#define DOSBOOT_INTERN_H

/*
   Copyright © 1995-2011, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Internal definitions for dosboot
   Lang: english
*/

#include <dos/dosextens.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <graphics/gfxbase.h>

#include "gadgets.h"

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
    struct Node           db_Node;		/* Node for linking into the list */
    char                 *db_BootDevice;	/* Device to boot up from	  */

    struct GfxBase       *bm_GfxBase;		/* Library bases	  	  */
    struct IntuitionBase *bm_IntuitionBase;
    struct Window        *bm_Window;		/* Window and gadgets		  */
    struct MainGadgets    bm_MainGadgets;

    struct BootConfig     bm_BootConfig;	/* Current HIDD configuration     */
    ULONG		  BootFlags;		/* Bootup flags			  */
    
    APTR		  animData;		/* Animation stuff		  */
    ULONG		  delayTicks;		/* Delay period. Can be adjusted by animation code */
};

/* Boot flags */
#define BF_NO_STARTUP_SEQUENCE 0x0001
#define BF_NO_DISPLAY_DRIVERS  0x0002

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase);
BOOL __dosboot_InitHidds(struct DosLibrary *dosBase);
void __dosboot_Boot(APTR BootLoaderBase, struct DosLibrary *DOSBase, ULONG Flags);

struct Screen *NoBootMediaScreen(struct DOSBootBase *DOSBootBase);
struct Screen *OpenBootScreen(struct DOSBootBase *DOSBootBase);
void CloseBootScreen(struct Screen *scr, struct DOSBootBase *DOSBootBase);

APTR anim_Init(struct Screen *scr, struct DOSBootBase *DOSBootBase);
void anim_Stop(struct DOSBootBase *DOSBootBase);
void anim_Animate(struct Screen *scr, struct DOSBootBase *DOSBootBase);

#undef GfxBase
#define GfxBase DOSBootBase->bm_GfxBase
#undef IntuitionBase
#define IntuitionBase DOSBootBase->bm_IntuitionBase

#endif /* DOSBOOT_INTERN_H */
