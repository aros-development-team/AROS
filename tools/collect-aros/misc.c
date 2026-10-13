/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/param.h>
#include <string.h>

#include "misc.h"

#ifdef _WIN32
#define PATH_SEPARATOR ';'

/* If we're running on MinGW, PATH is in native Windows form while
   COMPILER_PATH has ';' as entries separator but still has '/' as directory
   separator, so we have to convert it. This is what this magic for. */

void copy_path(char *to, char *from)
{
    do {
        if (*from == '/')
            *to = '\\';
        else
            *to = *from;
        to++;
    } while (*from++);
}
#else
#define PATH_SEPARATOR ':'
#define copy_path strcpy
#endif

char *program_name;
void nonfatal(const char *msg, const char *errorstr)
{
    if (msg != NULL)
        fprintf(stderr, "%s: %s: %s\n" , program_name, msg, errorstr);
    else
        fprintf(stderr, "%s: %s\n" , program_name, errorstr);
}

void fatal(const char *msg, const char *errorstr)
{
    nonfatal(msg, errorstr);
    exit(EXIT_FAILURE);
}

void set_compiler_path(void)
{
    static int path_set = 0;

    if (!path_set)
    {
        char *compiler_path = getenv("COMPILER_PATH");
        char *path          = getenv("PATH");

        if (compiler_path && path)
        {
            char *new_path;
            size_t compiler_path_len = strlen(compiler_path);
            size_t path_len          = strlen(path);

            new_path = malloc(5 + compiler_path_len + 1 + path_len + 1);
            if (new_path)
            {
                strcpy(new_path, "PATH=");
                copy_path(new_path + 5, compiler_path);
                new_path[5 + compiler_path_len] = PATH_SEPARATOR;
                strcpy(new_path + 5 + compiler_path_len + 1, path);

                if (putenv(new_path) == 0)
                    path_set = 1;
            }
        }
    }
}

#ifndef _HAVE_LIBIBERTY_

void *xmalloc(size_t size)
{
    void *ret = malloc(size);
    if (ret == NULL)
    {
        fatal("xmalloc", strerror(errno));
    }

    return ret;
}

char *make_temp_file(char *suffix __attribute__((unused)))
{
    int fd;
    /* Without libiberty there is no suffix support, but the directory honors
       $TMPDIR (POSIX convention, same as mkdtemp/Python tempfile): without it
       (or when empty) we fall back to /tmp as before. A hardcoded /tmp breaks
       large links for anyone whose /tmp is small, quota-limited, or mounted
       noexec — collect-aros fails closed there with no recourse. */
    const char *tmpdir = getenv("TMPDIR");
    char *template;
    size_t len;

    if (tmpdir == NULL || tmpdir[0] == '\0')
        tmpdir = "/tmp";
    len = strlen(tmpdir) + sizeof("/catmpXXXXXX");
    template = malloc(len);
    if (template == NULL)
        return NULL;
    snprintf(template, len, "%s/catmpXXXXXX", tmpdir);

    fd = mkstemp(template);
    if (fd == -1)
    {
        free(template);
        return NULL;
    }

    if (close(fd) != 0)
        fatal("make_temp_file()/close()", strerror(errno));

    return template;
}

#endif
