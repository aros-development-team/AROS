/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <proto/dos.h>
#include <proto/oop.h>

#include <hidd/pci.h>

#define DEBUG 0
#include <aros/debug.h>

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/* 
    The purpose of this file is to provide implementation for C functions which 
    are used by a module and are not located in librom.a

    
    A module cannot use arosc.library because arosc.library context is bound 
    with caller task and gets destroyed when the task exits. After the
    caller task exists, the module becomes instable
*/

#define IMPLEMENT()  bug("------IMPLEMENT(%s)\n", __func__)

/* malloc/calloc/realloc/free */

/* This is a copy of implementation from arosc.library so that mesa.library uses its 
   own space for malloc/calloc/realloc calls */

#define MEMALIGN_MAGIC ((size_t) 0x57E2CB09)
APTR __mempool = NULL;

void * malloc (size_t size)
{
    UBYTE *mem = NULL;

    D(bug("arosc_emul_malloc\n"));
    /* Allocate the memory */
    mem = AllocPooled (__mempool, size + AROS_ALIGN(sizeof(size_t)));
    if (mem)
    {
        *((size_t *)mem) = size;
        mem += AROS_ALIGN(sizeof(size_t));
    }
    else
        errno = ENOMEM;

    return mem;
}

void free (void * memory)
{
    D(bug("arosc_emul_free\n"));
    if (memory)
    {
        unsigned char *mem;
        size_t         size;

        mem = ((UBYTE *)memory) - AROS_ALIGN(sizeof(size_t));

        size = *((size_t *) mem);
        if (size == MEMALIGN_MAGIC)
            free(((void **) mem)[-1]);
        else 
        {
            size += AROS_ALIGN(sizeof(size_t));
            FreePooled (__mempool, mem, size);
        }
    }
}

void * calloc (size_t count, size_t size)
{
    ULONG * mem;
    D(bug("arosc_emul_calloc\n"));
    size *= count;

    /* Allocate the memory */
    mem = malloc (size);

    if (mem)
        memset (mem, 0, size);

    return mem;
}

void * realloc (void * oldmem, size_t size)
{
    
    UBYTE * mem, * newmem;
    size_t oldsize;
    
    D(bug("arosc_emul_realloc\n"));
    
    if (!oldmem)
        return malloc (size);

    mem = (UBYTE *)oldmem - AROS_ALIGN(sizeof(size_t));
    oldsize = *((size_t *)mem);

    /* Reduce or enlarge the memory ? */
    if (size < oldsize)
    {
        /* Don't change anything for small changes */
        if ((oldsize - size) < 4096)
            newmem = oldmem;
        else
            goto copy;
    }
    else if (size == oldsize) /* Keep the size ? */
        newmem = oldmem;
    else
    {
copy:
        newmem = malloc (size);

        if (newmem)
        {
            CopyMem (oldmem, newmem, size);
            free (oldmem);
        }
    }

    return newmem;
}

char *getenv (const char *name)
{
    /* For nouveau.hidd don't allow reading ENV. Maybe in future change
       to reading tool types? */
    return NULL;
}

const char *pci_name(OOP_Object *pdev)
{
    static char name[16];
    IPTR bus = 0, dev = 0, sub = 0;
    OOP_GetAttr(pdev, aHidd_PCIDevice_Bus, &bus);
    OOP_GetAttr(pdev, aHidd_PCIDevice_Bus, &dev);
    OOP_GetAttr(pdev, aHidd_PCIDevice_Bus, &sub);
    snprintf(name, sizeof(name), "%x:%2x.%x\n",
    	    (unsigned)bus, (unsigned)dev, (unsigned)sub);
    name[sizeof(name)-1] = 0;
    return name;
}

struct timeval;
struct timezone;

int gettimeofday (struct timeval * tv,struct timezone * tz)
{
    IMPLEMENT();
    return 0;
}

__noreturn void abort (void)
{
    IMPLEMENT();
    Alert(AT_DeadEnd | AO_HiddLib);
    for (;;);
}

/* File operations */
int fputs (const char * str, FILE * stream)
{
    IMPLEMENT();
    return EOF;
}

size_t fwrite (const void * restrict buf, size_t size, size_t nblocks, FILE * restrict stream)
{
    IMPLEMENT();
    return 0;
}

int fflush (FILE * stream)
{
    IMPLEMENT();
    return EOF;
}

int fprintf (FILE * fh, const char * format, ...)
{
    IMPLEMENT();
    return 0;
}

double atof (const char * str)
{
    return strtod (str, (char **)NULL);
}

/* private function to get the upper or lower bound (depending on the architecture) of the stack */
/* It has to go into a separate file so that proto/exec.h doesn't get included in the clib headers */

void *__alloca_get_stack_limit(void)
{
    #if AROS_STACK_GROWS_DOWNWARDS
    return FindTask(NULL)->tc_SPLower;
    #else
    return FindTask(NULL)->tc_SPUpper;
    #endif
}
	
int __init_emul(void)
{
    /* malloc/calloc/realloc/free */
    __mempool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED, 65536L, 4096L);

    if (!__mempool)
    {
        return 0;
    }

    return 1;
}


void __exit_emul(void)
{
    /* malloc/calloc/realloc/free */
    if (__mempool)
    {
        DeletePool(__mempool);
    }
}

ADD2INIT(__init_emul, 0);
ADD2EXIT(__exit_emul, 0);
