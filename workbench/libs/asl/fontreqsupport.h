/*
    (C) 1995-2001 AROS - The Amiga Research OS
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

LONG FOGetFonts(struct LayoutData *ld, struct AslBase_intern *AslBase);
struct ASLLVFontReqNode *FOGetActiveFont(struct LayoutData *ld, struct AslBase_intern *AslBase);
void FOChangeActiveFont(struct LayoutData *ld, WORD delta, UWORD quali, struct AslBase_intern *AslBase);
void FOActivateFont(struct LayoutData *ld, WORD which, LONG size, struct AslBase_intern *AslBase);
void FOSetFontString(STRPTR name, struct LayoutData *ld, struct AslBase_intern *AslBase);
void FOSetSizeString(LONG size, struct LayoutData *ld, struct AslBase_intern *AslBase);



