/*

Copyright (C) 2011 Neil Cafferkey

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
#include <aros/libcall.h>
#include "initializers.h"

#include "device.h"

#include "device_protos.h"

extern const APTR init_data;
extern const struct Resident rom_tag;
extern const TEXT device_name[];
extern const TEXT version_string[];


AROS_LH2(struct DevBase *, DevInit,
   AROS_LHA(struct DevBase *, dev_base, D0),
   AROS_LHA(struct DevBase *, seg_list, A0),
   struct DevBase *, base, 0, S2)
{
   AROS_LIBFUNC_INIT

   return DevInit(dev_base, seg_list, base);

   AROS_LIBFUNC_EXIT
}



AROS_LH3(BYTE, DevOpen,
   AROS_LHA(struct IOSana2Req *, request, A1),
   AROS_LHA(LONG, unit_num, D0),
   AROS_LHA(ULONG, flags, D1),
   struct DevBase *, base, 1, S2)
{
   AROS_LIBFUNC_INIT

   return DevOpen(request, unit_num, flags, base);

   AROS_LIBFUNC_EXIT
}



AROS_LH1(APTR, DevClose,
   AROS_LHA(struct IOSana2Req *, request, A1),
   struct DevBase *, base, 2, S2)
{
   AROS_LIBFUNC_INIT

   return DevClose(request, base);

   AROS_LIBFUNC_EXIT
}



AROS_LH0(APTR, DevExpunge,
   struct DevBase *, base, 3, S2)
{
   AROS_LIBFUNC_INIT

   return DevExpunge(base);

   AROS_LIBFUNC_EXIT
}



AROS_LH0(APTR, DevReserved,
   struct DevBase *, base, 4, S2)
{
   AROS_LIBFUNC_INIT

   return DevReserved(base);

   AROS_LIBFUNC_EXIT
}



AROS_LH1(VOID, DevBeginIO,
   AROS_LHA(struct IOSana2Req *, request, A1),
   struct DevBase *, base, 5, S2)
{
   AROS_LIBFUNC_INIT

   DevBeginIO(request, base);

   AROS_LIBFUNC_EXIT
}



AROS_LH1(VOID, DevAbortIO,
   AROS_LHA(struct IOSana2Req *, request, A1),
   struct DevBase *, base, 6, S2)
{
   AROS_LIBFUNC_INIT

   DevAbortIO(request, base);

   AROS_LIBFUNC_EXIT
}



static const APTR vectors[] =
{
   (APTR)AROS_SLIB_ENTRY(DevOpen, S2, 1),
   (APTR)AROS_SLIB_ENTRY(DevClose, S2, 2),
   (APTR)AROS_SLIB_ENTRY(DevExpunge, S2, 3),
   (APTR)AROS_SLIB_ENTRY(DevReserved, S2, 4),
   (APTR)AROS_SLIB_ENTRY(DevBeginIO, S2, 5),
   (APTR)AROS_SLIB_ENTRY(DevAbortIO, S2, 6),
   (APTR)-1
};


static const APTR init_table[] =
{
   (APTR)sizeof(struct DevBase),
   (APTR)vectors,
   (APTR)&init_data,
   (APTR)AROS_SLIB_ENTRY(DevInit, S2, 0),
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



