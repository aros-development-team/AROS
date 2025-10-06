#ifndef _BASE_H_
#define _BASE_H_
/*
 * Usergroup library base. 
 */

#include "config.h"

#include <dos/dos.h>
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef EXEC_TASKS_H
#include <exec/tasks.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif
#ifndef DEVICES_NETINFO_H
#include <devices/netinfo.h>
#endif
#ifndef LIBRARIES_USERGROUP_H
#include <libraries/usergroup.h>
#endif

#ifdef USE_PRAGMAS
#include <clib/exec_protos.h>
#include <pragmas/exec_sysbase_pragmas.h>
extern struct ExecBase *SysBase;
#include <proto/dos.h>
#endif

#include <errno.h>
#include <utmp.h>

#ifndef MAXGRP
#define MAXGRP          200
#endif

#ifndef MAXLINELENGTH
#define MAXLINELENGTH   1024
#endif

typedef enum { es_byte, es_word, es_long } errnop_t;

struct UserGroupBase {
    struct Library      libNode;

    struct Task         *owner;
    STRPTR              _ProgramName;

    int                 errnolast;
    void                *errnop;
    errnop_t            errnosize;

    struct lastlog      ll;
    union {
        LONG            uttell;
        LONG            lltell;
    };
    union {
        short           already_read;
        short           utent_left;
    };
    BPTR                utfile;
    struct utmp         *utentbuf;
    struct utmp         *utent;

    short               setent_done;

    char                buffer[MAXLOGNAME];
    struct SignalSemaphore ni_lock;
    struct MsgPort      *niport;
    struct NetInfoReq   *nireq;
    struct Device       *nidevice[2];
    APTR                niunit[2];
    APTR                nibuffer[2];
#if (0)
    BPTR                u_pwd_fp;
    BPTR                u_grp_fp;
    char                u_pwd_line[MAXLINELENGTH];
#define u_grp_line u_pwd_line
    struct passwd       u_passwd;
    struct group        u_group;
    char                *u_members[MAXGRP];
#endif
    char                cryptresult[1+4+4+11+1];
};

/*
 * errno handling
 */
void ug_SetErrno(struct Library *, int);

/*
 * netinfo.device IO
 */
struct NetInfoReq *ug_OpenUnit(struct Library *, ULONG);
void ug_CloseUnit(struct Library *, ULONG);
BYTE ug_DoIO(struct NetInfoReq *);
void SetDeviceErr(struct Library *);

/*
 * utmp IO
 */
void CleanupUTMP(struct Library *);

/*
 * Random numbers 
 */
ULONG LRandom(void);
int LRandomInit(void);

/*
 * support 
 */

int TimeInit(struct Library *);
void TimeCleanup(struct Library *);

#endif /* _BASE_H_ */
