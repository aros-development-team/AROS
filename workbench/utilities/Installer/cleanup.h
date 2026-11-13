/*
    Copyright (C) 1995-2003, The AROS Development Team. All rights reserved.
*/

/* cleanup.h -- Prototypes for all functions used before exiting program */

#ifndef _CLEANUP_H
#define _CLEANUP_H

extern void cleanup();
extern void end_alloc();
extern void outofmem(void *);

#endif /* _CLEANUP_H */

