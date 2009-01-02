/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>

#include <kern/amiga_includes.h>
#include <syslog.h>
#include <kern/amiga_cx.h>
#include <kern/amiga_dhcp.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_log.h>
#include <kern/amiga_rexx.h>
#include <libraries/miamipanel.h>
#include <utility/date.h>

#include <proto/miami.h>

#include <proto/dos.h>
#include <proto/miamipanel.h>
#include <proto/utility.h>
#define DOSBase logDOSBase
#define UtilityBase logUtilityBase
#undef DOSBase

struct MsgPort *logPort;
struct MsgPort *ExtLogPort;

struct Library *logDOSBase = NULL;
struct Library *logUtilityBase = NULL;

extern struct Task *AROSTCP_Task;
extern struct DosLibrary *DOSBase;

extern void REGARGFUN stuffchar();
extern int logname_changed(void *p, LONG new);

static struct SysLogPacket *log_poll(void);
static void log_task(void);
static void log_close(struct SysLogPacket *msg);
static BPTR logOpen(STRPTR name);

static struct SysLogPacket *log_messages = NULL;
static char *log_buffers = NULL;
static LONG log_messages_mem_size, log_buffers_mem_size;
static int GetLogMsgFail;


UBYTE consoledefname[] = "con:0/0/640/200/AROSTCP syslog/AUTO/INACTIVE";
extern UBYTE logfiledefname[];
STRPTR logfilename = logfiledefname;
STRPTR consolename = consoledefname;

struct log_cnf log_cnf = { LOG_BUFS, LOG_BUF_LEN, LOG_INFO};

LONG OpenGUIOnStartup = 1;

#define SOCKET_VERSION 3

struct SignalSemaphore LogLock;
struct MsgPort logReplyPort;

/*
 * Initialization function for the logging subsystem
 */

BOOL 
log_init(void)
{
  struct Message *msg;
  int i;
  ULONG sig;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_log.c) log_init()\n"));
#endif
/*  if (logReplyPort)
    return(TRUE);*/		  /* We're allready initialized */
  InitSemaphore(&LogLock);
  
  /*
   * Allocate buffers for log messages.
   * 
   * Save lengths to static variables, since the configuration variables might
   * change.
   */
  log_messages_mem_size = sizeof(struct SysLogPacket) * log_cnf.log_bufs;
  log_buffers_mem_size = log_cnf.log_bufs * log_cnf.log_buf_len * sizeof(char);
  if (log_messages = AllocMem(log_messages_mem_size, MEMF_CLEAR|MEMF_PUBLIC))
    if (log_buffers = AllocMem(log_buffers_mem_size, MEMF_CLEAR|MEMF_PUBLIC)) {
        logPort = NULL; /* NETTRACE will set this on success */
        GetLogMsgFail = 0;
		
#if defined(__AROS__)
		DSYSCALLS(__log(LOG_DEBUG,"Set initial SysLogPort");)
		ExtLogPort = FindPort("SysLog");
#else
        SetSysLogPort();
#endif
	logReplyPort.mp_Flags = PA_SIGNAL;
	logReplyPort.mp_SigBit = SIGBREAKB_CTRL_E;
	logReplyPort.mp_SigTask = FindTask(NULL);
	NewList(&logReplyPort.mp_MsgList);
	/*
	 * Start the NETTRACE process
	 */
D(Printf("Creating NETTRACE process...");)
        
#ifdef __MORPHOS__
	if (CreateNewProcTags(NP_Entry, (LONG)&log_task,
			      NP_Name, (LONG)LOG_TASK_NAME,
			      NP_Priority, LOG_TASK_PRI,
                  NP_CodeType,CODETYPE_PPC,
			      TAG_DONE, NULL))
#else
	if (CreateNewProcTags(NP_Entry, (LONG)&log_task,
			      NP_Name, (LONG)LOG_TASK_NAME,
			      NP_Priority, LOG_TASK_PRI,
			      TAG_DONE, NULL))
#endif
  {
	  for (;;) {
	    /*
	     * Wait for a signal for success or failure
	     */
	    sig = Wait(SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F);
            D(Printf(" got some reply...");)

	    if (sig & SIGBREAKF_CTRL_F && logPort == (struct MsgPort *)-1) {
	      /* Initializion failed */
              D(Printf(" initialization failed!\n");)
	      logPort = NULL;
	      break;
	    }
	    else if (msg = GetMsg(&logReplyPort)) { /* Got message back? */
              D(Printf(" done!\n");)
	      ReplyMsg(msg);
	      logReplyPort.mp_Flags = PA_IGNORE;
	      /* 
	       * Initialize messages
	       */
	      for (i = 0; i < log_cnf.log_bufs; i++) {
		D(Printf("Initialising log message %ld\n",i);)
		log_messages[i].Msg.mn_ReplyPort = &logReplyPort;
		log_messages[i].Msg.mn_Length = sizeof(struct SysLogPacket);
		log_messages[i].Level = 0;
		log_messages[i].String = log_buffers+i*log_cnf.log_buf_len;
		PutMsg(&logReplyPort, (struct Message *)&log_messages[i]);
	      }
              D(Printf("Finished!\n");)
	      return(TRUE);	/* We're done */
	    }
	  }
	}
    }
  /*
   * Something went wrong
   */
  log_deinit();
  return(FALSE);
}

/* A little stub for calling GetMsg w/ error reporting and cheking */
struct SysLogPacket *GetLogMsg(struct MsgPort *port)
{
  struct Message *msg;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_log.c) GetLogMsg()\n"));
#endif

  /* We should have a port, if not-> fail */
  if (port) {
    ObtainSemaphore(&LogLock);
    for (;;) {
      if (msg = GetMsg(port)) {  /* Get a message */
	SetSignal(0, SIGBREAKF_CTRL_E);
	ReleaseSemaphore(&LogLock);
        return (struct SysLogPacket *)msg;
      }
      port->mp_SigTask = FindTask(NULL);
      port->mp_Flags = PA_SIGNAL;
      WaitPort(port);
      port->mp_Flags = PA_IGNORE;
    }
  }
  DNETTRACE(else KPrintF("WARNING!!! port = NULL!\n");)

  ++GetLogMsgFail;		/* Increment number of failed messages */
  return NULL;
}

/*
 * This function may be called only if no other tasks (applications) are 
 * accessing the logging system (the messages, to be exact).
 */
void 
log_deinit(void)
{
  struct SysLogPacket *msg, *dump;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_log.c) log_deinit()\n"));
#endif

  D(Printf("log_deinit() called\n");)
  if (logPort) {	      /* Logport exists? (=> NETTRACE is running) */

      /*
       * Get a free message, Wait() for it if necessary
       */
      msg = GetLogMsg(&logReplyPort);
      D(Printf("Got next log message\n");)

      /* 
       * Initalize END_MESSAGE
       */
      msg->Msg.mn_ReplyPort = &logReplyPort;
      msg->Msg.mn_Length = sizeof(struct SysLogPacket);
      msg->Level = LOG_CLOSE;

      PutMsg(logPort, (struct Message *)msg);
      D(Printf("Sent LOG_CLOSE\n");)
      
      for (;;) {
	dump = GetLogMsg(&logReplyPort);
	D(Printf("Got next log message\n");)
	if (dump->Level == LOG_CLOSE) { /* got the Close message back */
	    D(Printf("LOG_CLOSE returned\n");)
	    break;		/* It was the last one */
	}
      }
  }
  if (log_buffers) {
    D(Printf("Freeing log_buffers\n");)
    FreeMem(log_buffers, log_buffers_mem_size);
    log_buffers = NULL;
  }
  if (log_messages) {
    D(Printf("Freeing log_messages\n");)
    FreeMem(log_messages, log_messages_mem_size);
    log_messages = NULL;
  }
}

/*
 * Functions following these defines may be called from the NETTRACE
 * task ONLY! These defines cause the SAS/C to generate calls to
 * dos.library and utility.library using these bases, respectively.
 */
#define DOSBase logDOSBase
#define UtilityBase logUtilityBase

struct Library *SocketBase = NULL;
struct Task *Nettrace_Task = NULL;

/* Main loop for NETTRACE */
static void SAVEDS
log_task(void)
{
  struct SysLogPacket *initmsg = NULL;
  ULONG rexxmask = 0, logmask = 0, sigmask = 0, cxmask = 0;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_log.c) log_task()\n"));
#endif

  DNETTRACE(KPrintF("NETTRACE started\n");)
  Nettrace_Task = FindTask(NULL); /* Store task pointer for AmiTCP */

  /* We need our own DosBase */
  if ((logDOSBase = OpenLibrary(DOSNAME, 0L)) == NULL)
    goto fail;

  if ((logUtilityBase = OpenLibrary("utility.library", 37L)) == NULL)
    goto fail;

  /* Allocate message to reply startup */
  if ((initmsg = AllocMem(sizeof(struct SysLogPacket), MEMF_CLEAR|MEMF_PUBLIC))
      == NULL)
    goto fail;

  /* Create our port for log messages */
  if ((logPort = CreateMsgPort()) == NULL)
    goto fail;

  logmask = 1<<logPort->mp_SigBit;

  /*
   * Initialize rexx subsystem
   */
  if (!(rexxmask = rexx_init()))
    goto fail;

  /*
   * Initialize Commodities Exchange subsystem
   */
  if (!(cxmask = cx_init()))
    goto fail;

  /*
   * Syncronize with AmiTCP/IP
   */
  DNETTRACE(KPrintF("NETTRACE initialization complete, sending message...");)
  initmsg->Msg.mn_ReplyPort = logPort;
  initmsg->Msg.mn_Length = sizeof(struct SysLogPacket);
  PutMsg(&logReplyPort, (struct Message *)initmsg);
  do {
    Wait(logmask);
  } while(initmsg != (struct SysLogPacket *)GetMsg(logPort));
#if !defined(__AROS__)
  D(KPrintF(" done!\n");)
#endif

  FreeMem(initmsg, sizeof(struct SysLogPacket));
  initmsg = NULL;

  sigmask = logmask | rexxmask | cxmask | SIGBREAKF_CTRL_F | SIGBREAKF_CTRL_C;
  
  /* 
   * Main loop of the NETTRACE
   */
  for (;;) {
    ULONG sig;
    struct SysLogPacket *msg;

    sig = Wait(sigmask | guimask);	  /* Wait for signals */
    DNETTRACE(KPrintF("NETTRACE: got some signal...");)
    do {
				/* Signal from the AmiTCP/IP: API ready */
      if ((sig & SIGBREAKF_CTRL_F) && (SocketBase == NULL)) {
	sig &= ~SIGBREAKF_CTRL_F;
#if !defined(__AROS__)
        D(KPrintF(" bsdsocket API ready\n");)
#endif
	/*
	 * Open a base to our own library so that ARexx message handling
	 * can use socket functions.
	 * This name does not work with the "nthLibrary" system
	 */
	if (SocketBase = OpenLibrary("bsdsocket.library", SOCKET_VERSION)) {
	  /*
	   * Make our ARexx port public
	   */
	  rexx_show();
	  /*
	   * Make our Commodities Exchange object public
	   */
	  cx_show();
	  /*
	   * Execute delayed DHCP startup
	   */
	  run_dhcp();
	  /*
	   * Open control panel
	   */
	  if (OpenGUIOnStartup)
	    gui_open();
	  sigmask &= ~SIGBREAKF_CTRL_F;
	}
      }
      if (sig & SIGBREAKF_CTRL_C) {
	/*
	 * Forward CTRL-C to our main task
	 */
	Signal(AROSTCP_Task, SIGBREAKF_CTRL_C);
	sig &= ~SIGBREAKF_CTRL_C;
      }

      if (sig & logmask) {	/* Process log messages,
				 * handles all ones pending.
				 */
	DNETTRACE(KPrintF(" log message\n");)
	if (msg = log_poll()) {	/* Got an LOG_CLOSE-message? */
	  DNETTRACE(KPrintF("Got LOG_CLOSE, exiting.\n");)
	  log_close(msg);
	  return;
	}
	sig &= ~logmask;
      }

      if (sig & rexxmask) {	  /* One rexx message at time */
	DNETTRACE(KPrintF(" REXX message\n");)
	if (SocketBase) {
	  if (!rexx_poll()) 
	    sig &= ~rexxmask;
	} else
	  sig &= ~rexxmask;
      }

      if (sig & cxmask) {	  /* One Exchange message at time */
	DNETTRACE(KPrintF(" CX message\n");)
	if (!cx_poll())
	  sig &= ~cxmask;
      }

      if (sig & guimask) {
	DNETTRACE(KPrintF(" GUI signal\n");)
	if (GUI_Running)
	  MiamiPanelEvent(sig & guimask);
	sig &= ~guimask;
      }

      sig |= SetSignal(0L, sigmask) & sigmask; /* Signals left? */
    } while (sig);
  }

 fail:				
  /* Initializion Failed */
  if (initmsg)
    FreeMem(initmsg, sizeof(struct SysLogPacket));

  log_close(NULL);

  logPort = (struct MsgPort *)-1;
  /* Inform AmiTCP that we failed */
  Signal(AROSTCP_Task, SIGBREAKF_CTRL_F); 
}

#ifdef CONSOLE_SYSLOG
static BPTR confile = NULL;
static BOOL conopenfail = FALSE;
static BOOL conwritefail = FALSE;
#endif

static BPTR logfile = NULL;
static BOOL fileopenfail = FALSE;
static BOOL filewritefail = FALSE;

static char *months =
  "Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec";

static char *wdays = 
  "Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat";

static char *levels = 
  "emerg\0"
  "alert\0"
  "crit \0"
  "err  \0"
  "warn \0"
  "note \0"
  "info \0"
  "debug";

void log_msg(struct SysLogPacket *msg)
{
      LONG i;
      int chars;
      struct ClockData clockdata;
      struct CSource cs;

      /* 28 for date, 14 for level */
# define LEVELBUF 28+14

      UBYTE buffer[LEVELBUF];

      cs.CS_Buffer = buffer;
      cs.CS_Length = LEVELBUF;
      cs.CS_CurChr = 0;

      Amiga2Date(msg->Time, &clockdata);

      csprintf(&cs,
#ifdef HAVE_TIMEZONES
	     "%s %s %02d %02d:%02d:%02d %s %4d [%s]: ",
#else
	     "%s %s %02d %02d:%02d:%02d %4d [%s]: ",
#endif
	     wdays + 4 * clockdata.wday,
	     months + 4 * (clockdata.month - 1),
	     clockdata.mday,
	     clockdata.hour,
	     clockdata.min,
	     clockdata.sec,
#ifdef HAVE_TIMEZONES
	     "UCT",	/* Universal Coordinated Time */
#endif
	     clockdata.year,
	     levels + 6 * ((msg->Level <= LOG_DEBUG) ? msg->Level : LOG_DEBUG)
	     );
      chars = strlen(msg->String) - 1;
      /* Remove last newline */
      if (msg->String[chars] == '\n') {
 	msg->String[chars] = '\0';
      }

      /* Replace all control chars with space */
      for (i = 0; msg->String[i]; ++i) {
       if ((msg->String)[i] < ' ')
	 (msg->String)[i] = ' ';
      }
#ifdef CONSOLE_SYSLOG
      /* If console is not open, open it */
      while (confile == NULL) {
	if ((confile = logOpen(consolename)) == NULL) {
	  if (!conopenfail) /* log only once */
	    __log(LOG_ERR,"Opening console log '%s' failed", consolename);
	  if (consolename == consoledefname) {
	    conopenfail = TRUE;
	    break;
	  }
	  /* try again with the default name */
	  consolename = consoledefname;
	  conopenfail = conwritefail = FALSE;
	}
      }
      if (confile != NULL) {
	int error = FPuts(confile, buffer) == -1;
	if ((!error) && msg->Tag)
	  error = FPrintf(confile, "%s: ", msg->Tag) == -1;
	error = error ||
	    FPuts(confile, msg->String) == -1 ||
	      FPutC(confile, '\n') == -1;
	Flush(confile);
	if (error && !conwritefail) {	/* To avoid loops */
	  conwritefail = TRUE;
	  __log(LOG_ERR, "log: Write failed to console '%s'", consolename);
	}
      }
#endif
      if (LOG_PRI(msg->Level) <= log_cnf.log_filter) {
      /* If log file is not open, open it */
  	while (logfile == NULL) {
  	  if ((logfile = logOpen(logfilename)) == NULL) {
  	    if (!fileopenfail) /* log only once */
  	      __log(LOG_ERR,"Opening log file '%s' failed", logfilename);
  	    if (logfilename == logfiledefname) {
  	      fileopenfail = TRUE;
  	      break;
  	    }
  	    /* try again with the default name */
  	    logfilename = logfiledefname;
  	    fileopenfail = filewritefail = FALSE;
  	  }
  	}
  	if (logfile != NULL) {
  	  int error = FPuts(logfile, buffer) == -1;
  	  if ((!error) && msg->Tag)
  	    error = FPrintf(logfile, "%s: ", msg->Tag) == -1;
  	  error = error ||
  	      FPuts(logfile, msg->String) == -1 ||
  		FPutC(logfile, '\n') == -1;
  	  Flush(logfile);
  	  if (error && !filewritefail) {  /* To avoid loops */
  	    filewritefail = TRUE;
  	    __log(LOG_ERR, "log: Write failed to file '%s'", logfilename);
  	  }
  	}
      }
}
/* 
 * Process all pending log messages 
 */
static
struct SysLogPacket *log_poll()
{     
  struct SysLogPacket *msg;
  static ULONG TotalFail;

  /* Process all messages */
  while (msg = (struct SysLogPacket *)GetMsg(logPort)) {

    if (msg == gui_timerio)
	gui_process_refresh();
    else {
      DNETTRACE(KPrintF("Message level = 0x%08lx\n", msg->Level);)
      if (msg->Level == LOG_CLOSE) {
	return (msg);
      }
      if ((msg->Level & LOG_CMDMASK) == LOG_GUIMSG) {
	gui_process_msg(msg);
        ReplyMsg((struct Message *)msg);
      } else {
        log_msg(msg);
        if (ExtLogPort)
	  PutMsg(ExtLogPort, (struct Message *)msg);
        else
  	  ReplyMsg((struct Message *)msg);
  	if (GetLogMsgFail != TotalFail) {
  	  int t = GetLogMsgFail;    /* Check if we have lost messages */

  	  __log(LOG_WARNING,"%ld log messages lost (total %ld lost)\n",
  	    t - TotalFail, TotalFail);
  	  TotalFail = t;
  	}
      }
    }
  }
  return(NULL);
}  

/* Close logging subsystem */
static
void log_close(struct SysLogPacket *msg)
{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_log.c) log_close()\n"));
#endif
  gui_close();
  cx_deinit();
  rexx_deinit();
  DNETTRACE(KPrintF("rexx_deinit() completed\n");)
#ifdef CONSOLE_SYSLOG
  if (confile) {
    Close(confile);
    DNETTRACE(KPrintF("confile closed\n");)
  }
#endif
  if (logfile) {
    Close(logfile);
    DNETTRACE(KPrintF("logfile closed\n");)
  }
  if (logUtilityBase) {
    CloseLibrary(logUtilityBase);
    logUtilityBase = NULL;
    DNETTRACE(KPrintF("logUtilityBase closed\n");)
  }
  if (logDOSBase) {		/* DOS not needed below */
    CloseLibrary(logDOSBase);
    logDOSBase = NULL;
    DNETTRACE(KPrintF("logDOSBase closed\n");)
  }
  if (SocketBase) {
    CloseLibrary((struct Library *)SocketBase);
    SocketBase = NULL;
    DNETTRACE(KPrintF("SocketBase closed\n");)
  }
  /*
   * Make sure that we get to end before task switch
   * and do not get messages from interrupts
   */
  Disable();

  if (logPort) {
    struct Message *m;

    while (m = GetMsg(logPort))	{/* Check for messages and reply */
      ReplyMsg(m);
      DNETTRACE(KPrintF("Message replied\n");)
    }

    DeleteMsgPort(logPort);	/* Delete port */
    logPort = NULL;
    DNETTRACE(KPrintF("logPort deleted\n");)
  }

  if (msg) {
    if (ExtLogPort)
      PutMsg(ExtLogPort, (struct Message *)msg);
    else
      ReplyMsg((struct Message *)msg);
    DNETTRACE(KPrintF("LOG_CLOSE replied\n");)
  }

  Nettrace_Task = NULL;

  /*
   * Interrupts are left disabled, 
   * they will be enabled again when this process dies 
   */
   DNETTRACE(KPrintF("log_close() finished\n");)
}

/*
 * Try first open w/ shared lock, then as an old file and finally as a new file
 */
static
BPTR logOpen(STRPTR name)
{
  BPTR file;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_log.c) logOpen()\n"));
#endif

  if ((file = Open(name, MODE_READWRITE)) ||
      (file = Open(name, MODE_OLDFILE)))
    Seek(file, 0, OFFSET_END);
  else
    file = Open(name, MODE_NEWFILE);
 
  return file;
}

/*
 * This function might be called by either AmiTCP or NETTRACE. If the
 * call is done by the AmiTCP, no DOS calls may be done, since the
 * DosBase used by these functions is the one of the NETTRACE, and is
 * not initialized at that time!
 */
int logname_changed(void *p, LONG new)
{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_log.c) logname_changed()\n"));
#endif
  if (p == &logfilename) {	/* Is logname requested? */
     /*
      * logfile may be non-NULL only if the NETTRACE is already initialized
      */
    if (logfile != NULL) {
      Close(logfile);
      logfile = NULL;
    }
    fileopenfail = filewritefail = FALSE;
    /*
     * setvalue() (who called this) will set the new value when we return 
     * TRUE.
     */
    return TRUE;
  }
#ifdef CONSOLE_SYSLOG
  if ( p == &consolename ) { /* Name of the console log */
    
    if (confile) { /* only if NETTRACE is already initialized */
      Close(confile);
      confile = NULL;
    }
    conopenfail = conwritefail = FALSE;
    /*
     * setvalue() (who called this) will set the new value when we return 
     * TRUE.
     */
    return TRUE;
  }
#endif
  /*
   * Some invalid pointer
   */
  return FALSE;
}
