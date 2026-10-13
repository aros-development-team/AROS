/*
 * sntp.c - portable SNTP (RFC 4330) packet handling and timestamp arithmetic.
 *
 * Derived from the SNTP helpers of AmiAuth (src/core/clock.c):
 *
 *   Copyright (c) 2026 Simon Dick
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * AROS changes (Copyright (C) 2026, The AROS Development Team, same terms):
 * 64-bit fixed-point timestamps with fractions, the full header decoded,
 * kiss code and leap-indicator checks, delay computation, formatting.
 */

#include <string.h>
#include <stdio.h>

#include "sntp.h"

static uint32_t rd_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

static void wr_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8); p[3] = (uint8_t)(v);
}

static sntp_ts rd_ts(const uint8_t *p)
{
    return SNTP_TS_MAKE(rd_be32(p), rd_be32(p + 4));
}

sntp_ts sntp_ts_from_usec(uint32_t secs, uint32_t usec)
{
    /* fraction = usec * 2^32 / 10^6, exact in 64 bits */
    uint32_t frac = (uint32_t)((((uint64_t)usec) << 32) / 1000000UL);

    return SNTP_TS_MAKE(secs, frac);
}

uint32_t sntp_ts_usec(sntp_ts ts)
{
    return (uint32_t)((((uint64_t)SNTP_TS_FRAC(ts)) * 1000000UL) >> 32);
}

void sntp_build_request(uint8_t pkt[SNTP_PACKET_SIZE], sntp_ts transmit)
{
    memset(pkt, 0, SNTP_PACKET_SIZE);
    pkt[0] = (0 << 6) | (4 << 3) | 3;      /* LI 0, version 4, mode client */
    wr_be32(pkt + 40, SNTP_TS_SECS(transmit));
    wr_be32(pkt + 44, SNTP_TS_FRAC(transmit));
}

int sntp_parse_reply(const uint8_t *pkt, size_t len, sntp_ts sent,
                     struct sntp_reply *r)
{
    memset(r, 0, sizeof(*r));

    /* a reply may carry extension fields or a MAC after the 48 bytes */
    if (len < SNTP_PACKET_SIZE)
        return SNTP_E_LENGTH;

    r->li        = (pkt[0] >> 6) & 3;
    r->version   = (pkt[0] >> 3) & 7;
    r->mode      = pkt[0] & 7;
    r->stratum   = pkt[1];
    r->poll      = pkt[2];
    r->precision = (int8_t)pkt[3];
    r->refid     = rd_be32(pkt + 12);
    r->reference = rd_ts(pkt + 16);
    r->originate = rd_ts(pkt + 24);
    r->receive   = rd_ts(pkt + 32);
    r->transmit  = rd_ts(pkt + 40);

    if (r->mode != 4)
        return SNTP_E_MODE;
    if (r->version < 1 || r->version > 4)
        return SNTP_E_VERSION;

    /* Whatever else it says, a packet that does not echo our transmit
     * timestamp is not the answer to our question. */
    if (r->originate != sent)
        return SNTP_E_ORIGIN;

    if (r->stratum == 0) {
        /* kiss-o'-death: the reference identifier is a 4-character code
         * such as RATE or DENY */
        int i;

        for (i = 0; i < 4; i++) {
            uint8_t ch = pkt[12 + i];
            r->kod[i] = (ch >= 32 && ch < 127) ? (char)ch : '?';
        }
        r->kod[4] = '\0';
        return SNTP_E_KOD;
    }
    if (r->stratum > 15)
        return SNTP_E_STRATUM;
    if (r->li == 3)
        return SNTP_E_UNSYNC;
    if (r->transmit == 0)
        return SNTP_E_NOTIME;

    return SNTP_OK;
}

int64_t sntp_offset(sntp_ts t1, sntp_ts t2, sntp_ts t3, sntp_ts t4)
{
    int64_t a = (int64_t)(t2 - t1);
    int64_t b = (int64_t)(t3 - t4);

    /* halve each term first so the sum cannot overflow */
    return a / 2 + b / 2 + ((a % 2) + (b % 2)) / 2;
}

int64_t sntp_delay(sntp_ts t1, sntp_ts t2, sntp_ts t3, sntp_ts t4)
{
    return (int64_t)(t4 - t1) - (int64_t)(t3 - t2);
}

sntp_ts sntp_ts_adjust(sntp_ts ts, int64_t offset)
{
    return ts + (uint64_t)offset;
}

void sntp_format_interval(int64_t fixed, char *buf, size_t buflen)
{
    const char *sign = "";
    uint64_t mag;
    unsigned long secs, usec;

    if (fixed < 0) {
        sign = "-";
        mag = (uint64_t)(-(fixed + 1)) + 1;     /* no overflow at INT64_MIN */
    } else
        mag = (uint64_t)fixed;

    secs = (unsigned long)(mag >> 32);
    usec = (unsigned long)sntp_ts_usec(mag);
    snprintf(buf, buflen, "%s%lu.%06lu", sign, secs, usec);
}

const char *sntp_strerror(int err)
{
    switch (err) {
    case SNTP_OK:        return "ok";
    case SNTP_E_LENGTH:  return "reply too short";
    case SNTP_E_MODE:    return "not a server reply";
    case SNTP_E_VERSION: return "unsupported NTP version";
    case SNTP_E_KOD:     return "server refused (kiss-o'-death)";
    case SNTP_E_STRATUM: return "stratum out of range";
    case SNTP_E_UNSYNC:  return "server clock not synchronised";
    case SNTP_E_NOTIME:  return "reply carries no time";
    case SNTP_E_ORIGIN:  return "reply does not match our request";
    }
    return "unknown error";
}
