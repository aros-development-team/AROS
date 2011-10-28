#ifndef ___AROSC_PRIVDATA_H
#define ___AROSC_PRIVDATA_H

#include <aros/relbase.h>
#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <devices/timer.h>
#include <proto/timer.h>
#include <dos/dos.h>
#include <aros/cpu.h>

#include <sys/types.h>
#include <sys/arosc.h>

#include "etask.h"
#include "__vfork.h"

struct _fdesc;
struct __env_item;

struct aroscbase
{
    struct Library acb_library;
    
    struct arosc_userdata acb_acud;
    
    /* arosc_userdata can grow as much as it wishes,
       as long as all new fields are added at the end of it.

       arosc_privdata can also grow at please, but it has no restrictions
       on the way things are modified in it, as long as arosc_userdata
       is always kept at its beginning.  */

    /* malloc.c */
    APTR acb_mempool;

    /* __env.c */
    struct __env_item *acb_env_list;

    /* __stdio.c */
    struct MinList acb_stdio_files;

    /* clock.c */
    struct DateStamp acb_datestamp;

    /* __open.c */
    APTR acb_fd_mempool;
    int acb_numslots;
    struct _fdesc **acb_fd_array;

    /* atexit.c */
    struct MinList acb_atexit_list;

    /* umask.c */
    mode_t acb_umask;

    /* Used by chdir() */
    int  acb_cd_changed;
    BPTR acb_cd_lock;

    /* gettimeofday */
    struct timerequest  acb_timereq;
    struct MsgPort      acb_timeport;
    LONG                acb_gmtoffset;

    /* __arosc_usedata  */
    APTR  acb_process_returnaddr;
    ULONG acb_usercount;

    /* __upath */
    char *acb_apathbuf;  /* Buffer that holds the AROS path converted from the
                             equivalent *nix path.  */
    int   acb_doupath;   /* BOOL - does the conversion need to be done?  */

    /* spawn* */
    char *acb_joined_args;
    int   acb_flags;

    /* strerror */
    char acb_fault_buf[100];

    /* __arosc_nixmain */
    int acb_parent_does_upath;

    /* flock.c */
    struct MinList acb_file_locks;
    
    /* __vfork.c */
    struct vfork_data *acb_vfork_data;
    
    /* __exec.c */
    BPTR acb_exec_seglist;
    char *acb_exec_args;
    char *acb_exec_taskname;
    APTR acb_exec_pool;
    char **acb_exec_tmparray;
    BPTR acb_exec_oldin, acb_exec_oldout, acb_exec_olderr;
    struct Library *acb_exec_aroscbase;
};

/* acb_flags */

/* When a program is started with the exec functions and from vfork,
   this is indicated in the flags of the library.
   This way the child can use the parent arosc library during its initialization
   phase */
#define EXEC_PARENT 1
#define VFORK_PARENT 2

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
/* FIXME: SHARE_ACPD_WITH_CHILD not implemented, is it still needed ?
   aroscbase can now be used from different tasks without the need
   for SHARE_ACPD_WITH_CHILD
*/
#define SHARE_ACPD_WITH_CHILD 16

/* !acb_flags */

struct aroscbase *__GM_GetBase();
#define __get_aroscbase() __GM_GetBase()

#define __env_list                            (__get_aroscbase()->acb_env_list)
#define __stdio_files                         (__get_aroscbase()->acb_stdio_files)
#define __numslots                            (__get_aroscbase()->acb_numslots)
#define __fd_mempool                          (__get_aroscbase()->acb_fd_mempool)
#define __fd_array                            (__get_aroscbase()->acb_fd_array)
#define __mempool                             (__get_aroscbase()->acb_mempool)
#define __datestamp                           (__get_aroscbase()->acb_datestamp)
#define __atexit_list                         (__get_aroscbase()->acb_atexit_list)
#define __umask                               (__get_aroscbase()->acb_umask)
#define __cd_changed                          (__get_aroscbase()->acb_cd_changed)
#define __cd_lock                             (__get_aroscbase()->acb_cd_lock)
#define __timereq                             (__get_aroscbase()->acb_timereq)
#define __timeport                            (__get_aroscbase()->acb_timeport)
#define __gmtoffset                           (__get_aroscbase()->acb_gmtoffset)
#define __apathbuf                            (__get_aroscbase()->acb_apathbuf)
#define __doupath                             (__get_aroscbase()->acb_doupath)
#define __flocks_list                         (__get_aroscbase()->acb_file_locks)

#endif /* !___AROSC_PRIVDATA_H */
