#ifndef TRANSLATOR_INTERN_H
#define TRANSLATOR_INTERN_H

/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <exec/libraries.h>

#include <libraries/speechcore.h>

struct DosLibrary;

struct TranslatorBase
{
    struct Library tb_Library;
    struct DosLibrary *tb_DOSBase;
    struct SCResource tb_Resource;
    APTR tb_ResourceMemory;
};

#endif /* TRANSLATOR_INTERN_H */
