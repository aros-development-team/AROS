#ifndef GRAPHICS_INTERN_H
#define GRAPHICS_INTERN_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/10/24 15:51:05  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/16 14:05:49  digulla
    Added #include <graphics/rastport.h>

    Revision 1.2  1996/08/13 13:59:42  digulla
    Added SysBase as a define and to GfxBase

    Revision 1.1  1996/08/12 14:27:51  digulla
    Base of graphics library

    Desc:
    Lang:
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif

extern struct GfxBase * GfxBase;

#ifdef SysBase
#undef SysBase
#endif
#define SysBase ((struct ExecBase *)(GfxBase->ExecBase))

/* Needed for close() */
#define expunge() \
    AROS_LC0(BPTR, expunge, struct GfxBase *, GfxBase, 3, Gfx)

#endif /* GRAPHICS_INTERN_H */
