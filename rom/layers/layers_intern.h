/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Internal information for layers.library.
    Lang:
*/
#ifndef LAYERS_INTERN_H
#define LAYERS_INTERN_H

#include <exec/libraries.h>
#include <aros/libcall.h>
#include <dos/dos.h>
#include <graphics/gfxbase.h>

extern struct GfxBase * GfxBase;

struct LayersBase
{
    struct Library       LibNode;
    struct ExecBase    * lb_SysBase;
    BPTR                 lb_SegList;
};

#endif
