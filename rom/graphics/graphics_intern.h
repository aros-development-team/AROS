#ifndef GRAPHICS_INTERN_H
#define GRAPHICS_INTERN_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

extern struct GfxBase * GfxBase;

#ifdef SysBase
#undef SysBase
#endif
#define SysBase ((struct ExecBase *)(GfxBase->ExecBase))

/* Needed for close() */
#define expunge() \
__AROS_LC0(BPTR, expunge, struct GfxBase *, GfxBase, 3, Gfx)

#endif /* GRAPHICS_INTERN_H */
