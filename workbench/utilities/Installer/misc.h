/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _MISC_H
#define _MISC_H

extern int strtostrs(char * in, char *** outarr);
extern char *collatestrings(int n, char ** instrs);
extern char *addquotes(char * array);
extern void freestrlist(STRPTR * string);
extern void manifest_log(char kind, const char *path);


#endif /* _MISC_H */

