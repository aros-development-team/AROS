/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#include <resolver_native.h>
#include "resolver.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void aros_rs_freeaddrinfo(struct aros_rs_addrinfo *result)
{
    while (result) {
        struct aros_rs_addrinfo *next = result->ai_next;
        free(result->ai_canonname);
        free(result->ai_addr);
        free(result);
        result = next;
    }
}

static int fits_i32(long value)
{
    return value >= INT32_MIN && value <= INT32_MAX;
}

int aros_rs_getaddrinfo(const char *node, const char *service,
                      const struct aros_rs_addrinfo *hints,
                      struct aros_rs_addrinfo **result)
{
    struct addrinfo native_hints = {0}, *native_result = NULL, *entry;
    struct aros_rs_addrinfo *head = NULL, **tail = &head;
    void *base;
    int error;

    if (!result) {
        errno = EINVAL;
        return EAI_SYSTEM;
    }
    *result = NULL;
    if (hints) {
        if (hints->ai_addrlen || hints->ai_canonname || hints->ai_addr || hints->ai_next)
            return EAI_BADFLAGS;
        native_hints.ai_flags = hints->ai_flags;
        native_hints.ai_family = hints->ai_family;
        native_hints.ai_socktype = hints->ai_socktype;
        native_hints.ai_protocol = hints->ai_protocol;
    }
    base = aros_dns_native_open();
    if (!base) {
        errno = ENETDOWN;
        return EAI_SYSTEM;
    }
    error = aros_dns_native_lookup(base, node, service,
                                   hints ? &native_hints : NULL, &native_result);
    /* The provider may write errno on EAI_SYSTEM. Freeing results and closing
       a library must not replace that operation's error. */
    int saved_errno = errno;
    if (error)
        goto done;
    for (entry = native_result; entry; entry = entry->ai_next) {
        size_t expected_len;
        if (entry->ai_family == AF_INET)
            expected_len = sizeof(struct sockaddr_in);
        else if (entry->ai_family == AF_INET6)
            expected_len = sizeof(struct sockaddr_in6);
        else {
            error = EAI_FAMILY;
            goto done;
        }
        if (!fits_i32(entry->ai_flags) || !fits_i32(entry->ai_family) ||
            !fits_i32(entry->ai_socktype) || !fits_i32(entry->ai_protocol) ||
            !entry->ai_addr || entry->ai_addrlen != expected_len ||
            entry->ai_addr->sa_len != expected_len ||
            entry->ai_addr->sa_family != entry->ai_family) {
            error = EAI_FAIL;
            goto done;
        }
        struct aros_rs_addrinfo *copy = calloc(1, sizeof(*copy));
        if (!copy) {
            error = EAI_MEMORY;
            goto done;
        }
        *tail = copy;
        tail = &copy->ai_next;
        copy->ai_flags = (int32_t)entry->ai_flags;
        copy->ai_family = (int32_t)entry->ai_family;
        copy->ai_socktype = (int32_t)entry->ai_socktype;
        copy->ai_protocol = (int32_t)entry->ai_protocol;
        copy->ai_addrlen = (uint32_t)entry->ai_addrlen;
        copy->ai_addr = malloc(expected_len);
        if (!copy->ai_addr) {
            error = EAI_MEMORY;
            goto done;
        }
        memcpy(copy->ai_addr, entry->ai_addr, expected_len);
        if (entry->ai_canonname) {
            size_t length = strlen(entry->ai_canonname);
            if (length == SIZE_MAX || !(copy->ai_canonname = malloc(length + 1))) {
                error = EAI_MEMORY;
                goto done;
            }
            memcpy(copy->ai_canonname, entry->ai_canonname, length + 1);
        }
    }
    if (!head)
        error = EAI_NONAME;
done:
    if (native_result)
        aros_dns_native_free(base, native_result);
    aros_dns_native_close(base);
    if (error) {
        aros_rs_freeaddrinfo(head);
        errno = saved_errno;
        return error;
    }
    *result = head;
    errno = saved_errno;
    return 0;
}

const char *aros_rs_gai_strerror(int error)
{
    /* Own immutable messages outlive every per-call library base, and can be
       read concurrently or after moving LookupHost to another Rust thread. */
    switch (error) {
    case 0: return "No error";
    case EAI_ADDRFAMILY: return "Address family unavailable for this host";
    case EAI_AGAIN: return "Temporary name resolution failure";
    case EAI_BADFLAGS: return "Invalid name resolution flags";
    case EAI_FAIL: return "Name resolution failure";
    case EAI_FAMILY: return "Unsupported address family";
    case EAI_MEMORY: return "Name resolution allocation failed";
    case EAI_NODATA: return "No address for this host";
    case EAI_NONAME: return "Host or service not found";
    case EAI_SERVICE: return "Service unavailable for this socket type";
    case EAI_SOCKTYPE: return "Unsupported socket type";
    case EAI_SYSTEM: return "Name resolution system error";
    default: return "Unknown name resolution error";
    }
}
