/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include "lib.h"
#include "debug.h"

/***********************************************************************/

static BOOL localSendRexxMsg(struct MsgPort *reply, STRPTR rxport, STRPTR rxcmd)
{
  BOOL success = FALSE;
  struct RexxMsg *rxmsg;

  ENTER();

  if((rxmsg = CreateRexxMsg(reply, NULL, NULL)) != NULL)
  {
    rxmsg->rm_Action = RXCOMM|RXFF_STRING|RXFF_NOIO;

     if((rxmsg->rm_Args[0] = (IPTR)CreateArgstring(rxcmd,strlen(rxcmd))) != 0)
     {
       struct MsgPort *port;

       Forbid();

       if((port = FindPort(rxport)) != NULL)
       {
         PutMsg(port, (struct Message *)rxmsg);

         success = TRUE;
       }

       Permit();

       if(success == FALSE)
         DeleteArgstring((APTR)rxmsg->rm_Args[0]);
     }

     if(success == FALSE)
       DeleteRexxMsg(rxmsg);
  }

  RETURN(success);
  return success;
}

/**************************************************************************/

void SAVEDS handler(void)
{
  struct Process *me = (struct Process *)FindTask(NULL);
  struct startMsg *smsg;
  struct MsgPort *port;
  BOOL res = FALSE;

  ENTER();

  WaitPort(&me->pr_MsgPort);
  smsg = (struct startMsg *)GetMsg(&me->pr_MsgPort);

  #if defined(__amigaos4__)
  port = AllocSysObject(ASOT_PORT, TAG_DONE);
  #else
  port = CreateMsgPort();
  #endif

  if(port != NULL)
    res = localSendRexxMsg(port, smsg->port, smsg->cmd);

  smsg->res = res;
  ReplyMsg((struct Message *)smsg);

  if(res == TRUE)
  {
    struct RexxMsg *rxmsg;

    WaitPort(port);
    rxmsg = (struct RexxMsg *)GetMsg(port);

    DeleteArgstring((APTR)rxmsg->rm_Args[0]);
    DeleteRexxMsg(rxmsg);
  }

  if(port != NULL)
  {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_PORT, port);
    #else
    DeleteMsgPort(port);
    #endif
  }

  ObtainSemaphore(&OpenURLBase->libSem);
  OpenURLBase->rexx_use--;
  ReleaseSemaphore(&OpenURLBase->libSem);

  #if !defined(__amigaos4__)
  // all systems except OS4 should leave this function in forbidden state
  Forbid();
  #endif

  LEAVE();
}

/**************************************************************************/

