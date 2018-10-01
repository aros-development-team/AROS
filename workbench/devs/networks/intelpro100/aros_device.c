/*

Copyright (C) 2011,2012 Neil Cafferkey

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
#include <aros/asmcall.h>
#include <aros/libcall.h>
#include "initializers.h"

#include "device.h"

#include "device_protos.h"


/* Private prototypes */

AROS_UFP3(struct DevBase *, AROSDevInit,
   AROS_UFPA(struct DevBase *, dev_base, D0),
   AROS_UFPA(APTR, seg_list, A0),
   AROS_UFPA(struct DevBase *, base, A6));
AROS_LD3(BYTE, AROSDevOpen,
   AROS_LDA(struct IOSana2Req *, request, A1),
   AROS_LDA(LONG, unit_num, D0),
   AROS_LDA(ULONG, flags, D1),
   struct DevBase *, base, 1, S2);
AROS_LD1(APTR, AROSDevClose,
   AROS_LDA(struct IOSana2Req *, request, A1),
   struct DevBase *, base, 2, S2);
AROS_LD0(APTR, AROSDevExpunge,
   struct DevBase *, base, 3, S2);
AROS_LD0(APTR, AROSDevReserved,
   struct DevBase *, base, 4, S2);
AROS_LD1(VOID, AROSDevBeginIO,
   AROS_LDA(struct IOSana2Req *, request, A1),
   struct DevBase *, base, 5, S2);
AROS_LD1(VOID, AROSDevAbortIO,
   AROS_LDA(struct IOSana2Req *, request, A1),
   struct DevBase *, base, 6, S2);
static BOOL RXFunction(struct IOSana2Req *request, APTR buffer, ULONG size);
static BOOL TXFunction(APTR buffer, struct IOSana2Req *request, ULONG size);
static UBYTE *DMATXFunction(struct IOSana2Req *request);
AROS_INTP(AROSInt);

extern const APTR init_data;
extern const struct Resident rom_tag;
extern const TEXT device_name[];
extern const TEXT version_string[];


static const APTR vectors[] =
{
   (APTR)AROS_SLIB_ENTRY(AROSDevOpen, S2, 1),
   (APTR)AROS_SLIB_ENTRY(AROSDevClose, S2, 2),
   (APTR)AROS_SLIB_ENTRY(AROSDevExpunge, S2, 3),
   (APTR)AROS_SLIB_ENTRY(AROSDevReserved, S2, 4),
   (APTR)AROS_SLIB_ENTRY(AROSDevBeginIO, S2, 5),
   (APTR)AROS_SLIB_ENTRY(AROSDevAbortIO, S2, 6),
   (APTR)-1
};


static const APTR init_table[] =
{
   (APTR)sizeof(struct DevBase),
   (APTR)vectors,
   (APTR)&init_data,
   (APTR)AROSDevInit,
};


const struct Resident aros_rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&aros_rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AUTOINIT,
   VERSION,
   NT_DEVICE,
   0,
   (TEXT *)device_name,
   (TEXT *)version_string,
   (APTR)init_table
};



/****i* intelpro100.device/AROSDevInit *************************************
*
*   NAME
*       AROSDevInit
*
****************************************************************************
*
*/

AROS_UFH3(struct DevBase *, AROSDevInit,
   AROS_UFHA(struct DevBase *, dev_base, D0),
   AROS_UFHA(APTR, seg_list, A0),
   AROS_UFHA(struct DevBase *, base, A6))
{
   AROS_USERFUNC_INIT

   base = DevInit(dev_base, seg_list, base);

   if(base != NULL)
      base->wrapper_int_code = (APTR)AROSInt;
   return base;

   AROS_USERFUNC_EXIT
}



/****i* intelpro100.device/AROSDevOpen *************************************
*
*   NAME
*       AROSDevOpen
*
****************************************************************************
*
*/

AROS_LH3(BYTE, AROSDevOpen,
   AROS_LHA(struct IOSana2Req *, request, A1),
   AROS_LHA(LONG, unit_num, D0),
   AROS_LHA(ULONG, flags, D1),
   struct DevBase *, base, 1, S2)
{
   AROS_LIBFUNC_INIT

   struct Opener *opener;
   BYTE error;

   error = DevOpen(request, unit_num, flags, base);

   /* Set up wrapper hooks to hide register-call functions */

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

   AROS_LIBFUNC_EXIT
}



/****i* intelpro100.device/AROSDevClose ************************************
*
*   NAME
*       AROSDevClose
*
****************************************************************************
*
*/

AROS_LH1(APTR, AROSDevClose,
   AROS_LHA(struct IOSana2Req *, request, A1),
   struct DevBase *, base, 2, S2)
{
   AROS_LIBFUNC_INIT

   return DevClose(request, base);

   AROS_LIBFUNC_EXIT
}



/****i* intelpro100.device/AROSDevExpunge **********************************
*
*   NAME
*       AROSDevExpunge
*
****************************************************************************
*
*/

AROS_LH0(APTR, AROSDevExpunge,
   struct DevBase *, base, 3, S2)
{
   AROS_LIBFUNC_INIT

   return DevExpunge(base);

   AROS_LIBFUNC_EXIT
}



/****i* intelpro100.device/AROSDevReserved *********************************
*
*   NAME
*       AROSDevReserved
*
****************************************************************************
*
*/

AROS_LH0(APTR, AROSDevReserved,
   struct DevBase *, base, 4, S2)
{
   AROS_LIBFUNC_INIT

   return DevReserved(base);

   AROS_LIBFUNC_EXIT
}



/****i* intelpro100.device/AROSDevBeginIO **********************************
*
*   NAME
*       AROSDevBeginIO
*
****************************************************************************
*
*/

AROS_LH1(VOID, AROSDevBeginIO,
   AROS_LHA(struct IOSana2Req *, request, A1),
   struct DevBase *, base, 5, S2)
{
   AROS_LIBFUNC_INIT

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

   DevBeginIO(request, base);

   AROS_LIBFUNC_EXIT
}



/****i* intelpro100.device/AROSDevAbortIO **********************************
*
*   NAME
*       AROSDevAbortIO -- Try to stop a request.
*
****************************************************************************
*
*/

AROS_LH1(VOID, AROSDevAbortIO,
   AROS_LHA(struct IOSana2Req *, request, A1),
   struct DevBase *, base, 6, S2)
{
   AROS_LIBFUNC_INIT

   DevAbortIO(request, base);

   AROS_LIBFUNC_EXIT
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
   struct Opener *opener;
   APTR cookie;

   opener = request->ios2_BufferManagement;
   cookie = request->ios2_StatData;
   request->ios2_Data = cookie;

   return AROS_UFC3(BOOL, (APTR)opener->real_rx_function,
      AROS_UFCA(APTR, cookie, A0),
      AROS_UFCA(APTR, buffer, A1),
      AROS_UFCA(ULONG, size, D0));
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
   struct Opener *opener;
   APTR cookie;

   opener = request->ios2_BufferManagement;
   cookie = request->ios2_StatData;
   request->ios2_Data = cookie;

   return AROS_UFC3(BOOL, (APTR)opener->real_tx_function,
      AROS_UFCA(APTR, buffer, A0),
      AROS_UFCA(APTR, cookie, A1),
      AROS_UFCA(ULONG, size, D0));
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
   struct Opener *opener;
   APTR cookie;

   opener = request->ios2_BufferManagement;
   cookie = request->ios2_StatData;
   request->ios2_Data = cookie;

   return AROS_UFC1(UBYTE *, (APTR)opener->real_dma_tx_function,
      AROS_UFCA(APTR, cookie, A0));
}



/****i* intelpro100.device/AROSInt *****************************************
*
*   NAME
*	AROSInt
*
****************************************************************************
*
*/
#undef SysBase

AROS_INTH2(AROSInt, APTR *, int_data, mask)
{
   AROS_INTFUNC_INIT

   BOOL (*int_code)(APTR, APTR, UBYTE);

   int_code = int_data[0];
   return int_code(int_data[1], int_code, mask);

   AROS_INTFUNC_EXIT
}



