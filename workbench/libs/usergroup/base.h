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
    int                 internal_errno;
    void                *errnop;
    errnop_t            errnosize;
    struct utmp         utentbuf[1];
    struct lastlog      ll;
    LONG                lltell;
    short               already_read;
    char                buffer[MAXLOGNAME];
    /*struct MsgPort    *niport;
    struct NetInfoReq   *nireq;
    struct Device       *nidevice[2];
    APTR                niunit[2];
    APTR                nibuffer[2];*/
    BPTR                u_pwd_fp;
    BPTR                u_grp_fp;
    char                u_pwd_line[MAXLINELENGTH];
#define u_grp_line u_pwd_line
    struct passwd       u_passwd;
    struct group        u_group;
    char                *u_members[MAXGRP];
    char                cryptresult[1+4+4+11+1];
};

/*
 * errno handling
 */
void SetErrno(int);

/*
 * netinfo.device IO
 */
extern struct SignalSemaphore ni_lock[];
struct NetInfoReq *OpenNIUnit(ULONG);
void CloseNIUnit(ULONG);
BYTE myDoIO(struct NetInfoReq *);
void SetDeviceErr(void);

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

int TimeInit(void);
void TimeCleanup(void);
 
static __inline void InitList(struct List *list)
{
    list->lh_Head = (struct Node*)&list->lh_Tail;
    list->lh_Tail = NULL;
    list->lh_TailPred = (struct Node*)&list->lh_Head;
}

#endif /* _BASE_H_ */
