#ifndef BOOTPREFS_H
#define BOOTPREFS_H

#include <libcore/base.h>

struct DefaultHidd {
	TEXT libname[64];
	TEXT hiddname[64];
};

struct BootConfig {
	struct BootConfig *self;
	/* prefered boot device */
	struct BootNode *boot;
	/* default hidds used in bootmenu and for fallback mode */
	struct DefaultHidd defaultgfx;
	struct DefaultHidd defaultkbd;
	struct DefaultHidd defaultmouse;
};

struct BootMenuBase {
    struct Node          bm_Node;
    struct ExpansionBase *bm_ExpansionBase;
	struct GfxBase       *bm_GfxBase;
	struct IntuitionBase *bm_IntuitionBase;
	struct BootConfig    bm_BootConfig;
	IPTR                 bm_Screen;
	IPTR                 bm_Window;
	IPTR                 bm_MainGadgets;
};

#endif


