/*

Copyright (C) 2000-2011 Neil Cafferkey

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
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <libraries/poseidon.h>
#include <devices/usb.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/poseidon.h>
#include <proto/alib.h>

#include "device.h"
#include "realtek8187.h"

#include "usb_protos.h"
#include "device_protos.h"
#include "unit_protos.h"


struct BusContext
{
   struct DevUnit *unit;
   struct DevBase *device;
   APTR binding;
   struct Hook unbind_hook;
   APTR usb_device;
   struct MsgPort *msg_port;
   struct MsgPort *tx_pipe_port;
   struct MsgPort *rx_pipe_port;
   APTR control_pipe;
   APTR tx_pipes[TX_SLOT_COUNT];
   APTR rx_pipes[RX_SLOT_COUNT];
   struct Interrupt tx_pipe_int;
   struct Interrupt rx_pipe_int;
};


/* Private prototypes */

static struct DevUnit *FindUSBUnit(ULONG index, struct DevBase *base);
static struct DevUnit *CreateUSBUnit(ULONG index,
   struct DevBase *base);
static VOID DeleteUSBUnitInternal(struct BusContext *context,
   struct DevBase *base);
static struct BusContext *AllocDevice(ULONG index, struct DevBase *base);
static VOID FreeDevice(struct BusContext *context, struct DevBase *base);
static BOOL IsDeviceCompatible(UWORD vendor_id, UWORD product_id,
   struct DevBase *base);
static VOID ReleaseBindingHook(struct Hook *hook, APTR object,
   APTR message);
static VOID TXPipeInt(REG(a1, struct BusContext *context),
   REG(a5, APTR int_code));
static VOID RXPipeInt(REG(a1, struct BusContext *context),
   REG(a5, APTR int_code));
static UBYTE ByteInHook(struct BusContext *context, ULONG offset);
static VOID ByteOutHook(struct BusContext *context, ULONG offset,
   UBYTE value);
static UWORD LEWordInHook(struct BusContext *context, ULONG offset);
static ULONG LELongInHook(struct BusContext *context, ULONG offset);
static VOID LEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value);
static VOID LELongOutHook(struct BusContext *context, ULONG offset,
   ULONG value);
static APTR AllocDMAMemHook(struct BusContext *context, UPINT size,
   UWORD alignment);
static VOID FreeDMAMemHook(struct BusContext *context, APTR mem);
static VOID SendFrameHook(struct BusContext *context, APTR data,
   ULONG length);
static VOID ReceiveFrameHook(struct BusContext *context, APTR buffer,
   ULONG length);


static const UWORD product_codes[] =
{
//   0x03f0, 0xca02   // L
   0x050d, 0x705e,
//   0x0769, 0x11f2,   // L
//   0x0789, 0x010c,   // L
   0x0846, 0x4260,
//   0x0846, 0x6100,   // L
//   0x0846, 0x6a00,   // L
//   0x0b05, 0x171d,   // L
//   0x0bda, 0x8187,   // L
   0x0bda, 0x8189,
   0x0bda, 0x8197,
   0x0bda, 0x8198,
//   0x0df6, 0x000d,   // L
   0x0df6, 0x0028,
   0x0df6, 0x0029,
//   0x114b, 0x0150,   // L
//   0x1371, 0x9401,   // L
//   0x13d1, 0xabe6,   // L
   0x1737, 0x0073,
//   0x18e8, 0x6232,   // L
//   0x1b75, 0x8187   // L
   0
};


static const struct TagItem unit_tags[] =
{
   {IOTAG_ByteIn, (UPINT)ByteInHook},
   {IOTAG_ByteOut, (UPINT)ByteOutHook},
   {IOTAG_LEWordIn, (UPINT)LEWordInHook},
   {IOTAG_LELongIn, (UPINT)LELongInHook},
   {IOTAG_LEWordOut, (UPINT)LEWordOutHook},
   {IOTAG_LELongOut, (UPINT)LELongOutHook},
   {IOTAG_AllocDMAMem, (UPINT)AllocDMAMemHook},
   {IOTAG_FreeDMAMem, (UPINT)FreeDMAMemHook},
   {IOTAG_SendFrame, (UPINT)SendFrameHook},
   {IOTAG_ReceiveFrame, (UPINT)ReceiveFrameHook},
   {TAG_END, 0}
};


/****i* realtek8180.device/GetUSBCount *************************************
*
*   NAME
*	GetUSBCount -- Get the number of compatible USB devices.
*
*   SYNOPSIS
*	count = GetUSBCount()
*
*	ULONG GetUSBCount();
*
****************************************************************************
*
*/

ULONG GetUSBCount(struct DevBase *base)
{
   ULONG count = 0;
   APTR device = NULL;
   UPINT vendor_id, product_id;

   psdLockReadPBase();
   while((device = psdGetNextDevice(device)) != NULL)
   {
      psdGetAttrs(PGA_DEVICE, device, DA_VendorID, (UPINT)&vendor_id,
         DA_ProductID, (UPINT)&product_id, TAG_END);
      if(IsDeviceCompatible(vendor_id, product_id, base))
         count++;
   }
   psdUnlockPBase();

   return count;
}



/****i* realtek8180.device/GetUSBUnit **************************************
*
*   NAME
*	GetUSBUnit -- Get a unit by number.
*
*   SYNOPSIS
*	unit = GetUSBUnit(index)
*
*	struct DevUnit *GetUSBUnit(ULONG);
*
****************************************************************************
*
*/

struct DevUnit *GetUSBUnit(ULONG index, struct DevBase *base)
{
   struct DevUnit *unit;

   unit = FindUSBUnit(index, base);

   if(unit == NULL)
   {
      unit = CreateUSBUnit(index, base);
      if(unit != NULL)
         AddTail((APTR)&base->usb_units, (APTR)unit);
   }

   return unit;
}



/****i* realtek8180.device/FindUSBUnit *************************************
*
*   NAME
*	FindUSBUnit -- Find a unit by number.
*
*   SYNOPSIS
*	unit = FindUSBUnit(index)
*
*	struct DevUnit *FindUSBUnit(ULONG);
*
****************************************************************************
*
*/

static struct DevUnit *FindUSBUnit(ULONG index, struct DevBase *base)
{
   struct DevUnit *unit, *tail;

   unit = (APTR)base->usb_units.mlh_Head;
   tail = (APTR)&base->usb_units.mlh_Tail;
   if(unit == tail)
      unit = NULL;

   return unit;
}



/****i* realtek8180.device/CreateUSBUnit ***********************************
*
*   NAME
*	CreateUSBUnit -- Create a unit.
*
*   SYNOPSIS
*	unit = CreateUSBUnit(index)
*
*	struct DevUnit *CreateUSBUnit(ULONG);
*
*   FUNCTION
*	Creates a new unit.
*
****************************************************************************
*
*/

static struct DevUnit *CreateUSBUnit(ULONG index,
   struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   struct DevUnit *unit;
   UWORD i;

   /* Get device from system */

   context = AllocDevice(index, base);
   if(context == NULL)
      success = FALSE;

   /* Create device driver unit */

   if(success)
   {
      context->device = base;
      context->unit = unit =
         CreateUnit(index, context, unit_tags, USB_BUS, base);
      if(unit == NULL)
         success = FALSE;
   }

   if(success)
   {
      if(!(WrapInt(&unit->tx_int, base)
         && WrapInt(&unit->mgmt_int, base)
         && WrapInt(&context->tx_pipe_int, base)
         && WrapInt(&context->rx_pipe_int, base)))
         success = FALSE;
   }

   /* Send out RX pipes */

   if(success)
   {
      for(i = 0; i < RX_SLOT_COUNT; i++)
         psdSendPipe(context->rx_pipes[i], unit->rx_buffers[i],
            FRAME_BUFFER_SIZE + R8180_MAXDESCSIZE);
   }

   if(!success)
   {
      if(context != NULL)
         DeleteUSBUnitInternal(context, base);
      unit = NULL;
   }

   return unit;
}



/****i* realtek8180.device/DeleteUSBUnit ***********************************
*
*   NAME
*	DeleteUSBUnit -- Delete a unit.
*
*   SYNOPSIS
*	DeleteUSBUnit(unit)
*
*	VOID DeleteUSBUnit(struct DevUnit *);
*
*   FUNCTION
*	Deletes a unit.
*
*   INPUTS
*	unit - Device unit (may be NULL).
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

VOID DeleteUSBUnit(struct DevUnit *unit, struct DevBase *base)
{
   if(unit != NULL)
      DeleteUSBUnitInternal(unit->card, base);

   return;
}



/****i* realtek8180.device/DeleteUSBUnitInternal ***************************
*
*   NAME
*	DeleteUSBUnitInternal -- Delete a unit.
*
*   SYNOPSIS
*	DeleteUSBUnitInternal(context)
*
*	VOID DeleteUSBUnitInternal(struct BusContext *);
*
*   FUNCTION
*	Deletes a unit.
*
*   INPUTS
*	context
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

static VOID DeleteUSBUnitInternal(struct BusContext *context,
   struct DevBase *base)
{
   struct DevUnit *unit;
   UWORD i;
   APTR pipe;

   unit = context->unit;
   if(unit != NULL)
   {
      /* Abort pipes */

      context->tx_pipe_port->mp_Flags = PA_SIGNAL;
      context->tx_pipe_port->mp_SigTask = FindTask(NULL);
      for(i = 0; i < TX_SLOT_COUNT; i++)
      {
         pipe = context->tx_pipes[i];
         if(pipe != NULL)
         {
            psdAbortPipe(pipe);
            psdWaitPipe(pipe);
         }
      }

      context->rx_pipe_port->mp_Flags = PA_SIGNAL;
      context->rx_pipe_port->mp_SigTask = FindTask(NULL);
      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         pipe = context->rx_pipes[i];
         if(pipe != NULL)
         {
            psdAbortPipe(pipe);
            psdWaitPipe(pipe);
         }
      }

      /* Remove interrupt wrapper code */

      UnwrapInt(&context->rx_pipe_int, base);
      UnwrapInt(&context->tx_pipe_int, base);
      UnwrapInt(&unit->mgmt_int, base);
      UnwrapInt(&unit->tx_int, base);

      /* Shut down unit and release hardware */

      DeleteUnit(unit, base);
      context->unit = NULL;
   }
   FreeDevice(context, base);

   return;
}



/****i* realtek8180.device/AllocDevice *************************************
*
*   NAME
*	AllocDevice -- Take control of a device.
*
*   SYNOPSIS
*	context = AllocDevice(index)
*
*	struct BusContext *AllocDevice(ULONG);
*
****************************************************************************
*
*/

static struct BusContext *AllocDevice(ULONG index, struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   APTR device = NULL, interface = NULL, endpoint;
   UWORD i = 0;
   UPINT vendor_id, product_id;
   struct Hook *hook;

   /* Allocate context */

   context = AllocMem(sizeof(struct BusContext), MEMF_PUBLIC | MEMF_CLEAR);
   if(context == NULL)
      success = FALSE;

   if(success)
   {
      context->tx_pipe_port = AllocVec(sizeof(struct MsgPort),
         MEMF_PUBLIC | MEMF_CLEAR);
      context->rx_pipe_port = AllocVec(sizeof(struct MsgPort),
         MEMF_PUBLIC | MEMF_CLEAR);
      if(context->tx_pipe_port == NULL || context->rx_pipe_port == NULL)
         success = FALSE;
   }

   if(success)
   {
      NewList(&context->tx_pipe_port->mp_MsgList);
      context->tx_pipe_port->mp_Flags = PA_SOFTINT;
      context->tx_pipe_port->mp_SigTask = &context->tx_pipe_int;

      context->tx_pipe_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      context->tx_pipe_int.is_Code = (APTR)TXPipeInt;
      context->tx_pipe_int.is_Data = context;

      NewList(&context->rx_pipe_port->mp_MsgList);
      context->rx_pipe_port->mp_Flags = PA_SOFTINT;
      context->rx_pipe_port->mp_SigTask = &context->rx_pipe_int;

      context->rx_pipe_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      context->rx_pipe_int.is_Code = (APTR)RXPipeInt;
      context->rx_pipe_int.is_Data = context;

      context->msg_port = CreateMsgPort();
      if(context->msg_port == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Prepare unbinding hook */

      hook = &context->unbind_hook;
      hook->h_Entry = HookEntry;
      hook->h_SubEntry = (APTR)ReleaseBindingHook;
      hook->h_Data = context;

      /* Find a compatible device */

      psdLockReadPBase();
      device = psdGetNextDevice(NULL);
      while(i <= index && device != NULL)
      {
         psdGetAttrs(PGA_DEVICE, device, DA_VendorID, (UPINT)&vendor_id,
            DA_ProductID, (UPINT)&product_id, TAG_END);
         if(IsDeviceCompatible(vendor_id, product_id, base))
            i++;
         if(i <= index)
            device = psdGetNextDevice(device);
      }

      /* Create binding to device */

      if(device != NULL)
         context->binding = psdClaimAppBinding(ABA_ReleaseHook, hook,
            ABA_Device, device, TAG_END);
      psdUnlockPBase();

      context->usb_device = device;
      if(device == NULL || context->binding == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Get interface */

      interface = psdFindInterface(device, NULL, TAG_END);
      if(interface == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Allocate endpoint pipes */

      context->control_pipe = psdAllocPipe(device, context->msg_port, NULL);
      if(context->control_pipe == NULL)
         success = FALSE;

      endpoint = psdFindEndpoint(interface, NULL, EA_IsIn, FALSE,
         EA_TransferType, USEAF_BULK, EA_EndpointNum, 4, TAG_END);
      for(i = 0; i < TX_SLOT_COUNT; i++)
      {
         context->tx_pipes[i] =
            psdAllocPipe(device, context->tx_pipe_port, endpoint);
         if(context->tx_pipes[i] == NULL)
            success = FALSE;
      }

      endpoint = psdFindEndpoint(interface, NULL, EA_IsIn, TRUE,
         EA_TransferType, USEAF_BULK, EA_EndpointNum, 3, TAG_END);
      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         context->rx_pipes[i] =
            psdAllocPipe(device, context->rx_pipe_port, endpoint);
         if(context->rx_pipes[i] == NULL)
            success = FALSE;
      }
   }

   if(!success)
   {
      FreeDevice(context, base);
      context = NULL;
   }

   return context;
}



/****i* realtek8180.device/FreeDevice **************************************
*
*   NAME
*	FreeDevice -- Release a device.
*
*   SYNOPSIS
*	FreeDevice(context)
*
*	VOID FreeDevice(struct BusContext *);
*
****************************************************************************
*
*/

static VOID FreeDevice(struct BusContext *context, struct DevBase *base)
{
   APTR pipe;
   UWORD i;

   if(context != NULL)
   {
      psdReleaseAppBinding(context->binding);

      /* Free pipes */

      if(context->control_pipe != NULL)
         psdFreePipe(context->control_pipe);

      for(i = 0; i < TX_SLOT_COUNT; i++)
      {
         pipe = context->tx_pipes[i];
         if(pipe != NULL)
            psdFreePipe(pipe);
      }

      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         pipe = context->rx_pipes[i];
         if(pipe != NULL)
            psdFreePipe(pipe);
      }

      /* Free message ports */

      FreeVec(context->tx_pipe_port);
      FreeVec(context->rx_pipe_port);
      DeleteMsgPort(context->msg_port);

      /* Free context */

      FreeMem(context, sizeof(struct BusContext));
   }

   return;
}



/****i* realtek8180.device/IsDeviceCompatible ******************************
*
*   NAME
*	IsDeviceCompatible
*
*   SYNOPSIS
*	compatible = IsDeviceCompatible(vendor_id, product_id)
*
*	BOOL IsDeviceCompatible(UWORD, UWORD);
*
****************************************************************************
*
*/

static BOOL IsDeviceCompatible(UWORD vendor_id, UWORD product_id,
   struct DevBase *base)
{
   BOOL compatible = FALSE;
   const UWORD *p;

   for(p = product_codes; p[0] != 0; p += 2)
   {
      if(p[0] == vendor_id && p[1] == product_id)
         compatible = TRUE;
   }

   return compatible;
}



/****i* realtek8180.device/ReleaseBindingHook ******************************
*
*   NAME
*	ReleaseBindingHook
*
*   SYNOPSIS
*	ReleaseBindingHook(hook, object, message)
*
*	VOID ReleaseBindingHook(struct Hook *, APTR, APTR);
*
****************************************************************************
*
*/

static VOID ReleaseBindingHook(struct Hook *hook, APTR object, APTR message)
{
   struct DevBase *base;
   struct DevUnit *unit;
   struct BusContext *context;

   /* Record loss of device and inform unit's task */

   context = hook->h_Data;
   unit = context->unit;
   if(unit != NULL)
   {
      base = unit->device;
      if((unit->flags & UNITF_ONLINE) != 0)
         unit->flags |= UNITF_WASONLINE;
      unit->flags &= ~(UNITF_HAVEADAPTER | UNITF_ONLINE);
      Signal(unit->task, unit->card_removed_signal);
   }

   return;
}



/****i* realtek8180.device/TXPipeInt ***************************************
*
*   NAME
*	TXPipeInt
*
*   SYNOPSIS
*	TXPipeInt(context, int_code)
*
*	VOID TXPipeInt(struct BusContext *, APTR);
*
****************************************************************************
*
*/

static VOID TXPipeInt(REG(a1, struct BusContext *context),
   REG(a5, APTR int_code))
{
   struct DevBase *base;
   struct DevUnit *unit;
   APTR pipe;

   unit = context->unit;
   base = unit->device;

   while((pipe = GetMsg(context->tx_pipe_port)) != NULL)
      RetireTXSlot(unit, base);

   return;
}



/****i* realtek8180.device/RXPipeInt ***************************************
*
*   NAME
*	RXPipeInt
*
*   SYNOPSIS
*	RXPipeInt(context, int_code)
*
*	VOID RXPipeInt(struct BusContext *, APTR);
*
****************************************************************************
*
*/

static VOID RXPipeInt(REG(a1, struct BusContext *context),
   REG(a5, APTR int_code))
{
   struct DevBase *base;
   struct DevUnit *unit;
   APTR pipe;
   UWORD slot, i;

   unit = context->unit;
   base = unit->device;

   while((pipe = GetMsg(context->rx_pipe_port)) != NULL)
   {
      if((unit->flags & UNITF_ONLINE) != 0)
      {
         /* Find slot number for the pipe */

         for(i = 0; context->rx_pipes[i] != pipe && i < RX_SLOT_COUNT; i++);
         unit->rx_slot = slot = i;

         unit->rx_descs[slot] = unit->rx_buffers[slot]
            + psdGetPipeActual(pipe) - unit->rx_desc_size;
         RXInt(context->unit, RXInt);
      }
   }

   return;
}



/****i* realtek8180.device/ReadControlPipe *********************************
*
*   NAME
*	ReadControlPipe
*
*   SYNOPSIS
*	mask = ReadControlPipe(context, )
*
*	UBYTE ReadControlPipe(struct BusContext *, );
*
****************************************************************************
*
*/

static LONG ReadControlPipe(struct BusContext *context, ULONG offset,
   APTR data, ULONG length, struct DevBase *base)
{
   LONG ioerr;

   /* Patch pipe's message port to signal this task when finished */

   context->msg_port->mp_SigTask = FindTask(NULL);

   psdPipeSetup(context->control_pipe, URTF_IN | URTF_DEVICE | URTF_VENDOR,
      R8180UCMD_REG, 0xfe00 + (0xffff & offset), offset >> 16);
   ioerr = psdDoPipe(context->control_pipe, data, length);

   return ioerr;
}



/****i* realtek8180.device/WriteControlPipe ********************************
*
*   NAME
*	WriteControlPipe
*
*   SYNOPSIS
*	mask = WriteControlPipe(context, )
*
*	UBYTE WriteControlPipe(struct BusContext *, );
*
****************************************************************************
*
*/

static LONG WriteControlPipe(struct BusContext *context, ULONG offset,
   APTR data, ULONG length, struct DevBase *base)
{
   LONG ioerr;

   /* Patch pipe's message port to signal this task when finished */

   context->msg_port->mp_SigTask = FindTask(NULL);

   psdPipeSetup(context->control_pipe, URTF_OUT | URTF_DEVICE | URTF_VENDOR,
      R8180UCMD_REG, 0xfe00 + (offset & 0xffff), offset >> 16);
   ioerr = psdDoPipe(context->control_pipe, data, length);

   return ioerr;
}



/****i* realtek8180.device/ByteInHook **************************************
*
*   NAME
*	ByteInHook
*
*   SYNOPSIS
*	value = ByteInHook(context, offset)
*
*	UBYTE ByteInHook(struct BusContext *, ULONG);
*
****************************************************************************
*
*/

static UBYTE ByteInHook(struct BusContext *context, ULONG offset)
{
   struct DevBase *base;
   UBYTE value;

   base = context->device;
   ReadControlPipe(context, offset, &value, sizeof(value), base);

   return value;
}



/****i* realtek8180.device/ByteOutHook *************************************
*
*   NAME
*	ByteOutHook
*
*   SYNOPSIS
*	ByteOutHook(context, offset, value)
*
*	VOID ByteOutHook(struct BusContext *, ULONG, UBYTE);
*
****************************************************************************
*
*/

static VOID ByteOutHook(struct BusContext *context, ULONG offset,
   UBYTE value)
{
   struct DevBase *base;

   base = context->device;
   WriteControlPipe(context, offset, &value, sizeof(value), base);

   return;
}



/****i* realtek8180.device/LEWordInHook ************************************
*
*   NAME
*	LEWordInHook
*
*   SYNOPSIS
*	value = LEWordInHook(context, offset)
*
*	UWORD LEWordInHook(struct BusContext *, ULONG);
*
****************************************************************************
*
*/

static UWORD LEWordInHook(struct BusContext *context, ULONG offset)
{
   struct DevBase *base;
   UWORD value;

   base = context->device;
   ReadControlPipe(context, offset, &value, sizeof(value), base);

   return LEWord(value);
}



/****i* realtek8180.device/LELongInHook ************************************
*
*   NAME
*	LELongInHook
*
*   SYNOPSIS
*	value = LELongInHook(context, offset)
*
*	ULONG LELongInHook(struct BusContext *, ULONG);
*
****************************************************************************
*
*/

static ULONG LELongInHook(struct BusContext *context, ULONG offset)
{
   struct DevBase *base;
   ULONG value;

   base = context->device;
   ReadControlPipe(context, offset, &value, sizeof(value), base);

   return LELong(value);
}



/****i* realtek8180.device/LEWordOutHook ***********************************
*
*   NAME
*	LEWordOutHook
*
*   SYNOPSIS
*	LEWordOutHook(context, offset, value)
*
*	VOID LEWordOutHook(struct BusContext *, ULONG, UWORD);
*
****************************************************************************
*
*/

static VOID LEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value)
{
   struct DevBase *base;

   base = context->device;
   value = MakeLEWord(value);
   WriteControlPipe(context, offset, &value, sizeof(value), base);

   return;
}



/****i* realtek8180.device/LELongOutHook ***********************************
*
*   NAME
*	LELongOutHook
*
*   SYNOPSIS
*	LELongOutHook(context, offset, value)
*
*	VOID LELongOutHook(struct BusContext *, ULONG, ULONG);
*
****************************************************************************
*
*/

static VOID LELongOutHook(struct BusContext *context, ULONG offset,
   ULONG value)
{
   struct DevBase *base;

   base = context->device;
   value = MakeLELong(value);
   WriteControlPipe(context, offset, &value, sizeof(value), base);

   return;
}



/****i* realtek8180.device/AllocDMAMemHook *********************************
*
*   NAME
*       AllocDMAMemHook
*
*   SYNOPSIS
*       mem = AllocDMAMemHook(context, size, alignment)
*
*       APTR AllocDMAMemHook(struct BusContext *, UPINT, UWORD);
*
****************************************************************************
*
*/

static APTR AllocDMAMemHook(struct BusContext *context, UPINT size,
   UWORD alignment)
{
   struct DevBase *base;
   APTR mem = NULL;

   base = context->device;
   mem = AllocVec(size, MEMF_PUBLIC);

   return mem;
}



/****i* realtek8180.device/FreeDMAMemHook **********************************
*
*   NAME
*       FreeDMAMemHook
*
*   SYNOPSIS
*       FreeDMAMemHook(context, mem)
*
*       VOID FreeDMAMemHook(struct BusContext *, APTR);
*
****************************************************************************
*
*/

static VOID FreeDMAMemHook(struct BusContext *context, APTR mem)
{
   struct DevBase *base;

   base = context->device;
   FreeVec(mem);

   return;
}



/****i* realtek8180.device/SendFrameHook ***********************************
*
*   NAME
*	SendFrameHook
*
*   SYNOPSIS
*	mask = SendFrameHook(context, frame, length) ???
*
*	UBYTE SendFrameHook(struct BusContext *, UBYTE *, ULONG); ???
*
****************************************************************************
*
*/

static VOID SendFrameHook(struct BusContext *context, APTR data,
   ULONG length)
{
   struct DevBase *base;
   struct DevUnit *unit;

   unit = context->unit;
   base = unit->device;

   psdSendPipe(context->tx_pipes[unit->tx_in_slot], data, length);

   return;
}



/****i* realtek8180.device/ReceiveFrameHook ********************************
*
*   NAME
*	ReceiveFrameHook
*
*   SYNOPSIS
*	mask = ReceiveFrameHook(context, buffer, length) ???
*
*	UBYTE ReceiveFrameHook(struct BusContext *, UBYTE *, ULONG); ???
*
****************************************************************************
*
*/

static VOID ReceiveFrameHook(struct BusContext *context, APTR buffer,
   ULONG length)
{
   struct DevBase *base;
   struct DevUnit *unit;

   unit = context->unit;
   base = unit->device;

   psdSendPipe(context->rx_pipes[unit->rx_slot], buffer, length);

   return;
}



