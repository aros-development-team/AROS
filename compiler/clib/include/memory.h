#ifndef _MEMORY_H
#define _MEMORY_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI-C header file memory.h
    Lang: english
*/
#ifndef _SYS_TYPES_H
#   include <sys/types.h>
#endif

void * malloc (size_t bytes);
void free (void * memory);

#endif /* _MEMORY_H */
