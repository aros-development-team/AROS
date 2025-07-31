/*
    Copyright 2025, The AROS Development Team. All rights reserved.
*/

//#define DEBUG 1
#include <aros/debug.h>

#include <stdlib.h>
#include <string.h>

const char *opt_no ="no";

void *os_malloc(size_t size) {
    D(bug("[VMWareSVGA:util] %s()\n", __func__));
    return malloc(size);
}

void *os_calloc(size_t count, size_t size)  {
    D(bug("[VMWareSVGA:util] %s()\n", __func__));
    return calloc(count, size);
}

void os_free(void *ptr)  {
    D(bug("[VMWareSVGA:util] %s()\n", __func__));
    free(ptr);
}

void *os_realloc(void *ptr, size_t old_size, size_t size) {
    D(bug("[VMWareSVGA:util] %s()\n", __func__));
    return realloc(ptr, size + 0*(old_size));
};

void *
os_malloc_aligned(size_t size, size_t alignment)
{
    void *ptr;
    D(bug("[VMWareSVGA:util] %s()\n", __func__));
    alignment = (alignment + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
    if(posix_memalign(&ptr, alignment, size) != 0)
        return NULL;
    return ptr;
}

void os_free_aligned(void *ptr)
{
    D(bug("[VMWareSVGA:util] %s()\n", __func__));
    free(ptr);
}

const char *
os_get_option(const char *name)
{
    D(bug("[VMWareSVGA:util] %s()\n", __func__));
    if (!strcmp(name, "SVGA_NO_LOGGING"))
    {
        D(bug("[VMWareSVGA:util] %s: returning 'no'\n", __func__));
        return opt_no;
    }
    return NULL;
}
