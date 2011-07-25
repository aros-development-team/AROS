/*

File: notification.c
Author: Neil Cafferkey
Copyright (C) 2001-2008 Neil Cafferkey

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


#include "handler_protos.h"


static VOID NotifyObject(struct Handler *handler, struct Object *object);



/****i* ram.handler/MatchNotifyRequests ************************************
*
*   NAME
*	MatchNotifyRequests --
*
*   SYNOPSIS
*	MatchNotifyRequests(handler)
*
*	VOID MatchNotifyRequests(struct Handler *);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

VOID MatchNotifyRequests(struct Handler *handler)
{
   struct Notification *notification, *next_notification, *tail;
   struct Object *object;

   next_notification = (APTR)handler->notifications.mlh_Head;
   tail = (APTR)&handler->notifications.mlh_Tail;

   while(next_notification != tail)
   {
      notification = next_notification;
      next_notification = (APTR)((struct MinNode *)notification)->mln_Succ;

      object = GetHardObject(handler, NULL,
         notification->request->nr_FullName, NULL);
      if(object != NULL)
      {
         Remove((APTR)notification);
         AddTail((APTR)&object->notifications, (APTR)notification);
      }
   }

   return;
}



/****i* ram.handler/UnmatchNotifyRequests **********************************
*
*   NAME
*	UnmatchNotifyRequests --
*
*   SYNOPSIS
*	UnmatchNotifyRequests(handler, object)
*
*	VOID UnmatchNotifyRequests(struct Handler *, struct Object *);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

VOID UnmatchNotifyRequests(struct Handler *handler, struct Object *object)
{
   struct Notification *notification, *next_notification, *tail;

   next_notification = (APTR)object->notifications.mlh_Head;
   tail = (APTR)&object->notifications.mlh_Tail;

   while(next_notification != tail)
   {
      notification = next_notification;
      next_notification = (APTR)((struct MinNode *)notification)->mln_Succ;

      Remove((APTR)notification);
      AddTail((APTR)&handler->notifications, (APTR)notification);
   }

   return;
}



/****i* ram.handler/NotifyAll **********************************************
*
*   NAME
*	NotifyAll --
*
*   SYNOPSIS
*	NotifyAll(handler, object, notify_links)
*
*	VOID NotifyAll(struct Handler *, struct Object *, BOOL);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

VOID NotifyAll(struct Handler *handler, struct Object *object,
   BOOL notify_links)
{
   struct MinNode *tail, *link_node;

   if(notify_links)
   {
      object = GetRealObject(object);
      link_node = object->hard_link.mln_Succ;
      if(link_node != NULL)
      {
         tail = (APTR)&HARDLINK(link_node)->elements.mlh_Tail;
      }
      else
         notify_links = FALSE;
   }

   if(notify_links)
   {
      link_node = &object->hard_link;

      while(link_node != tail)
      {
         object = HARDLINK(link_node);
         NotifyObject(handler, object);
         NotifyObject(handler, object->parent);
         link_node = link_node->mln_Succ;
      }
   }
   else
   {
      NotifyObject(handler, object);
      NotifyObject(handler, object->parent);
   }

   return;
}



/****i* ram.handler/NotifyObject *******************************************
*
*   NAME
*	NotifyObject --
*
*   SYNOPSIS
*	NotifyObject(handler, object)
*
*	VOID NotifyObject(struct Handler *, struct Object *);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

static VOID NotifyObject(struct Handler *handler, struct Object *object)
{
   struct Notification *notification, *tail;

   if(object != NULL)
   {
      notification = (APTR)object->notifications.mlh_Head;
      tail = (APTR)&object->notifications.mlh_Tail;

      while(notification != tail)
      {
         Notify(handler, notification);
         notification = (APTR)((struct MinNode *)notification)->mln_Succ;
      }
   }

   return;
}



/****i* ram.handler/FindNotification ***************************************
*
*   NAME
*	FindNotification --
*
*   SYNOPSIS
*	notification = FindNotification(handler,
*	    request)
*
*	struct Notification *FindNotification(struct Handler *,
*	    struct NotifyRequest);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

struct Notification *FindNotification(struct Handler *handler,
   struct NotifyRequest *request)
{
   struct Notification *notification, *tail;
   struct MinList *list;
   BOOL found;
   struct Object *object;

   /* Find which list the request should be in */

   object = GetHardObject(handler, NULL, request->nr_FullName, NULL);
   if(object != NULL)
      list = &object->notifications;
   else
      list = &handler->notifications;

   /* Search the list */

   notification = (APTR)list->mlh_Head;
   tail = (APTR)&list->mlh_Tail;
   found = FALSE;

   while(notification != tail && !found)
   {
      if(notification->request == request)
         found = TRUE;
      else
         notification = (APTR)((struct MinNode *)notification)->mln_Succ;
   }

   if(!found)
      notification = NULL;

   return notification;
}



/****i* ram.handler/ReceiveNotifyReply *************************************
*
*   NAME
*	ReceiveNotifyReply --
*
*   SYNOPSIS
*	ReceiveNotifyReply(handler, message)
*
*	VOID ReceiveNotifyReply(struct Handler *, struct NotifyMessage *);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

VOID ReceiveNotifyReply(struct Handler *handler,
   struct NotifyMessage *message)
{
   struct NotifyRequest *request;
   struct Notification *notification = NULL;

   /* Get request and free message */

   request = message->nm_NReq;
   FreePooled(handler->public_pool, message, sizeof(struct NotifyMessage));

   /* Get notification if EndNotify() hasn't been called */

   if(request->nr_FullName != NULL)
      notification = FindNotification(handler, request);
   if(notification != NULL)
   {
      request->nr_MsgCount--;

      /* Send a new notification if there's one pending */

      if((request->nr_Flags & NRF_MAGIC) != 0)
      {
         request->nr_Flags &= ~NRF_MAGIC;
         Notify(handler, notification);
      }
   }

   return;
}



/****i* ram.handler/Notify *************************************************
*
*   NAME
*	Notify --
*
*   SYNOPSIS
*	Notify(handler, notification)
*
*	VOID Notify(struct Handler *, struct Notification *);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

VOID Notify(struct Handler *handler, struct Notification *notification)
{
   struct NotifyMessage *message;
   struct NotifyRequest *request;
   ULONG flags;

   request = notification->request;
   flags = request->nr_Flags;
   if((flags & NRF_SEND_MESSAGE) != 0)
   {
      /* Send message now or remember to send it later */

      if((flags & NRF_WAIT_REPLY) == 0 || request->nr_MsgCount == 0)
      {
         message = AllocPooled(handler->public_pool,
            sizeof(struct NotifyMessage));
         if(message != NULL)
         {
            ((struct Message *)message)->mn_ReplyPort =
               handler->notify_port;
            ((struct Message *)message)->mn_Length =
               sizeof(struct NotifyMessage);
            message->nm_NReq = request;
            message->nm_Class = NOTIFY_CLASS;
            message->nm_Code = NOTIFY_CODE;

            PutMsg(request->nr_stuff.nr_Msg.nr_Port, (APTR)message);
            request->nr_MsgCount++;
         }
      }
      else
         request->nr_Flags |= NRF_MAGIC;
   }
   else
   {
      Signal(request->nr_stuff.nr_Signal.nr_Task,
         1 << (request->nr_stuff.nr_Signal.nr_SignalNum));
   }

   return;
}



