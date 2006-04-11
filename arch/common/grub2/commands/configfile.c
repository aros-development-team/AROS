/* configfile.c - command to manually load config file  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/term.h>
#include <grub/misc.h>

static grub_err_t
grub_cmd_configfile (struct grub_arg_list *state __attribute__ ((unused)),
             int argc, char **args)

{
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  grub_cls ();
  grub_normal_execute (args[0], 1);

  return 0;
}


#ifdef GRUB_UTIL
void
grub_configfile_init (void)
{
  grub_register_command ("configfile", grub_cmd_configfile,
                        GRUB_COMMAND_FLAG_BOTH, "configfile FILE",
                        "Load another config file.", 0);
}

void
grub_configfile_fini (void)
{
  grub_unregister_command ("configfile");
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void) mod;                   /* To stop warning. */
  grub_register_command ("configfile", grub_cmd_configfile,
                        GRUB_COMMAND_FLAG_BOTH, "configfile FILE",
                        "Load another config file.", 0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("configfile");
}
#endif /* ! GRUB_UTIL */
