/****************************************************************************
**  File:       pipedebug.c
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
*/
#include   <string.h>	// For strlen
#include   <stdio.h>	// For snprintf

#include   <libraries/dos.h>
#include   <libraries/dosextens.h>
#include   <exec/exec.h>

#include   <proto/exec.h>
#include   <proto/dos.h>
#include   <proto/alib.h>

#include   "pipedebug.h"



/*---------------------------------------------------------------------------
** pipedebug.c
** -----------
** This module contains debugging functions.  In need only be included if the
** other modules are compiled with DEBUG defined.
**
** Visible Functions
** -----------------
**	int   InitDebugIO    (NodePri)
**	void  CleanupDebugIO ()
**	BPTR  DebugOpen      (name, mode)
**	void  DebugClose     (fh)
**	int   DebugWrite     (fh, buf, len)
**	void  OutStr         (str, fh)
**	void  OutLONG        (n, fh)
**
** Macros (in pipedebug.h)
** -----------------------
**	OS (s)
**	NL
**	OL (n)
**
** Local Functions
** ---------------
**	void  DebugIO (Handler, Type, Arg1, Arg2, Arg3)
*/

#ifdef BADDR
#define   BPTRtoCptr(Bp)   BADDR(Bp)
#define   CptrtoBPTR(Cp)   MKBADDR(Cp)
#else
#define   BPTRtoCptr(Bp)   ((char *) ((ULONG) (Bp) << 2))
#define   CptrtoBPTR(Cp)   ((BPTR)   ((ULONG) (Cp) >> 2))
#endif


#define   MEMFLAGS   (MEMF_PUBLIC | MEMF_CLEAR)



static struct MsgPort    *DebugDOSPort  =  NULL;
static struct Message    *DebugMsg      =  NULL;
static struct DosPacket  *DebugPkt      =  NULL;
BPTR                      DebugFH       =  0;

static void DebugIO(struct MsgPort *Handler, SIPTR Type, SIPTR Arg1, SIPTR Arg2,SIPTR Arg3);



/*---------------------------------------------------------------------------
** InitDebugIO() allocates things for the debugging functions, and opens a
** window for output.  It MUST be called before any of the I/O operations
** are used.  CleanupDebugIO() frees the resources allocated here, and closes
** the window.
**      The routines DebugOpen(), DebugClose(), and DebugWrite() mimic their
** corresponding DOS functions, except they use a private reply port, not
** the process' DOS port.  DOS does bad things if a handler request comes in
** while it is waiting or a reply to one of its requests made on your behalf.
**      The return value is nonzero iff no error occurred.
*/

int  InitDebugIO (NodePri)

BYTE  NodePri;

{
  DebugDOSPort=  NULL;
  DebugMsg=      NULL;
  DebugPkt=      NULL;
  DebugFH=       0;

  if ( ((DebugDOSPort= CreatePort (NULL, NodePri)) == NULL)  ||
       ((DebugMsg= (struct Message   *) AllocMem (sizeof (struct Message),   MEMFLAGS)) == NULL) ||
       ((DebugPkt= (struct DosPacket *) AllocMem (sizeof (struct DosPacket), MEMFLAGS)) == NULL) ||
       ((DebugFH= DebugOpen (DEBUG_CON_NAME, MODE_NEWFILE)) == 0) )
    { CleanupDebugIO ();
      return FALSE;
    }

  return TRUE;
}



/*---------------------------------------------------------------------------
** Cleanup things allocated by InitDebugIO, and close the window.	
*/

void  CleanupDebugIO ()

{

  if (DebugFH != 0)
    DebugClose (DebugFH);

  if (DebugPkt != NULL)
    FreeMem (DebugPkt, sizeof (struct DosPacket));

  if (DebugMsg != NULL)
    FreeMem (DebugMsg, sizeof (struct Message));

  if (DebugDOSPort != NULL)
    { FreeSignal (DebugDOSPort->mp_SigBit);
      FreeMem (DebugDOSPort, sizeof (struct MsgPort));
    }
}



/*---------------------------------------------------------------------------
** DebugOpen() performs just like the DOS function Open().
** InitDebugIO() MUST have been called and returned successful before calling
** this function.
*/

BPTR  DebugOpen (name, mode)

char  *name;
int   mode;

{
  BSTR               Bname;
  struct MsgPort     *HandlerPID;
  struct FileLock    *Lock;
  struct FileHandle  *handle;

  Bname = MKBADDR(AllocVec(strlen(name) + 1, MEMF_ANY));
  if (!Bname)
    return 0;

  CopyMem(name, AROS_BSTR_ADDR(Bname), strlen(name));
  AROS_BSTR_setstrlen(Bname, strlen(name));

  HandlerPID= DeviceProc (name);
  if (HandlerPID == NULL) {
    FreeVec(BADDR(Bname));
    return 0;
  }

  Lock= (struct FileLock *) IoErr ();


  if ((handle= (struct FileHandle *) AllocMem (sizeof (struct FileHandle), MEMFLAGS)) == NULL) {
    FreeVec(BADDR(Bname));
    return 0;
  }

  handle->fh_Pos= -1;
  handle->fh_End= -1;
  handle->fh_Type= HandlerPID;


  DebugIO (HandlerPID, mode, (SIPTR)CptrtoBPTR (handle), (SIPTR)CptrtoBPTR (Lock), (SIPTR)Bname);
  FreeVec(BADDR(Bname));

  if (DebugPkt->dp_Res1 == 0)
    { FreeMem (handle, sizeof (struct FileHandle));
      return 0;
    }

  return  CptrtoBPTR (handle);
}



/*---------------------------------------------------------------------------
** DebugClose() performs just like the DOS function Close().
** InitDebugIO() MUST have been called and returned successful before calling
** this function.
*/

void  DebugClose (fh)

BPTR  fh;

{ struct FileHandle  *handle;

  handle= (struct FileHandle *) BPTRtoCptr (fh);
  DebugIO (handle->fh_Type, 1007, handle->fh_Arg1, 0, 0);
  FreeMem (handle, sizeof (struct FileHandle));
}



/*---------------------------------------------------------------------------
** DebugWrite() performs just like the DOS function Write().
** InitDebugIO() MUST have been called and returned successful before calling
** this function.
*/

int  DebugWrite (fh, buf, len)

BPTR   fh;
BYTE   *buf;
ULONG  len;

{ struct FileHandle  *handle;

  handle= (struct FileHandle *) BPTRtoCptr (fh);
  DebugIO (handle->fh_Type, ACTION_WRITE, handle->fh_Arg1, (SIPTR)buf, len);
  return DebugPkt->dp_Res1;
}



/*---------------------------------------------------------------------------
** DebugIO() sets up the DosPacket with the specified information, initiates
** the request, and waits for the reply.
*/

static void  DebugIO (Handler, Type, Arg1, Arg2, Arg3)

struct MsgPort  *Handler;
SIPTR           Type;
SIPTR           Arg1;
SIPTR           Arg2;
SIPTR           Arg3;

{
  DebugMsg->mn_ReplyPort=    DebugDOSPort;
  DebugMsg->mn_Node.ln_Type= NT_MESSAGE;
  DebugMsg->mn_Node.ln_Name= (char *) DebugPkt;

  DebugPkt->dp_Link= DebugMsg;
  DebugPkt->dp_Port= DebugDOSPort;
  DebugPkt->dp_Type= Type;
  DebugPkt->dp_Arg1= Arg1;
  DebugPkt->dp_Arg2= Arg2;
  DebugPkt->dp_Arg3= Arg3;

  PutMsg (Handler, DebugMsg);
  (void) WaitPort (DebugDOSPort);
  (void) GetMsg (DebugDOSPort);     /* assume it is DebugMsg */
}



/*---------------------------------------------------------------------------
** OutStr() outputs the null-terminated string "str" to the filehandle "fh".
*/

void  OutStr (str, fh)

char  *str;
BPTR  fh;

{
  DebugWrite (fh, str, strlen (str));
}



/*---------------------------------------------------------------------------
** OutLONG() outputs the hex representaion of "n" to the filehandle "fh".
** The conversion function stcu_d() is used -- this may not be available
** on all systems.  In that case, such a function will need to be written.
*/

void  OutLONG (n, fh)

ULONG  n;
BPTR   fh;

{
   char buff[16];
   int i = sizeof(buff)-1;

   buff[i--] = 0;
   for (buff[i] = '0'; n > 0; i--) {
      buff[i] = "0123456789abcdef"[n & 0xf];
      n >>= 4;
   }
   
   FPuts(fh, &buff[i]);
}
