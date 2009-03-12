#ifndef BOOTMENU_INTERN_H
#define BOOTMENU_INTERN_H

/*
	Copyright © 1995-2006, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Internal definitions for bootmenu
   Lang: english
*/

#include <aros/libcall.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <libcore/base.h>
#include <libraries/expansionbase.h>
#include <libraries/bootmenu.h>
#include "gadgets.h"
#include LC_LIBDEFS_FILE

LIBBASETYPE {
    struct Node          bm_Node;
    struct ExpansionBase *bm_ExpansionBase;
    struct GfxBase       *bm_GfxBase;
    struct IntuitionBase *bm_IntuitionBase;
    struct BootConfig    bm_BootConfig;
    struct Screen        *bm_Screen;
    struct Window        *bm_Window;
    struct MainGadgets   bm_MainGadgets;
    BOOL		 bm_Force;
};

#undef ExpansionBase
#define ExpansionBase BootMenuBase->bm_ExpansionBase
#undef GfxBase
#define GfxBase BootMenuBase->bm_GfxBase
#undef IntuitionBase
#define IntuitionBase BootMenuBase->bm_IntuitionBase

void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase);

#endif /* BOOTMENU_INTERN_H */

