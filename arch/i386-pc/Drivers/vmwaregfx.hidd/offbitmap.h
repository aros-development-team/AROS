#ifndef OFFBITMAP_H
#define OFFBITMAP_H

#include <oop/oop.h>
#include "vmwaregfxclass.h"

OOP_Class *init_vmwaregfxoffbmclass(struct VMWareGfx_staticdata *);
void free_vmwaregfxoffbmclass(struct VMWareGfx_staticdata *);

#endif
