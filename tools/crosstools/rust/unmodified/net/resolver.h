/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#ifndef AROS_RUST_RESOLVER_H
#define AROS_RUST_RESOLVER_H

#include <stdint.h>

struct sockaddr;

/* Rust libc's record is deliberately distinct from native netdb.h, whose
   first four fields are C long and whose address length is size_t. */
struct aros_rs_addrinfo {
    int32_t ai_flags;
    int32_t ai_family;
    int32_t ai_socktype;
    int32_t ai_protocol;
    uint32_t ai_addrlen;
    char *ai_canonname;
    struct sockaddr *ai_addr;
    struct aros_rs_addrinfo *ai_next;
};

int aros_rs_getaddrinfo(const char *node, const char *service,
                      const struct aros_rs_addrinfo *hints,
                      struct aros_rs_addrinfo **result);
void aros_rs_freeaddrinfo(struct aros_rs_addrinfo *result);
const char *aros_rs_gai_strerror(int error);

#endif
