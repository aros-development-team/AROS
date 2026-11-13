/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _ACCOUNTS_LOCALE_H
#define _ACCOUNTS_LOCALE_H

#include <exec/types.h>

#define CATCOMP_NUMBERS
#include "strings.h"

CONST_STRPTR MSG(ULONG id);
VOID Locale_Initialize(VOID);
VOID Locale_Deinitialize(VOID);

#endif /* _ACCOUNTS_LOCALE_H */
