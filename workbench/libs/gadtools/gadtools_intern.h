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
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif
#ifndef PROTO_UTILITY_H
#   include <proto/utility.h>
#endif
#ifndef LIBRARIES_GADTOOLS_H
#   include <libraries/gadtools.h>
#endif

struct VisualInfo;

/* Some external stuff (gadtools_init.c) */
struct GadToolsBase_intern; /* prerefrence */

/* Internal prototypes */
BOOL renderlabel(struct GadToolsBase_intern *GadToolsBase,
		 struct Gadget *gad, struct RastPort *rport, LONG redraw);
void drawdisabledpattern(struct GadToolsBase_intern *GadToolsBase,
			 struct RastPort *rport, UWORD pen,
			 WORD left, WORD top, UWORD width, UWORD height);
struct IntuiText *makeitext(struct GadToolsBase_intern *GadToolsBase,
			    struct NewGadget *ng);
void drawbevelsbyhand(struct GadToolsBase_intern *GadToolsBase,
                      struct RastPort *rport,
                      WORD left, WORD top, WORD width, WORD height,
                      struct TagItem *taglist);

Class *makebuttonclass(struct GadToolsBase_intern *GadToolsBase);
Class *makecheckclass(struct GadToolsBase_intern *GadToolsBase);

struct Gadget *makebutton(struct GadToolsBase_intern *GadToolsBase,
                          struct TagItem stdgadtags[],
			  struct VisualInfo *vi,
                          struct TagItem *taglist);
struct Gadget *makecheckbox(struct GadToolsBase_intern *GadToolsBase,
                            struct TagItem stdgadtags[],
			    struct VisualInfo *vi,
			    struct TagItem *taglist);

struct GadToolsBase_intern
{
    struct Library    library;
    struct ExecBase * sysbase;
    BPTR              seglist;

    struct IntuitionBase * intuibase;
    struct Library       * dosbase;
    struct GfxBase       * gfxbase;
    struct Library       * utilitybase;

    Class * buttonclass;
    Class * checkclass;
};

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers  and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct IntuitionBase IntuiBase;
typedef struct GfxBase GraphicsBase;

#define GTB(gtb)        ((struct GadToolsBase_intern *)gtb)
/*
#undef SysBase
#define SysBase (GTB(GadToolsBase)->sysbase)
*/
extern struct ExecBase * SysBase;
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

struct VisualInfo
{
    struct Screen   * vi_screen;
    struct DrawInfo * vi_dri;
};

#define TAG_Left     0
#define TAG_Top      1
#define TAG_Width    2
#define TAG_Height   3
#define TAG_IText    4
#define TAG_Previous 5
#define TAG_ID       6
#define TAG_DrawInfo 7
#define TAG_UserData 8
#define TAG_Num      9

#endif /* GADTOOLS_INTERN_H */
