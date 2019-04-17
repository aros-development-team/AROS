/*
    Copyright © 2009-2019, The AROS Development Team. All rights reserved.
    $Id: emul_arosc.c 56280 2019-04-17 18:42:57Z NicJA $
*/

#include <aros/debug.h>

#include <proto/dos.h>
#include <proto/timer.h>

#include <stdlib.h>
#include <unistd.h>

#define IMPLEMENT()  bug("------IMPLEMENT(%s)\n", __func__)
 
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
    This implementation of atexit is different than the definition of atexit
    function due to how libraries work in AROS.
   
    Under Linux, when an .so file is used by an application, the library's code
    is being shared but the library's data (global, static variables) are COPIED for
    each process. Then, an atexit call inside .so will only operate on COPY of data
    and thus can for example free memory allocated by one process without
    influencing other processes.
   
    Under AROS, when a .library file is used by an application, library code AND
    library data is shared. This means, an atexit call inside .library which was
    initially coded under Linux cannot be executed when process is finishing 
    (for example at CloseLibrary) because such call will most likely free shared 
    data which will make other processes crash. The best approximation of atexit
    in case of .library is to call the atexit functions at library expunge/exit.

    TODO: Check atexit() usage and determine best time to call atexit registered
    functions.
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
