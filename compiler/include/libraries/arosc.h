#ifndef _AROSC_H
#define _AROSC_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: macros for the shared library version of clib
    Lang: english
*/


#include <dos/dos.h>
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/semaphores.h>
#include <proto/exec.h>

#include <sys/types.h>

struct AroscUserData
{
    struct AroscUserData *olduserdata;

    /*
      This will be useful in case the structure will be extended.
      In this way the library will always know what can "offer" to the
      program
    */
    ULONG usersize;

    /* these fields are initialized by the program */
    void *errnoptr;
    void **stdinptr;
    void **stdoutptr;
    void **stderrptr;
    void *startup_jmp_bufptr;
    void *startup_errorptr;
    
    /* these fields are for internal use only */
    void *env_list;
    struct MinList stdio_files;
    struct SignalSemaphore startup_memsem;
    struct DateStamp startup_datestamp;
    void *startup_mempool;
    int numslots;
    void **fd_array;
    void *stdfiles[3];

    /* these fields are initialized internally and passed to the program */
    const unsigned short int *ctype_b;
    const int *ctype_toupper;
    const int *ctype_tolower;

    /*more stuff*/
    struct MinList atexit_list;
    mode_t umask;

    /* Used by chdir() */
    BOOL startup_cd_changed;
    BPTR startup_cd_lock;

    /* Used by multi-byte functions */
    int		*mb_cur_max;

    /* Used by time.h functions */
    int		*daylight;
    long int	*timezone;
    char	***tzname;
};

extern struct Library *aroscbase;

#define AROSCNAME "arosc.library"

#if defined(_CLIB_KERNEL_) || defined(_CLIB_LIBRARY_) || defined(_CLIB_USERDATA_)

#define CLIB_USES_ETASK 1

#if CLIB_USES_ETASK
#  include "etask.h"
#  define AROSC_USERDATA(task) (struct AroscUserData *)(GetIntETask(task==NULL ? FindTask(NULL) : task)->iet_AroscUserData)
#else
#  define AROSC_USERDATA(task) (struct AroscUserData *)(task==NULL ? FindTask(NULL) : task)->tc_UserData)
#endif

#define GETUSER struct AroscUserData *clib_userdata = AROSC_USERDATA(0)

#else

#define GETUSER const char clib_dummyvar __attribute__((unused))

#endif

#ifdef _CLIB_LIBRARY_
#define clib_userdata (AROSC_USERDATA(0))
#endif

#if defined(_CLIB_KERNEL_) || defined(_CLIB_LIBRARY_)
#define stdin               (*(FILE **)       (clib_userdata->stdinptr))
#define stdout              (*(FILE **)       (clib_userdata->stdoutptr))
#define stderr              (*(FILE **)       (clib_userdata->stderrptr))
#define __startup_jmp_buf   (*(jmp_buf *)     (clib_userdata->startup_jmp_bufptr))
#define __startup_error     (*(LONG *)        (clib_userdata->startup_errorptr))
#define __env_list          (*((__env_item **)(&(clib_userdata->env_list))))
#define __stdio_files                         (clib_userdata->stdio_files)
#define __numslots                            (clib_userdata->numslots)
#define __fd_array          ((fdesc **)       (clib_userdata->fd_array))
#define __startup_memsem                      (clib_userdata->startup_memsem)
#define __startup_mempool   ((APTR)           (clib_userdata->startup_mempool))
#define __startup_datestamp                   (clib_userdata->startup_datestamp)
#define __stdfiles                            (clib_userdata->stdfiles)
#define __atexit_list                         (clib_userdata->atexit_list)
#define __umask                               (clib_userdata->umask)
#define __startup_cd_changed                  (clib_userdata->startup_cd_changed)
#define __startup_cd_lock                     (clib_userdata->startup_cd_lock)
#define __mb_cur_max                          (clib_userdata->mb_cur_max)
#define	daylight                              (clib_userdata->daylight)
#define tzname		                      (clib_userdata->tzname)

/* Special, there is a type called timezone as well */
#define _timezone                             (clib_userdata->timezone)

#endif

#endif /*!_AROSC_H*/





