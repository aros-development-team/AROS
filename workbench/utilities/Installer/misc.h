/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MISC_H
#define _MISC_H

extern int strtostrs(char * in, char *** outarr);
extern char *collatestrings(int n, char ** instrs);
extern char *addquotes(char * array);
extern void freestrlist(STRPTR * string);


#endif /* _MISC_H */

