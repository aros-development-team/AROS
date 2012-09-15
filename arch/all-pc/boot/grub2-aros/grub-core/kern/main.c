/* main.c - the kernel main routine */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2006,2008,2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/symbol.h>
#include <grub/dl.h>
#include <grub/term.h>
#include <grub/file.h>
#include <grub/device.h>
#include <grub/env.h>
#include <grub/mm.h>
#include <grub/command.h>
#include <grub/reader.h>
#include <grub/parser.h>

/* This is actualy platform-independant but used only on loongson and sparc.  */
#if defined (GRUB_MACHINE_MIPS_LOONGSON) || defined (GRUB_MACHINE_MIPS_QEMU_MIPS) || defined (GRUB_MACHINE_SPARC64)
grub_addr_t
grub_modules_get_end (void)
{
  struct grub_module_info *modinfo;

  modinfo = (struct grub_module_info *) grub_modbase;

  /* Check if there are any modules.  */
  if ((modinfo == 0) || modinfo->magic != GRUB_MODULE_MAGIC)
    return grub_modbase;

  return grub_modbase + modinfo->size;
}
#endif

/* Load all modules in core.  */
static void
grub_load_modules (void)
{
  struct grub_module_header *header;
  FOR_MODULES (header)
  {
    /* Not an ELF module, skip.  */
    if (header->type != OBJ_TYPE_ELF)
      continue;

    if (! grub_dl_load_core ((char *) header + sizeof (struct grub_module_header),
			     (header->size - sizeof (struct grub_module_header))))
      grub_fatal ("%s", grub_errmsg);

    if (grub_errno)
      grub_print_error ();
  }
}

static void
grub_load_config (void)
{
  struct grub_module_header *header;
  FOR_MODULES (header)
  {
    /* Not an embedded config, skip.  */
    if (header->type != OBJ_TYPE_CONFIG)
      continue;
    
    grub_parser_execute ((char *) header +
			 sizeof (struct grub_module_header));
    break;
  }
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

static void
grub_set_prefix_and_root (void)
{
  char *device = NULL;
  char *path = NULL;
  char *fwdevice = NULL;
  char *fwpath = NULL;
  char *prefix = NULL;
  struct grub_module_header *header;

  FOR_MODULES (header)
    if (header->type == OBJ_TYPE_PREFIX)
      prefix = (char *) header + sizeof (struct grub_module_header);

  grub_register_variable_hook ("root", 0, grub_env_write_root);

  if (prefix)
    {
      char *pptr = NULL;
      if (prefix[0] == '(')
	{
	  pptr = grub_strrchr (prefix, ')');
	  if (pptr)
	    {
	      device = grub_strndup (prefix + 1, pptr - prefix - 1);
	      pptr++;
	    }
	}
      if (!pptr)
	pptr = prefix;
      if (pptr[0])
	path = grub_strdup (pptr);
    }
  if ((!device || device[0] == ',' || !device[0]) || !path)
    grub_machine_get_bootlocation (&fwdevice, &fwpath);

  if (!device && fwdevice)
    device = fwdevice;
  else if (fwdevice && (device[0] == ',' || !device[0]))
    {
      /* We have a partition, but still need to fill in the drive.  */
      char *comma, *new_device;

      for (comma = fwdevice; *comma; )
	{
	  if (comma[0] == '\\' && comma[1] == ',')
	    {
	      comma += 2;
	      continue;
	    }
	  if (*comma == ',')
	    break;
	  comma++;
	}
      if (*comma)
	{
	  char *drive = grub_strndup (fwdevice, comma - fwdevice);
	  new_device = grub_xasprintf ("%s%s", drive, device);
	  grub_free (drive);
	}
      else
	new_device = grub_xasprintf ("%s%s", fwdevice, device);

      grub_free (fwdevice);
      grub_free (device);
      device = new_device;
    }
  else
    grub_free (fwdevice);
  if (fwpath && !path)
    path = fwpath;
  else
    grub_free (fwpath);
  if (device)
    {
      char *prefix_set;
    
      prefix_set = grub_xasprintf ("(%s)%s", device, path ? : "");
      if (prefix_set)
	{
	  grub_env_set ("prefix", prefix_set);
	  grub_free (prefix_set);
	}
      grub_env_set ("root", device);
    }

  grub_free (device);
  grub_free (path);
  grub_print_error ();
}

/* Load the normal mode module and execute the normal mode if possible.  */
static void
grub_load_normal_mode (void)
{
  /* Load the module.  */
  grub_dl_load ("normal");

  /* Print errors if any.  */
  grub_print_error ();
  grub_errno = 0;

  grub_command_execute ("normal", 0, 0);
}

/* The main routine.  */
void __attribute__ ((noreturn))
grub_main (void)
{
  /* First of all, initialize the machine.  */
  grub_machine_init ();

  /* Hello.  */
  grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
  grub_printf ("Welcome to GRUB!\n\n");
  grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);

  /* Load pre-loaded modules and free the space.  */
  grub_register_exported_symbols ();
#ifdef GRUB_LINKER_HAVE_INIT
  grub_arch_dl_init_linker ();
#endif  
  grub_load_modules ();

  /* It is better to set the root device as soon as possible,
     for convenience.  */
  grub_set_prefix_and_root ();
  grub_env_export ("root");
  grub_env_export ("prefix");

  grub_register_core_commands ();

  grub_load_config ();
  grub_load_normal_mode ();
  grub_rescue_run ();
}
