/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: internal language list support functions.
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <aros/symbolsets.h>
#include <exec/semaphores.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "locale_intern.h"
#include LC_LIBDEFS_FILE


struct LanguageMapping
{
    char *BaseName;
    char *NativeName;
}
languagesSupported[] =
{
    {"", ""},
    {NULL, NULL}
};

char *GetNativeName(char *baseName)
{
    char *nativeName = baseName;

    return nativeName;
}
