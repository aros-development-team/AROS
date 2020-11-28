/*
  Copyright (C) 2014 Szilard Biro

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

static void StarterFunc(void)
{
    ThreadInfo *inf;
    int i, j;
    int foundkey = TRUE;
#ifdef USE_ASYNC_CANCEL
    APTR oldexcept;
#endif

    DB2(bug("%s()\n", __FUNCTION__));

    inf = (ThreadInfo *)FindTask(NULL)->tc_UserData;
    // trim the name
    //inf->task->tc_Node.ln_Name[inf->oldlen];

    // we have to set the priority here to avoid race conditions
    SetTaskPri(inf->task, inf->attr.param.sched_priority);

#ifdef USE_ASYNC_CANCEL
    // set the exception handler for async cancellation
    oldexcept = inf->task->tc_ExceptCode;
#ifdef __AROS__
    inf->task->tc_ExceptCode = &AROS_ASMSYMNAME(CancelHandler);
#else
    inf->task->tc_ExceptCode = &CancelHandler;
#endif
    SetExcept(SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C);
#endif

    // set a jump point for pthread_exit
    if (!setjmp(inf->jmp))
    {
        // custom stack requires special handling
        if (inf->attr.stackaddr != NULL && inf->attr.stacksize > 0)
        {
            struct StackSwapArgs swapargs;
            struct StackSwapStruct stack; 

            swapargs.Args[0] = (IPTR)inf->arg;
            stack.stk_Lower = inf->attr.stackaddr;
            stack.stk_Upper = (APTR)((IPTR)stack.stk_Lower + inf->attr.stacksize);
            stack.stk_Pointer = stack.stk_Upper; 

            inf->ret = (void *)NewStackSwap(&stack, inf->start, &swapargs);
        }
        else
        {
            inf->ret = inf->start(inf->arg);
        }
    }

#ifdef USE_ASYNC_CANCEL
    // remove the exception handler
    SetExcept(0, SIGBREAKF_CTRL_C);
    inf->task->tc_ExceptCode = oldexcept;
#endif

    // destroy all non-NULL TLS key values
    // since the destructors can set the keys themselves, we have to do multiple iterations
    ObtainSemaphoreShared(&tls_sem);
    for (j = 0; foundkey && j < PTHREAD_DESTRUCTOR_ITERATIONS; j++)
    {
        foundkey = FALSE;
        for (i = 0; i < PTHREAD_KEYS_MAX; i++)
        {
            if (tlskeys[i].used && tlskeys[i].destructor && inf->tlsvalues[i])
            {
                void *oldvalue = inf->tlsvalues[i];
                inf->tlsvalues[i] = NULL;
                tlskeys[i].destructor(oldvalue);
                foundkey = TRUE;
            }
        }
    }
    ReleaseSemaphore(&tls_sem);

    if (!inf->detached)
    {
        // tell the parent thread that we are done
        Forbid();
        inf->finished = TRUE;
        Signal(inf->parent, SIGF_PARENT);
    }
    else
    {
        // no one is waiting for us, do the clean up
        ObtainSemaphore(&thread_sem);
        memset(inf, 0, sizeof(ThreadInfo));
        ReleaseSemaphore(&thread_sem);
    }
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg)
{
    ThreadInfo *inf;
    char name[NAMELEN];
    size_t oldlen;
    pthread_t threadnew;

    D(bug("%s(%p, %p, %p, %p)\n", __FUNCTION__, thread, attr, start, arg));

    if (thread == NULL || start == NULL)
        return EINVAL;

    ObtainSemaphore(&thread_sem);

    // grab an empty thread slot
    threadnew = GetThreadId(NULL);
    if (threadnew == PTHREAD_THREADS_MAX)
    {
        ReleaseSemaphore(&thread_sem);
        return EAGAIN;
    }

    // prepare the ThreadInfo structure
    inf = GetThreadInfo(threadnew);
    memset(inf, 0, sizeof(ThreadInfo));
    inf->start = start;
    inf->arg = arg;
    inf->parent = FindTask(NULL);
    if (attr)
        inf->attr = *attr;
    else
        pthread_attr_init(&inf->attr);
    NEWLIST((struct List *)&inf->cleanup);
    inf->cancelstate = PTHREAD_CANCEL_ENABLE;
    inf->canceltype = PTHREAD_CANCEL_DEFERRED;

    // let's trick CreateNewProc into allocating a larger buffer for the name
    snprintf(name, sizeof(name), "pthread thread #%d", threadnew);
    oldlen = strlen(name);
    memset(name + oldlen, ' ', sizeof(name) - oldlen - 1);
    name[sizeof(name) - 1] = '\0';

    // start the child thread
    inf->task = (struct Task *)CreateNewProcTags(NP_Entry, StarterFunc,
#ifdef __MORPHOS__
        NP_CodeType, CODETYPE_PPC,
        (inf->attr.stackaddr == NULL && inf->attr.stacksize > 0) ? NP_PPCStackSize : TAG_IGNORE, inf->attr.stacksize,
#else
        (inf->attr.stackaddr == NULL && inf->attr.stacksize > 0) ? NP_StackSize : TAG_IGNORE, inf->attr.stacksize,
#endif
        NP_UserData, inf,
        NP_Name, name,
        TAG_DONE);

    if (!inf->task)
    {
        inf->parent = NULL;
        ReleaseSemaphore(&thread_sem);
        return EAGAIN;
    }

    ReleaseSemaphore(&thread_sem);

    *thread = threadnew;

    return 0;
}
