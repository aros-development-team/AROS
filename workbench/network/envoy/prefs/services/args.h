#ifndef SVCPREFS_ARGS_H
#define SVCPREFS_ARGS_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>

enum Argument { FROM, USE, SAVE, PUBSCREEN, COUNT };

BOOL ReadArguments(int argc, char **argv);
VOID FreeArguments(VOID);
IPTR GetArgument(enum Argument arg);

#define ARG(a) GetArgument((a))

#endif
