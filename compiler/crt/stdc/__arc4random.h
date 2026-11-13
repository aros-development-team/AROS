/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Private prototype for the shared arc4random() core.
*/

#ifndef __ARC4RANDOM_H
#define __ARC4RANDOM_H

#include <stddef.h>

struct StdCIntBase;

/* Fill buf with n cryptographically-strong bytes (from entropy.resource when
   available, otherwise the local fallback). */
void __arc4random_buf(struct StdCIntBase *StdCBase, void *buf, size_t n);

#endif /* __ARC4RANDOM_H */
