#ifndef DOSBOOT_INTERN_H
#define DOSBOOT_INTERN_H

/*
	Copyright © 1995-2006, The AROS Development Team. All rights reserved.
   $Id: dosboot_intern.h 29897 2008-10-27 09:27:10Z sonic $

   Desc: Internal definitions for dosboot
   Lang: english
*/

#include <aros/libcall.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <libcore/base.h>
#include <libraries/expansionbase.h>
#include <libraries/bootmenu.h>
//#include "gadgets.h"
#include LC_LIBDEFS_FILE

LIBBASETYPE {
    struct Node          db_Node;
};

//#undef ExpansionBase
//#define ExpansionBase DOSBootBase->bm_ExpansionBase
//#undef GfxBase
//#define GfxBase DOSBootBase->bm_GfxBase
//#undef IntuitionBase
//#define IntuitionBase DOSBootBase->bm_IntuitionBase

//void InitBootConfig(struct BootConfig *bootcfg, APTR BootLoaderBase);

#endif /* DOSBOOT_INTERN_H */

