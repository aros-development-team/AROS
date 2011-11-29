/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.

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

BOOL FRGetDirectory(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase);
BOOL FRGetVolumes(struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRSetPath(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase);
BOOL FRNewPath(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase);
BOOL FRAddPath(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase);
BOOL FRParentPath(struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRSetFile(STRPTR file, struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRFreeListviewNode(struct ASLLVFileReqNode *node, struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRFreeListviewList(struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRReSortListview(struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRChangeActiveLVItem(struct LayoutData *ld, WORD delta, UWORD quali, struct Gadget *gad, struct AslBase_intern *AslBase);
void FRActivateMainStringGadget(struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRMultiSelectOnOff(struct LayoutData *ld, BOOL onoff, struct AslBase_intern *AslBase);
void FRSetPattern(STRPTR pattern, struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRSelectRequester(struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRDeleteRequester(struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRNewDrawerRequester(struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRRenameRequester(struct LayoutData *ld, struct AslBase_intern *AslBase);
void FRClickOnVolumes(struct LayoutData *ld, struct AslBase_intern *AslBase);





