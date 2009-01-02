/*

File: header.c
Author: Neil Cafferkey
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
#include <exec/nodes.h>

#include "rev.h"

struct DevData *DevInit();


/* Return an error immediately if someone tries to run the device */

int main()
{
   return -1;
}


const TEXT device_name[] = DEV_NAME;
static const TEXT version_string[] = DEV_IDSTRING;


const struct Resident romtag =
{
   RTC_MATCHWORD,
   (struct Resident *)&romtag,
   (APTR)&romtag + sizeof(struct Resident),
   0,
   DEV_VERSION,
   NT_DEVICE,
   0,
   (STRPTR)device_name,
   (STRPTR)version_string,
   (APTR)DevInit
};


