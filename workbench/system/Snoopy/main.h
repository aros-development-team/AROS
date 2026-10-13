/*
    Copyright (C) 2006-2026, The AROS Development Team. All rights reserved.
*/

#ifndef MAIN_H
#define MAIN_H

#define PATTERNLEN (100)
#define PARSEDPATTERNLEN (PATTERNLEN * 2 + 5)
#define MAX_STR_LEN (200)

struct Setup;

void clean_exit(char *s);
void main_output(CONST_STRPTR action, CONST_STRPTR target, CONST_STRPTR option, IPTR result, BOOL canInterrupt, BOOL expand);
LONG main_parsepattern(struct Setup *settings);

#endif

