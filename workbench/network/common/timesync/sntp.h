/*
 * sntp.h - portable SNTP (RFC 4330) packet handling and timestamp arithmetic.
 *
 * Derived from the SNTP helpers of AmiAuth (src/core/clock.c),
 * Copyright (c) 2026 Simon Dick, BSD 2-Clause License (see sntp.c for the
 * full notice).  Extended for the AROS TimeSync service with 64-bit 32.32
 * fixed-point timestamps, so the offset keeps its sub-second part.
 *
 * Nothing in here touches the system: no I/O, no OS calls.  It can be
 * compiled and tested on any host.
 */

#ifndef TIMESYNC_SNTP_H
#define TIMESYNC_SNTP_H

#include <stddef.h>
#include <stdint.h>

#define SNTP_PORT           123
#define SNTP_PACKET_SIZE    48

/* Seconds from the NTP epoch (1900-01-01) to the Unix (1970) and Amiga
 * (1978) epochs. */
#define SNTP_UNIX_DELTA     2208988800UL
#define SNTP_AMIGA_DELTA    2461449600UL

/* An NTP timestamp: 32 bits of seconds since 1900 and 32 bits of fraction,
 * held in one unsigned 64-bit value.  Differences between two timestamps
 * are taken modulo 2^64 and read as signed, so they stay right across the
 * 2036 era rollover as long as the two are less than 68 years apart. */
typedef uint64_t sntp_ts;

#define SNTP_TS_SECS(ts)    ((uint32_t)((ts) >> 32))
#define SNTP_TS_FRAC(ts)    ((uint32_t)(ts))
#define SNTP_TS_MAKE(s, f)  ((((sntp_ts)(uint32_t)(s)) << 32) | (uint32_t)(f))

/* Build a timestamp from whole seconds (NTP epoch) and microseconds, and
 * read the microseconds back out of one. */
sntp_ts  sntp_ts_from_usec(uint32_t secs, uint32_t usec);
uint32_t sntp_ts_usec(sntp_ts ts);

/* What a server told us, as parsed by sntp_parse_reply(). */
struct sntp_reply {
    uint8_t  li;            /* leap indicator (3 = clock not synchronised) */
    uint8_t  version;
    uint8_t  mode;          /* 4 = server */
    uint8_t  stratum;       /* 0 = kiss-o'-death, 1..15 usable */
    uint8_t  poll;
    int8_t   precision;
    uint32_t refid;
    char     kod[5];        /* the kiss code (refid as text) when stratum is 0 */
    sntp_ts  reference;
    sntp_ts  originate;     /* our transmit timestamp echoed back (T1) */
    sntp_ts  receive;       /* when the server got the request (T2) */
    sntp_ts  transmit;      /* when the server sent the reply (T3) */
};

/* sntp_parse_reply() results */
enum {
    SNTP_OK = 0,
    SNTP_E_LENGTH,          /* shorter than a packet */
    SNTP_E_MODE,            /* not a server reply */
    SNTP_E_VERSION,         /* NTP version we do not speak */
    SNTP_E_KOD,             /* kiss-o'-death: the server refuses us */
    SNTP_E_STRATUM,         /* stratum out of range */
    SNTP_E_UNSYNC,          /* server's own clock is not synchronised */
    SNTP_E_NOTIME,          /* zero transmit timestamp */
    SNTP_E_ORIGIN           /* does not echo our request: not a reply to us */
};

/* Fill a client request.  `transmit` goes into the transmit-timestamp
 * field and comes back as the originate timestamp; its fraction is the
 * caller's place for a random nonce (sntp_parse_reply() insists on an exact
 * echo, which a blind forger cannot produce). */
void sntp_build_request(uint8_t pkt[SNTP_PACKET_SIZE], sntp_ts transmit);

/* Parse `len` bytes of a reply.  `sent` is the transmit timestamp of the
 * request it must answer.  Returns SNTP_OK and fills *r, or one of the
 * SNTP_E_* codes (r is filled as far as parsing got, so the kiss code and
 * stratum can still be reported). */
int sntp_parse_reply(const uint8_t *pkt, size_t len, sntp_ts sent,
                     struct sntp_reply *r);

/* RFC 4330 section 5: the clock offset  ((T2 - T1) + (T3 - T4)) / 2  and
 * the round-trip delay  (T4 - T1) - (T3 - T2), both as signed 32.32 fixed
 * point seconds.  T1/T4 are the client's send/receive times, T2/T3 the
 * server's receive/transmit times. */
int64_t sntp_offset(sntp_ts t1, sntp_ts t2, sntp_ts t3, sntp_ts t4);
int64_t sntp_delay(sntp_ts t1, sntp_ts t2, sntp_ts t3, sntp_ts t4);

/* Add a signed 32.32 offset to a timestamp. */
sntp_ts sntp_ts_adjust(sntp_ts ts, int64_t offset);

/* Render a signed 32.32 value as "[-]seconds.microseconds" into buf
 * (at least 24 bytes). */
void sntp_format_interval(int64_t fixed, char *buf, size_t buflen);

const char *sntp_strerror(int err);

#endif /* TIMESYNC_SNTP_H */
