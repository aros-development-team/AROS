/* main.c - the kernel main routine */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002, 2003, 2005  Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/symbol.h>
#include <grub/dl.h>
#include <grub/term.h>
#include <grub/rescue.h>
#include <grub/file.h>
#include <grub/device.h>
#include <grub/env.h>

/* Load all modules in core.  */
static void
grub_load_modules (void)
{
  struct grub_module_info *modinfo;
  struct grub_module_header *header;
  grub_addr_t modbase;

  modbase = grub_arch_modules_addr ();
  modinfo = (struct grub_module_info *) modbase;
  
  /* Check if there are any modules.  */
  if ((modinfo == 0) || modinfo->magic != GRUB_MODULE_MAGIC)
    return;

  for (header = (struct grub_module_header *) (modbase + modinfo->offset);
       header < (struct grub_module_header *) (modbase + modinfo->size);
       header = (struct grub_module_header *) ((char *) header + header->size))
    {
      if (! grub_dl_load_core ((char *) header + header->offset,
			       (header->size - header->offset)))
	grub_fatal ("%s", grub_errmsg);
    }

  /* Add the region where modules reside into dynamic memory.  */
  grub_mm_init_region ((void *) modinfo, modinfo->size);
}

/* Write hook for the environment variables of root. Remove surrounding
   parentheses, if any.  */
static char *
grub_env_write_root (struct grub_env_var *var __attribute__ ((unused)),
		     const char *val)
{
  /* XXX Is it better to check the existence of the device?  */
  grub_size_t len = grub_strlen (val);
  
  if (val[0] == '(' && val[len - 1] == ')')
    return grub_strndup (val + 1, len - 2);

  return grub_strdup (val);
}

/* Set the root device according to the dl prefix.  */
static void
grub_set_root_dev (void)
{
  const char *prefix;

  grub_register_variable_hook ("root", 0, grub_env_write_root);
  
  prefix = grub_env_get ("prefix");
  
  if (prefix)
    {
      char *dev;

      dev = grub_file_get_device_name (prefix);
      if (dev)
	{
	  grub_env_set ("root", dev);
	  grub_free (dev);
	}
    }
}

/* Load the normal mode module and execute the normal mode if possible.  */
static void
grub_load_normal_mode (void)
{
  /* Load the module.  */
  grub_dl_load ("normal");
  
  /* Ignore any error, because we have the rescue mode anyway.  */
  grub_errno = GRUB_ERR_NONE;
}

/* The main routine.  */
void
grub_main (void)
{
  /* First of all, initialize the machine.  */
  grub_machine_init ();

  /* Hello.  */
  grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
  grub_printf ("Welcome to GRUB!\n\n");
  grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);

  /* It is better to set the root device as soon as possible,
     for convenience.  */
  grub_set_root_dev ();

  /* Load pre-loaded modules and free the space.  */
  grub_register_exported_symbols ();
  grub_load_modules ();

  /* Load the normal mode module.  */
  grub_load_normal_mode ();
  
  /* Enter the rescue mode.  */
  grub_enter_rescue_mode ();
}
