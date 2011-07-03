#ifndef BOOTMENU_GADGETS_H
#define BOOTMENU_GADGETS_H

#include <intuition/classusr.h>
#include <intuition/intuition.h>

struct DOSBootBase;

struct ButtonGadget {
	struct Gadget *gadget;
	WORD XY1[6];
	WORD XY2[6];
	struct Border uborder1;
	struct Border uborder2;
	struct Border sborder1;
	struct Border sborder2;
};

struct MainGadgets {
	struct ButtonGadget *boot;
	struct ButtonGadget *bootnss;
	struct ButtonGadget *bootopt;
	struct ButtonGadget *displayopt;
	struct ButtonGadget *expboarddiag;
	struct ButtonGadget *use;
	struct ButtonGadget *cancel;
};

#define BUTTON_BOOT            1
#define BUTTON_BOOT_WNSS       2
#define BUTTON_BOOT_OPTIONS    3
#define BUTTON_DISPLAY_OPTIONS 4
#define BUTTON_EXPBOARDDIAG    5
#define BUTTON_USE             6
#define BUTTON_CANCEL          7
#define BUTTON_CONTINUE        8

struct ButtonGadget *createButton(ULONG, ULONG, ULONG, ULONG, struct Gadget *, STRPTR, UWORD, struct DOSBootBase *);
void freeButtonGadget(struct ButtonGadget *, struct DOSBootBase *);
#endif
