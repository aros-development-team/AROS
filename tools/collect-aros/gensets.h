/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _GENSETS_H_
#define _GENSETS_H_

#include <stdio.h>

typedef struct setnode
{
    char *secname;
    int   off_setname;
    unsigned long pri;
    struct setnode *next;
} setnode;

int gensets(FILE *in, FILE *out);
void parse_secname(const char *secname, setnode **setlist_ptr);
int emit_sets(setnode *setlist, FILE *out);

#endif /* !_GENSETS_H_ */
