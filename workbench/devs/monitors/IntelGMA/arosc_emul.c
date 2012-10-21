/*
    Copyright 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <proto/dos.h>
#include <proto/timer.h>

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

#define IMPLEMENT()  bug("[GMA Gallium]------IMPLEMENT(%s)\n", __func__)

/* malloc/calloc/realloc/free */

/* This is a copy of implementation from arosc.library so that module uses its
   own space for malloc/calloc/realloc calls */

#define MEMALIGN_MAGIC ((size_t) 0x57E2CB09)
extern APTR i915MemPool;

void * malloc (size_t size)
{
    UBYTE *mem = NULL;

    D(bug("arosc_emul_malloc\n"));
    /* Allocate the memory */
    mem = AllocPooled (i915MemPool, size + AROS_ALIGN(sizeof(size_t)));
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
            FreePooled (i915MemPool, mem, size);
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

struct timezone;

int gettimeofday (struct timeval * tv,struct timezone * tz)
{
    struct MsgPort * timerport = CreateMsgPort();
    struct timerequest * timereq = (struct timerequest *)CreateIORequest(timerport, sizeof(*timereq));


    if (timereq)
    {
        if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)timereq, 0) == 0)
        {
            #define TimerBase ((struct Device *)timereq->tr_node.io_Device)

            GetSysTime(tv);

            #undef TimerBase

            CloseDevice((struct IORequest *)timereq);
        }
    }

    DeleteIORequest((struct IORequest *)timereq);
    DeleteMsgPort(timerport);

    return 0;
}

__noreturn void abort (void)
{
    IMPLEMENT();
    for(;;);
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

int vfprintf(FILE * restrict stream, const char * restrict format,
    va_list arg)
{
    vkprintf(format,arg);
    return 0;
}

double atof (const char * str)
{
    return strtod (str, (char **)NULL);
}

int puts (const char * str)
{
    bug("%s\n", str);
    return 1;
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

