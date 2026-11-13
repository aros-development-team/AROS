/*

Author: Neil Cafferkey
Copyright (C) 2002-2011 Neil Cafferkey

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

#include "datatype.h"

#include "datatype_protos.h"

extern const struct Resident rom_tag;
extern const TEXT datatype_name[];
extern const TEXT version_string[];


AROS_LH2(struct DTBase *, LibInit,
   AROS_LHA(struct DTBase *, lib_base, D0),
   AROS_LHA(struct DTBase *, seg_list, A0),
   struct DTBase *, base, 0, DT)
{
   AROS_LIBFUNC_INIT

   return LibInit(lib_base, seg_list, base);

   AROS_LIBFUNC_EXIT
}



AROS_LH1(struct DTBase *, LibOpen,
   AROS_LHA(ULONG, version, D0),
   struct DTBase *, base, 1, DT)
{
   AROS_LIBFUNC_INIT

   return LibOpen(version, base);

   AROS_LIBFUNC_EXIT
}



AROS_LH0(APTR, LibClose,
   struct DTBase *, base, 2, DT)
{
   AROS_LIBFUNC_INIT

   return LibClose(base);

   AROS_LIBFUNC_EXIT
}



AROS_LH0(APTR, LibExpunge,
   struct DTBase *, base, 3, DT)
{
   AROS_LIBFUNC_INIT

   return LibExpunge(base);

   AROS_LIBFUNC_EXIT
}



AROS_LH0(APTR, LibReserved,
   struct DTBase *, base, 4, DT)
{
   AROS_LIBFUNC_INIT

   return LibReserved(base);

   AROS_LIBFUNC_EXIT
}



AROS_LH0(struct IClass *, ObtainClass,
   struct DTBase *, base, 5, DT)
{
   AROS_LIBFUNC_INIT

   return ObtainClass(base);

   AROS_LIBFUNC_EXIT
}


static const APTR vectors[] =
{
   (APTR)AROS_SLIB_ENTRY(LibOpen, DT, 1),
   (APTR)AROS_SLIB_ENTRY(LibClose, DT, 2),
   (APTR)AROS_SLIB_ENTRY(LibExpunge, DT, 3),
   (APTR)AROS_SLIB_ENTRY(LibReserved, DT, 4),
   (APTR)AROS_SLIB_ENTRY(ObtainClass, DT, 5),
   (APTR)-1
};


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
   SMALLINITBYTE(OFFSET(Node, ln_Type), NT_LIBRARY),
   SMALLINITPINT(OFFSET(Node, ln_Name), datatype_name),
   SMALLINITBYTE(OFFSET(Library, lib_Flags), LIBF_SUMUSED | LIBF_CHANGED),
   SMALLINITWORD(OFFSET(Library, lib_Version), VERSION),
   SMALLINITWORD(OFFSET(Library, lib_Revision), REVISION),
   SMALLINITPINT(OFFSET(Library, lib_IdString), version_string),
   INITEND
};


static const APTR init_table[] =
{
   (APTR)sizeof(struct DTBase),
   (APTR)vectors,
   (APTR)&init_data,
   (APTR)AROS_SLIB_ENTRY(LibInit, DT, 0),
};


const struct Resident aros_rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&aros_rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AUTOINIT,
   VERSION,
   NT_LIBRARY,
   0,
   (TEXT *)datatype_name,
   (TEXT *)version_string,
   (APTR)init_table
};



