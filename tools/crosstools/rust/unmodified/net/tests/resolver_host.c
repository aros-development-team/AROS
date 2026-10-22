/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#include <resolver_native.h>
#include "resolver.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alloc_hooks.h"

_Static_assert(sizeof(struct aros_rs_addrinfo) == 48, "Rust bridge record");
_Static_assert(offsetof(struct aros_rs_addrinfo, ai_addrlen) == 16, "Rust length offset");
_Static_assert(offsetof(struct aros_rs_addrinfo, ai_addr) == 32, "Rust address offset");

struct fixture {
    struct addrinfo a[2];
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
    char canon[32];
    int opens, lookups, frees, closes, missing, lookup_error, empty;
    int allocations, fail_at, live, expect_hints;
    void *base;
};
static _Thread_local struct fixture f;
static int cases;
static _Thread_local int rendezvous_once;
static pthread_mutex_t gate_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_condition = PTHREAD_COND_INITIALIZER;
static int gate_count;

void *test_malloc(size_t size) {
    f.allocations++;
    if (f.fail_at && f.allocations == f.fail_at) { errno = ENOMEM; return NULL; }
    void *p = malloc(size);
    assert(p); f.live++; return p;
}
void *test_calloc(size_t n, size_t size) {
    void *p = test_malloc(n * size);
    if (p) memset(p, 0, n * size);
    return p;
}
void test_free(void *p) { if (p) { f.live--; assert(f.live >= 0); free(p); } }

static void reset(void) {
    assert(!f.live && !f.base);
    memset(&f, 0, sizeof(f));
    f.v4.sin_len = sizeof(f.v4); f.v4.sin_family = AF_INET;
    f.v4.sin_port = 0x3412; f.v4.sin_addr = 0x04030201;
    f.v6.sin6_len = sizeof(f.v6); f.v6.sin6_family = AF_INET6;
    f.v6.sin6_port = 0x7856; f.v6.sin6_flowinfo = 0x12345678;
    f.v6.sin6_scope_id = 17;
    for (int i = 0; i < 16; i++) f.v6.sin6_addr[i] = (uint8_t)(i + 1);
    strcpy(f.canon, "native.example");
    f.a[0] = (struct addrinfo){-1, AF_INET, 1, 6, sizeof(f.v4), f.canon, (void *)&f.v4, &f.a[1]};
    f.a[1] = (struct addrinfo){INT32_MAX, AF_INET6, 2, 17, sizeof(f.v6), NULL, (void *)&f.v6, NULL};
    errno = 0;
}
void *aros_dns_native_open(void) {
    f.opens++;
    if (f.missing) return NULL;
    assert(!f.base); f.base = malloc(1); assert(f.base); return f.base;
}
void aros_dns_native_close(void *base) {
    assert(base == f.base); assert(f.lookups == 1); assert(f.frees == !f.empty); f.closes++;
    free(base); f.base = NULL; errno = EBADF;
}
int aros_dns_native_lookup(void *base, const char *node, const char *service,
                         const struct addrinfo *h, struct addrinfo **result) {
    assert(base == f.base && base); assert(!strcmp(node, "host")); assert(!strcmp(service, "443"));
    f.lookups++;
    if (rendezvous_once) {
        rendezvous_once = 0;
        assert(!pthread_mutex_lock(&gate_mutex));
        gate_count++;
        if (gate_count == 8) assert(!pthread_cond_broadcast(&gate_condition));
        while (gate_count != 8) assert(!pthread_cond_wait(&gate_condition, &gate_mutex));
        assert(!pthread_mutex_unlock(&gate_mutex));
    }
    if (f.expect_hints) {
        assert(h && h->ai_flags == -17 && h->ai_family == AF_INET6);
        assert(h->ai_socktype == INT32_MAX && h->ai_protocol == INT32_MIN);
        assert(!h->ai_addrlen && !h->ai_addr && !h->ai_canonname && !h->ai_next);
    } else assert(!h);
    *result = f.empty ? NULL : f.a;
    errno = EACCES;
    return f.lookup_error;
}
void aros_dns_native_free(void *base, struct addrinfo *result) {
    assert(base == f.base && base); assert(result == f.a); f.frees++;
    /* Poison provider-owned storage: a shallow copy must never survive this. */
    memset(&f.v4, 0xa5, sizeof(f.v4)); memset(&f.v6, 0xa5, sizeof(f.v6));
    memset(f.canon, 0xa5, sizeof(f.canon)); errno = EIO;
}
static void pairing(void) {
    assert(f.opens == 1 && f.lookups == 1 && f.closes == 1 && !f.base);
    assert(f.frees == !f.empty);
    assert(errno == EACCES);
}
static void failure(int expected) {
    struct aros_rs_addrinfo *out = (void *)1;
    assert(aros_rs_getaddrinfo("host", "443", NULL, &out) == expected);
    assert(!out && !f.live); pairing(); cases++;
}
static void success(void) {
    struct sockaddr_in v4 = f.v4; struct sockaddr_in6 v6 = f.v6;
    struct aros_rs_addrinfo *out = NULL;
    assert(!aros_rs_getaddrinfo("host", "443", NULL, &out)); pairing();
    assert(out && out->ai_next && !out->ai_next->ai_next);
    assert(out->ai_flags == -1 && out->ai_family == AF_INET && out->ai_socktype == 1 && out->ai_protocol == 6);
    assert(out->ai_addrlen == sizeof(v4) && out->ai_addr != (void *)&f.v4);
    assert(!memcmp(out->ai_addr, &v4, sizeof(v4)));
    assert(out->ai_canonname != f.canon && !strcmp(out->ai_canonname, "native.example"));
    assert(out->ai_next->ai_family == AF_INET6 && out->ai_next->ai_flags == INT32_MAX);
    assert(out->ai_next->ai_socktype == 2 && out->ai_next->ai_protocol == 17);
    assert(out->ai_next->ai_addrlen == sizeof(v6) && !out->ai_next->ai_canonname);
    assert(!memcmp(out->ai_next->ai_addr, &v6, sizeof(v6)));
    assert(f.allocations == 5 && f.live == 5);
    aros_rs_freeaddrinfo(out); assert(!f.live); cases++;
}
static void *thread_success(void *unused) {
    (void)unused;
    rendezvous_once = 1;
    for (int i = 0; i < 50; i++) {
        reset(); struct aros_rs_addrinfo *out;
        assert(!aros_rs_getaddrinfo("host", "443", NULL, &out)); pairing();
        assert(!strcmp(out->ai_canonname, "native.example"));
        aros_rs_freeaddrinfo(out); assert(!f.live);
    }
    return NULL;
}
struct moved { struct aros_rs_addrinfo *out; int live; };
static void *free_moved(void *arg) {
    struct moved *m = arg; f.live = m->live;
    assert(!strcmp(m->out->ai_canonname, "native.example"));
    assert(((struct sockaddr_in6 *)m->out->ai_next->ai_addr)->sin6_scope_id == 17);
    aros_rs_freeaddrinfo(m->out); assert(!f.live); return NULL;
}
int main(void) {
    reset(); success();
    for (int cut = 1; cut <= 5; cut++) { reset(); f.fail_at = cut; failure(EAI_MEMORY); }
    reset(); f.lookup_error = EAI_SYSTEM; failure(EAI_SYSTEM);
    reset(); f.lookup_error = EAI_AGAIN; failure(EAI_AGAIN);
    reset(); f.empty = 1; failure(EAI_NONAME);
    reset(); f.empty = 1; f.lookup_error = EAI_NONAME; failure(EAI_NONAME);
    reset(); f.missing = 1; struct aros_rs_addrinfo *out = (void *)1;
    assert(aros_rs_getaddrinfo("host", "443", NULL, &out) == EAI_SYSTEM);
    assert(!out && errno == ENETDOWN && f.opens == 1 && !f.lookups && !f.closes); cases++;
    reset(); assert(aros_rs_getaddrinfo("host", "443", NULL, NULL) == EAI_SYSTEM);
    assert(errno == EINVAL && !f.opens); cases++;
    for (int field = 0; field < 4; field++) {
        for (int sign = 0; sign < 2; sign++) {
            reset(); long *slots[] = {&f.a[1].ai_flags, &f.a[1].ai_family, &f.a[1].ai_socktype, &f.a[1].ai_protocol};
            *slots[field] = sign ? (long)INT32_MIN - 1 : (long)INT32_MAX + 1;
            failure(field == 1 ? EAI_FAMILY : EAI_FAIL);
        }
    }
    reset(); f.a[1].ai_addr = NULL; failure(EAI_FAIL);
    reset(); f.a[1].ai_addrlen = SIZE_MAX; failure(EAI_FAIL);
    reset(); f.a[1].ai_addrlen--; failure(EAI_FAIL);
    reset(); f.v6.sin6_family = AF_INET; failure(EAI_FAIL);
    reset(); f.v6.sin6_len--; failure(EAI_FAIL);
    for (int ancillary = 0; ancillary < 4; ancillary++) {
        reset(); struct aros_rs_addrinfo h = {0};
        if (ancillary == 0) h.ai_addrlen = 1;
        if (ancillary == 1) h.ai_canonname = (void *)1;
        if (ancillary == 2) h.ai_addr = (void *)1;
        if (ancillary == 3) h.ai_next = (void *)1;
        out = (void *)1;
        assert(aros_rs_getaddrinfo("host", "443", &h, &out) == EAI_BADFLAGS);
        assert(!out && !f.opens); cases++;
    }
    reset(); f.expect_hints = 1;
    struct aros_rs_addrinfo h = {-17, AF_INET6, INT32_MAX, INT32_MIN, 0, NULL, NULL, NULL};
    assert(!aros_rs_getaddrinfo("host", "443", &h, &out)); pairing();
    aros_rs_freeaddrinfo(out); assert(!f.live); cases++;
    reset(); struct moved m;
    assert(!aros_rs_getaddrinfo("host", "443", NULL, &m.out)); pairing(); m.live = f.live; f.live = 0;
    pthread_t mover; assert(!pthread_create(&mover, NULL, free_moved, &m)); assert(!pthread_join(mover, NULL)); cases++;
    pthread_t workers[8];
    for (int i = 0; i < 8; i++) assert(!pthread_create(&workers[i], NULL, thread_success, NULL));
    for (int i = 0; i < 8; i++) assert(!pthread_join(workers[i], NULL));
    assert(gate_count == 8);
    const char *message = aros_rs_gai_strerror(EAI_SYSTEM);
    reset(); success(); assert(!strcmp(message, "Name resolution system error"));
    for (int e = 0; e <= EAI_SYSTEM; e++) assert(aros_rs_gai_strerror(e) && *aros_rs_gai_strerror(e));
    assert(!strcmp(aros_rs_gai_strerror(-999), "Unknown name resolution error"));
    aros_rs_freeaddrinfo(NULL);
    printf("PASS %d cases, 400 concurrent lookups, cross-thread disposal, all 5 allocation cutpoints\n", cases);
    return 0;
}
