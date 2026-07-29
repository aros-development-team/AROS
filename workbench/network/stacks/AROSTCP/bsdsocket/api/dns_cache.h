/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/*
 * dns_cache.h -- a small, TTL-aware cache of raw resolver responses.
 *
 * The cache stores the unmodified DNS response packet bytes keyed by the
 * (lowercased) query name and query type (T_A / T_AAAA / T_PTR). A hit hands
 * the stored bytes straight back to the caller, which replays them through
 * the very same getanswer() parser that a fresh response would take -- so no
 * second, divergent serialisation is needed.
 *
 * The cache is shared between all resolver callers (each of which runs in its
 * own task) and is guarded internally by its own SignalSemaphore.
 */

#ifndef API_DNS_CACHE_H
#define API_DNS_CACHE_H

#include <exec/types.h>

/*
 * Global enable flag. Defaults to TRUE (caching on). Setting it to FALSE
 * makes every lookup a miss and every insert a no-op, so the resolver
 * degrades to its uncached behaviour.
 */
extern BOOL dns_cache_enabled;

/*
 * Initialise the DNS response cache (semaphore + empty slot table). Safe to
 * call more than once and safe to call concurrently; the first caller wins.
 * The lookup/insert routines self-initialise, so an explicit call is optional.
 */
void dns_cache_init(void);

/*
 * Look up a cached raw DNS response for (name, qtype).
 *
 * On a hit the stored response bytes are copied into 'buf' (which must be at
 * least the stored length) and the response length is returned (> 0). On a
 * miss -- unknown key, expired entry, disabled cache, or a buffer too small --
 * zero is returned and 'buf' is left untouched.
 */
int dns_cache_lookup(const char *name, int qtype, UBYTE *buf, int buflen);

/*
 * Insert a raw DNS response for (name, qtype) into the cache. 'resp'/'resplen'
 * are the bytes as returned by res_search()/res_query(). The response is
 * copied, so the caller keeps ownership of its buffer. The entry expiry is
 * derived from the minimum resource-record TTL in the response.
 *
 * This is best effort: it silently does nothing when the cache is disabled,
 * on a bad argument, on an allocation failure, or when the response carries
 * no usable (non-zero) TTL.
 */
void dns_cache_insert(const char *name, int qtype,
                      const UBYTE *resp, int resplen);

#endif /* API_DNS_CACHE_H */
