/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Random bytes for the stack: the keys of the TCP initial sequence numbers
 * (RFC 6528), the ephemeral ports (RFC 6056), the IPv4 and IPv6 fragment
 * identifications (RFC 7739), the IIDs of temporary addresses (RFC 8981).
 *
 * They come from the C runtime's arc4random_buf(), which draws on
 * entropy.resource (and has its own fallback where there is none). Each
 * call reaches GetEntropy(), which gathers and reseeds - too much for every
 * packet - so the bytes are drawn POOL at a time into a pool, handed out
 * from it and wiped.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/synch.h>

#include <stdlib.h>

#include <kern/amiga_includes.h>
#include <protos/kern/kern_random_protos.h>

#define POOL    256

static UBYTE pool[POOL];
static int left;                        /* bytes not handed out, at the end of pool */

void
kern_random(void *buf, size_t len)
{
    UBYTE *p = buf;
    spl_t s = splnet();

    while(len > 0) {
        size_t n;
        UBYTE *from;

        if(left == 0) {
            arc4random_buf(pool, POOL);
            left = POOL;
        }
        n = len < (size_t)left ? len : (size_t)left;
        from = pool + POOL - left;
        bcopy(from, p, n);
        bzero(from, n);
        left -= n;
        p += n;
        len -= n;
    }
    splx(s);
}

u_int32_t
kern_random32(void)
{
    u_int32_t v;

    kern_random(&v, sizeof(v));
    return v;
}
