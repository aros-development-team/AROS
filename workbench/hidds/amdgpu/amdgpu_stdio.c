/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: Diagnostics from the statically linked Mesa and libdrm code,
          sent to the debug log instead of posixc stdio, which cannot be
          used from the hidd's context.
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <stdarg.h>
#include <stdio.h>

int fprintf(FILE *stream, const char *fmt, ...)
{
    char lbuf[512];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(lbuf, sizeof(lbuf), fmt, ap);
    va_end(ap);
    bug("[Amdgpu:mesa] %s", lbuf);
    return 0;
}

int vfprintf(FILE *stream, const char *fmt, va_list ap)
{
    char lbuf[512];

    vsnprintf(lbuf, sizeof(lbuf), fmt, ap);
    bug("[Amdgpu:mesa] %s", lbuf);
    return 0;
}

int fputs(const char *s, FILE *stream)
{
    bug("[Amdgpu:mesa] %s\n", s ? s : "(null)");
    return 0;
}

int printf(const char *fmt, ...)
{
    char lbuf[512];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(lbuf, sizeof(lbuf), fmt, ap);
    va_end(ap);
    bug("[Amdgpu:mesa] %s", lbuf);
    return 0;
}

int puts(const char *s)
{
    bug("[Amdgpu:mesa] %s\n", s ? s : "(null)");
    return 0;
}

int fputc(int c, FILE *stream)
{
    bug("%c", c);
    return c;
}

int putchar(int c)
{
    bug("%c", c);
    return c;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    const char *p = ptr;
    size_t i, n = size * nmemb;

    for (i = 0; i < n; i++)
        bug("%c", p[i]);
    return nmemb;
}

int fflush(FILE *stream)
{
    return 0;
}

void abort(void)
{
    struct Task *me = FindTask(NULL);

    bug("[Amdgpu:mesa] abort() in task '%s'\n", me->tc_Node.ln_Name ? me->tc_Node.ln_Name : "?");
    RemTask(NULL);
    for (;;)
        ;
}
