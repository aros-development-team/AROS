/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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
void drawdisabledpattern(struct RastPort *rport, UWORD pen, WORD left, WORD top, UWORD width, UWORD height);
BOOL renderlabel(struct Gadget *gad, struct RastPort *rport, LONG labelplace);

#endif /* AROSCHECKBOX_INTERN_H */
