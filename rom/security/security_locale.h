/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library locale support
*/
#ifndef _SECURITY_LOCALE_H
#define _SECURITY_LOCALE_H

#define CATCOMP_NUMBERS
#include "strings.h"

struct SecurityBase;

struct LocaleInfo
{
    APTR        li_LocaleBase;
    APTR        li_Catalog;
};

extern void OpenLoc(struct SecurityBase *secBase, struct LocaleInfo *li);
extern void CloseLoc(struct SecurityBase *secBase, struct LocaleInfo *li);
extern CONST_STRPTR GetString(struct SecurityBase *secBase, struct LocaleInfo *li, LONG id);
#define GetLocS(base,li,id)     GetString(base, li, id)
extern CONST_STRPTR GetLocStr(struct SecurityBase *secBase, LONG id);
#define GetLogStr(base,id)      GetLocS(base, &(base)->LogInfo, id)

#endif /* _SECURITY_LOCALE_H */
