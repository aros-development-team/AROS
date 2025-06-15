#ifndef _STDC_UTF8_H
#define _STDC_UTF8_H

#include <stddef.h>
#include <wchar.h>
#include <errno.h>

#include <aros/types/mbstate_t.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UTF-8 encoding of single wide character */
static inline size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps) {
    (void)ps; // stateless

    if (!s)
        return 1; // UTF-8 is stateless

    if (wc < 0x80) {
        s[0] = (char)wc;
        return 1;
    } else if (wc < 0x800) {
        s[0] = 0xC0 | (wc >> 6);
        s[1] = 0x80 | (wc & 0x3F);
        return 2;
    } else if (wc < 0x10000) {
        if (wc >= 0xD800 && wc <= 0xDFFF) goto error;
        s[0] = 0xE0 | (wc >> 12);
        s[1] = 0x80 | ((wc >> 6) & 0x3F);
        s[2] = 0x80 | (wc & 0x3F);
        return 3;
    } else if (wc <= 0x10FFFF) {
        s[0] = 0xF0 | (wc >> 18);
        s[1] = 0x80 | ((wc >> 12) & 0x3F);
        s[2] = 0x80 | ((wc >> 6) & 0x3F);
        s[3] = 0x80 | (wc & 0x3F);
        return 4;
    }

error:
    errno = EILSEQ;
    return (size_t)-1;
}

/* Legacy wrapper for wcrtomb using static state (which we ignore) */
static inline int wctomb(char *s, wchar_t wc) {
    return (int)wcrtomb(s, wc, NULL);
}

/* UTF-8 decoding of a single multibyte character */
static inline int mbtowc(wchar_t *pwc, const char *s, size_t n) {
    if (!s)
        return 0; // stateless encoding

    if (n == 0)
        return -1;

    unsigned char c = (unsigned char)s[0];
    if (c < 0x80) {
        if (pwc) *pwc = c;
        return c ? 1 : 0;
    }

    int len;
    wchar_t wc;

    if ((c & 0xE0) == 0xC0) {
        len = 2;
        wc = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        len = 3;
        wc = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        len = 4;
        wc = c & 0x07;
    } else {
        errno = EILSEQ;
        return -1;
    }

    if (len > (int)n) {
        errno = EILSEQ;
        return -1;
    }

    for (int i = 1; i < len; ++i) {
        if ((s[i] & 0xC0) != 0x80) {
            errno = EILSEQ;
            return -1;
        }
        wc = (wc << 6) | (s[i] & 0x3F);
    }

    // Check for overlong or invalid ranges
    if ((len == 2 && wc < 0x80) ||
        (len == 3 && wc < 0x800) ||
        (len == 4 && wc < 0x10000) ||
        (wc > 0x10FFFF) ||
        (wc >= 0xD800 && wc <= 0xDFFF)) {
        errno = EILSEQ;
        return -1;
    }

    if (pwc)
        *pwc = wc;

    return len;
}

/* Legacy multibyte length query */
static inline int mblen(const char *s, size_t n) {
    return mbtowc(NULL, s, n);
}

/* Convert wide string to multibyte (UTF-8) */
static inline size_t wcstombs(char *dest, const wchar_t *src, size_t n) {
    size_t total = 0;
    char buf[4];
    mbstate_t ps = {0};

    while (*src) {
        size_t len = wcrtomb(buf, *src++, &ps);
        if (len == (size_t)-1)
            return (size_t)-1;

        if (total + len > n)
            break;

        if (dest) {
            for (size_t i = 0; i < len; ++i)
                dest[total + i] = buf[i];
        }

        total += len;
    }

    return total;
}

/* Convert multibyte string (UTF-8) to wide string */
static inline size_t mbstowcs(wchar_t *dest, const char *src, size_t n) {
    size_t count = 0;
    mbstate_t ps = {0};
    wchar_t wc;
    int len;

    while (*src && count < n) {
        len = mbtowc(&wc, src, MB_CUR_MAX);
        if (len == -1)
            return (size_t)-1;

        if (dest)
            dest[count] = wc;

        src += len;
        ++count;
    }

    return count;
}

#ifdef __cplusplus
}
#endif

#endif /* _STDC_UTF8_H */
