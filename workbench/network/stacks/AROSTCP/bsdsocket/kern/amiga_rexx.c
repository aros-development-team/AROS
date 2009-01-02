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
#include <sys/malloc.h>

#include <kern/amiga_config.h>
#include <kern/amiga_includes.h>
#include <dos/rdargs.h>

#include <rexx/storage.h>
#include <rexx/rxslib.h>

#include <proto/rexxsyslib.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>

/*
 * The rexx port and error name may change, if there is already an
 * TCP/IP task running
 */ 
#define D_REXX_PORT_NAME     "AROSTCP"
#define REXX_ERROR_POSTFIX  ".LASTERROR"
#define D_REXX_ERROR_NAME     D_REXX_PORT_NAME REXX_ERROR_POSTFIX

UBYTE   T_REXX_PORT_NAME[]  = D_REXX_PORT_NAME;
UBYTE  *REXX_PORT_NAME      = T_REXX_PORT_NAME;

UBYTE   T_REXX_ERROR_NAME[] = D_REXX_ERROR_NAME;
UBYTE  *REXX_ERROR_NAME     = T_REXX_ERROR_NAME;

#define	REXX_RETURN_ERROR   ((struct RexxMsg *)-1L)

extern LONG nthLibrary;

#include <kern/amiga_rexx.h>

#ifdef __MORPHOS__
struct Library *UtilityBase = NULL;
struct Library *RexxSysBase = NULL;
#else
struct UtilityBase *UtilityBase = NULL;
struct RxsLib *RexxSysBase = NULL;
#endif

struct MsgPort *ARexxPort = NULL;

ULONG 
rexx_init(void)
{
  if (
#ifdef DEBUG
      (REXX_PORT_NAME = 
       bsd_malloc(sizeof(D_REXX_PORT_NAME) + 3, M_CFGVAR, M_WAITOK)) &&
      (REXX_ERROR_NAME = 
       bsd_malloc(sizeof(D_REXX_ERROR_NAME) + 3, M_CFGVAR, M_WAITOK)) &&
#endif
      (UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37)) &&
      (RexxSysBase = OpenLibrary("rexxsyslib.library", 0L))) {
    ARexxPort = CreateMsgPort();
    if (ARexxPort) {
#ifdef DEBUG
      strcpy(REXX_PORT_NAME, D_REXX_PORT_NAME);
      strcpy(REXX_ERROR_NAME, D_REXX_PORT_NAME);
      if (nthLibrary) {
	REXX_PORT_NAME[sizeof(D_REXX_PORT_NAME)-1] = '.'; 
	REXX_PORT_NAME[sizeof(D_REXX_PORT_NAME)] = '0' + nthLibrary;
	REXX_PORT_NAME[sizeof(D_REXX_PORT_NAME)+1] = '\0';
	REXX_ERROR_NAME[sizeof(D_REXX_PORT_NAME)-1] = '.'; 
	REXX_ERROR_NAME[sizeof(D_REXX_PORT_NAME)] = '0' + nthLibrary;
	REXX_ERROR_NAME[sizeof(D_REXX_PORT_NAME)+1] = '\0';
      }
      strcat(REXX_ERROR_NAME, REXX_ERROR_POSTFIX);
#endif

      ARexxPort->mp_Node.ln_Name = REXX_PORT_NAME; 
      return (ULONG)1 << ARexxPort->mp_SigBit;
    }
  }
  rexx_deinit();
  return (0);
}

static BOOL rexx_shown = FALSE;

BOOL
rexx_show(void)
{
  if (!rexx_shown && ARexxPort) {
    AddPort(ARexxPort);	/* No return value! */
    rexx_shown = TRUE;
    return TRUE;
  }
  return FALSE;
}

BOOL
rexx_hide(void)
{
  if (rexx_shown) {
    if (ARexxPort) {
      /*
       * Remove the port from the system's message port list so that the
       * port cannot be found any more
       */
      RemPort(ARexxPort);
    }
    rexx_shown = FALSE;
  }
  return TRUE;
}

void rexx_deinit(void)
{
  struct RexxMsg *rmsg;
  static STRPTR errstr = "99: Port Closed!";

  if (RexxSysBase) {
    if (ARexxPort) {
      rexx_hide();
      /*
       * Reply to all messages received with error code set.
       */
      while(rmsg = (struct RexxMsg *)GetMsg(ARexxPort)){
	SetRexxVar((struct Message *)rmsg, REXX_ERROR_NAME, 
		   errstr, strlen(errstr));
	if (rmsg != REXX_RETURN_ERROR){
	  rmsg->rm_Result2 = 0;
	  rmsg->rm_Result1 = 100;
	  ReplyMsg((struct Message *)rmsg);
	}
      }
      DeleteMsgPort(ARexxPort);
      ARexxPort = NULL;
    }
    CloseLibrary(RexxSysBase);
    RexxSysBase = NULL;
  }
  if (UtilityBase) {
    CloseLibrary((APTR)UtilityBase);
    UtilityBase = NULL;
  }
}

BOOL rexx_poll(void)
{
  struct RexxMsg *rmsg;

  if ((rmsg = (struct RexxMsg *)GetMsg(ARexxPort))
      && rmsg != REXX_RETURN_ERROR 
      && IsRexxMsg(rmsg)) {
    UBYTE rbuf[REPLYBUFLEN];
    struct CSource result;
    struct CSource csarg;
    UBYTE *errstr = NULL;
    LONG error = 0;

    result.CS_Buffer = rbuf; 
    result.CS_Length = REPLYBUFLEN - 1;
    result.CS_CurChr = 0;

    csarg.CS_Buffer = ARG0(rmsg);
    csarg.CS_Length = LengthArgstring(ARG0(rmsg)) + 1;
    csarg.CS_CurChr = 0;
    csarg.CS_Buffer[csarg.CS_Length - 1] = '\n'; /* Sentinel */

    rmsg->rm_Result1 = rmsg->rm_Result2 = 0;

    if (error = parseline(&csarg, &errstr, &result)) {
      SetRexxVar((struct Message*)rmsg, REXX_ERROR_NAME,
		 errstr, (long)strlen(errstr));
      rmsg->rm_Result1 = error;
    } else {
      if (rmsg->rm_Action & (1L << RXFB_RESULT)) {
	rmsg->rm_Result2 = (LONG)
	  CreateArgstring(result.CS_Buffer, (LONG)strlen(result.CS_Buffer));
      }
    }

    csarg.CS_Buffer[csarg.CS_Length - 1] = '\0';

    ReplyMsg((struct Message *)rmsg);

    if (result.CS_Buffer != rbuf) /* We've allocated memory */
      bsd_free(result.CS_Buffer, M_TEMP); 

    return TRUE;
  }
  return FALSE;
}
