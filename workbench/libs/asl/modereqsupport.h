/*
    (C) 1995-97 AROS - The Amiga Research OS

    Desc:
    Lang: english
*/

#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif
#ifndef DOS_EXALL_H
#   include <dos/exall.h>
#endif
#ifndef DOS_DOSEXTENS_H
#   include <dos/dosextens.h>
#endif

#ifndef LAYOUT_H
#   include "layout.h"
#endif

LONG SMGetModes(struct LayoutData *ld, struct AslBase_intern *AslBase);
struct DisplayMode *SMGetActiveMode(struct LayoutData *ld, struct AslBase_intern *AslBase);
void SMChangeActiveLVItem(struct LayoutData *ld, WORD delta, UWORD quali, struct AslBase_intern *AslBase);
UWORD SMGetOverscan(struct LayoutData *ld, struct DisplayMode *dispmode,
		    struct Rectangle **rect, struct AslBase_intern *AslBase);
void SMFixValues(struct LayoutData *ld, struct DisplayMode *dispmode,
		 LONG *width, LONG *height, LONG *depth, struct AslBase_intern *AslBase);
void SMActivateMode(struct LayoutData *ld, WORD which, struct AslBase_intern *AslBase);
void SMRestore(struct LayoutData *ld, struct AslBase_intern *AslBase);
LONG SMGetStringValue(struct LayoutData *ld, Object *obj, struct AslBase_intern *AslBase);
LONG SMGetWidth(struct LayoutData *ld, struct AslBase_intern *AslBase);
LONG SMGetHeight(struct LayoutData *ld, struct AslBase_intern *AslBase);
BOOL SMGetAutoScroll(struct LayoutData *ld, struct AslBase_intern *AslBase);






