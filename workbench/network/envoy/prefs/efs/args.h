#ifndef ARGS_H
#define ARGS_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/
#include <exec/types.h>

enum Argument { FROM, USE, SAVE, PUBSCREEN, COUNT };
#define ARG(a) (GetArgument(a))

BOOL ReadArguments(int argc, char **argv);
VOID FreeArguments(VOID);
IPTR GetArgument(enum Argument id);
#endif
