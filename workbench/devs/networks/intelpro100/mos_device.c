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
#include <utility/utility.h>

#include "device.h"

/* Private prototypes */

static BYTE MOSDevOpen();
static APTR MOSDevClose();
static APTR MOSDevExpunge();
static VOID MOSDevBeginIO();
static VOID MOSDevAbortIO();
static BOOL RXFunction(struct IOSana2Req *request, APTR buffer, ULONG size);
static BOOL TXFunction(APTR buffer, struct IOSana2Req *request, ULONG size);
static UBYTE *DMATXFunction(struct IOSana2Req *request);
static BOOL MOSInt();

extern const APTR mos_vectors[];
extern const APTR mos_init_table[];
extern const struct Resident rom_tag;
extern const APTR init_table[];
extern const TEXT device_name[];
extern const TEXT version_string[];


static const TEXT openpci_name[] = "openpci.library";


const struct Resident mos_rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&mos_rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AUTOINIT | RTF_PPC,
   VERSION,
   NT_DEVICE,
   0,
   (STRPTR)device_name,
   (STRPTR)version_string,
   (APTR)mos_init_table
};


static const struct EmulLibEntry int_trap =
{
   TRAP_LIB,
   0,
   (APTR)MOSInt
};


#include "device.c"


static const APTR mos_vectors[] =
{
   (APTR)FUNCARRAY_32BIT_NATIVE,
   (APTR)MOSDevOpen,
   (APTR)MOSDevClose,
   (APTR)MOSDevExpunge,
   (APTR)DevReserved,
   (APTR)MOSDevBeginIO,
   (APTR)MOSDevAbortIO,
   (APTR)-1
};


static const APTR mos_init_table[] =
{
   (APTR)sizeof(struct DevBase),
   (APTR)mos_vectors,
   (APTR)&init_data,
   (APTR)DevInit
};



/****i* intelpro100.device/MOSDevOpen **************************************
*
*   NAME
*	MOSDevOpen
*
****************************************************************************
*
*/

static BYTE MOSDevOpen()
{
   struct IOSana2Req *request;
   struct Opener *opener;
   BYTE error;

   request = (APTR)REG_A1;
   error = DevOpen(request, REG_D0, REG_D1, (APTR)REG_A6);

   /* Set up wrapper hooks to hide 68k emulation */

   if(error == 0)
   {
      opener = request->ios2_BufferManagement;
      opener->real_rx_function = opener->rx_function;
      opener->real_tx_function = opener->tx_function;
      opener->rx_function = (APTR)RXFunction;
      opener->tx_function = (APTR)TXFunction;
      if(opener->dma_tx_function != NULL)
      {
         opener->real_dma_tx_function = opener->dma_tx_function;
         opener->dma_tx_function = (APTR)DMATXFunction;
      }
   }

   return error;
}



/****i* intelpro100.device/MOSDevClose *************************************
*
*   NAME
*	MOSDevClose
*
****************************************************************************
*
*/

static APTR MOSDevClose()
{
   return DevClose((APTR)REG_A1, (APTR)REG_A6);
}



/****i* intelpro100.device/MOSDevExpunge ***********************************
*
*   NAME
*	MOSDevExpunge
*
****************************************************************************
*
*/

static APTR MOSDevExpunge()
{
   return DevExpunge((APTR)REG_A6);
}



/****i* intelpro100.device/MOSDevBeginIO ***********************************
*
*   NAME
*	MOSDevBeginIO
*
****************************************************************************
*
*/

static VOID MOSDevBeginIO()
{
   struct IOSana2Req *request = (APTR)REG_A1;

   /* Replace caller's cookie with our own */

   switch(request->ios2_Req.io_Command)
   {
   case CMD_READ:
   case CMD_WRITE:
   case S2_MULTICAST:
   case S2_BROADCAST:
   case S2_READORPHAN:
      request->ios2_StatData = request->ios2_Data;
      request->ios2_Data = request;
   }

   DevBeginIO(request, (APTR)REG_A6);

   return;
}



/****i* intelpro100.device/MOSDevAbortIO ***********************************
*
*   NAME
*	MOSDevAbortIO -- Try to stop a request.
*
****************************************************************************
*
*/

static VOID MOSDevAbortIO()
{
   DevAbortIO((APTR)REG_A1, (APTR)REG_A6);
}



/****i* intelpro100.device/RXFunction **************************************
*
*   NAME
*	RXFunction
*
****************************************************************************
*
*/

static BOOL RXFunction(struct IOSana2Req *request, APTR buffer, ULONG size)
{
   struct DevBase *base;
   struct EmulCaos context;
   struct Opener *opener;
   APTR cookie;

   base = (struct DevBase *)request->ios2_Req.io_Device;
   opener = request->ios2_BufferManagement;
   cookie = request->ios2_StatData;
   request->ios2_Data = cookie;

   context.caos_Un.Function = (APTR)opener->real_rx_function;
   context.reg_a0 = (ULONG)cookie;
   context.reg_a1 = (ULONG)buffer;
   context.reg_d0 = size;
   return MyEmulHandle->EmulCall68k(&context);
}



/****i* intelpro100.device/TXFunction **************************************
*
*   NAME
*	TXFunction
*
****************************************************************************
*
*/

static BOOL TXFunction(APTR buffer, struct IOSana2Req *request, ULONG size)
{
   struct DevBase *base;
   struct EmulCaos context;
   struct Opener *opener;
   APTR cookie;

   base = (struct DevBase *)request->ios2_Req.io_Device;
   opener = request->ios2_BufferManagement;
   cookie = request->ios2_StatData;
   request->ios2_Data = cookie;

   context.caos_Un.Function = (APTR)opener->real_tx_function;
   context.reg_a0 = (ULONG)buffer;
   context.reg_a1 = (ULONG)cookie;
   context.reg_d0 = size;
   return MyEmulHandle->EmulCall68k(&context);
}



/****i* intelpro100.device/DMATXFunction ***********************************
*
*   NAME
*	DMATXFunction
*
****************************************************************************
*
*/

static UBYTE *DMATXFunction(struct IOSana2Req *request)
{
   struct DevBase *base;
   struct EmulCaos context;
   struct Opener *opener;
   APTR cookie;

   base = (struct DevBase *)request->ios2_Req.io_Device;
   opener = request->ios2_BufferManagement;
   cookie = request->ios2_StatData;
   request->ios2_Data = cookie;

   context.caos_Un.Function = (APTR)opener->real_dma_tx_function;
   context.reg_a0 = (ULONG)cookie;
   return (UBYTE *)MyEmulHandle->EmulCall68k(&context);
}



/****i* intelpro100.device/MOSInt ******************************************
*
*   NAME
*	MOSInt
*
****************************************************************************
*
*/

static BOOL MOSInt()
{
   APTR *int_data;
   BOOL (*int_code)(APTR, APTR);

   int_data = (APTR)REG_A1;
   int_code = int_data[0];
   return int_code(int_data[1], int_code);
}



