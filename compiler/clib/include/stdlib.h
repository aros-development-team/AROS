#ifndef _STDLIB_H
#define _STDLIB_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI-C header file stdlib.h
    Lang: english
*/
#ifndef _SYS_TYPES_H
#   include <sys/types.h>
#endif

void qsort(void *array, size_t count, size_t elementsize,
	int (*comparefunction)(const void * element1, const void * element2));

#endif /* _STDLIB_H */
