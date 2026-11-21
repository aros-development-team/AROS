/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    kaffeine-jit68 engine: the private interface between the operations of
    interface version 1 (kj68_engine.c) and the translator
    (kj68_translate.c). Nothing here is part of the external interface.
*/

#ifndef KJ68_INTERN_H
#define KJ68_INTERN_H

#include <stddef.h>
#include <stdint.h>
#include "kaffeine-jit68-abi-v1.h"

/* The translator's state of a domain (code memory, glue, blocks) and of a
   CPU (registers, page flags, fast table); kj68_translate.c defines them. */
struct HJBackend;
struct HJCpu;

/* The inline access check works on 4 KiB pages of guest memory. */
#define KJ68_PAGE_SHIFT      12
#define KJ68_PAGE_READ_HOST  0x01u   /* a READ_HOST range touches the page */
#define KJ68_PAGE_WRITE_HOST 0x02u   /* a WRITE_HOST range touches the page */

/* The translator's largest block, which the engine declares as
   max_block_insns. */
#define KJ68_BLOCK_INSNS 64

/* Cumulative counts, 64 bits, saturating; get_stats saturates them again
   to the widths of struct KJ68Stats. */
struct KJ68Counters
{
    uint64_t blocks, insns, runs, invalidations, code_resets, retired;
};

struct KJ68Domain
{
    const struct KJ68HostServices *host;
    APTR host_domain;
    struct KJ68MemoryMap *map;
    /* The map as last checked. base and size are those of domain_create
       for the whole life of the domain. */
    ULONG map_generation;
    UBYTE *base;
    ULONG size;
    ULONG num_ranges;
    struct KJ68Range ranges[KJ68_MAX_RANGES];
    UBYTE *pages;                 /* KJ68_PAGE_* of each 4 KiB page */
    ULONG npages;
    ULONG pages_generation;       /* changes whenever pages change */
    ULONG cpus;                   /* live CPU objects */
    int busy;                     /* inside run() */
    struct KJ68Counters counters;
    struct HJBackend *code;
};

struct KJ68CPU
{
    struct KJ68Domain *domain;
    APTR host_cpu;
    struct HJCpu *tr;
};

#ifdef __cplusplus
extern "C" {
#endif

/* The engine's table for an ABI major, or NULL: what the library function
   KJ68_GetEngine (LVO -30) returns on AROS; host builds call it directly. */
const struct KJ68EngineOps *KJ68_GetEngineTable(ULONG abi_major);

/* kj68_engine.c */
void *kj_alloc(size_t size);            /* zeroed; NULL on failure */
void kj_free(void *p, size_t size);     /* NULL is ignored */
void kj_count(uint64_t *counter, uint64_t n);

/* kj68_translate.c: the translator as the engine uses it */
int kj_tr_runs(void);
const char *kj_tr_exec_memory_kind(void);
int kj_tr_encoder_self_test(char *message, unsigned long size);
LONG kj_tr_domain_create(struct KJ68Domain *dom);   /* KJ68_OK, ERR_NOHOST, ERR_NOMEM */
void kj_tr_domain_destroy(struct KJ68Domain *dom);
ULONG kj_tr_code_bytes(const struct KJ68Domain *dom);
LONG kj_tr_cpu_create(struct KJ68CPU *cpu);          /* KJ68_OK, ERR_NOMEM */
void kj_tr_cpu_destroy(struct KJ68CPU *cpu);
ULONG kj_tr_get_reg(const struct KJ68CPU *cpu, ULONG reg);
LONG kj_tr_set_reg(struct KJ68CPU *cpu, ULONG reg, ULONG value);
void kj_tr_invalidate(struct KJ68Domain *dom);
/* After a new map generation is taken: drops every translation of the
   domain, whatever its guest bytes. */
void kj_tr_map_changed(struct KJ68Domain *dom);
/* Writes every version 1 member of exit but size_of, which the caller has
   zeroed; the map and the pages are current. */
void kj_tr_run(struct KJ68CPU *cpu, ULONG budget, struct KJ68Exit *exit);

/* The translator's own functions, kept from kaffeine_jit.c */
int HJ_Available(void);
const char *HJ_ExecMemoryKind(void);
int HJ_EncoderSelfTest(char *message, unsigned long size);

#ifdef KJ68_TEST_SEAMS
/* Host test build only: the n-th allocation from now (kj_alloc or the
   executable memory) fails, 0 for none; live counts; a counter preset. */
extern long kj68_test_fail_alloc_at;
extern long kj68_test_allocs_live;
extern long kj68_test_exec_live;
int kj_test_fail_next(void);
void kj68_test_preset_counters(struct KJ68Domain *dom, uint64_t counts, uint64_t retired);
#endif

#ifdef __cplusplus
}
#endif

#endif /* KJ68_INTERN_H */
