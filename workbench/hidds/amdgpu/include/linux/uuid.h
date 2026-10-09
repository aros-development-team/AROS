/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_UUID_H_
#define _LINUX_UUID_H_
#include <linux/types.h>
#include <linux/string.h>
#include <linux/random.h>
#define UUID_SIZE 16
typedef struct { __u8 b[UUID_SIZE]; } guid_t;
typedef struct { __u8 b[UUID_SIZE]; } uuid_t;
#define GUID_INIT(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7) \
    ((guid_t){ { (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff, ((a) >> 24) & 0xff, (b) & 0xff, ((b) >> 8) & 0xff, (c) & 0xff, ((c) >> 8) & 0xff, (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) } })
#define UUID_INIT(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7) \
    ((uuid_t){ { ((a) >> 24) & 0xff, ((a) >> 16) & 0xff, ((a) >> 8) & 0xff, (a) & 0xff, ((b) >> 8) & 0xff, (b) & 0xff, ((c) >> 8) & 0xff, (c) & 0xff, (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) } })
static inline bool guid_equal(const guid_t *u1, const guid_t *u2) { return memcmp(u1, u2, sizeof(guid_t)) == 0; }
static inline void guid_copy(guid_t *dst, const guid_t *src) { memcpy(dst, src, sizeof(guid_t)); }
static inline void import_guid(guid_t *dst, const __u8 *src) { memcpy(dst, src, sizeof(guid_t)); }
static inline void export_guid(__u8 *dst, const guid_t *src) { memcpy(dst, src, sizeof(guid_t)); }
static inline bool guid_is_null(const guid_t *guid) { static const guid_t null_guid; return guid_equal(guid, &null_guid); }
static inline void guid_gen(guid_t *u) { int i; for (i = 0; i < UUID_SIZE; i++) u->b[i] = get_random_u32(); u->b[7] = (u->b[7] & 0x0f) | 0x40; u->b[8] = (u->b[8] & 0x3f) | 0x80; }
static inline bool uuid_equal(const uuid_t *u1, const uuid_t *u2) { return memcmp(u1, u2, sizeof(uuid_t)) == 0; }
static inline void uuid_copy(uuid_t *dst, const uuid_t *src) { memcpy(dst, src, sizeof(uuid_t)); }
#define guid_null ((guid_t){ { 0 } })
#endif
