/*
    Copyright (C) 2009-2019, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define IMPLEMENT()  bug("------IMPLEMENT(%s)\n", __func__)

/*
 * malloc/free/calloc/realloc overrides via Exec AllocMem/FreeMem.
 *
 * Each module has its own StdCBase mempool, so a ralloc object allocated
 * in one module and freed in another corrupts the wrong pool's freelist.
 * Going straight to AllocMem/FreeMem makes the allocator pool-independent;
 * the magic cookie lets free() safely no-op a foreign pointer (e.g. one
 * libc allocated before this override took effect).
 *
 * Since the vc4gallium refactor the vc4 driver lives in mesa3dgl.library,
 * so this is only needed mesa-side; vc4gallium.hidd uses AllocMem directly.
 */
#define AROSC_MALLOC_MAGIC  0xCA113EDCu

struct aros_malloc_hdr
{
    ULONG  magic;
    ULONG  total;     /* allocation size including header */
    size_t user_size; /* size requested by caller */
};

#define HDR_BYTES AROS_ALIGN(sizeof(struct aros_malloc_hdr))

void *malloc(size_t size)
{
    ULONG total = HDR_BYTES + size;
    UBYTE *raw = (UBYTE *)AllocMem(total, MEMF_ANY);
    if (!raw)
    {
        errno = ENOMEM;
        return NULL;
    }
    struct aros_malloc_hdr *h = (struct aros_malloc_hdr *)raw;
    h->magic = AROSC_MALLOC_MAGIC;
    h->total = total;
    h->user_size = size;
    return raw + HDR_BYTES;
}

void free(void *ptr)
{
    if (!ptr)
        return;
    struct aros_malloc_hdr *h = (struct aros_malloc_hdr *)((UBYTE *)ptr - HDR_BYTES);
    if (h->magic != AROSC_MALLOC_MAGIC)
        return;
    h->magic = 0;
    FreeMem((UBYTE *)h, h->total);
}

void *calloc(size_t nmemb, size_t size)
{
    size_t user = nmemb * size;
    ULONG total = HDR_BYTES + user;
    UBYTE *raw = (UBYTE *)AllocMem(total, MEMF_ANY | MEMF_CLEAR);
    if (!raw)
    {
        errno = ENOMEM;
        return NULL;
    }
    struct aros_malloc_hdr *h = (struct aros_malloc_hdr *)raw;
    h->magic = AROSC_MALLOC_MAGIC;
    h->total = total;
    h->user_size = user;
    return raw + HDR_BYTES;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr)
        return malloc(size);
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }
    struct aros_malloc_hdr *h = (struct aros_malloc_hdr *)((UBYTE *)ptr - HDR_BYTES);
    if (h->magic != AROSC_MALLOC_MAGIC)
        return NULL;
    size_t old = h->user_size;
    void *np = malloc(size);
    if (np)
    {
        memcpy(np, ptr, old < size ? old : size);
        free(ptr);
    }
    return np;
}
 
/*
    The purpose of this file is to provide implementation for C functions part
    of arosnixc.library in code where one does not want to use this library.
*/

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

int usleep (useconds_t usec)
{
    IMPLEMENT();
    return 0;
}

/*
    atexit differs from the standard definition because AROS .library code
    AND data is shared across processes (unlike a shared object on a paging
    OS, whose data is per-process). Running atexit handlers at CloseLibrary
    would free shared data and crash other users, so we defer them to
    library expunge/exit.

    TODO: Check atexit() usage and determine best time to call the handlers.
*/

static struct exit_list {
    struct exit_list *next;
    void (*func)(void);
} *exit_list = NULL;

int atexit(void (*function)(void))
{
    struct exit_list *el;

    el = malloc(sizeof(*el));
    if (el == NULL)
        return -1;

    el->next = exit_list;
    el->func = function;
    exit_list = el;

    return 0;
}

void __exit_emul(void)
{
    while (exit_list) {
        struct exit_list *el = exit_list->next;

        exit_list->func();
        free(exit_list);
        exit_list = el;
    }
}

ADD2EXIT(__exit_emul, 0);
