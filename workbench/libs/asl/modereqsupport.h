/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

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
void SMActivateMode(struct LayoutData *ld, WORD which, LONG depth, struct AslBase_intern *AslBase);
void SMRestore(struct LayoutData *ld, struct AslBase_intern *AslBase);
LONG SMGetStringValue(struct LayoutData *ld, Object *obj, struct AslBase_intern *AslBase);
LONG SMGetWidth(struct LayoutData *ld, struct AslBase_intern *AslBase);
LONG SMGetHeight(struct LayoutData *ld, struct AslBase_intern *AslBase);
LONG SMGetDepth(struct LayoutData *ld, LONG *realdepth, struct AslBase_intern *AslBase);
BOOL SMGetAutoScroll(struct LayoutData *ld, struct AslBase_intern *AslBase);
void SMSetDepth(struct LayoutData *ld, UWORD id, struct AslBase_intern *AslBase);
void SMSetOverscan(struct LayoutData *ld, UWORD oscan, struct AslBase_intern *AslBase);
void SMSetAutoScroll(struct LayoutData *ld, BOOL onoff, struct AslBase_intern *AslBase);
void SMOpenPropertyWindow(struct LayoutData *ld, struct AslBase_intern *AslBase);
void SMClosePropertyWindow(struct LayoutData *ld, struct AslBase_intern *AslBase);
ULONG SMHandlePropertyEvents(struct LayoutData *ld, struct IntuiMessage *imsg, struct AslBase_intern *AslBase);
void SMRefreshPropertyWindow(struct LayoutData *ld, struct DisplayMode *dispmode, struct AslBase_intern *AslBase);










