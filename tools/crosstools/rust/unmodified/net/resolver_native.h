/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#ifndef AROS_RUST_RESOLVER_NATIVE_H
#define AROS_RUST_RESOLVER_NATIVE_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void *aros_dns_native_open(void);
void aros_dns_native_close(void *base);
int aros_dns_native_lookup(void *base, const char *node, const char *service,
                           const struct addrinfo *hints, struct addrinfo **result);
void aros_dns_native_free(void *base, struct addrinfo *result);

#endif
