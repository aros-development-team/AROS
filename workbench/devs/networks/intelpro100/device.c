/*

Copyright (C) 2000-2005 Neil Cafferkey

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
#include <exec/resident.h>
#include <exec/errors.h>
#include <utility/utility.h>
#include "initializers.h"

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include "device.h"

#include "pci_protos.h"
#include "request_protos.h"


/* Private prototypes */

static struct DevBase *DevInit(REG(d0, struct DevBase *dev_base),
   REG(a0, APTR seg_list), REG(BASE_REG, struct DevBase *base));
static BYTE DevOpen(REG(a1, struct IOSana2Req *request),
   REG(d0, ULONG unit_num), REG(d1, ULONG flags),
   REG(BASE_REG, struct DevBase *base));
static APTR DevClose(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base));
static APTR DevExpunge(REG(BASE_REG, struct DevBase *base));
static APTR DevReserved();
static VOID DevBeginIO(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base));
static VOID DevAbortIO(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base));
static VOID DeleteDevice(struct DevBase *base);
static struct DevUnit *GetUnit(ULONG unit_num, struct DevBase *base);

/* extern const APTR vectors[]; */
extern const APTR init_table[];


/* Return an error immediately if someone tries to run the device */

LONG Main()
{
   return -1;
}


const TEXT device_name[] = DEVICE_NAME;
static const TEXT version_string[] =
   DEVICE_NAME " " STR(VERSION) "." STR(REVISION) " (" DATE ")\n";
static const TEXT utility_name[] = UTILITYNAME;
static const TEXT prometheus_name[] = "prometheus.library";
static const TEXT powerpci_name[] = "powerpci.library";
static const TEXT timer_name[] = TIMERNAME;


const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AUTOINIT,
   VERSION,
   NT_DEVICE,
   0,
   (STRPTR)device_name,
   (STRPTR)version_string,
   (APTR)init_table
};


static const APTR vectors[] =
{
   (APTR)DevOpen,
   (APTR)DevClose,
   (APTR)DevExpunge,
   (APTR)DevReserved,
   (APTR)DevBeginIO,
   (APTR)DevAbortIO,
   (APTR)-1
};


#ifdef __MORPHOS__
#pragma pack(2)
#endif
static const struct
{
   SMALLINITBYTEDEF(type);
   SMALLINITPINTDEF(name);
   SMALLINITBYTEDEF(flags);
   SMALLINITWORDDEF(version);
   SMALLINITWORDDEF(revision);
   SMALLINITPINTDEF(id_string);
   INITENDDEF;
}
init_data =
{
   SMALLINITBYTE(OFFSET(Node, ln_Type), NT_DEVICE),
   SMALLINITPINT(OFFSET(Node, ln_Name), device_name),
   SMALLINITBYTE(OFFSET(Library, lib_Flags), LIBF_SUMUSED | LIBF_CHANGED),
   SMALLINITWORD(OFFSET(Library, lib_Version), VERSION),
   SMALLINITWORD(OFFSET(Library, lib_Revision), REVISION),
   SMALLINITPINT(OFFSET(Library, lib_IdString), version_string),
   INITEND
};
#ifdef __MORPHOS__
#pragma pack()
#endif


const APTR init_table[] =
{
   (APTR)sizeof(struct DevBase),
   (APTR)vectors,
   (APTR)&init_data,
   (APTR)DevInit
};


static const ULONG rx_tags[] =
{
   S2_CopyToBuff,
   S2_CopyToBuff16,
};


static const ULONG tx_tags[] =
{
   S2_CopyFromBuff,
   S2_CopyFromBuff16,
   S2_CopyFromBuff32
};



/****i* intelpro100.device/DevInit *****************************************
*
*   NAME
*	DevInit
*
*   SYNOPSIS
*	dev_base = DevInit(dev_base, seg_list)
*
*	struct DevBase *DevInit(struct DevBase *, APTR);
*
****************************************************************************
*
*/

static struct DevBase *DevInit(REG(d0, struct DevBase *dev_base),
   REG(a0, APTR seg_list), REG(BASE_REG, struct DevBase *base))
{
   BOOL success = TRUE;

   /* Initialise base structure */

   dev_base->sys_base = (APTR)base;
   base = dev_base;
   base->seg_list = seg_list;
   NewList((APTR)(&base->pci_units));

   /* Open libraries, resources and devices */

   base->utility_base = (APTR)OpenLibrary(utility_name, UTILITY_VERSION);
   base->prometheus_base = OpenLibrary(prometheus_name, PROMETHEUS_VERSION);
   if(base->prometheus_base == NULL)
      base->powerpci_base = OpenLibrary(powerpci_name, POWERPCI_VERSION);
#ifdef __MORPHOS__
   base->openpci_base = OpenLibrary(openpci_name, OPENPCI_VERSION);
#endif

   if(base->utility_base == NULL || base->prometheus_base == NULL
      && base->powerpci_base == NULL && base->openpci_base == NULL)
      success = FALSE;

/*   if(OpenDevice(timer_name, UNIT_ECLOCK, (APTR)&base->timer_request, 0)*/
   if(OpenDevice(timer_name, UNIT_VBLANK, (APTR)&base->timer_request, 0)
      != 0)
      success = FALSE;

#ifdef __MORPHOS__
   base->wrapper_int_code = (APTR)&int_trap;
#endif

   if(!success)
   {
      DeleteDevice(base);
      base = NULL;
   }

   return base;
}



/****i* intelpro100.device/DevOpen *****************************************
*
*   NAME
*	DevOpen
*
*   SYNOPSIS
*	error = DevOpen(request, unit_num, flags)
*
*	BYTE DevOpen(struct IOSana2Req *, ULONG, ULONG);
*
****************************************************************************
*
*/

static BYTE DevOpen(REG(a1, struct IOSana2Req *request),
   REG(d0, ULONG unit_num), REG(d1, ULONG flags),
   REG(BASE_REG, struct DevBase *base))
{
   struct DevUnit *unit;
   BYTE error = 0;
   struct Opener *opener;
   struct TagItem *tag_list;
   UWORD i;

   base->device.dd_Library.lib_OpenCnt++;
   base->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;

   request->ios2_Req.io_Unit = NULL;
   tag_list = request->ios2_BufferManagement;
   request->ios2_BufferManagement = NULL;

   /* Check request size */

   if(request->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req))
      error = IOERR_OPENFAIL;

   /* Get the requested unit */

   if(error == 0)
   {
      request->ios2_Req.io_Unit = (APTR)(unit = GetUnit(unit_num, base));
      if(unit == NULL)
         error = IOERR_OPENFAIL;
   }

   /* Handle device sharing */

   if(error == 0)
   {
      if(unit->open_count != 0 && ((unit->flags & UNITF_SHARED) == 0
         || (flags & SANA2OPF_MINE) != 0))
         error = IOERR_UNITBUSY;
      unit->open_count++;
   }

   if(error == 0)
   {
      if((flags & SANA2OPF_MINE) == 0)
         unit->flags |= UNITF_SHARED;
      else if((flags & SANA2OPF_PROM) != 0)
         unit->flags |= UNITF_PROM;

      /* Set up buffer-management structure and get hooks */

      request->ios2_BufferManagement = opener =
         AllocVec(sizeof(struct Opener), MEMF_PUBLIC | MEMF_CLEAR);
      if(opener == NULL)
         error = IOERR_OPENFAIL;
   }

   if(error == 0)
   {
      NewList(&opener->read_port.mp_MsgList);
      opener->read_port.mp_Flags = PA_IGNORE;
      NewList((APTR)&opener->initial_stats);

      for(i = 0; i < 2; i++)
         opener->rx_function = (APTR)GetTagData(rx_tags[i],
            (UPINT)opener->rx_function, tag_list);
      for(i = 0; i < 3; i++)
         opener->tx_function = (APTR)GetTagData(tx_tags[i],
            (UPINT)opener->tx_function, tag_list);

      opener->filter_hook =
         (APTR)GetTagData(S2_PacketFilter, (UPINT)NULL, tag_list);
      opener->dma_tx_function =
         (APTR)GetTagData(S2_DMACopyFromBuff32, (UPINT)NULL, tag_list);

      Disable();
      AddTail((APTR)&unit->openers, (APTR)opener);
      Enable();
   }

   /* Back out if anything went wrong */

   if(error != 0)
      DevClose(request, base);

   /* Return */

   request->ios2_Req.io_Error = error;
   return error;
}



/****i* intelpro100.device/DevClose ****************************************
*
*   NAME
*	DevClose
*
*   SYNOPSIS
*	seg_list = DevClose(request)
*
*	APTR DevClose(struct IOSana2Req *);
*
****************************************************************************
*
*/

static APTR DevClose(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base))
{
   struct DevUnit *unit;
   APTR seg_list;
   struct Opener *opener;

   /* Free buffer-management resources */

   opener = (APTR)request->ios2_BufferManagement;
   if(opener != NULL)
   {
      Disable();
      Remove((APTR)opener);
      Enable();
      FreeVec(opener);
   }

   /* Delete the unit if it's no longer in use */

   unit = (APTR)request->ios2_Req.io_Unit;
   if(unit != NULL)
   {
      if((--unit->open_count) == 0)
      {
         Remove((APTR)unit);
         switch(unit->bus)
         {
         case PCI_BUS:
            DeletePCIUnit(unit, base);
         }
      }
   }

   /* Expunge the device if a delayed expunge is pending */

   seg_list = NULL;

   if((--base->device.dd_Library.lib_OpenCnt) == 0)
   {
      if((base->device.dd_Library.lib_Flags & LIBF_DELEXP) != 0)
         seg_list = DevExpunge(base);
   }

   return seg_list;
}



/****i* intelpro100.device/DevExpunge **************************************
*
*   NAME
*	DevExpunge
*
*   SYNOPSIS
*	seg_list = DevExpunge()
*
*	APTR DevExpunge(VOID);
*
****************************************************************************
*
*/

static APTR DevExpunge(REG(BASE_REG, struct DevBase *base))
{
   APTR seg_list;

   if(base->device.dd_Library.lib_OpenCnt == 0)
   {
      seg_list = base->seg_list;
      Remove((APTR)base);
      DeleteDevice(base);
   }
   else
   {
      base->device.dd_Library.lib_Flags |= LIBF_DELEXP;
      seg_list = NULL;
   }

   return seg_list;
}



/****i* intelpro100.device/DevReserved *************************************
*
*   NAME
*	DevReserved
*
*   SYNOPSIS
*	result = DevReserved()
*
*	APTR DevReserved(VOID);
*
****************************************************************************
*
*/

static APTR DevReserved()
{
   return NULL;
}



/****i* intelpro100.device/DevBeginIO **************************************
*
*   NAME
*	DevBeginIO
*
*   SYNOPSIS
*	DevBeginIO(request)
*
*	VOID DevBeginIO(struct IORequest *);
*
****************************************************************************
*
*/

static VOID DevBeginIO(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base))
{
   struct DevUnit *unit;

   request->ios2_Req.io_Error = 0;
   unit = (APTR)request->ios2_Req.io_Unit;

   if(AttemptSemaphore(&unit->access_lock))
      ServiceRequest(request, base);
   else
      PutRequest(unit->request_ports[GENERAL_QUEUE], (APTR)request, base);

   return;
}



/****i* intelpro100.device/DevAbortIO **************************************
*
*   NAME
*	DevAbortIO -- Try to stop a request.
*
*   SYNOPSIS
*	DevAbortIO(request)
*
*	VOID DevAbortIO(struct IOSana2Req *);
*
*   FUNCTION
*	Do our best to halt the progress of a request.
*
****************************************************************************
*
* Disable() used instead of a semaphore because device uses interrupts.
*
*/

static VOID DevAbortIO(REG(a1, struct IOSana2Req *request),
   REG(BASE_REG, struct DevBase *base))
{
   struct DevUnit *unit;

   unit = (APTR)request->ios2_Req.io_Unit;

   Disable();
   if(request->ios2_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE &&
      (request->ios2_Req.io_Flags & IOF_QUICK) == 0)
   {
      Remove((APTR)request);
      request->ios2_Req.io_Error = IOERR_ABORTED;
      request->ios2_WireError = S2WERR_GENERIC_ERROR;
      ReplyMsg((APTR)request);
   }
   Enable();

   return;
}



/****i* intelpro100.device/DeleteDevice ************************************
*
*   NAME
*	DeleteDevice
*
*   SYNOPSIS
*	DeleteDevice()
*
*	VOID DeleteDevice(VOID);
*
****************************************************************************
*
*/

VOID DeleteDevice(struct DevBase *base)
{
   UWORD neg_size, pos_size;

   /* Close devices */

   CloseDevice((APTR)&base->timer_request);

   /* Close libraries */

   if(base->openpci_base != NULL)
      CloseLibrary(base->openpci_base);
   if(base->powerpci_base != NULL)
      CloseLibrary(base->powerpci_base);
   if(base->prometheus_base != NULL)
      CloseLibrary(base->prometheus_base);
   if(base->utility_base != NULL)
      CloseLibrary((APTR)base->utility_base);

   /* Free device's memory */

   neg_size = base->device.dd_Library.lib_NegSize;
   pos_size = base->device.dd_Library.lib_PosSize;
   FreeMem((UBYTE *)base - neg_size, pos_size + neg_size);

   return;
}



/****i* intelpro100.device/GetUnit *****************************************
*
*   NAME
*	GetUnit -- Get a unit by number.
*
*   SYNOPSIS
*	unit = GetUnit(unit_num)
*
*	struct DevUnit *GetUnit(ULONG);
*
****************************************************************************
*
*/

static struct DevUnit *GetUnit(ULONG unit_num, struct DevBase *base)
{
   struct DevUnit *unit;
   ULONG pci_limit;

   pci_limit = GetPCICount(base);

   if(unit_num < pci_limit)
      unit = GetPCIUnit(unit_num, base);
   else
      unit = NULL;

   return unit;
}



