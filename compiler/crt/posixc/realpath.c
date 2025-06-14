/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // for getcwd, chdir, readlink, etc.

#ifndef SYMLOOP_MAX
#define SYMLOOP_MAX 40
#endif

static char *
__realpath(const char *path, char *resolved_path, int count);

/*****************************************************************************

    NAME */

        char *realpath(

/*  SYNOPSIS */
        const char *path, char *resolved_path)

/*  FUNCTION
        Resolves the absolute, canonical pathname of the given path by
        resolving all symbolic links, relative components, and references
        to `.` and `..`.

    INPUTS
        path
            Pointer to the null-terminated pathname to resolve.
        resolved_path
            Buffer where the resolved absolute pathname is stored.
            If NULL, the function allocates a buffer of size PATH_MAX internally,
            which should be freed by the caller.

    RESULT
        Returns a pointer to the resolved_path buffer on success.
        Returns NULL on failure, setting errno to indicate the error.

    NOTES
        - The resolved path is always null-terminated.
        - Symbolic link loops are detected and prevented by limiting
          the number of symlinks followed (SYMLOOP_MAX).
        - The function temporarily changes the working directory internally,
          but restores it before returning.
        - The function assumes a POSIX-like environment with UTF-8 encoding.
        - If resolved_path is NULL, the caller is responsible for freeing
          the returned buffer.

    EXAMPLE
        char buf[PATH_MAX];
        char *res = realpath("/some/path", buf);
        if (res) {
            // use res
        } else {
            // handle error
        }

    BUGS
        None known.

    SEE ALSO
        getcwd(), readlink(), chdir()

    INTERNALS
        Implements path resolution by recursive symlink expansion and directory
        traversal, saving and restoring the current directory during processing.

******************************************************************************/
{
    char *ret;
    int dofree = 0;
    int fd;

    if (resolved_path == NULL) {
        resolved_path = malloc(PATH_MAX);
        if (resolved_path == NULL) {
            return NULL;
        }
        dofree = 1;
    }

    /* Save current working directory by opening "." */
    fd = open(".", O_RDONLY);
    if (fd == -1) {
        if (dofree)
            free(resolved_path);
        return NULL;
    }

    ret = __realpath(path, resolved_path, SYMLOOP_MAX);

    /* Restore current working directory */
    fchdir(fd);
    close(fd);

    if (ret == NULL && dofree) {
        free(resolved_path);
    }

    return ret;
}

static char *
__realpath(const char *path, char *resolved_path, int count)
{
    char buf[PATH_MAX + 1];
    char *slash;
    int len;

    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }

    if (path[0] == '\0') {
        errno = ENOENT;
        return NULL;
    }

    if (count <= 0) {
        /* Too many symbolic links */
        errno = ELOOP;
        return NULL;
    }

    /* If path is a directory or can be chdir'ed into, resolve with getcwd */
    if (chdir(path) == 0) {
        if (getcwd(resolved_path, PATH_MAX) == NULL) {
            /* getcwd failed */
            return NULL;
        }
        return resolved_path;
    }

    /* Otherwise, find the last '/' in path */
    slash = strrchr(path, '/');
    if (slash) {
        len = slash - path;
        if (len > PATH_MAX) {
            errno = ENAMETOOLONG;
            return NULL;
        }
        /* Copy directory part */
        memcpy(buf, path, len);
        buf[len] = '\0';

        /* Change directory to that path */
        if (chdir(buf) == -1)
            return NULL;

        /* Move past the '/' */
        path = slash + 1;
    } else {
        /* No directory part: use current directory */
        if (chdir(".") == -1)
            return NULL;
    }

    /* Try to readlink the last component */
    len = readlink(path, buf, PATH_MAX);
    if (len > 0) {
        if (len >= PATH_MAX) {
            errno = ENAMETOOLONG;
            return NULL;
        }
        buf[len] = '\0';
        /* Recursively resolve the link target */
        return __realpath(buf, resolved_path, count - 1);
    }

    /* Not a symlink, build the absolute path */
    if (getcwd(resolved_path, PATH_MAX) == NULL)
        return NULL;

    len = strlen(resolved_path);
    if (len + 1 + strlen(path) + 1 > PATH_MAX) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    /* Append '/' and the final component */
    if (len > 0 && resolved_path[len - 1] != '/') {
        resolved_path[len++] = '/';
    }

    strcpy(resolved_path + len, path);

    return resolved_path;
}

