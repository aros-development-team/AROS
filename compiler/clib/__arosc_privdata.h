#ifndef ___AROSC_PRIVDATA_H
#define ___AROSC_PRIVDATA_H

#include <exec/semaphores.h>
#include <devices/timer.h>
#include <proto/timer.h>
#include <dos/dos.h>

#include <sys/types.h>
#include <sys/arosc.h>

#include "etask.h"
#include "__vfork.h"

struct _fdesc;
struct __env_item;

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
    APTR acpd_mempool;
    struct SignalSemaphore acpd_memsem;

    /* __env.c */
    struct __env_item *acpd_env_list;

    /* __stdio.c */
    struct MinList acpd_stdio_files;

    /* clock.c */
    struct DateStamp acpd_datestamp;

    /* __open.c */
    APTR acpd_fd_mempool;
    int    acpd_numslots;
    struct _fdesc **acpd_fd_array;

    /* atexit.c */
    struct MinList acpd_atexit_list;

    /* umask.c */
    mode_t acpd_umask;

    /* Used by chdir() */
    int  acpd_cd_changed;
    BPTR acpd_cd_lock;

    /* gettimeofday */
    struct timerequest  acpd_timereq;
    struct MsgPort      acpd_timeport;
    struct Device      *acpd_TimerBase;
    LONG                acpd_gmtoffset;

    /* __arosc_usedata  */
    APTR  acpd_process_returnaddr;
    ULONG acpd_usercount;

    /* __upath */
    char *acpd_apathbuf;  /* Buffer that holds the AROS path converted from the
                             equivalent *nix path.  */
    int   acpd_doupath;   /* BOOL - does the conversion need to be done?  */

    /* spawn* */
    char *acpd_joined_args;
    int   acpd_flags;

    /* strerror */
    char acpd_fault_buf[100];

    /* __arosc_nixmain */
    int acpd_parent_does_upath;

    /* flock.c */
    struct MinList acpd_file_locks;
    
    /* __vfork.c */
    struct vfork_data *acpd_vfork_data;
    
    /* __exec.c */
    BPTR acpd_exec_seglist;
    char *acpd_exec_args;
    char *acpd_exec_taskname;
    char **acpd_exec_tmparray;
    BPTR acpd_exec_oldin, acpd_exec_oldout, acpd_exec_olderr;
    struct Library *acpd_exec_aroscbase;
};

/* acpd_flags */

/* By default arosc.library keeps the old arosc_privdata if it's opened another
   time in the same process. Setting this flag forces it to create new 
   arosc_privdata. */
#define CREATE_NEW_ACPD 1

/* Programs compiled with -nix flag are cloning dos.library environment 
   variables before execution and restoring them during exit. Cloning 
   prevents clobbering the process environment variables by another program
   ran with RunCommand() or similar means. Setting this flag disables
   environment cloning. */
#define DO_NOT_CLONE_ENV_VARS 2

/* This flag is set by vfork() to correctly report child process ID during
   execution of child code, even though that it's actually executed by parent
   process until execve() is called. */
#define PRETEND_CHILD 4

/* By default arosc.library creates new arosc_privdata when opened if 
   pr_ReturnAddr has changed (for example during RunCommand()). Setting 
   this flag prevents creation of new arosc_privdata. */
#define KEEP_OLD_ACPD 8

/* By default a new process will get new ACPD when it(or any other library
   it uses) opens arosc.library. This flag prohibits that and forces the
   child process to share ACPD with parent process */
#define SHARE_ACPD_WITH_CHILD 16

/* !acpd_flags */

#define __get_arosc_privdata() ((struct arosc_privdata *)__get_arosc_userdata())

#define __oldprivdata                         (__get_arosc_privdata()->acpd_oldprivdata)
#define __env_list                            (__get_arosc_privdata()->acpd_env_list)
#define __stdio_files                         (__get_arosc_privdata()->acpd_stdio_files)
#define __numslots                            (__get_arosc_privdata()->acpd_numslots)
#define __fd_mempool                          (__get_arosc_privdata()->acpd_fd_mempool)
#define __fd_array                            (__get_arosc_privdata()->acpd_fd_array)
#define __memsem                              (__get_arosc_privdata()->acpd_memsem)
#define __mempool                             (__get_arosc_privdata()->acpd_mempool)
#define __datestamp                           (__get_arosc_privdata()->acpd_datestamp)
#define __atexit_list                         (__get_arosc_privdata()->acpd_atexit_list)
#define __umask                               (__get_arosc_privdata()->acpd_umask)
#define __cd_changed                          (__get_arosc_privdata()->acpd_cd_changed)
#define __cd_lock                             (__get_arosc_privdata()->acpd_cd_lock)
#define __timereq                             (__get_arosc_privdata()->acpd_timereq)
#define __timeport                            (__get_arosc_privdata()->acpd_timeport)
#define TimerBase                             (__get_arosc_privdata()->acpd_TimerBase)
#define __apathbuf                            (__get_arosc_privdata()->acpd_apathbuf)
#define __doupath                             (__get_arosc_privdata()->acpd_doupath)
#define __flocks_list                         (__get_arosc_privdata()->acpd_file_locks)
#define __gmtoffset                           (__get_arosc_privdata()->acpd_gmtoffset)

#define __aros_startup          ((struct aros_startup *)GetIntETask(FindTask(NULL))->iet_startup)
#define __aros_startup_jmp_buf  (__aros_startup->as_startup_jmp_buf)
#define __aros_startup_error    (__aros_startup->as_startup_error)

#endif /* !___AROSC_PRIVDATA_H */
