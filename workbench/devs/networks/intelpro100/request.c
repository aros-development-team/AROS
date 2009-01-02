/*

Copyright (C) 2001-2005 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/


#include <exec/types.h>
#include <exec/errors.h>
#include <exec/initializers.h>
#include <devices/newstyle.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include "device.h"

#include "request_protos.h"
#include "unit_protos.h"


#define KNOWN_EVENTS \
   (S2EVENT_ERROR | S2EVENT_TX | S2EVENT_RX | S2EVENT_ONLINE \
   | S2EVENT_OFFLINE | S2EVENT_BUFF | S2EVENT_HARDWARE | S2EVENT_SOFTWARE)


static BOOL CmdInvalid(struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdRead(struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdWrite(struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdFlush(struct IORequest *request, struct DevBase *base);
static BOOL CmdS2DeviceQuery(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdGetStationAddress(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdConfigInterface(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdBroadcast(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdTrackType(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdUntrackType(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdGetTypeStats(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdGetSpecialStats(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdGetGlobalStats(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdOnEvent(struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdReadOrphan(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdOnline(struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdOffline(struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdDeviceQuery(struct IOStdReq *request,
   struct DevBase *base);
static BOOL CmdAddMulticastAddresses(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdDelMulticastAddresses(struct IOSana2Req *request,
   struct DevBase *base);


static const UWORD supported_commands[] =
{
   CMD_READ,
   CMD_WRITE,
   CMD_FLUSH,
   S2_DEVICEQUERY,
   S2_GETSTATIONADDRESS,
   S2_CONFIGINTERFACE,
   S2_ADDMULTICASTADDRESS,
   S2_DELMULTICASTADDRESS,
   S2_MULTICAST,
   S2_BROADCAST,
   S2_TRACKTYPE,
   S2_UNTRACKTYPE,
   S2_GETTYPESTATS,
   S2_GETSPECIALSTATS,
   S2_GETGLOBALSTATS,
   S2_ONEVENT,
   S2_READORPHAN,
   S2_ONLINE,
   S2_OFFLINE,
   NSCMD_DEVICEQUERY,
   S2_ADDMULTICASTADDRESSES,
   S2_DELMULTICASTADDRESSES,
   0
};

static const struct Sana2DeviceQuery sana2_info =
{
   0,
   0,
   0,
   0,
   ETH_ADDRESSSIZE * 8,
   ETH_MTU,
   0,
   S2WireType_Ethernet
};

const TEXT badmulticast_name[] = "Bad multicasts";
const TEXT retries_name[] = "Retries";
const TEXT fifo_underruns_name[] = "Underruns";

const TEXT *const special_stat_names[] =
{
   badmulticast_name,
   retries_name,
   fifo_underruns_name
};



/****i* intelpro100.device/ServiceRequest **********************************
*
*   NAME
*	ServiceRequest -- Attempt to service a device request.
*
*   SYNOPSIS
*	ServiceRequest(request)
*
*	VOID ServiceRequest(struct IORequest *);
*
*   FUNCTION
*	Attempts to carry out a request. The relevant unit's semaphore must
*	be obtained before calling this function. This function releases the
*	semaphore before returning.
*
*   INPUTS
*	request - The request to service.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

VOID ServiceRequest(struct IOSana2Req *request, struct DevBase *base)
{
   BOOL complete;

   switch(request->ios2_Req.io_Command)
   {
   case CMD_READ:
      complete = CmdRead(request, base);
      break;
   case CMD_WRITE:
      complete = CmdWrite(request, base);
      break;
   case CMD_FLUSH:
      complete = CmdFlush((APTR)request, base);
      break;
   case S2_DEVICEQUERY:
      complete = CmdS2DeviceQuery((APTR)request, base);
      break;
   case S2_GETSTATIONADDRESS:
      complete = CmdGetStationAddress((APTR)request, base);
      break;
   case S2_CONFIGINTERFACE:
      complete = CmdConfigInterface((APTR)request, base);
      break;
   case S2_ADDMULTICASTADDRESS:
      complete = CmdAddMulticastAddresses((APTR)request, base);
      break;
   case S2_DELMULTICASTADDRESS:
      complete = CmdDelMulticastAddresses((APTR)request, base);
      break;
   case S2_MULTICAST:
      complete = CmdWrite((APTR)request, base);
      break;
   case S2_BROADCAST:
      complete = CmdBroadcast((APTR)request, base);
      break;
   case S2_TRACKTYPE:
      complete = CmdTrackType((APTR)request, base);
      break;
   case S2_UNTRACKTYPE:
      complete = CmdUntrackType((APTR)request, base);
      break;
   case S2_GETTYPESTATS:
      complete = CmdGetTypeStats((APTR)request, base);
      break;
   case S2_GETSPECIALSTATS:
      complete = CmdGetSpecialStats((APTR)request, base);
      break;
   case S2_GETGLOBALSTATS:
      complete = CmdGetGlobalStats((APTR)request, base);
      break;
   case S2_ONEVENT:
      complete = CmdOnEvent((APTR)request, base);
      break;
   case S2_READORPHAN:
      complete = CmdReadOrphan((APTR)request, base);
      break;
   case S2_ONLINE:
      complete = CmdOnline((APTR)request, base);
      break;
   case S2_OFFLINE:
      complete = CmdOffline((APTR)request, base);
      break;
   case NSCMD_DEVICEQUERY:
      complete = CmdDeviceQuery((APTR)request, base);
      break;
   case S2_ADDMULTICASTADDRESSES:
      complete = CmdAddMulticastAddresses((APTR)request, base);
      break;
   case S2_DELMULTICASTADDRESSES:
      complete = CmdDelMulticastAddresses((APTR)request, base);
      break;
   default:
      complete = CmdInvalid((APTR)request, base);
   }

   if(complete && ((request->ios2_Req.io_Flags & IOF_QUICK) == 0))
      ReplyMsg((APTR)request);

   ReleaseSemaphore(&((struct DevUnit *)request->ios2_Req.io_Unit)->
      access_lock);
   return;
}



/****i* intelpro100.device/CMD_INVALID *************************************
*
*   NAME
*	CMD_INVALID -- Reject invalid commands.
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdInvalid(struct IOSana2Req *request, struct DevBase *base)
{
   request->ios2_Req.io_Error = IOERR_NOCMD;
   request->ios2_WireError = S2WERR_GENERIC_ERROR;

   return TRUE;
}



/****** intelpro100.device/CMD_READ ****************************************
*
*   NAME
*	CMD_READ -- Read data.
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdRead(struct IOSana2Req *request, struct DevBase *base)
{
   struct DevUnit *unit;
   struct Opener *opener;
   BOOL complete = FALSE;

   unit = (APTR)request->ios2_Req.io_Unit;

   if((unit->flags & UNITF_ONLINE) != 0)
   {
      opener = request->ios2_BufferManagement;
      PutRequest(&opener->read_port, (APTR)request, base);
   }
   else
   {
      request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
      request->ios2_WireError = S2WERR_UNIT_OFFLINE;
      complete = TRUE;
   }

   /* Return */

   return complete;
}



/****** intelpro100.device/CMD_WRITE ***************************************
*
*   NAME
*	CMD_WRITE -- Write data.
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

/****** intelpro100.device/S2_MULTICAST ************************************
*
*   NAME
*	S2_MULTICAST
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdWrite(struct IOSana2Req *request, struct DevBase *base)
{
   struct DevUnit *unit;
   BYTE error = 0;
   ULONG wire_error;
   BOOL complete = FALSE;

   /* Check request is valid */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_ONLINE) == 0)
   {
      error = S2ERR_OUTOFSERVICE;
      wire_error = S2WERR_UNIT_OFFLINE;
   }
   else if((request->ios2_Req.io_Command == S2_MULTICAST) &&
      ((request->ios2_DstAddr[0] & 0x1) == 0))
   {
      error = S2ERR_BAD_ADDRESS;
      wire_error = S2WERR_BAD_MULTICAST;
   }

   /* Queue request for sending */

   if(error == 0)
      PutRequest(unit->request_ports[WRITE_QUEUE], (APTR)request, base);
   else
   {
      request->ios2_Req.io_Error = error;
      request->ios2_WireError = wire_error;
      complete = TRUE;
   }

   /* Return */

   return complete;
}



/****** intelpro100.device/CMD_FLUSH ***************************************
*
*   NAME
*	CMD_FLUSH
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdFlush(struct IORequest *request, struct DevBase *base)
{
   FlushUnit((APTR)request->io_Unit, EVENT_QUEUE, IOERR_ABORTED, base);

   return TRUE;
}



/****** intelpro100.device/S2_DEVICEQUERY **********************************
*
*   NAME
*	S2_DEVICEQUERY -- Query device capabilities.
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdS2DeviceQuery(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   struct Sana2DeviceQuery *info;
   ULONG size_available, size;

   /* Copy device info */

   unit = (APTR)request->ios2_Req.io_Unit;
   info = request->ios2_StatData;
   size = size_available = info->SizeAvailable;
   if(size > sizeof(struct Sana2DeviceQuery))
      size = sizeof(struct Sana2DeviceQuery);

   CopyMem(&sana2_info, info, size);
   info->BPS = unit->speed;

   info->SizeAvailable = size_available;
   info->SizeSupplied = size;

   /* Return */

   return TRUE;
}



/****** intelpro100.device/S2_GETSTATIONADDDRESS ***************************
*
*   NAME
*	S2_GETSTATIONADDDRESS
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdGetStationAddress(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;

   /* Copy addresses */

   unit = (APTR)request->ios2_Req.io_Unit;
   CopyMem(unit->address, request->ios2_SrcAddr, ETH_ADDRESSSIZE);
   CopyMem(unit->default_address, request->ios2_DstAddr, ETH_ADDRESSSIZE);

   /* Return */

   return TRUE;
}



/****** intelpro100.device/S2_CONFIGINTERFACE ******************************
*
*   NAME
*	S2_CONFIGINTERFACE --
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdConfigInterface(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;

   /* Configure adapter */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_CONFIGURED) == 0)
   {
      CopyMem(request->ios2_SrcAddr, unit->address, ETH_ADDRESSSIZE);
      if((unit->flags & UNITF_HAVEADAPTER) != 0)
         ConfigureAdapter(unit, base);
      unit->flags |= UNITF_CONFIGURED;
   }
   else
   {
      request->ios2_Req.io_Error = S2ERR_BAD_STATE;
      request->ios2_WireError = S2WERR_IS_CONFIGURED;
   }

   /* Return */

   return TRUE;
}



/****** intelpro100.device/S2_BROADCAST ************************************
*
*   NAME
*	S2_BROADCAST
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdBroadcast(struct IOSana2Req *request,
   struct DevBase *base)
{
   /* Fill in the broadcast address as destination */

   *((ULONG *)request->ios2_DstAddr) = 0xffffffff;
   *((UWORD *)(request->ios2_DstAddr + 4)) = 0xffff;

   /* Queue the write as normal */

   return CmdWrite(request, base);
}



/****** intelpro100.device/S2_TRACKTYPE ************************************
*
*   NAME
*	S2_TRACKTYPE
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdTrackType(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   struct Opener *opener;
   ULONG packet_type, wire_error;
   struct TypeTracker *tracker;
   struct TypeStats *initial_stats;
   BYTE error = 0;

   unit = (APTR)request->ios2_Req.io_Unit;
   packet_type = request->ios2_PacketType;

   /* Get global tracker */

   tracker = (struct TypeTracker *)
      FindTypeStats(unit, &unit->type_trackers, packet_type, base);

   if(tracker != NULL)
      tracker->user_count++;
   else
   {
      tracker =
         AllocMem(sizeof(struct TypeTracker), MEMF_PUBLIC | MEMF_CLEAR);
      if(tracker != NULL)
      {
         tracker->packet_type = packet_type;
         tracker->user_count = 1;

         Disable();
         AddTail((APTR)&unit->type_trackers, (APTR)tracker);
         Enable();
      }
   }

   /* Store initial figures for this opener */

   opener = request->ios2_BufferManagement;
   initial_stats = FindTypeStats(unit, &opener->initial_stats, packet_type,
      base);

   if(initial_stats != NULL)
   {
      error = S2ERR_BAD_STATE;
      wire_error = S2WERR_ALREADY_TRACKED;
   }

   if(error == 0)
   {
      initial_stats = AllocMem(sizeof(struct TypeStats), MEMF_PUBLIC);
      if(initial_stats == NULL)
      {
         error = S2ERR_NO_RESOURCES;
         wire_error = S2WERR_GENERIC_ERROR;
      }
   }

   if(error == 0)
   {
      CopyMem(tracker, initial_stats, sizeof(struct TypeStats));
      AddTail((APTR)&opener->initial_stats, (APTR)initial_stats);
   }

   /* Return */

   request->ios2_Req.io_Error = error;
   request->ios2_WireError = wire_error;
   return TRUE;
}



/****** intelpro100.device/S2_UNTRACKTYPE **********************************
*
*   NAME
*	S2_UNTRACKTYPE
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdUntrackType(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   struct Opener *opener;
   ULONG packet_type;
   struct TypeTracker *tracker;
   struct TypeStats *initial_stats;

   unit = (APTR)request->ios2_Req.io_Unit;
   packet_type = request->ios2_PacketType;

   /* Get global tracker and initial figures */

   tracker = (struct TypeTracker *)
      FindTypeStats(unit, &unit->type_trackers, packet_type, base);
   opener = request->ios2_BufferManagement;
   initial_stats = FindTypeStats(unit, &opener->initial_stats, packet_type,
      base);

   /* Decrement tracker usage and free unused structures */

   if(initial_stats != NULL)
   {
      if((--tracker->user_count) == 0)
      {
         Disable();
         Remove((APTR)tracker);
         Enable();
         FreeMem(tracker, sizeof(struct TypeTracker));
      }

      Remove((APTR)initial_stats);
      FreeMem(initial_stats, sizeof(struct TypeStats));
   }
   else
   {
      request->ios2_Req.io_Error = S2ERR_BAD_STATE;
      request->ios2_WireError = S2WERR_NOT_TRACKED;
   }

   /* Return */

   return TRUE;
}



/****** intelpro100.device/S2_GETTYPESTATS *********************************
*
*   NAME
*	S2_GETTYPESTATS
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdGetTypeStats(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   struct Opener *opener;
   ULONG packet_type;
   struct TypeStats *initial_stats, *tracker;
   struct Sana2PacketTypeStats *stats;

   unit = (APTR)request->ios2_Req.io_Unit;
   packet_type = request->ios2_PacketType;

   /* Get global tracker and initial figures */

   tracker = FindTypeStats(unit, &unit->type_trackers, packet_type, base);
   opener = request->ios2_BufferManagement;
   initial_stats = FindTypeStats(unit, &opener->initial_stats, packet_type,
      base);

   /* Copy and adjust figures */

   if(initial_stats != NULL)
   {
      stats = request->ios2_StatData;
      CopyMem(&tracker->stats, stats, sizeof(struct Sana2PacketTypeStats));
      stats->PacketsSent -= initial_stats->stats.PacketsSent;
      stats->PacketsReceived -= initial_stats->stats.PacketsReceived;
      stats->BytesSent -= initial_stats->stats.BytesSent;
      stats->BytesReceived -= initial_stats->stats.BytesReceived;
      stats->PacketsDropped -= initial_stats->stats.PacketsDropped;
   }
   else
   {
      request->ios2_Req.io_Error = S2ERR_BAD_STATE;
      request->ios2_WireError = S2WERR_NOT_TRACKED;
   }

   /* Return */

   return TRUE;
}



/****** intelpro100.device/S2_GETSPECIALSTATS ******************************
*
*   NAME
*	S2_GETSPECIALSTATS
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdGetSpecialStats(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   UWORD i, stat_count;
   struct Sana2SpecialStatHeader *header;
   struct Sana2SpecialStatRecord *record;

   /* Update and fill in stats */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_ONLINE) != 0)
   {
      Disable();
      UpdateStats(unit, base);
      Enable();
   }

   header = request->ios2_StatData;
   record = (APTR)(header + 1);

   stat_count = header->RecordCountMax;
   if(stat_count > SPECIAL_STAT_COUNT)
      stat_count = SPECIAL_STAT_COUNT;

   for(i = 0; i < stat_count; i++)
   {
      record->Type = (S2WireType_Ethernet << 16) + i;
      record->Count = unit->special_stats[i];
      record->String = special_stat_names[i];
      record++;
   }

   header->RecordCountSupplied = stat_count;

   /* Return */

   return TRUE;
}



/****** intelpro100.device/S2_GETGLOBALSTATS *******************************
*
*   NAME
*	S2_GETGLOBALSTATS
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdGetGlobalStats(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;

   /* Update and copy stats */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_ONLINE) != 0)
   {
      Disable();
      UpdateStats(unit, base);
      Enable();
   }
   CopyMem(&unit->stats, request->ios2_StatData,
      sizeof(struct Sana2DeviceStats));

   /* Return */

   return TRUE;
}



/****** intelpro100.device/S2_ONEVENT **************************************
*
*   NAME
*	S2_ONEVENT --
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdOnEvent(struct IOSana2Req *request, struct DevBase *base)
{
   struct DevUnit *unit;
   ULONG events, wanted_events;
   BOOL complete = FALSE;

   /* Check if we understand the event types */

   unit = (APTR)request->ios2_Req.io_Unit;
   wanted_events = request->ios2_WireError;
   if((wanted_events & ~KNOWN_EVENTS) != 0)
   {
      request->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
      events = S2WERR_BAD_EVENT;
   }
   else
   {
      if((unit->flags & UNITF_ONLINE) != 0)
         events = S2EVENT_ONLINE;
      else
         events = S2EVENT_OFFLINE;

      events &= wanted_events;
   }

   /* Reply request if a wanted event has already occurred */

   if(events != 0)
   {
      request->ios2_WireError = events;
      complete = TRUE;
   }
   else
      PutRequest(unit->request_ports[EVENT_QUEUE], (APTR)request, base);

   /* Return */

   return complete;
}



/****** intelpro100.device/S2_READORPHAN ***********************************
*
*   NAME
*	S2_READORPHAN
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdReadOrphan(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   BYTE error = 0;
   ULONG wire_error;
   BOOL complete = FALSE;

   /* Check request is valid */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_ONLINE) == 0)
   {
      error = S2ERR_OUTOFSERVICE;
      wire_error = S2WERR_UNIT_OFFLINE;
   }

   /* Queue request */

   if(error == 0)
      PutRequest(unit->request_ports[ADOPT_QUEUE], (APTR)request, base);
   else
   {
      request->ios2_Req.io_Error = error;
      request->ios2_WireError = wire_error;
      complete = TRUE;
   }

   /* Return */

   return complete;
}



/****** intelpro100.device/S2_ONLINE ***************************************
*
*   NAME
*	S2_ONLINE
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdOnline(struct IOSana2Req *request, struct DevBase *base)
{
   struct DevUnit *unit;
   BYTE error = 0;
   ULONG wire_error;
   UWORD i;

   /* Check request is valid */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_CONFIGURED) == 0)
   {
      error = S2ERR_BAD_STATE;
      wire_error = S2WERR_NOT_CONFIGURED;
   }
   if((unit->flags & UNITF_HAVEADAPTER) == 0)
   {
      error = S2ERR_OUTOFSERVICE;
      wire_error = S2WERR_RCVREL_HDW_ERR;
   }

   /* Clear global and special stats and put adapter back online */

   if((error == 0) && ((unit->flags & UNITF_ONLINE) == 0))
   {
      UpdateStats(unit, base);
      unit->stats.PacketsReceived = 0;
      unit->stats.PacketsSent = 0;
      unit->stats.BadData = 0;
      unit->stats.Overruns = 0;
      unit->stats.UnknownTypesReceived = 0;
      unit->stats.Reconfigurations = 0;

      for(i = 0; i < SPECIAL_STAT_COUNT; i++)
         unit->special_stats[i] = 0;

      GoOnline(unit, base);
   }

   /* Return */

   request->ios2_Req.io_Error = error;
   request->ios2_WireError = wire_error;
   return TRUE;
}



/****** intelpro100.device/S2_OFFLINE **************************************
*
*   NAME
*	S2_OFFLINE
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdOffline(struct IOSana2Req *request, struct DevBase *base)
{
   struct DevUnit *unit;

   /* Put adapter offline */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_ONLINE) != 0)
      GoOffline(unit, base);

   /* Return */

   return TRUE;
}



/****** intelpro100.device/NSCMD_DEVICEQUERY *******************************
*
*   NAME
*	NSCMD_DEVICEQUERY -- Query device capabilities.
*
*   FUNCTION
*	See New-style Device documentation.
*
****************************************************************************
*
* Note that we have to pretend the request structure is an IOStdReq.
*
*/

static BOOL CmdDeviceQuery(struct IOStdReq *request,
   struct DevBase *base)
{
   struct NSDeviceQueryResult *info;

   /* Set structure size twice */

   info = request->io_Data;
   request->io_Actual = info->SizeAvailable =
      (ULONG)OFFSET(NSDeviceQueryResult, SupportedCommands) + sizeof(APTR);

   /* Report device details */

   info->DeviceType = NSDEVTYPE_SANA2;
   info->DeviceSubType = 0;

   info->SupportedCommands = (APTR)supported_commands;

   /* Return */

   return TRUE;
}



/****** intelpro100.device/S2_ADDMULTICASTADDRESS **************************
*
*   NAME
*	S2_ADDMULTICASTADDRESS
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

/****** intelpro100.device/S2_ADDMULTICASTADDRESSES ************************
*
*   NAME
*	S2_ADDMULTICASTADDRESSES
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdAddMulticastAddresses(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   BYTE error = 0;
   ULONG wire_error;
   BOOL complete = FALSE;
   UBYTE *lower_bound, *upper_bound;

   unit = (APTR)request->ios2_Req.io_Unit;

   lower_bound = request->ios2_SrcAddr;
   if(request->ios2_Req.io_Command == S2_ADDMULTICASTADDRESS)
      upper_bound = lower_bound;
   else
      upper_bound = request->ios2_DstAddr;

   if(!AddMulticastRange(unit, lower_bound, upper_bound, base))
   {
      error = S2ERR_NO_RESOURCES;
      wire_error = S2WERR_GENERIC_ERROR;
   }

   /* Update hardware filter if unit is online */

   if(error == 0 && (unit->flags & UNITF_ONLINE) != 0
      && (unit->flags & UNITF_PROM) == 0)
   {
      PutRequest(unit->request_ports[WRITE_QUEUE], (APTR)request, base);
   }
   else
   {
      request->ios2_Req.io_Error = error;
      request->ios2_WireError = wire_error;
      complete = TRUE;
   }

   /* Return */

   return complete;
}



/****** intelpro100.device/S2_DELMULTICASTADDRESS **************************
*
*   NAME
*	S2_DELMULTICASTADDRESS
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

/****** intelpro100.device/S2_DELMULTICASTADDRESSES ************************
*
*   NAME
*	S2_DELMULTICASTADDRESSES
*
*   FUNCTION
*	See SANA-II documentation.
*
****************************************************************************
*
*/

static BOOL CmdDelMulticastAddresses(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   BYTE error = 0;
   ULONG wire_error;
   BOOL complete = FALSE;
   UBYTE *lower_bound, *upper_bound;

   unit = (APTR)request->ios2_Req.io_Unit;

   lower_bound = request->ios2_SrcAddr;
   if(request->ios2_Req.io_Command == S2_DELMULTICASTADDRESS)
      upper_bound = lower_bound;
   else
      upper_bound = request->ios2_DstAddr;

   if(!RemMulticastRange(unit, lower_bound, upper_bound, base))
   {
      error = S2ERR_BAD_STATE;
      wire_error = S2WERR_BAD_MULTICAST;
   }

   /* Update hardware filter if unit is online */

   if(error == 0 && (unit->flags & UNITF_ONLINE) != 0
      && (unit->flags & UNITF_PROM) == 0)
   {
      PutRequest(unit->request_ports[WRITE_QUEUE], (APTR)request, base);
   }
   else
   {
      request->ios2_Req.io_Error = error;
      request->ios2_WireError = wire_error;
      complete = TRUE;
   }

   /* Return */

   return complete;
}



/****i* intelpro100.device/PutRequest **************************************
*
*   NAME
*	PutRequest
*
*   SYNOPSIS
*	PutRequest(port, request)
*
*	VOID PutRequest(struct MsgPort *, struct IORequest *);
*
****************************************************************************
*
*/

VOID PutRequest(struct MsgPort *port, struct IORequest *request,
   struct DevBase *base)
{
   request->io_Flags &= ~IOF_QUICK;
   PutMsg(port, (APTR)request);

   return;
}



