#ifndef __CON_HANDLER_INTERN_H
#define __CON_HANDLER_INTERN_H
/*
    Copyright (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

/* AROS includes */
#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <hidd/hidd.h>

/* POSIX includes */
#include <sys/types.h>
#include <dirent.h>

#define CONTASK_STACKSIZE 8192
#define CONTASK_PRIORITY 0

#define CONSOLEBUFFER_SIZE 256
#define INPUTBUFFER_SIZE 256
#define CMD_HISTORY_SIZE 32

struct conTaskParams
{
    struct conbase *conbase;
    struct Task	*parentTask;
    struct IOFileSys *iofs;
    ULONG initSignal;
};

struct conbase
{
    struct Device	device;
    struct ExecBase   * sysbase;
    struct DosLibrary * dosbase;
    struct IntuitionBase *intuibase;

    BPTR seglist;
};

struct filehandle
{
    struct IOStdReq	*conreadio;
    struct IOStdReq	conwriteio;
    struct MsgPort	*conreadmp;
    struct MsgPort	*conwritemp;
    struct Window	*window;
    struct Task 	*contask;
    struct MsgPort      *contaskmp;
    struct MinList	pendingReads;
    struct MinList	pendingWrites;
    UBYTE		*wintitle;
    WORD		conbufferpos;
    WORD		conbuffersize;
    WORD		inputstart; 	/* usually 0, but needed for multi-lines (CONTROL RETURN) */
    WORD		inputpos; 	/* cursor pos. inside line */
    WORD		inputsize; 	/* length of input string */
    WORD		canreadsize;
    WORD		historysize;
    WORD		historypos;
    WORD		historyviewpos;
    WORD		usecount;
    UWORD		flags;

    UBYTE		consolebuffer[CONSOLEBUFFER_SIZE + 2];
    UBYTE		inputbuffer[INPUTBUFFER_SIZE + 2];
    UBYTE		historybuffer[CMD_HISTORY_SIZE][INPUTBUFFER_SIZE + 1];

};

/* filehandle flags */

#define FHFLG_READPENDING  1
#define FHFLG_WRITEPENDING 2
#define FHFLG_CANREAD      4
#define FHFLG_WAIT	   8 	/* filename contained WAIT */

typedef struct IntuitionBase IntuiBase;

#ifdef SysBase
#   undef SysBase
#endif
#define SysBase conbase->sysbase
#ifdef DOSBase
#   undef DOSBase
#endif
#define DOSBase conbase->dosbase
#ifdef IntuitionBase
#   undef IntuitionBase
#endif
#define IntuitionBase conbase->intuibase

VOID conTaskEntry(struct conTaskParams *param);

#endif /* __CON_HANDLER_INTERN_H */
