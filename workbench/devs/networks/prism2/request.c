/*

Copyright (C) 2001-2012 Neil Cafferkey

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
   | S2EVENT_OFFLINE | S2EVENT_BUFF | S2EVENT_HARDWARE | S2EVENT_SOFTWARE \
   | S2EVENT_CONNECT | S2EVENT_DISCONNECT)


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
static BOOL CmdGetSignalQuality(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdGetNetworks(struct IOSana2Req *request,
   struct DevBase *base);
static BOOL CmdSetOptions(struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdSetKey(struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdGetNetworkInfo(struct IOSana2Req *request,
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
   S2_GETSIGNALQUALITY,
   S2_GETNETWORKS,
   S2_SETOPTIONS,
   S2_SETKEY,
   S2_GETNETWORKINFO,
//   P2_DISASSOCIATE,
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



/****i* prism2.device/ServiceRequest ***************************************
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
*	request
*
*   RESULT
*	None.
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
      complete = CmdS2DeviceQuery(request, base);
      break;
   case S2_GETSTATIONADDRESS:
      complete = CmdGetStationAddress(request, base);
      break;
   case S2_CONFIGINTERFACE:
      complete = CmdConfigInterface(request, base);
      break;
   case S2_ADDMULTICASTADDRESS:
      complete = CmdAddMulticastAddresses(request, base);
      break;
   case S2_DELMULTICASTADDRESS:
      complete = CmdDelMulticastAddresses(request, base);
      break;
   case S2_MULTICAST:
      complete = CmdWrite(request, base);
      break;
   case S2_BROADCAST:
      complete = CmdBroadcast(request, base);
      break;
   case S2_TRACKTYPE:
      complete = CmdTrackType(request, base);
      break;
   case S2_UNTRACKTYPE:
      complete = CmdUntrackType(request, base);
      break;
   case S2_GETTYPESTATS:
      complete = CmdGetTypeStats(request, base);
      break;
   case S2_GETSPECIALSTATS:
      complete = CmdGetSpecialStats(request, base);
      break;
   case S2_GETGLOBALSTATS:
      complete = CmdGetGlobalStats(request, base);
      break;
   case S2_ONEVENT:
      complete = CmdOnEvent(request, base);
      break;
   case S2_READORPHAN:
      complete = CmdReadOrphan(request, base);
      break;
   case S2_ONLINE:
      complete = CmdOnline(request, base);
      break;
   case S2_OFFLINE:
      complete = CmdOffline(request, base);
      break;
   case NSCMD_DEVICEQUERY:
      complete = CmdDeviceQuery((APTR)request, base);
      break;
   case S2_ADDMULTICASTADDRESSES:
      complete = CmdAddMulticastAddresses(request, base);
      break;
   case S2_DELMULTICASTADDRESSES:
      complete = CmdDelMulticastAddresses(request, base);
      break;
   case S2_GETSIGNALQUALITY:
      complete = CmdGetSignalQuality(request, base);
      break;
   case S2_GETNETWORKS:
      complete = CmdGetNetworks(request, base);
      break;
   case S2_SETOPTIONS:
      complete = CmdSetOptions(request, base);
      break;
   case S2_SETKEY:
      complete = CmdSetKey(request, base);
      break;
   case S2_GETNETWORKINFO:
      complete = CmdGetNetworkInfo(request, base);
      break;
   default:
      complete = CmdInvalid(request, base);
   }

   if(complete && ((request->ios2_Req.io_Flags & IOF_QUICK) == 0))
      ReplyMsg((APTR)request);

   ReleaseSemaphore(
      &((struct DevUnit *)request->ios2_Req.io_Unit)->access_lock);
   return;
}



/****i* prism2.device/CMD_INVALID ******************************************
*
*   NAME
*	CMD_INVALID -- Reject an invalid command.
*
*   FUNCTION
*
*   INPUTS
*	None.
*
*   RESULTS
*	io_Error - IOERR_NOCMD.
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

static BOOL CmdInvalid(struct IOSana2Req *request, struct DevBase *base)
{
   request->ios2_Req.io_Error = IOERR_NOCMD;
   request->ios2_WireError = S2WERR_GENERIC_ERROR;

   return TRUE;
}



/****** prism2.device/CMD_READ *********************************************
*
*   NAME
*	CMD_READ -- Read data.
*
*   FUNCTION
*
*   INPUTS
*	io_Flags
*	ios2_PacketType
*	ios2_Data
*
*   RESULTS
*	io_Flags
*	io_Error
*	ios2_WireError
*	ios2_SrcAddr
*	ios2_DstAddr
*	ios2_DataLength
*	ios2_Data
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



/****** prism2.device/CMD_WRITE ********************************************
*
*   NAME
*	CMD_WRITE -- Write data.
*
*   FUNCTION
*
*   INPUTS
*	io_Flags
*	ios2_DstAddr
*	ios2_PacketType
*	ios2_DataLength
*	ios2_Data
*
*   RESULTS
*	io_Error
*	ios2_WireError
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

/****** prism2.device/S2_MULTICAST *****************************************
*
*   NAME
*	S2_MULTICAST
*
*   FUNCTION
*
*   INPUTS
*	io_Flags
*	ios2_DstAddr - multicast address.
*	ios2_PacketType
*	ios2_DataLength
*	ios2_Data
*
*   RESULTS
*	io_Error
*	ios2_WireError
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

//error = 1;
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



/****** prism2.device/CMD_FLUSH ********************************************
*
*   NAME
*	CMD_FLUSH
*
*   FUNCTION
*
*   INPUTS
*	None.
*
*   RESULTS
*	io_Error
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

static BOOL CmdFlush(struct IORequest *request, struct DevBase *base)
{
   FlushUnit((APTR)request->io_Unit, EVENT_QUEUE, IOERR_ABORTED, base);

   return TRUE;
}



/****** prism2.device/S2_DEVICEQUERY ***************************************
*
*   NAME
*	S2_DEVICEQUERY -- Query device capabilities.
*
*   FUNCTION
*
*   INPUTS
*	ios2_StatData - Pointer to Sana2DeviceQuery structure.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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



/****** prism2.device/S2_GETSTATIONADDDRESS ********************************
*
*   NAME
*	S2_GETSTATIONADDDRESS
*
*   FUNCTION
*
*   INPUTS
*	None.
*
*   RESULTS
*	io_Error
*	ios2_WireError
*	ios2_SrcAddr - current address.
*	ios2_DstAddr - default address (zero if none?).
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



/****** prism2.device/S2_CONFIGINTERFACE ***********************************
*
*   NAME
*	S2_CONFIGINTERFACE
*
*   FUNCTION
*
*   INPUTS
*	ios2_SrcAddr - address to use.
*
*   RESULTS
*	io_Error
*	ios2_WireError
*	ios2_SrcAddr - address used.
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

static BOOL CmdConfigInterface(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   BYTE error = 0;
   ULONG wire_error = S2WERR_GENERIC_ERROR;

   /* Configure adapter */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_CONFIGURED) != 0)
   {
      error = S2ERR_BAD_STATE;
      wire_error = S2WERR_IS_CONFIGURED;
   }
   else if((unit->flags & UNITF_HAVEADAPTER) == 0)
   {
      error = S2ERR_BAD_STATE;
   }

   if(error == 0)
   {
      CopyMem(request->ios2_SrcAddr, unit->address, ETH_ADDRESSSIZE);
      ConfigureAdapter(unit, base);
      GoOnline(unit, base);
      unit->flags |= UNITF_CONFIGURED;
   }
   else
   {
      request->ios2_Req.io_Error = error;
      request->ios2_WireError = wire_error;
   }

   /* Return */

   return TRUE;
}



/****** prism2.device/S2_BROADCAST *****************************************
*
*   NAME
*	S2_BROADCAST
*
*   FUNCTION
*
*   INPUTS
*	io_Flags
*	ios2_PacketType
*	ios2_DataLength
*	ios2_Data
*
*   RESULTS
*	io_Error
*	ios2_WireError
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

static BOOL CmdBroadcast(struct IOSana2Req *request,
   struct DevBase *base)
{
   UWORD i;

   /* Fill in the broadcast address as destination */

   for(i = 0; i < ETH_ADDRESSSIZE; i++)
      request->ios2_DstAddr[i] = 0xff;

   /* Queue the write as normal */

   return CmdWrite(request, base);
}



/****** prism2.device/S2_TRACKTYPE *****************************************
*
*   NAME
*	S2_TRACKTYPE
*
*   FUNCTION
*
*   INPUTS
*	ios2_PacketType - packet type to start tracking.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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
   if(packet_type <= ETH_MTU)
      packet_type = ETH_MTU;

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



/****** prism2.device/S2_UNTRACKTYPE ***************************************
*
*   NAME
*	S2_UNTRACKTYPE
*
*   FUNCTION
*
*   INPUTS
*	ios2_PacketType - packet type to stop tracking.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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
   if(packet_type <= ETH_MTU)
      packet_type = ETH_MTU;

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



/****** prism2.device/S2_GETTYPESTATS **************************************
*
*   NAME
*	S2_GETTYPESTATS
*
*   FUNCTION
*
*   INPUTS
*	ios2_PacketType - packet type to get statistics on.
*	ios2_StatData - pointer to a Sana2PacketTypeStats structure.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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



/****** prism2.device/S2_GETSPECIALSTATS ***********************************
*
*   NAME
*	S2_GETSPECIALSTATS
*
*   FUNCTION
*
*   INPUTS
*	ios2_StatData - Pointer to Sana2SpecialStatHeader structure.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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

static BOOL CmdGetSpecialStats(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   UWORD i, stat_count;
   struct Sana2SpecialStatHeader *header;
   struct Sana2SpecialStatRecord *record;

   /* Fill in stats */

   unit = (APTR)request->ios2_Req.io_Unit;
   header = request->ios2_StatData;
   record = (APTR)(header + 1);

   stat_count = header->RecordCountMax;
   if(stat_count > STAT_COUNT)
      stat_count = STAT_COUNT;

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



/****** prism2.device/S2_GETGLOBALSTATS ************************************
*
*   NAME
*	S2_GETGLOBALSTATS
*
*   FUNCTION
*
*   INPUTS
*	ios2_StatData - Pointer to Sana2DeviceStats structure.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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

static BOOL CmdGetGlobalStats(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;

   /* Update and copy stats */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_ONLINE) != 0)
      UpdateStats(unit, base);
   CopyMem(&unit->stats, request->ios2_StatData,
      sizeof(struct Sana2DeviceStats));

   /* Return */

   return TRUE;
}



/****** prism2.device/S2_ONEVENT *******************************************
*
*   NAME
*	S2_ONEVENT
*
*   FUNCTION
*
*   INPUTS
*	ios2_WireError
*
*   RESULTS
*	io_Error
*	ios2_WireError
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
//if(wanted_events & S2EVENT_CONNECT) unit->special_stats[8]++;

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



/****** prism2.device/S2_READORPHAN ****************************************
*
*   NAME
*	S2_READORPHAN
*
*   FUNCTION
*
*   INPUTS
*	io_Flags
*	ios2_Data
*
*   RESULTS
*	io_Flags
*	io_Error
*	ios2_WireError
*	ios2_PacketType - A copy of the packet's type field.
*	ios2_SrcAddr
*	ios2_DstAddr
*	ios2_DataLength
*	ios2_Data
*
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



/****** prism2.device/S2_ONLINE ********************************************
*
*   NAME
*	S2_ONLINE
*
*   FUNCTION
*
*   INPUTS
*	None.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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

   if(error == 0 && (unit->flags & UNITF_ONLINE) == 0)
   {
      unit->stats.PacketsReceived = 0;
      unit->stats.PacketsSent = 0;
      unit->stats.BadData = 0;
      unit->stats.Overruns = 0;
      unit->stats.UnknownTypesReceived = 0;
      unit->stats.Reconfigurations = 0;

      for(i = 0; i < STAT_COUNT; i++)
         unit->special_stats[i] = 0;

      GoOnline(unit, base);
   }

   /* Return */

   request->ios2_Req.io_Error = error;
   request->ios2_WireError = wire_error;
   return TRUE;
}



/****** prism2.device/S2_OFFLINE *******************************************
*
*   NAME
*	S2_OFFLINE
*
*   FUNCTION
*
*   INPUTS
*	None.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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



/****** prism2.device/NSCMD_DEVICEQUERY ************************************
*
*   NAME
*	NSCMD_DEVICEQUERY -- Query device capabilities.
*
*   FUNCTION
*
*   INPUTS
*	io_Length - ???.
*	io_Data - pointer to NSDeviceQueryResult structure.
*
*   RESULTS
*	io_Error
*	io_Actual - size of structure device can handle.
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



/****** prism2.device/S2_ADDMULTICASTADDRESS *******************************
*
*   NAME
*	S2_ADDMULTICASTADDRESS
*
*   FUNCTION
*
*   INPUTS
*	ios2_SrcAddr - multicast address.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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

/****** prism2.device/S2_ADDMULTICASTADDRESSES *****************************
*
*   NAME
*	S2_ADDMULTICASTADDRESSES
*
*   FUNCTION
*
*   INPUTS
*	ios2_SrcAddr - lower bound.
*	ios2_DstAddr - upper bound.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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

static BOOL CmdAddMulticastAddresses(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   UBYTE *lower_bound, *upper_bound;

   unit = (APTR)request->ios2_Req.io_Unit;

   lower_bound = request->ios2_SrcAddr;
   if(request->ios2_Req.io_Command == S2_ADDMULTICASTADDRESS)
      upper_bound = lower_bound;
   else
      upper_bound = request->ios2_DstAddr;

   if(!AddMulticastRange(unit, lower_bound, upper_bound, base))
   {
      request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
      request->ios2_WireError = S2WERR_GENERIC_ERROR;
   }

   /* Return */

   return TRUE;
}



/****** prism2.device/S2_DELMULTICASTADDRESS *******************************
*
*   NAME
*	S2_DELMULTICASTADDRESS
*
*   FUNCTION
*
*   INPUTS
*	ios2_SrcAddr - multicast address.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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

/****** prism2.device/S2_DELMULTICASTADDRESSES *****************************
*
*   NAME
*	S2_DELMULTICASTADDRESSES
*
*   FUNCTION
*
*   INPUTS
*	ios2_SrcAddr - lower bound.
*	ios2_DstAddr - upper bound.
*
*   RESULTS
*	io_Error
*	ios2_WireError
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

static BOOL CmdDelMulticastAddresses(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   UBYTE *lower_bound, *upper_bound;

   unit = (APTR)request->ios2_Req.io_Unit;

   lower_bound = request->ios2_SrcAddr;
   if(request->ios2_Req.io_Command == S2_DELMULTICASTADDRESS)
      upper_bound = lower_bound;
   else
      upper_bound = request->ios2_DstAddr;

   if(!RemMulticastRange(unit, lower_bound, upper_bound, base))
   {
      request->ios2_Req.io_Error = S2ERR_BAD_STATE;
      request->ios2_WireError = S2WERR_BAD_MULTICAST;
   }

   /* Return */

   return TRUE;
}



/****** prism2.device/S2_GETSIGNALQUALITY **********************************
*
*   NAME
*	S2_GETSIGNALQUALITY -- Get signal quality statistics.
*
*   FUNCTION
*	This command fills in the supplied Sana2SignalQuality structure with
*	current signal and noise levels. The unit for these figures is dBm.
*	Typically, they are negative values.
*
*   INPUTS
*	ios2_StatData - Pointer to Sana2SignalQuality structure.
*
*   RESULTS
*	io_Error - Zero if successful; non-zero otherwise.
*	ios2_WireError - More specific error code.
*
****************************************************************************
*
*/

static BOOL CmdGetSignalQuality(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;

   /* Update and copy stats */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_ONLINE) != 0)
   {
      UpdateSignalQuality(unit, base);
      CopyMem(&unit->signal_quality, request->ios2_StatData,
         sizeof(struct Sana2SignalQuality));
   }
   else
   {
      request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
      request->ios2_WireError = S2WERR_UNIT_OFFLINE;
   }

   /* Return */

   return TRUE;
}



/****** prism2.device/S2_GETNETWORKS ***************************************
*
*   NAME
*	S2_GETNETWORKS -- Scan for available networks.
*
*   FUNCTION
*	This command supplies details of available networks. If the scan
*	should be limited to one specific network, the S2INFO_SSID tag
*	should specify its name.
*
*	If this command completes successfully, ios2_StatData will contain
*	an array of pointers to tag lists, each of which contains
*	information on a single network. The device will set ios2_DataLength
*	to the number of elements in this array.
*
*	The returned taglists are allocated from the supplied memory pool.
*	To discard the results of this command, the entire memory pool
*	should be destroyed.
*
*   INPUTS
*	ios2_Data - Pointer to an Exec memory pool.
*	ios2_StatData - Pointer to taglist that specifies parameters to use.
*
*   RESULTS
*	io_Error - Zero if successful; non-zero otherwise.
*	ios2_WireError - More specific error code.
*	ios2_DataLength - Number of tag lists returned.
*	ios2_Data - Remains unchanged.
*	ios2_StatData - Pointer to an array of tag lists.
*
****************************************************************************
*
*/

static BOOL CmdGetNetworks(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   BOOL complete = FALSE;
   const TEXT *ssid;
   const struct TagItem *tag_list;

   /* Request a new scan and queue request to receive results */

   unit = (APTR)request->ios2_Req.io_Unit;
   if((unit->flags & UNITF_ONLINE) != 0)
   {
      PutRequest(unit->request_ports[SCAN_QUEUE], (APTR)request, base);
      tag_list = (const struct TagItem *)request->ios2_StatData;
      ssid = (const TEXT *)GetTagData(S2INFO_SSID, (UPINT)NULL, tag_list);
      StartScan(unit, ssid, base);
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



/****** prism2.device/S2_SETOPTIONS ****************************************
*
*   NAME
*	S2_SETOPTIONS -- Associate with a network.
*
*   FUNCTION
*	Associate with a specified network using the parameters supplied.[?]
*	Set various parameters for the network interface. This command
*	should be called before going online to set any essential parameters
*	not covered elsewhere.
*
*   INPUTS
*	ios2_Data - Pointer to taglist that specifies the network and
*	    parameters to use.
*
*   RESULTS
*	io_Error - Zero if successful; non-zero otherwise.
*	ios2_WireError - More specific error code.
*
****************************************************************************
*
*/

static BOOL CmdSetOptions(struct IOSana2Req *request, struct DevBase *base)
{
   struct DevUnit *unit;

   /*  */

   unit = (APTR)request->ios2_Req.io_Unit;
   SetOptions(unit, request->ios2_Data, base);
#if 1
   if((unit->flags & UNITF_ONLINE) != 0)
//&& FindTagItem(P2OPT_Key, request->ios2_Data) == NULL)
      ConfigureAdapter(unit, base);
#endif
   unit->stats.Reconfigurations++;

   /* Return */

   return TRUE;
}



/****** prism2.device/S2_SETKEY ********************************************
*
*   NAME
*	S2_SETKEY -- Set an encryption key.
*
*   FUNCTION
*
*   INPUTS
*	ios2_WireError - Key index.
*	ios2_PacketType - Encryption type (e.g. S2ENC_WEP).
*	ios2_DataLength - Key length.
*	ios2_Data - Key.
*	ios2_StatData - RX counter number (NULL if unused).
*
*   RESULTS
*	io_Error
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

static BOOL CmdSetKey(struct IOSana2Req *request, struct DevBase *base)
{
   struct DevUnit *unit;

   unit = (APTR)request->ios2_Req.io_Unit;
   SetKey(unit, request->ios2_WireError, request->ios2_PacketType,
      request->ios2_Data, request->ios2_DataLength, request->ios2_StatData,
      base);

   /* Return */

   return TRUE;
}



/****** prism2.device/S2_GETNETWORKINFO ************************************
*
*   NAME
*	S2_GETNETWORKINFO -- Get information on current network.
*
*   FUNCTION
*
*   INPUTS
*	ios2_Data - Pointer to an Exec memory pool.
*
*   RESULTS
*	ios2_Data - Remains unchanged.
*	ios2_StatData - Pointer to a tag list.
*	io_Error - Zero if successful; non-zero otherwise.
*	ios2_WireError - More specific error code.
*
****************************************************************************
*
*/

static BOOL CmdGetNetworkInfo(struct IOSana2Req *request,
   struct DevBase *base)
{
   struct DevUnit *unit;
   APTR pool;

   /* Request information on current network */

   unit = (APTR)request->ios2_Req.io_Unit;
   pool = request->ios2_Data;
   if((unit->flags & UNITF_ONLINE) != 0)
   {
      request->ios2_StatData = GetNetworkInfo(unit, pool, base);
      if(request->ios2_StatData == NULL)
         request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
   }
   else
   {
      request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
      request->ios2_WireError = S2WERR_UNIT_OFFLINE;
   }

   /* Return */

   return TRUE;
}



/****i* prism2.device/PutRequest *******************************************
*
*   NAME
*	PutRequest
*
*   SYNOPSIS
*	PutRequest(port, request)
*
*	VOID PutRequest(struct MsgPort *, struct IORequest *);
*
*   FUNCTION
*
*   INPUTS
*	port
*	request
*
*   RESULT
*	None.
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



