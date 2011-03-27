#ifndef __CON_HANDLER_INTERN_H
#define __CON_HANDLER_INTERN_H
/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

/* AROS includes */
#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <hidd/hidd.h>
#include <aros/asmcall.h>
#include <libraries/gadtools.h>

/*
** stegerg:
**
** if BETTER_WRITE_HANDLING is #defined then writes are sent to
** console.device in smaller parts (max. 256 bytes or upto next
** LINEFEED).
**
** NOTE: Could be problematic with control sequences in case of
**       the 256-Byte-Block write (write size is >256 but no LINE-
**       FEED was found in this first (or better actual) 256 bytes
**       to be written.
**
**/

#define BETTER_WRITE_HANDLING 	1
#define RMB_FREEZES_OUTPUT	1

#define CONTASK_STACKSIZE 	(AROS_STACKSIZE)
#define CONTASK_PRIORITY 	5

#define CONSOLEBUFFER_SIZE 	256
#define INPUTBUFFER_SIZE 	256
#define CMD_HISTORY_SIZE 	32
#define PASTEBUFSIZE            16384

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
    struct Device     * inputbase;
};

struct filehandle
{
    struct IOStdReq	*conreadio;
    struct IOStdReq	conwriteio;
    struct MsgPort	*conreadmp;
    struct MsgPort	*conwritemp;
    struct Window	*window;
    struct Menu         *menu;
    void                *vi; /* visual-info required by GadTools */
    struct Task 	*contask;
    struct Task		*breaktask;
    struct Task		*lastwritetask;
    struct MsgPort      *contaskmp;
    struct MinList	pendingReads;
    struct MinList	pendingWrites;
    struct NewWindow	nw;
    UBYTE		*wintitle;
    UBYTE               *screenname;
#if BETTER_WRITE_HANDLING
    LONG		partlywrite_actual;
    LONG		partlywrite_size;
#endif
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

  /* If pastebuffer != 0, this contains data to paste from ConClip */
    WORD                pastebufferpos;
    WORD                pastebuffersize;
    STRPTR              pastebuffer;
};

/* filehandle flags */

#define FHFLG_READPENDING   	1
#define FHFLG_WRITEPENDING  	2
#define FHFLG_CANREAD       	4
#define FHFLG_WAIT	    	8   /* filename contained WAIT */
#define FHFLG_RAW   	    	16  /* in RAW mode */
#define FHFLG_ASYNCCONSOLEREAD	32  /* There is a pending async console.device CMD_READ request */
#define FHFLG_AUTO  	    	64  /* filename contained AUTO */
#define FHFLG_CONSOLEDEVICEOPEN 128
#define FHFLG_EOF   	    	256
#define FHFLG_NOECHO            512  /* input echoing is disabled */
#define FHFLG_NOWRITE           1024 /* disable do_write() completely, no output */

typedef struct IntuitionBase IntuiBase;

/* FIXME: Remove these #define xxxBase hacks
   Do not use this in new code !
*/
#ifdef InputBase
#   undef InputBase
#endif
#define InputBase conbase->inputbase

AROS_UFP3(VOID, conTaskEntry,
    AROS_UFPA(STRPTR, argstr, A0),
    AROS_UFPA(ULONG, arglen, D0),
    AROS_UFPA(struct ExecBase *, sysbase, A6)
);

#endif /* __CON_HANDLER_INTERN_H */
