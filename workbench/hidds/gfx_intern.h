#ifndef GFX_INTERN_H
#define GFX_INTERN_H
/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif



/* Library stuff */
struct gfxhiddbase
{
    struct Library 	library;
    struct ExecBase	*sysbase;
    BPTR		seglist;

    struct Library	*utilitybase;
    struct Library	*oopbase;
    
    Class		*gfxclass;
    Class		*gcclass;
	
};


static Class *init_gfxclass (struct gfxhiddbase *gfxhiddbase);
static Class *init_gcclass(struct gfxhiddbase *);
static VOID cleanup_gfxclass(Class *class, struct gfxhiddbase *);
static VOID cleanup_gcclass(Class *class, struct gfxhiddbase *);


#define expunge() \
AROS_LC0(BPTR, expunge, struct gfxhiddbase *, LIBBASE, 3, X11Gfx)

#endif /* X11GFX_INTERN_H */
