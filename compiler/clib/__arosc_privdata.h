#ifndef ___AROSC_PRIVDATA_H
#define ___AROSC_PRIVDATA_H

#include <exec/semaphores.h>
#include <devices/timer.h>
#include <proto/timer.h>
#include <dos/dos.h>

#include <sys/types.h>
#include <sys/arosc.h>

#include "etask.h"

struct arosc_privdata
{
    /* All stuff visible by the user */
    struct arosc_userdata acpd_acud;

    /* arosc_userdata can grow as much as it wishes,
       as long as all new fields are added at the end of it.

       arosc_privdata can also grow at please, but it has no restrictions
       on the way things are modified in it, as long as arosc_userdata
       is always kept at its beginning.  */

    /* Keep in here the pointer to the already existing arosc_privdata
       structure.

       This is a structure which gets allocated on a per-process basis,
       however CLI programs are invoked as sort of subroutines of the CLI
       process, although they must behave as if they were the only ones
       having possession of their Process structure. Hence, this field
       is used to save/restore the data of the previous CLI process.  */
    struct arosc_privdata *acpd_oldprivdata;

    /* malloc.c */
    void *acpd_startup_mempool;
    struct SignalSemaphore acpd_startup_memsem;

    /* __env.c */
    void *acpd_env_list;

    /* __stdio.c */
    struct MinList acpd_stdio_files;
    void   *acpd_stdfiles[3];

    /* clock.c */
    struct DateStamp acpd_startup_datestamp;

    /* __open.c */
    int    acpd_numslots;
    void **acpd_fd_array;

    /* atexit.c */
    struct MinList acpd_atexit_list;

    /* umask.c */
    mode_t acpd_umask;

    /* Used by chdir() */
    int  acpd_startup_cd_changed;
    BPTR acpd_startup_cd_lock;

    /* gettimeofday */
    struct timerequest  acpd_timereq;
    struct MsgPort      acpd_timeport;
    struct Device      *acpd_TimerBase;

    /* __arosc_usedata  */
    APTR acpd_process_returnaddr;
    ULONG acpd_usercount;

    /* __upath */
    char *acpd_apathbuf;  /* Buffer that holds the AROS path converted from the
                             equivalent *nix path.  */
    int   acpd_doupath;   /* BOOL - does the conversion need to be done?  */

    /* spawn* */
    char *acpd_joined_args;
    int   acpd_spawned;

    /* strerror */
    char acpd_fault_buf[100];

    /* __arosc_nixmain */
    int acpd_parent_does_upath;
};

#define __get_arosc_privdata() ((struct arosc_privdata *)__get_arosc_userdata())

#define __oldprivdata                         (__get_arosc_privdata()->acpd_oldprivdata)
#define __env_list          (*((__env_item **)(&(__get_arosc_privdata()->acpd_env_list))))
#define __stdio_files                         (__get_arosc_privdata()->acpd_stdio_files)
#define __numslots                            (__get_arosc_privdata()->acpd_numslots)
#define __fd_array          ((fdesc **)       (__get_arosc_privdata()->acpd_fd_array))
#define __startup_memsem                      (__get_arosc_privdata()->acpd_startup_memsem)
#define __startup_mempool   ((APTR)           (__get_arosc_privdata()->acpd_startup_mempool))
#define __startup_datestamp                   (__get_arosc_privdata()->acpd_startup_datestamp)
#define __stdfiles                            (__get_arosc_privdata()->acpd_stdfiles)
#define __atexit_list                         (__get_arosc_privdata()->acpd_atexit_list)
#define __umask                               (__get_arosc_privdata()->acpd_umask)
#define __startup_cd_changed                  (__get_arosc_privdata()->acpd_startup_cd_changed)
#define __startup_cd_lock                     (__get_arosc_privdata()->acpd_startup_cd_lock)
#define __timereq                             (__get_arosc_privdata()->acpd_timereq)
#define __timeport                            (__get_arosc_privdata()->acpd_timeport)
#define TimerBase                             (__get_arosc_privdata()->acpd_TimerBase)
#define __apathbuf                            (__get_arosc_privdata()->acpd_apathbuf)
#define __doupath                             (__get_arosc_privdata()->acpd_doupath)

#define __aros_startup          ((struct aros_startup *)GetIntETask(FindTask(NULL))->iet_startup)
#define __aros_startup_jmp_buf  (__aros_startup->as_startup_jmp_buf)
#define __aros_startup_error    (__aros_startup->as_startup_error)

#endif /* !___AROSC_PRIVDATA_H */
