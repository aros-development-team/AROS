/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#define __BSDSOCKET_NOLIBBASE__
#include <exec/libraries.h>
#include "resolver_native.h"
#include <proto/exec.h>
#include <aros/libcall.h>
#include <errno.h>

void *aros_dns_native_open(void)
{
    struct Library *base = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 4);
    /* Resolver vectors are Roadshow extensions. Refuse older bases before
       reading a function vector outside their negative table. */
    if (base && base->lib_NegSize < 137 * LIB_VECTSIZE) {
        CloseLibrary(base);
        base = NULL;
    }
    return base;
}

void aros_dns_native_close(void *base)
{
    CloseLibrary((struct Library *)base);
}

int aros_dns_native_lookup(void *base, const char *node, const char *service,
                           const struct addrinfo *hints, struct addrinfo **result)
{
    int error = (int)AROS_LC4(LONG, RS_getaddrinfo,
        AROS_LCA(const char *, node, A0),
        AROS_LCA(const char *, service, A1),
        AROS_LCA(const struct addrinfo *, hints, A2),
        AROS_LCA(struct addrinfo **, result, A3),
        struct Library *, (struct Library *)base, 136, BSDSocket);
    if (error == EAI_SYSTEM)
        errno = (int)AROS_LC0(LONG, Errno, struct Library *, (struct Library *)base, 27, BSDSocket);
    return error;
}

void aros_dns_native_free(void *base, struct addrinfo *result)
{
    AROS_LC1NR(void, RS_freeaddrinfo,
        AROS_LCA(struct addrinfo *, result, A0),
        struct Library *, (struct Library *)base, 135, BSDSocket);
}
