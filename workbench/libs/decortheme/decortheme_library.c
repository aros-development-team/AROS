/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: decortheme.library initialization
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "decortheme_intern.h"

struct Library *DecoratorBase;

static ULONG DecorTheme_Init(struct DecorThemeBase *DecorThemeBase)
{
    DecoratorBase = OpenLibrary("decorator.library", 0);
    return (DecoratorBase != NULL);
}

static ULONG DecorTheme_Expunge(struct DecorThemeBase *DecorThemeBase)
{
    if (DecoratorBase != NULL)
    {
        CloseLibrary(DecoratorBase);
        DecoratorBase = NULL;
    }
    return TRUE;
}

ADD2INITLIB(DecorTheme_Init, 0)
ADD2EXPUNGELIB(DecorTheme_Expunge, 0)
