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


struct GadToolsBase_intern; /* prerefrence */

/* Internal prototypes */


struct GadToolsBase_intern
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

#define GTB(gtb)        ((struct GadToolsBase_intern *)gtb)
#undef SysBase
#define SysBase (GTB(GadToolsBase)->sysbase)
#undef IntuitionBase
#define IntuitionBase (GTB(GadToolsBase)->intuibase)
#undef DOSBase
#define DOSBase (GTB(GadToolsBase)->dosbase)
#undef GfxBase
#define GfxBase (GTB(GadToolsBase)->gfxbase)
#undef UtilityBase
#define UtilityBase (GTB(GadToolsBase)->utilitybase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct GadToolsBase_intern *, GadToolsBase, 3, GadTools)

#endif /* GADTOOLS_INTERN_H */
