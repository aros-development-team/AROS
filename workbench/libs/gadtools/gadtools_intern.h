#ifndef GADTOOLS_INTERN_H
#define GADTOOLS_INTERN_H

/* Include files */
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef PROTO_GRAPHICS_H
#   include <proto/graphics.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef PROTO_INTUITION_H
#   include <proto/intuition.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef PROTO_UTILITY_H
#   include <proto/utility.h>
#endif
#ifndef LIBRARIES_GADTOOLS_H
#   include <libraries/gadtools.h>
#endif

/* Some external stuff (gadtools_init.c) */


struct GadtoolsBase_intern; /* prerefrence */

/* Internal prototypes */


struct GadtoolsBase_intern
{
    struct Library    library;
    struct ExecBase * sysbase;
    BPTR              seglist;

    struct IntuitionBase * intuibase;
    struct Library       * dosbase;
    struct GfxBase       * gfxbase;
    struct Library       * utilitybase;
};

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers  and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct IntuitionBase IntuiBase;
typedef struct GfxBase GraphicsBase;

#define GTB(gtb)        ((struct GadtoolsBase_intern *)gtb)
#undef SysBase
#define SysBase (GTB(GadtoolsBase)->sysbase)
#undef IntuitionBase
#define IntuitionBase (GTB(GadtoolsBase)->intuibase)
#undef DOSBase
#define DOSBase (GTB(GadtoolsBase)->dosbase)
#undef GfxBase
#define GfxBase (GTB(GadtoolsBase)->gfxbase)
#undef UtilityBase
#define UtilityBase (GTB(GadtoolsBase)->utilitybase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct GadtoolsBase_intern *, GadtoolsBase, 3, Gadtools)

#endif /* GADTOOLS_INTERN_H */
