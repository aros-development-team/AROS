/*
    Copyright © 2006-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef MAIN_H
#define MAIN_H

#define PATTERNLEN (100)
#define PARSEDPATTERNLEN (PATTERNLEN * 2 + 5)
#define MAX_STR_LEN (200)

void clean_exit(char *s);
void main_output(CONST_STRPTR action, CONST_STRPTR target, CONST_STRPTR option, IPTR result, BOOL canInterrupt, BOOL expand);
void main_parsepattern(void);

#endif

