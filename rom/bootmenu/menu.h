#ifndef BOOTMENU_DRIVER_H
#define BOOTMENU_DRIVER_H

#include "bootmenu_intern.h"

BOOL initHidds(struct BootConfig *, struct BootMenuBase *);
BOOL initScreen(struct BootMenuBase *, struct BootConfig *);
BOOL buttonsPressed(struct BootMenuBase *, struct DefaultHidd *);

#endif

