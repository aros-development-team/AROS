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
#include <libcore/base.h>
#include <libraries/bootmenu.h>
#include "gadgets.h"
#include LC_LIBDEFS_FILE

LIBBASETYPE {
	struct LibHeader lh;
	struct BootConfig bcfg;
	struct Screen *scr;
	struct Window *win;
	struct MainGadgets maingadgets;
};

#endif /* BOOTMENU_INTERN_H */

