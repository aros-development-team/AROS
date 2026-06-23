#ifndef ___PWDGRP_H
#define ___PWDGRP_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Internal helpers shared by the reentrant passwd/group accessors
    (getpwnam_r(), getpwuid_r(), getgrnam_r(), getgrgid_r()).

    The non-reentrant getpw*()/getgr*() functions return a pointer into a
    single shared buffer. The _r variants below deep-copy that result into a
    caller-provided buffer so the caller owns the returned data and it is not
    overwritten by a subsequent lookup.
*/

#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/* Copy the strings referenced by a struct passwd into the caller buffer.
   Returns 0 on success or ERANGE when the buffer is too small. */
static inline int __posix_pwd_to_buf(const struct passwd *src,
    struct passwd *dst, char *buffer, size_t bufsize)
{
    char *p = buffer;
    char *end = buffer + bufsize;

    dst->pw_uid = src->pw_uid;
    dst->pw_gid = src->pw_gid;

#define __PWD_COPY_STR(field)                                       \
    do {                                                            \
        if (src->field == NULL)                                     \
            dst->field = NULL;                                      \
        else {                                                      \
            size_t _l = strlen(src->field) + 1;                     \
            if ((size_t)(end - p) < _l)                             \
                return ERANGE;                                      \
            memcpy(p, src->field, _l);                              \
            dst->field = p;                                         \
            p += _l;                                                \
        }                                                           \
    } while (0)

    __PWD_COPY_STR(pw_name);
    __PWD_COPY_STR(pw_passwd);
    __PWD_COPY_STR(pw_dir);
    __PWD_COPY_STR(pw_shell);
    __PWD_COPY_STR(pw_gecos);

#undef __PWD_COPY_STR

    return 0;
}

/* Copy the strings and member array referenced by a struct group into the
   caller buffer. Returns 0 on success or ERANGE when too small. */
static inline int __posix_grp_to_buf(const struct group *src,
    struct group *dst, char *buffer, size_t bufsize)
{
    char *p = buffer;
    char *end = buffer + bufsize;
    char **memarr;
    size_t nmem = 0, i, pad, arrbytes;

    dst->gr_gid = src->gr_gid;

    if (src->gr_mem)
        while (src->gr_mem[nmem])
            nmem++;

    /* Align p to a pointer boundary for the gr_mem array. */
    pad = ((size_t)(-(intptr_t)p)) & (__alignof__(char *) - 1);
    if ((size_t)(end - p) < pad)
        return ERANGE;
    p += pad;

    memarr = (char **)p;
    arrbytes = (nmem + 1) * sizeof(char *);
    if ((size_t)(end - p) < arrbytes)
        return ERANGE;
    p += arrbytes;

#define __GRP_COPY_STR(field)                                       \
    do {                                                            \
        if (src->field == NULL)                                     \
            dst->field = NULL;                                      \
        else {                                                      \
            size_t _l = strlen(src->field) + 1;                     \
            if ((size_t)(end - p) < _l)                             \
                return ERANGE;                                      \
            memcpy(p, src->field, _l);                              \
            dst->field = p;                                         \
            p += _l;                                                \
        }                                                           \
    } while (0)

    __GRP_COPY_STR(gr_name);
    __GRP_COPY_STR(gr_passwd);

#undef __GRP_COPY_STR

    for (i = 0; i < nmem; i++)
    {
        size_t _l = strlen(src->gr_mem[i]) + 1;
        if ((size_t)(end - p) < _l)
            return ERANGE;
        memcpy(p, src->gr_mem[i], _l);
        memarr[i] = p;
        p += _l;
    }
    memarr[nmem] = NULL;
    dst->gr_mem = memarr;

    return 0;
}

#endif /* ___PWDGRP_H */
