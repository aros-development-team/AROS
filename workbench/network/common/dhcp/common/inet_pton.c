/*
 * Copyright (c) 1996 by Internet Software Consortium.
 * Copyright (c) 2005 by Pavel Fedin.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * AROS note: Standalone inet_pton / inet_ntop that do NOT go through the
 * Miami library jump table.  The Miami library's inet_pton was originally
 * written for IPv4 only and does not recognise AF_INET6, so we provide our
 * own implementation here (named aros_inet_pton/aros_inet_ntop to avoid
 * conflicting with Miami's prototype declarations) for the DHCP client.
 */

#ifdef __AROS__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <exec/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

/* Undefine any system macros so our functions are callable by name */
#ifdef inet_pton
#undef inet_pton
#endif
#ifdef inet_ntop
#undef inet_ntop
#endif
/* ------------------------------------------------------------------ */
/* aros_inet_pton                                                           */
/* ------------------------------------------------------------------ */

static int aros_inet_pton4(const char *src, unsigned char *dst);
static int aros_inet_pton6(const char *src, unsigned char *dst);

int
aros_inet_pton(int af, const char * __restrict src, void * __restrict dst)
{
    switch (af) {
    case AF_INET:
        return aros_inet_pton4(src, (unsigned char *)dst);
    case AF_INET6:
        return aros_inet_pton6(src, (unsigned char *)dst);
    default:
        errno = EAFNOSUPPORT;
        return -1;
    }
}

static int
aros_inet_pton4(const char *src, unsigned char *dst)
{
    static const char digits[] = "0123456789";
    int saw_digit, octets, ch;
    unsigned char tmp[NS_INADDRSZ], *tp;

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0') {
        const char *pch;

        if ((pch = strchr(digits, ch)) != NULL) {
            unsigned int nw = *tp * 10 + (pch - digits);
            if (saw_digit && *tp == 0)
                return 0;
            if (nw > 255)
                return 0;
            *tp = nw;
            if (!saw_digit) {
                if (++octets > 4)
                    return 0;
                saw_digit = 1;
            }
        } else if (ch == '.' && saw_digit) {
            if (octets == 4)
                return 0;
            *++tp = 0;
            saw_digit = 0;
        } else
            return 0;
    }
    if (octets < 4)
        return 0;
    memcpy(dst, tmp, NS_INADDRSZ);
    return 1;
}

static int
aros_inet_pton6(const char *src, unsigned char *dst)
{
    static const char xdigits_l[] = "0123456789abcdef",
                      xdigits_u[] = "0123456789ABCDEF";
    unsigned char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
    const char *xdigits, *curtok;
    int ch, saw_xdigit;
    unsigned int val;

    memset((tp = tmp), '\0', NS_IN6ADDRSZ);
    endp = tp + NS_IN6ADDRSZ;
    colonp = NULL;
    if (*src == ':')
        if (*++src != ':')
            return 0;
    curtok = src;
    saw_xdigit = 0;
    val = 0;
    while ((ch = *src++) != '\0') {
        const char *pch;

        if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
            pch = strchr((xdigits = xdigits_u), ch);
        if (pch != NULL) {
            val <<= 4;
            val |= (pch - xdigits);
            if (val > 0xffff)
                return 0;
            saw_xdigit = 1;
            continue;
        }
        if (ch == ':') {
            curtok = src;
            if (!saw_xdigit) {
                if (colonp)
                    return 0;
                colonp = tp;
                continue;
            }
            if (tp + NS_INT16SZ > endp)
                return 0;
            *tp++ = (unsigned char)(val >> 8) & 0xff;
            *tp++ = (unsigned char) val & 0xff;
            saw_xdigit = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
            aros_inet_pton4(curtok, tp) > 0) {
            tp += NS_INADDRSZ;
            saw_xdigit = 0;
            break;
        }
        return 0;
    }
    if (saw_xdigit) {
        if (tp + NS_INT16SZ > endp)
            return 0;
        *tp++ = (unsigned char)(val >> 8) & 0xff;
        *tp++ = (unsigned char) val & 0xff;
    }
    if (colonp != NULL) {
        const int n = tp - colonp;
        int i;
        for (i = 1; i <= n; i++) {
            endp[-i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return 0;
    memcpy(dst, tmp, NS_IN6ADDRSZ);
    return 1;
}

/* ------------------------------------------------------------------ */
/* aros_inet_ntop                                                           */
/* ------------------------------------------------------------------ */

static const char *aros_inet_ntop4(const unsigned char *src, char *dst, LONG size);
static const char *aros_inet_ntop6(const unsigned char *src, char *dst, LONG size);

const char *
aros_inet_ntop(int af, const void * __restrict src, char * __restrict dst, LONG size)
{
    switch (af) {
    case AF_INET:
        return aros_inet_ntop4((const unsigned char *)src, dst, size);
    case AF_INET6:
        return aros_inet_ntop6((const unsigned char *)src, dst, size);
    default:
        errno = EAFNOSUPPORT;
        return NULL;
    }
}

static const char *
aros_inet_ntop4(const unsigned char *src, char *dst, LONG size)
{
    char tmp[sizeof "255.255.255.255"];
    int len;

    len = snprintf(tmp, sizeof tmp, "%u.%u.%u.%u", src[0], src[1], src[2], src[3]);
    if (len < 0 || (LONG)len >= size) {
        errno = ENOSPC;
        return NULL;
    }
    memcpy(dst, tmp, len + 1);
    return dst;
}

static const char *
aros_inet_ntop6(const unsigned char *src, char *dst, LONG size)
{
    char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
    struct { int base, len; } best, cur;
    unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
    int i;

    memset(words, '\0', sizeof words);
    for (i = 0; i < NS_IN6ADDRSZ; i++)
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));

    best.base = -1; best.len = 0;
    cur.base  = -1; cur.len  = 0;
    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
        if (words[i] == 0) {
            if (cur.base == -1)
                cur.base = i, cur.len = 1;
            else
                cur.len++;
        } else {
            if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                    best = cur;
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1 && (best.base == -1 || cur.len > best.len))
        best = cur;
    if (best.base != -1 && best.len < 2)
        best.base = -1;

    tp = tmp;
    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
        if (best.base != -1 && i >= best.base && i < (best.base + best.len)) {
            if (i == best.base)
                *tp++ = ':';
            continue;
        }
        if (i != 0)
            *tp++ = ':';
        if (i == 6 && best.base == 0 &&
            (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
            if (!aros_inet_ntop4(src + 12, tp, (LONG)(sizeof tmp - (tp - tmp))))
                return NULL;
            tp += strlen(tp);
            break;
        }
        tp += sprintf(tp, "%x", words[i]);
    }
    if (best.base != -1 && (best.base + best.len) == (NS_IN6ADDRSZ / NS_INT16SZ))
        *tp++ = ':';
    *tp++ = '\0';

    if ((LONG)(tp - tmp) > size) {
        errno = ENOSPC;
        return NULL;
    }
    strcpy(dst, tmp);
    return dst;
}

#endif /* __AROS__ */
