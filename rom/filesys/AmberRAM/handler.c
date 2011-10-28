/*

File: handler.c
Author: Neil Cafferkey
Copyright (C) 2001-2008 Neil Cafferkey
Copyright (C) 2011 The AROS Development Team

This file is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this file; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#include <aros/debug.h>
#include <aros/asmcall.h>

#include "handler_protos.h"


/* Global variables */

#ifndef __AROS__
struct ExecBase *SysBase;
#endif

/* Data prototypes */

#ifndef __AROS__
extern struct ExecBase *AbsExecBase;
#endif

/* Function prototypes */

static struct DosPacket *GetPacket(struct MsgPort *port);
static VOID ReplyPacket(struct MsgPort *proc_port, struct DosPacket *packet,
   PINT result1, PINT result2);
static TEXT *BStr(struct Handler *h, UBYTE *b_str);
static TEXT *BStr2(struct Handler *h, UBYTE *b_str);

LONG AmberMain(void)
{
   struct Handler *handler;
   struct MsgPort *proc_port;
   struct DosPacket *packet;
   PINT result;
   BOOL exit;
   LONG link_type;
   APTR link_target;
   ULONG port_signals;
   struct Message *message;

   /* Open libraries */
   handler = AllocMem(sizeof(struct Handler), MEMF_CLEAR);
   if(handler == NULL)
   	return RETURN_FAIL;		

#ifndef __AROS__
   SysBase = AbsExecBase;
#endif

   DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, DOS_VERSION);
   if(DOSBase == NULL) {
	FreeMem(handler, sizeof(struct Handler));
   	return RETURN_FAIL;
   }

   /* Process start-up packet */

   proc_port = &((*(struct Process *)FindTask(NULL)).pr_MsgPort);

   WaitPort(proc_port);
   packet = GetPacket(proc_port);

   exit = CmdStartup(handler, BStr(handler, BADDR(packet->dp_Arg1)),
      BADDR(packet->dp_Arg3), proc_port);

   ReplyPacket(proc_port, packet, DOSBOOL(!exit), IoErr());

   /* Set up port signals */

   port_signals = 1 << (proc_port->mp_SigBit);
   Signal(proc_port->mp_SigTask, port_signals);
   if(!exit)
      port_signals |= 1 << (handler->notify_port->mp_SigBit);

   /* Service packet requests */

   while(!exit)
   {
      Wait(port_signals);

      /* Accept returning messages from notification clients */

      while((message = GetMsg(handler->notify_port)) != NULL)
      {
         ReceiveNotifyReply(handler, (APTR)message);
      }

      /* Service packet requests */

      while((packet = GetPacket(proc_port)) != NULL)
      {
         result = DOSTRUE;
         SetIoErr(0);

         switch(packet->dp_Type)
         {
         case ACTION_DIE:

            if(!CmdDie(handler))
               result = DOSFALSE;
            exit = result;
            break;

         case ACTION_IS_FILESYSTEM:

            if(!CmdIsFileSystem())
               result = DOSFALSE;
            break;

         case ACTION_FINDOUTPUT:

            if(!CmdFind(handler, BADDR(packet->dp_Arg1),
               BADDR(packet->dp_Arg2), BStr(handler, BADDR(packet->dp_Arg3)),
               MODE_NEWFILE))
               result = DOSFALSE;
            break;

         case ACTION_FINDUPDATE:

            if(!CmdFind(handler, BADDR(packet->dp_Arg1),
               BADDR(packet->dp_Arg2), BStr(handler, BADDR(packet->dp_Arg3)),
               MODE_READWRITE))
               result = DOSFALSE;
            break;

         case ACTION_FINDINPUT:

            if(!CmdFind(handler, BADDR(packet->dp_Arg1),
               BADDR(packet->dp_Arg2), BStr(handler, BADDR(packet->dp_Arg3)),
               MODE_OLDFILE))
               result = DOSFALSE;
            break;

         case ACTION_FH_FROM_LOCK:

            result = CmdFHFromLock(handler, BADDR(packet->dp_Arg1),
               BADDR(packet->dp_Arg2));
            break;

         case ACTION_END:

            if(!CmdEnd(handler, (APTR)packet->dp_Arg1))
               result = DOSFALSE;
            break;

         case ACTION_READ:

            result = CmdRead(handler, (struct Opening *)packet->dp_Arg1,
               (UBYTE *)packet->dp_Arg2, packet->dp_Arg3);
            break;

         case ACTION_WRITE:

            result = CmdWrite(handler, (struct Opening *)packet->dp_Arg1,
               (UBYTE *)packet->dp_Arg2, packet->dp_Arg3);
            break;

         case ACTION_SEEK:
            result = CmdSeek(handler, (struct Opening *)packet->dp_Arg1,
               packet->dp_Arg2, packet->dp_Arg3);
            break;

         case ACTION_SET_FILE_SIZE:

            result = CmdSetFileSize(handler, (APTR)packet->dp_Arg1,
               packet->dp_Arg2, packet->dp_Arg3);
            break;

         case ACTION_LOCATE_OBJECT:

            result = (PINT)MKBADDR(CmdLocateObject(handler,
               BADDR(packet->dp_Arg1), BStr(handler, BADDR(packet->dp_Arg2)),
               packet->dp_Arg3));
            break;

         case ACTION_FREE_LOCK:

            if(!CmdFreeLock(handler, BADDR(packet->dp_Arg1)))
               result = DOSFALSE;
            break;

         case ACTION_COPY_DIR:

            result = (PINT)MKBADDR(CmdCopyDir(handler, BADDR(packet->dp_Arg1)));
            break;

         case ACTION_COPY_DIR_FH:

            result = (PINT)MKBADDR(CmdCopyDirFH(handler, (APTR)packet->dp_Arg1));
            break;

         case ACTION_PARENT:

            result = (PINT)MKBADDR(CmdParent(handler, BADDR(packet->dp_Arg1)));
            break;

         case ACTION_PARENT_FH:

            result = (PINT)MKBADDR(CmdParentFH(handler, (APTR)packet->dp_Arg1));
            break;

         case ACTION_SAME_LOCK:

            if(!CmdSameLock(handler, BADDR(packet->dp_Arg1), BADDR(packet->dp_Arg2)))
               result = DOSFALSE;
            break;

         case ACTION_CREATE_DIR:

            result = (PINT)MKBADDR(CmdCreateDir(handler, BADDR(packet->dp_Arg1),
               BStr(handler, BADDR(packet->dp_Arg2))));
            break;

         case ACTION_EXAMINE_OBJECT:

            if(!CmdExamineObject(handler, BADDR(packet->dp_Arg1),
               BADDR(packet->dp_Arg2)))
               result = DOSFALSE;
            break;

         case ACTION_EXAMINE_FH:

            if(!CmdExamineFH(handler, (APTR)packet->dp_Arg1,
               BADDR(packet->dp_Arg2)))
               result = DOSFALSE;
            break;

         case ACTION_EXAMINE_NEXT:

            if(!CmdExamineNext(handler, BADDR(packet->dp_Arg2)))
               result = DOSFALSE;
            break;

         case ACTION_EXAMINE_ALL:

            if(!CmdExamineAll(handler, BADDR(packet->dp_Arg1),
               (APTR)packet->dp_Arg2, packet->dp_Arg3, packet->dp_Arg4,
               (APTR)packet->dp_Arg5))
               result = DOSFALSE;
            break;

         case ACTION_EXAMINE_ALL_END:

            CmdExamineAllEnd(handler, BADDR(packet->dp_Arg1),
               (APTR)packet->dp_Arg2, packet->dp_Arg3, packet->dp_Arg4,
               (APTR)packet->dp_Arg5);
            break;

         case ACTION_INFO:

            if(!CmdInfo(handler, BADDR(packet->dp_Arg2)))
               result = DOSFALSE;
            break;

         case ACTION_DISK_INFO:

            if(!CmdInfo(handler, BADDR(packet->dp_Arg1)))
               result = DOSFALSE;
            break;

         case ACTION_SET_PROTECT:

            if(!CmdSetProtect(handler, BADDR(packet->dp_Arg2),
               BStr(handler, BADDR(packet->dp_Arg3)), packet->dp_Arg4))
               result = DOSFALSE;
            break;

         case ACTION_SET_COMMENT:

            if(!CmdSetComment(handler, BADDR(packet->dp_Arg2),
               BStr(handler, BADDR(packet->dp_Arg3)),
               BStr2(handler, BADDR(packet->dp_Arg4))))
               result = DOSFALSE;
            break;

         case ACTION_RENAME_OBJECT:

            if(!CmdRenameObject(handler, BADDR(packet->dp_Arg1),
               BStr(handler, BADDR(packet->dp_Arg2)), BADDR(packet->dp_Arg3),
               BStr2(handler, BADDR(packet->dp_Arg4))))
               result = DOSFALSE;
            break;

         case ACTION_RENAME_DISK:

            if(!CmdRenameDisk(handler, BStr(handler, BADDR(packet->dp_Arg1))))
               result = DOSFALSE;
            break;

         case ACTION_SET_DATE:

            if(!CmdSetDate(handler, BADDR(packet->dp_Arg2),
               BStr(handler, BADDR(packet->dp_Arg3)), (APTR)packet->dp_Arg4))
               result = DOSFALSE;
            break;

         case ACTION_DELETE_OBJECT:

            if(!CmdDeleteObject(handler, BADDR(packet->dp_Arg1),
               BStr(handler, BADDR(packet->dp_Arg2))))
               result = DOSFALSE;
            break;

         case ACTION_CURRENT_VOLUME:

            result = (PINT)MKBADDR(CmdCurrentVolume(handler));
            break;

         case ACTION_CHANGE_MODE:

            result = CmdChangeMode(handler, packet->dp_Arg1, BADDR(packet->dp_Arg2),
               packet->dp_Arg3);
            break;

         case ACTION_MAKE_LINK:

            link_target = (APTR)packet->dp_Arg3;
            link_type = packet->dp_Arg4;
            if(link_type == LINK_HARD)
               link_target = BADDR(link_target);
            if(!CmdMakeLink(handler, BADDR(packet->dp_Arg1),
               BStr(handler, BADDR(packet->dp_Arg2)), link_target, link_type))
               result = DOSFALSE;
            break;

         case ACTION_READ_LINK:

            result = CmdReadLink(handler, BADDR(packet->dp_Arg1),
               (APTR)packet->dp_Arg2, (APTR)packet->dp_Arg3,
               packet->dp_Arg4);
            break;

         case ACTION_WRITE_PROTECT:

            if(!CmdWriteProtect(handler, packet->dp_Arg1,
               (ULONG)packet->dp_Arg2))
               result = DOSFALSE;
            break;

         case ACTION_FLUSH:

            if(!CmdFlush())
               result = DOSFALSE;
            break;

         case ACTION_ADD_NOTIFY:

            if(!CmdAddNotify(handler, (APTR)packet->dp_Arg1))
               result = DOSFALSE;
            break;

         case ACTION_REMOVE_NOTIFY:

            if(!CmdRemoveNotify(handler, (APTR)packet->dp_Arg1))
               result = DOSFALSE;
            break;

         default:
            result = DOSFALSE;
            SetIoErr(ERROR_ACTION_NOT_KNOWN);
         }

         ReplyPacket(proc_port, packet, result, IoErr());
      }

   }

   CloseLibrary((APTR)DOSBase);

   DeleteHandler(handler);

   return RETURN_OK;
}

static struct DosPacket *GetPacket(struct MsgPort *port)
{
   struct Message *message;
   struct DosPacket *packet;

   message = GetMsg(port);
   if(message != NULL)
      packet = (struct DosPacket *)((struct Node *)message)->ln_Name;
   else
      packet = NULL;

   return packet;
}



static VOID ReplyPacket(struct MsgPort *proc_port, struct DosPacket *packet,
   PINT result1, PINT result2)
{
   struct MsgPort *port;
   struct Message *message;

   port = packet->dp_Port;
   message = packet->dp_Link;

   packet->dp_Port = proc_port;
   packet->dp_Res1 = result1;
   packet->dp_Res2 = result2;

   PutMsg(port, message);

   return;
}

static TEXT *BStr(struct Handler *h, UBYTE *b_str)
{
   UBYTE length;

#ifdef AROS_FAST_BSTR
   length = StrSize(b_str);
   CopyMem(b_str, h->b_buffer, length);
#else
   length = *b_str;
   if(length != 0)
      CopyMem(b_str + 1, h->b_buffer, length);
#endif
   *(h->b_buffer + length) = '\0';

   return h->b_buffer;
}



static TEXT *BStr2(struct Handler *h, UBYTE *b_str)
{
   UBYTE length;

#ifdef AROS_FAST_BSTR
   length = StrSize(b_str);
   CopyMem(b_str, h->b_buffer2, length);
#else
   length = *b_str;
   if(length != 0)
      CopyMem(b_str + 1, h->b_buffer2, length);
#endif
   *(h->b_buffer2 + length) = '\0';

   return h->b_buffer2;
}



UBYTE *MkBStr(struct Handler *h, TEXT *str)
{
   UBYTE length;

   length = StrLen(str);
#ifdef AROS_FAST_BSTR
   CopyMem(str, h->b_buffer, length + 1);
#else
   if(length != 0)
      CopyMem(str, h->b_buffer + 1, length);
   *h->b_buffer = length;
#endif

   return h->b_buffer;
}



