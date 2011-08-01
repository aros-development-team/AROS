#ifndef BOOTMENU_DRIVER_H
#define BOOTMENU_DRIVER_H

#include "dosboot_intern.h"

int bootmenu_Init(LIBBASETYPEPTR LIBBASE, BOOL WantBootMenu);
void selectBootDevice(LIBBASETYPEPTR DOSBootBase);

#endif
