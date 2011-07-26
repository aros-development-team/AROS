/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: Miscellaneous support functions.
    Lang: English
*/

#include <bootconsole.h>
#include <runtime.h>
#include <stdarg.h>
#include <string.h>

#include "bootstrap.h"
#include "support.h"

/* This comes from librom32 */
int __vcformat (void *data, int(*outc)(int, void *), const char * format, va_list args);

char *__bs_remove_path(char *in)
{
    char *p;

    /* Go to the end of string */
    for (p = in; *p; p++);
    /* Now go backwards until we find a separator */
    while (p > in && p[-1] != '/' && p[-1] != ':') p--;

    return p;
}

/* Own memcpy(), because librom's one can use CopyMem() */
void *memcpy(void *dest, const void *src, size_t len)
{
    void *ret = dest;

    while (len >= 4)
    {
        *(unsigned long *)dest = *(unsigned long *)src;
        len-=4;
        dest+=4;
        src+=4;
    }
    if (len >= 2)
    {
        *(unsigned short *)dest = *(unsigned short *)src;
        dest+=2;
        src+=2;
        len-=2;
    }
    if (len == 1)
    {
        *(unsigned char *)dest = *(unsigned char *)src;
        dest += 1;
    }

    return ret;
}

/*
 * Our extremely simple working memory allocator.
 * We can't just use some memory region because kickstart modules are placed there by GRUB.
 * We risk clobbering loaded kickstart in such a case.
 * The only 100% usable memory is memory contained in our own file.
 * So we reserve some workspace here. I hope 1MB is more than enough for out needs.
 * This space ends up in .bss section, so it does not occupy this megabyte on disk.
 */
static char workspace[0x1000000];

static char *MemPtr = workspace;

void *__bs_malloc(unsigned long size)
{
    char *start = MemPtr;
    char *end;

    /* Longword-align the size */
    size = (size + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
    end  = start + size;

    /*
     * _start is provided by linker script, it marks start of
     * our executable. This is the end of allocatable region.
     * We also count reserved space for stack which is placed in the
     * end of our working memory.
     */
    if (end > workspace + sizeof(workspace))
    	return 0;

    MemPtr = end;

    return start;
}

/* This routine resets the allocator and releases all previously allocated memory */
void __bs_free(void)
{
    MemPtr = workspace;
}

static int kputc(int c, void *data)
{
    con_Putc(c);

    return 1;
}

/* KNOWN BUG: %llu and %lld will not work here. See notice in compiler/clib/__vcformat.c. */
void kprintf(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);

    __vcformat(0, kputc, format, ap);

    va_end(ap);
}

/* The same as kprintf(). Needed for ELF loader. */
void DisplayError(char *format, ...)
{
    va_list ap;

    va_start(ap, format);

    __vcformat(0, kputc, format, ap);

    va_end(ap);
}
