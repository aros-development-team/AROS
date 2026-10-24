/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#ifndef TEST_RESOLVER_NATIVE_H
#define TEST_RESOLVER_NATIVE_H
#include <stddef.h>
#include <stdint.h>
/* Deliberately model AROS LP64, never the host's netdb or socket headers. */
#define AF_INET 2
#define AF_INET6 28
#define EAI_ADDRFAMILY 1
#define EAI_AGAIN 2
#define EAI_BADFLAGS 3
#define EAI_FAIL 4
#define EAI_FAMILY 5
#define EAI_MEMORY 6
#define EAI_NODATA 7
#define EAI_NONAME 8
#define EAI_SERVICE 9
#define EAI_SOCKTYPE 10
#define EAI_SYSTEM 11
struct sockaddr { uint8_t sa_len, sa_family; char sa_data[14]; };
struct sockaddr_in { uint8_t sin_len, sin_family; uint16_t sin_port; uint32_t sin_addr; char sin_zero[8]; };
struct sockaddr_in6 { uint8_t sin6_len, sin6_family; uint16_t sin6_port; uint32_t sin6_flowinfo; uint8_t sin6_addr[16]; uint32_t sin6_scope_id; };
struct addrinfo {
    long ai_flags, ai_family, ai_socktype, ai_protocol;
    size_t ai_addrlen;
    char *ai_canonname;
    struct sockaddr *ai_addr;
    struct addrinfo *ai_next;
};
_Static_assert(sizeof(long) == 8, "test requires LP64");
_Static_assert(sizeof(struct addrinfo) == 64, "native addrinfo size");
_Static_assert(offsetof(struct addrinfo, ai_addrlen) == 32, "native length offset");
_Static_assert(sizeof(struct sockaddr_in) == 16, "native IPv4");
_Static_assert(sizeof(struct sockaddr_in6) == 28, "native IPv6");
void *aros_dns_native_open(void);
void aros_dns_native_close(void *base);
int aros_dns_native_lookup(void *, const char *, const char *, const struct addrinfo *, struct addrinfo **);
void aros_dns_native_free(void *, struct addrinfo *);
#endif
