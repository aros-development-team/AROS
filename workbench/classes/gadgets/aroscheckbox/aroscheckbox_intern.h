/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSCHECKBOX_INTERN_H
#define AROSCHECKBOX_INTERN_H



#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_CGHOOKS_H
#   include <intuition/cghooks.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#   include <intuition/gadgetclass.h>
#endif

/* Predeclaration */
struct CBBase_intern;

#define GLOBAL_INTUIBASE

#define TURN_OFF_DEBUG


/* Support */
#define G(obj) ((struct Gadget *)(obj))


/* CheckboxClass definitions */
struct CheckData {
    struct DrawInfo *dri;
    UWORD flags;
    LONG labelplace;
};

#define CF_Checked     0x0001
#define CF_CustomImage 0x0002


/* Prototypes */
void drawdisabledpattern(struct CBBase_intern *AROSCheckboxBase, struct RastPort *rport, UWORD pen, WORD left, WORD top, UWORD width, UWORD height);
BOOL renderlabel(struct CBBase_intern *AROSCheckboxBase,
		 struct Gadget *gad, struct RastPort *rport, LONG labelplace);



/* Library stuff */
struct CBBase_intern
{
    struct Library 	library;
    struct ExecBase	*sysbase;
    BPTR		seglist;

    #ifndef GLOBAL_INTUIBASE
    struct IntuitionBase *intuitionbase;
    #endif
    struct GfxBase	*gfxbase;
    struct Library	*utilitybase;
    
    struct IClass	*classptr;
	
};

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers	and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct GfxBase GraphicsBase;
typedef struct IntuitionBase IntuiBase;

#undef CBB
#define CBB(b) ((struct CBBase_intern *)b)
#undef UtilityBase
#define UtilityBase 	CBB(AROSCheckboxBase)->utilitybase


#ifndef GLOBAL_INTUIBASE
#undef IntuitionBase
#define IntuitionBase	CBB(AROSCheckboxBase)->intuitionbase
#endif

#undef GfxBase
#define GfxBase		CBB(AROSCheckboxBase)->gfxbase
#undef SysBase
#define SysBase		CBB(AROSCheckboxBase)->sysbase


#define expunge() \
AROS_LC0(BPTR, expunge, struct CBBase_intern *, AROSCheckboxBase, 3, AROSCheckbox)

#endif /* AROSCHECKBOX_INTERN_H */
