/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996   Erich Boleyn  <erich@uruk.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "shared.h"

void
cmain (void)
{
  printf ("\n\nGRUB loading, please wait...\n");

  /*
   *  Here load the true second-stage boot-loader.
   */

  if (grub_open (config_file))
    {
      int ret = grub_read ((char *) 0x8000, -1);
      
      grub_close ();

      if (ret)
	chain_stage2 (0, 0x8200);
    }

  /*
   *  If not, then print error message and die.
   */

  print_error ();

  stop ();
}
