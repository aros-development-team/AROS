#ifndef BOOTPREFS_H
#define BOOTPREFS_H

#include <libcore/base.h>

struct DefaultHidd {
	TEXT libname[64];
	TEXT hiddname[64];
};

struct BootConfig {
	struct BootConfig *self;
	/* default hidds used in bootmenu and for fallback mode */
	struct DefaultHidd defaultgfx;
	struct DefaultHidd defaultkbd;
	struct DefaultHidd defaultmouse;
	/* prefered boot device */
	struct BootNode *boot;
	/* boot with startup-sequence ? */
	BOOL startup_sequence;
};

struct BootMenuBase {
	struct LibHeader lh;
	struct GfxBase *GfxBase;
	struct IntuitionBase *IntuitionBase;
	struct BootConfig bcfg;
};

#endif

