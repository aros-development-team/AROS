#ifndef SPEECHCORE_AROS_RESOURCE_H
#define SPEECHCORE_AROS_RESOURCE_H

/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <dos/dos.h>

#include <libraries/speechcore.h>

struct DosLibrary;

APTR LoadSpeechResource(struct DosLibrary *DOSBase, CONST_STRPTR path,
                        struct SCResource *resource);

#endif /* SPEECHCORE_AROS_RESOURCE_H */
