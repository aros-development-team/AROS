/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_STRING_H_
#define _LINUX_STRING_H_

#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/overflow.h>
#include <linux/bitops.h>
#include <linux/array_size.h>

ssize_t sized_strscpy(char *dst, const char *src, size_t size);
/* two-argument form takes the destination array's size */
#define __strscpy_pick(dst, src, size, ...) sized_strscpy(dst, src, size)
#define strscpy(dst, src, ...)  __strscpy_pick(dst, src, ##__VA_ARGS__, sizeof(dst), )
#define strscpy_pad(dst, src, ...) strscpy(dst, src, ##__VA_ARGS__)
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);
char *strsep(char **stringp, const char *delim);
char *skip_spaces(const char *str);
char *strim(char *s);
char *strreplace(char *s, char old, char new);
const char *kbasename(const char *path);
bool sysfs_streq(const char *s1, const char *s2);
void *memchr_inv(const void *start, int c, size_t bytes);
int  match_string(const char *const *array, size_t n, const char *string);
int  __sysfs_match_string(const char *const *array, size_t n, const char *string);
#define sysfs_match_string(a, s) __sysfs_match_string(a, ARRAY_SIZE(a), s)
char *strnstr(const char *s1, const char *s2, size_t len);
size_t strnlen_compat(const char *s, size_t maxlen);
#ifndef strnlen
#define strnlen(s, n)           strnlen_compat(s, n)
#endif
#define kmemdup_nul(s, l, g)    kstrndup(s, l, g)
#define memcpy_and_pad(d, ds, s, c, p) do { memcpy(d, s, c); if ((ds) > (c)) memset((char *)(d) + (c), p, (ds) - (c)); } while (0)
#define memset_startat(o, v, m) memset((char *)(o) + offsetof(typeof(*(o)), m), v, sizeof(*(o)) - offsetof(typeof(*(o)), m))
#define memset_after(o, v, m)   memset((char *)(o) + offsetofend(typeof(*(o)), m), v, sizeof(*(o)) - offsetofend(typeof(*(o)), m))
#define memzero_explicit(s, n)  memset(s, 0, n)
#define strtomem_pad(dest, src, pad) do { size_t __l = strnlen(src, sizeof(dest)); memcpy(dest, src, __l); memset((char *)(dest) + __l, pad, sizeof(dest) - __l); } while (0)
#define strtomem(dest, src)     memcpy(dest, src, strnlen(src, sizeof(dest)))
#define strstarts(str, prefix)  (strncmp(str, prefix, strlen(prefix)) == 0)

static inline bool mem_is_zero(const void *s, size_t n) { return memchr_inv(s, 0, n) == NULL; }
static inline size_t str_has_prefix(const char *str, const char *prefix) { size_t len = strlen(prefix); return strncmp(str, prefix, len) == 0 ? len : 0; }
static inline char *strnchr(const char *s, size_t count, int c) { while (count--) { if (*s == (char)c) return (char *)s; if (*s++ == '\0') break; } return NULL; }
/*
 * Size of an on-stack object as the compiler sees it: DEFINE_FLEX() places
 * the struct inside a union sized for the trailing array, so sizeof(*p)
 * would miss that array. Fall back to sizeof when the size is not known.
 */
#if defined(__has_builtin)
# if __has_builtin(__builtin_dynamic_object_size)
#  define __compat_objsize(p)   __builtin_dynamic_object_size(p, 1)
# endif
#endif
#ifndef __compat_objsize
# define __compat_objsize(p)    __builtin_object_size(p, 1)
#endif
#define __member_size(p)        ((__compat_objsize(p) == (size_t)-1) ? sizeof(*(p)) : __compat_objsize(p))
#define __struct_size(p)        ((__compat_objsize(p) == (size_t)-1) ? sizeof(*(p)) : __compat_objsize(p))
static inline int strtomem_pad_compat(void) { return 0; }

#endif /* _LINUX_STRING_H_ */
