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
#include <sys/synch.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <stdio.h>
#include <version.h>
/*#include <string.h>*/

#if 1 /* NC */
#include <signal.h>		/* from the C compilers includes */
#endif

#include <kern/amiga_includes.h>
#include <libraries/miamipanel.h>

#include <proto/dos.h>
//#include <proto/miamipanel.h>

#include <kern/amiga_time.h>
#include <kern/amiga_log.h>

ULONG sana_init(void);
void sana_deinit(void);
BOOL sana_poll(void);

#include <kern/amiga_main_protos.h>
#include <kern/amiga_config.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_netdb.h>
#include <kern/amiga_rc.h>
#include <kern/amiga_log.h>
#include <kern/kern_malloc_protos.h>
#include <api/amiga_api.h>

/*
 * include prototypes for initialization functions
 */
#include <kern/uipc_domain_protos.h>    /* domaininit() */

/*
 * The main module of the AMITCP/IP.
 */

/*
 * Global variable so AMITCP/IP task information can be utilized.
 */
struct Task * AROSTCP_Task;

extern struct ExecBase * SysBase;
extern struct Library * MasterSocketBase;
extern WORD nthLibrary;

static ULONG sanamask = 0, 
  sig = 0, signalmask = 0, timermask = 0, 
  breakmask = 0;

UBYTE *taskname = NULL;
/*BPTR db_lock = NULL;*/
ULONG EnableDebug = 0;
BOOL  initialized = FALSE;

TEXT interfaces_path[FILENAME_MAX];
TEXT netdb_path[FILENAME_MAX];
TEXT db_path[FILENAME_MAX];
TEXT config_path[FILENAME_MAX];
UBYTE logfiledefname[FILENAME_MAX];
UBYTE dhclient_path[FILENAME_MAX];

/*
TEXT hequiv_path[FILENAME_MAX];
TEXT inetdconf_path[FILENAME_MAX];
*/
STRPTR version = "$VER: " STACK_RELEASE;

int
main(int argc, char *argv[])
{
  BYTE oldpri;
  STRPTR oldname;
  int retval;
  struct Library *TestSocketBase;

#ifndef __AROS__
  SysBase = *(struct ExecBase **)4;
#else
D(bug("[AROSTCP](amiga_main.c) main()\n"));
#endif

  TestSocketBase = OpenLibrary("bsdsocket.library",0);
  if (TestSocketBase) {
	CloseLibrary(TestSocketBase);
	error_request("Another TCP/IP stack is running, please quit it first.");
	return 21;
  }

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) main: Kernel launching ...\n"));
#else
  D(Printf(STACK_NAME " kernel started\n");)
#endif

  /*
   * Disable CTRL-C(D) Break signal.
   */
  signal(SIGINT, SIG_IGN);

  /*
   * Initialize AROSTCP_Task to point the Task structure of this task.
   */
  AROSTCP_Task = FindTask(NULL);
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) main: AROSTCP_Task @ 0x%p\n",AROSTCP_Task));
#endif
  D(Printf("AROSTCP_Task @ 0x%p\n",AROSTCP_Task);)

  /*
   * Get a lock on the 'db' directory so we don't need an assign.
   */
   BPTR  db_path_lock = NULL;

   char tmpconfigpath[1024];

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) main: Setting environment default Paths ... \n"));
#endif

   db_path_lock = GetProgramDir();
#if defined(__AROS__)
   db_path_lock = ParentDir(db_path_lock);
#endif

  NameFromLock(db_path_lock, interfaces_path, FILENAME_MAX);
  
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) main: Directory tree root: %s\n", interfaces_path));
#endif
  D(Printf("Directory tree root: %s\n", interfaces_path);)
  strcpy(netdb_path, interfaces_path);
  strcpy(db_path, interfaces_path);
  strcpy(config_path, interfaces_path);
  strcpy(logfiledefname, "T:");             /* NicJA: Default to storing logs in Temp for launching
                                               from read only media                          */
  strcpy(dhclient_path, interfaces_path);
/*strcpy(hequiv_path, interfaces_path);
  strcpy(inetdconf_path, interfaces_path);*/
  AddPart(interfaces_path, _PATH_DB, FILENAME_MAX);
  AddPart(netdb_path, _PATH_DB, FILENAME_MAX);
  AddPart(db_path, _PATH_DB, FILENAME_MAX);
  AddPart(config_path, _PATH_DB, FILENAME_MAX);
  AddPart(logfiledefname, _PATH_SYSLOG, FILENAME_MAX);
  AddPart(dhclient_path, _PATH_DHCLIENT, FILENAME_MAX);
/*AddPart(hequiv_path, _PATH_HEQUIV, FILENAME_MAX);
  AddPart(inetdconf_path, _PATH_INETDCONF, FILENAME_MAX);*/

/* NicJA : Allow user specified config location (from Env: variable) */
	if (GetVar("AROSTCP/Config", tmpconfigpath, 1024, GVF_GLOBAL_ONLY) != -1)
	{
   	db_path_lock = NULL;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) main: Env: Var AROSTCP/Config set.\n"));
D(bug("[AROSTCP](amiga_main.c) main: Attempting to use '%s' for config location..\n", tmpconfigpath));
#endif
	  if (db_path_lock = Lock(tmpconfigpath, ACCESS_READ))
	  {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) main: successfully locked config dir '%s'\n", tmpconfigpath));
#endif
        strcpy(interfaces_path, tmpconfigpath);
        strcpy(netdb_path, tmpconfigpath);
        strcpy(db_path, tmpconfigpath);
        strcpy(config_path, tmpconfigpath);
     	  //UnLock(db_path_lock);
      }
#warning "TODO: NicJA - Attempt to create chosen config location"
#warning "TODO: NicJA - and copy defaults if it doesnt currently exist and is possible?"
	}

  AddPart(interfaces_path, _FILE_SANA2CONFIG, FILENAME_MAX);
  AddPart(netdb_path, _FILE_NETDB, FILENAME_MAX);
  AddPart(config_path, _FILE_CONFIG, FILENAME_MAX);

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) main: Log file defaulting to '%s'\n", logfiledefname));
#endif

  /* Save pointer to this tasks old name */
  oldname = AROSTCP_Task->tc_Node.ln_Name;

  if (init_all()) {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) main: preparing AROSTCP_Task\n"));
#endif
    /* Set our priority */
    oldpri = SetTaskPri(AROSTCP_Task, 5);

    /* Set our Task name */
    if (!taskname) {
#ifdef DEBUG
      if (nthLibrary) {
	if (taskname = bsd_malloc(16, M_CFGVAR, M_WAITOK)) {
	  strcpy(taskname, "bsdsocket.library");
	  taskname[6] = '.'; taskname[7] = '0' + nthLibrary;
	}
      } else {
#endif
	taskname = "bsdsocket.library";
#ifdef DEBUG
      }
#endif
    }
    if (taskname) AROSTCP_Task->tc_Node.ln_Name = taskname;

    initialized = TRUE;           /* Global initialization flag */

#ifdef DEBUG
    /* Show our task address */
    printf("%s Task address : %lx\n", taskname, (long) AROSTCP_Task);
#endif

    /* Initialize signal mask for the wait */
    breakmask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F;
    signalmask = timermask | breakmask | sanamask;

    /*
     * Now when everything else is successfully initialized,
     * let the timeouts roll!
     */
    timer_send();

    for(;;) {
      sig = Wait(signalmask);     /* Sleep until we are signalled. */

      do {
        if (sig & sanamask) {
          if (!sana_poll()) sig &= ~sanamask;
	     }

        if (sig & timermask) {
	       if (!timer_poll()) sig &= ~timermask;
	     }

	     sig |= SetSignal(0L, signalmask) & signalmask;
      } while (sig & (~breakmask));

      if (sig & breakmask) {
	     int i;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) main: Task recieved CTRL-C\n"));
#endif
	    /* We recieved CTRL-C
	     * NETTRACE task keeps one base open, it is not counted. */
        api_hide();		          /* hides the API from users */

        /* Try three times with a short delay */
        for (i = 0; i < 3 && MasterSocketBase->lib_OpenCnt > 1; i++) {
          api_sendbreaktotasks(); /* send brk to all tasks w/ SBase open */ 
          Delay(50);		          /* give tasks time to close socket base */
        }

        if (MasterSocketBase->lib_OpenCnt > 1) {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) main: Got CTRL-C while %ld %s still open.\n",
          MasterSocketBase->lib_OpenCnt - 1,
	             (MasterSocketBase->lib_OpenCnt == 2) ? "library" : "libraries"));
#endif

          __log(LOG_ERR, "Got CTRL-C while %ld %s still open.\n",
	       MasterSocketBase->lib_OpenCnt - 1,
	             (MasterSocketBase->lib_OpenCnt == 2) ? "library" : "libraries");

          api_show(); /* stopping not successfull, show API to users */ 
        } else {
          break;
        }
      }
    }
    retval = 0;
  } else
    retval = 20;

  /* free all resources */
  deinit_all();
  initialized = FALSE;

  SetTaskPri(AROSTCP_Task, oldpri);
  AROSTCP_Task->tc_Node.ln_Name = oldname;

  return retval;
}

/*
 * Do all initializations
 */
BOOL
init_all(void)
{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) init_all()\n"));
#endif
  /*
   * Initialize malloc semaphore
   */
  malloc_init();
  D(Printf("malloc_init() complete\n");)

  /*
   * initialize concurrency control subsystem
   */  
  spl_init();
  D(Printf("spl_init() complete\n");)
  
  /*
   * initialize sleep queues
   */
  sleep_init();
  D(Printf("sleep_init() complete\n");)
  
  /* 
   * Read command line arguments and configuration file
   */
  if (!readconfig())
    return FALSE;
  D(Printf("readconfig() complete\n");)

  /*
   * initialize logging system
   */
  if (!log_init())
    return FALSE;
  D(Printf("log_init() complete\n");)

  /* 
   * initialize the mbuf subsystem
   */
  if (!mbinit())
    return FALSE;
  D(Printf("mbinit() complete\n");)

  /*
   * initialize timer
   */
  if ((timermask = timer_init()) == 0L)
    return FALSE;
  D(Printf("timer_init() complete\n");)

  /*
   * initialize API
   */
  if (!api_init())
    return FALSE;
  D(Printf("api_init() complete\n");)
	
  /*
   * initialize SANA-II subsystem
   */
  if ((sanamask = sana_init()) == 0L)
    return FALSE;
  D(Printf("sana_init() complete\n");)
	    
  /*
   * initialize domains (initializes all protocols)
   */
  domaininit();
  D(Printf("domaininit() complete\n");)

  pfil_init();
  D(Printf("pfil_init() complete\n");)

  loconfig();
  D(Printf("loconfig() complete\n");)
	    
  /*
   * Initialize NetDataBase
   */
  if (init_netdb() != 0)
    return FALSE;
  D(Printf("init_netdb() complete\n");)

  /*
   * Make API visible
   */
  if (api_show() == FALSE)
    return FALSE;
  D(Printf("api_show() complete\n");)

  if (Nettrace_Task) {
    D(Printf("Initialization complete, signalling NETTRACE\n");)
    Signal(Nettrace_Task, SIGBREAKF_CTRL_F);
  } else
    return FALSE;

  rc_start();
  D(Printf("rc_start() complete, initialization finished\n");)

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) init_all: Initialisation successfull.\n"));
#endif

  return TRUE;
}

/*
 * clean up everything
 */
void
deinit_all(void)
{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) deinit_all()\n"));
#endif
  /*
   * make sure we are out of critical section
   */
  spl0();
  D(Printf("spl0() completed\n");)

  api_hide();			/* hides the API from users */
  D(Printf("api_hide() completed\n");)

  /*
   * Deinitialize network database.
   */
  netdb_deinit();
  
  /*
   * Deinitialize network interfaces
   */
  sana_deinit();

  /*
   * Deinitialize timers
   */
  timer_deinit();

  /*
   * Free all resources allocated by mbufs.
   */
  mbdeinit();
  D(Printf("mbdeinit() completed\n");)

  log_deinit();
  D(Printf("log_deinit() completed\n");)

  /*
   * Free memory pool.
   */
  malloc_deinit();
  D(Printf("malloc_deinit() completed\n");)

  /*
   * Check that there are no libraries open (to our API). We can continue only
   * if all bases are closed.
   */
  api_deinit();  /* NOTICE: this waits until every api user has exited */
}

/*
 * Notification function for taskname
 */ 
int taskname_changed(void *p, LONG new)
{
  UBYTE *newname = (UBYTE *)new;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_main.c) taskname_changed()\n"));
#endif
  
  AROSTCP_Task->tc_Node.ln_Name = newname;
  if (initialized)
    printf("New task name %s\n", newname);

  return TRUE;
}
