/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/kstrtox.h>
#include <linux/sort.h>
#include <linux/crc32.h>
#include <linux/hex.h>
#include <linux/bitops.h>
#include <linux/bitmap.h>
#include <linux/math.h>

/* --- strings ---------------------------------------------------------- */

size_t strnlen_compat(const char *s, size_t maxlen)
{
    size_t n = 0;
    while (n < maxlen && s[n])
        n++;
    return n;
}

ssize_t sized_strscpy(char *dst, const char *src, size_t size)
{
    size_t len;

    if (size == 0)
        return -E2BIG;
    len = strnlen_compat(src, size);
    if (len == size) {
        memcpy(dst, src, size - 1);
        dst[size - 1] = 0;
        return -E2BIG;
    }
    memcpy(dst, src, len + 1);
    return len;
}

size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);

    if (size) {
        size_t n = len >= size ? size - 1 : len;
        memcpy(dst, src, n);
        dst[n] = 0;
    }
    return len;
}

size_t strlcat(char *dst, const char *src, size_t size)
{
    size_t dlen = strnlen_compat(dst, size);
    size_t slen = strlen(src);

    if (dlen == size)
        return size + slen;
    if (slen < size - dlen) {
        memcpy(dst + dlen, src, slen + 1);
    } else {
        memcpy(dst + dlen, src, size - dlen - 1);
        dst[size - 1] = 0;
    }
    return dlen + slen;
}

char *strsep(char **stringp, const char *delim)
{
    char *s = *stringp, *e;

    if (!s)
        return NULL;
    e = strpbrk(s, delim);
    if (e)
        *e++ = 0;
    *stringp = e;
    return s;
}

char *skip_spaces(const char *str)
{
    while (isspace((unsigned char)*str))
        str++;
    return (char *)str;
}

char *strim(char *s)
{
    size_t size = strlen(s);
    char *end;

    if (size) {
        end = s + size - 1;
        while (end >= s && isspace((unsigned char)*end))
            end--;
        *(end + 1) = 0;
    }
    return skip_spaces(s);
}

char *strreplace(char *s, char old, char new)
{
    for (; *s; s++)
        if (*s == old)
            *s = new;
    return s;
}

const char *kbasename(const char *path)
{
    const char *tail = strrchr(path, '/');
    return tail ? tail + 1 : path;
}

bool sysfs_streq(const char *s1, const char *s2)
{
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    if (*s1 == *s2)
        return true;
    if (!*s1 && *s2 == '\n' && !s2[1])
        return true;
    if (*s1 == '\n' && !s1[1] && !*s2)
        return true;
    return false;
}

void *memchr_inv(const void *start, int c, size_t bytes)
{
    const unsigned char *p = start;
    while (bytes--) {
        if (*p != (unsigned char)c)
            return (void *)p;
        p++;
    }
    return NULL;
}

int match_string(const char *const *array, size_t n, const char *string)
{
    size_t i;
    for (i = 0; i < n; i++) {
        if (!array[i])
            break;
        if (!strcmp(array[i], string))
            return i;
    }
    return -EINVAL;
}

int __sysfs_match_string(const char *const *array, size_t n, const char *str)
{
    size_t i;
    for (i = 0; i < n; i++) {
        if (!array[i])
            break;
        if (sysfs_streq(array[i], str))
            return i;
    }
    return -EINVAL;
}

char *strnstr(const char *s1, const char *s2, size_t len)
{
    size_t l2 = strlen(s2);

    if (!l2)
        return (char *)s1;
    while (len >= l2) {
        len--;
        if (!memcmp(s1, s2, l2))
            return (char *)s1;
        s1++;
    }
    return NULL;
}

/* --- kstrto* ------------------------------------------------------------- */

static int parse_prefix(const char **s, unsigned int *base, bool *neg)
{
    *neg = false;
    if (**s == '-') { *neg = true; (*s)++; }
    else if (**s == '+') (*s)++;
    if (*base == 0) {
        if ((*s)[0] == '0' && ((*s)[1] == 'x' || (*s)[1] == 'X'))
            *base = 16;
        else if ((*s)[0] == '0')
            *base = 8;
        else
            *base = 10;
    }
    if (*base == 16 && (*s)[0] == '0' && ((*s)[1] == 'x' || (*s)[1] == 'X'))
        *s += 2;
    return 0;
}

int kstrtoull(const char *s, unsigned int base, unsigned long long *res)
{
    unsigned long long v = 0;
    bool neg;
    int digits = 0;

    parse_prefix(&s, &base, &neg);
    if (neg)
        return -EINVAL;
    while (*s) {
        unsigned int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        else if (*s == '\n' && !s[1]) break;
        else return -EINVAL;
        if (d >= base)
            return -EINVAL;
        if (v > (ULLONG_MAX - d) / base)
            return -ERANGE;
        v = v * base + d;
        digits++;
        s++;
    }
    if (!digits)
        return -EINVAL;
    *res = v;
    return 0;
}

int kstrtoll(const char *s, unsigned int base, long long *res)
{
    unsigned long long v;
    int ret;

    if (*s == '-') {
        ret = kstrtoull(s + 1, base, &v);
        if (ret)
            return ret;
        if (v > (unsigned long long)LLONG_MAX + 1)
            return -ERANGE;
        *res = -(long long)v;
        return 0;
    }
    ret = kstrtoull(s, base, &v);
    if (ret)
        return ret;
    if (v > LLONG_MAX)
        return -ERANGE;
    *res = v;
    return 0;
}

#define KSTRTO_UNSIGNED(name, type, max)                                \
int name(const char *s, unsigned int base, type *res)                   \
{                                                                       \
    unsigned long long v; int ret = kstrtoull(s, base, &v);             \
    if (ret) return ret;                                                \
    if (v > (max)) return -ERANGE;                                      \
    *res = (type)v; return 0;                                           \
}
#define KSTRTO_SIGNED(name, type, min, max)                             \
int name(const char *s, unsigned int base, type *res)                   \
{                                                                       \
    long long v; int ret = kstrtoll(s, base, &v);                       \
    if (ret) return ret;                                                \
    if (v < (min) || v > (max)) return -ERANGE;                         \
    *res = (type)v; return 0;                                           \
}
KSTRTO_UNSIGNED(kstrtoul, unsigned long, ULONG_MAX)
KSTRTO_SIGNED(kstrtol, long, LONG_MIN, LONG_MAX)
KSTRTO_UNSIGNED(kstrtouint, unsigned int, UINT_MAX)
KSTRTO_SIGNED(kstrtoint, int, INT_MIN, INT_MAX)
KSTRTO_UNSIGNED(kstrtou64, u64, U64_MAX)
KSTRTO_SIGNED(kstrtos64, s64, S64_MIN, S64_MAX)
KSTRTO_UNSIGNED(kstrtou32, u32, U32_MAX)
KSTRTO_SIGNED(kstrtos32, s32, S32_MIN, S32_MAX)
KSTRTO_UNSIGNED(kstrtou16, u16, U16_MAX)
KSTRTO_SIGNED(kstrtos16, s16, S16_MIN, S16_MAX)
KSTRTO_UNSIGNED(kstrtou8, u8, U8_MAX)
KSTRTO_SIGNED(kstrtos8, s8, S8_MIN, S8_MAX)

int kstrtobool(const char *s, bool *res)
{
    if (!s)
        return -EINVAL;
    switch (s[0]) {
    case 'y': case 'Y': case '1': *res = true; return 0;
    case 'n': case 'N': case '0': *res = false; return 0;
    case 'o': case 'O':
        switch (s[1]) {
        case 'n': case 'N': *res = true; return 0;
        case 'f': case 'F': *res = false; return 0;
        }
        break;
    }
    return -EINVAL;
}

/* --- sort ------------------------------------------------------------- */

static void generic_swap(void *a, void *b, int size)
{
    char t;
    do {
        t = *(char *)a;
        *(char *)a++ = *(char *)b;
        *(char *)b++ = t;
    } while (--size > 0);
}

/* heap sort: no allocation, any element size */
void sort(void *base, size_t num, size_t size, int (*cmp)(const void *, const void *), void (*swp)(void *, void *, int))
{
    int i = (num / 2 - 1) * size, n = num * size, c, r;

    if (!swp)
        swp = generic_swap;
    if (num < 2)
        return;

    for (; i >= 0; i -= size) {
        for (r = i; r * 2 + size < n; r = c) {
            c = r * 2 + size;
            if (c < n - size && cmp((char *)base + c, (char *)base + c + size) < 0)
                c += size;
            if (cmp((char *)base + r, (char *)base + c) >= 0)
                break;
            swp((char *)base + r, (char *)base + c, size);
        }
    }
    for (i = n - size; i > 0; i -= size) {
        swp(base, (char *)base + i, size);
        for (r = 0; r * 2 + size < i; r = c) {
            c = r * 2 + size;
            if (c < i - size && cmp((char *)base + c, (char *)base + c + size) < 0)
                c += size;
            if (cmp((char *)base + r, (char *)base + c) >= 0)
                break;
            swp((char *)base + r, (char *)base + c, size);
        }
    }
}

/* --- crc32 (bitwise, little used) --------------------------------------- */

/* kernel semantics: no pre/post inversion, the caller seeds with ~0 and
   inverts the result itself */
u32 crc32_le(u32 crc, const void *p, size_t len)
{
    const u8 *b = p;
    int i;

    while (len--) {
        crc ^= *b++;
        for (i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (0xedb88320 & -(crc & 1));
    }
    return crc;
}

u32 crc32_be(u32 crc, const void *p, size_t len)
{
    const u8 *b = p;
    int i;

    while (len--) {
        crc ^= (u32)*b++ << 24;
        for (i = 0; i < 8; i++)
            crc = (crc << 1) ^ ((crc & 0x80000000) ? 0x04c11db7 : 0);
    }
    return crc;
}

/* --- hex ------------------------------------------------------------- */

const char hex_asc[] = "0123456789abcdef";
const char hex_asc_upper[] = "0123456789ABCDEF";

int hex2bin(u8 *dst, const char *src, size_t count)
{
    while (count--) {
        int hi = hex_to_bin(*src++);
        int lo = hex_to_bin(*src++);
        if (hi < 0 || lo < 0)
            return -EINVAL;
        *dst++ = (hi << 4) | lo;
    }
    return 0;
}

char *bin2hex(char *dst, const void *src, size_t count)
{
    const unsigned char *s = src;
    while (count--)
        dst = hex_byte_pack(dst, *s++);
    return dst;
}

/* --- bit searching ----------------------------------------------------- */

unsigned long _find_next_bit(const unsigned long *addr, unsigned long nbits, unsigned long start, unsigned long invert)
{
    unsigned long tmp;

    if (start >= nbits)
        return nbits;
    tmp = addr[start / BITS_PER_LONG] ^ invert;
    tmp &= BITMAP_FIRST_WORD_MASK(start);
    start = round_down(start, BITS_PER_LONG);
    while (!tmp) {
        start += BITS_PER_LONG;
        if (start >= nbits)
            return nbits;
        tmp = addr[start / BITS_PER_LONG] ^ invert;
    }
    return min(start + __ffs(tmp), nbits);
}

unsigned long _find_first_bit(const unsigned long *addr, unsigned long size)
{
    return _find_next_bit(addr, size, 0, 0UL);
}

unsigned long _find_first_zero_bit(const unsigned long *addr, unsigned long size)
{
    return _find_next_bit(addr, size, 0, ~0UL);
}

unsigned long _find_last_bit(const unsigned long *addr, unsigned long size)
{
    if (size) {
        unsigned long val = BITMAP_LAST_WORD_MASK(size);
        unsigned long idx = (size - 1) / BITS_PER_LONG;
        do {
            val &= addr[idx];
            if (val)
                return idx * BITS_PER_LONG + __fls(val);
            val = ~0ul;
        } while (idx--);
    }
    return size;
}

unsigned long _find_next_and_bit(const unsigned long *addr1, const unsigned long *addr2, unsigned long nbits, unsigned long start)
{
    for (; start < nbits; start++)
        if (test_bit(start, addr1) && test_bit(start, addr2))
            return start;
    return nbits;
}

unsigned long int_sqrt(unsigned long x)
{
    unsigned long b, m, y = 0;

    if (x <= 1)
        return x;
    m = 1UL << (__fls(x) & ~1UL);
    while (m != 0) {
        b = y + m;
        y >>= 1;
        if (x >= b) {
            x -= b;
            y += m;
        }
        m >>= 2;
    }
    return y;
}

#if !defined(__SIZEOF_INT128__)
u64 mul_u64_u64_div_u64(u64 a, u64 mul, u64 div)
{
    /* long division on a 128-bit intermediate, bit by bit */
    u64 hi = 0, lo = 0, q = 0, r = 0;
    int i;
    /* a * mul -> hi:lo */
    {
        u64 a_lo = (u32)a, a_hi = a >> 32, m_lo = (u32)mul, m_hi = mul >> 32;
        u64 p0 = a_lo * m_lo, p1 = a_lo * m_hi, p2 = a_hi * m_lo, p3 = a_hi * m_hi;
        u64 mid = (p0 >> 32) + (u32)p1 + (u32)p2;
        lo = (mid << 32) | (u32)p0;
        hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
    }
    for (i = 127; i >= 0; i--) {
        u64 bit = i >= 64 ? (hi >> (i - 64)) & 1 : (lo >> i) & 1;
        r = (r << 1) | bit;
        q <<= 1;
        if (r >= div) {
            r -= div;
            q |= 1;
        }
    }
    return q;
}
#endif

int panic_timeout = 0;
