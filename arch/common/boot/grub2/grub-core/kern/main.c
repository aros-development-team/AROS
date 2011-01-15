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

void
grub_module_iterate (int (*hook) (struct grub_module_header *header))
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
      if (hook (header))
	break;
    }
}

/* This is actualy platform-independant but used only on yeeloong and sparc.  */
#if defined (GRUB_MACHINE_MIPS_YEELOONG) || defined (GRUB_MACHINE_SPARC64)
grub_addr_t
grub_modules_get_end (void)
{
  struct grub_module_info *modinfo;
  grub_addr_t modbase;

  modbase = grub_arch_modules_addr ();
  modinfo = (struct grub_module_info *) modbase;

  /* Check if there are any modules.  */
  if ((modinfo == 0) || modinfo->magic != GRUB_MODULE_MAGIC)
    return modbase;

  return modbase + modinfo->size;
}
#endif

/* Load all modules in core.  */
static void
grub_load_modules (void)
{
  auto int hook (struct grub_module_header *);
  int hook (struct grub_module_header *header)
    {
      /* Not an ELF module, skip.  */
      if (header->type != OBJ_TYPE_ELF)
        return 0;

      if (! grub_dl_load_core ((char *) header + sizeof (struct grub_module_header),
			       (header->size - sizeof (struct grub_module_header))))
	grub_fatal ("%s", grub_errmsg);

      if (grub_errno)
	grub_print_error ();

      return 0;
    }

  grub_module_iterate (hook);
}

static void
grub_load_config (void)
{
  auto int hook (struct grub_module_header *);
  int hook (struct grub_module_header *header)
    {
      /* Not an embedded config, skip.  */
      if (header->type != OBJ_TYPE_CONFIG)
	return 0;

      grub_parser_execute ((char *) header +
			   sizeof (struct grub_module_header));
      return 1;
    }

  grub_module_iterate (hook);
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

  /* Something went wrong.  Print errors here to let user know why we're entering rescue mode.  */
  grub_print_error ();
  grub_errno = 0;

  grub_command_execute ("normal", 0, 0);
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

  /* Load pre-loaded modules and free the space.  */
  grub_register_exported_symbols ();
#ifdef GRUB_LINKER_HAVE_INIT
  grub_arch_dl_init_linker ();
#endif  
  grub_load_modules ();

  /* It is better to set the root device as soon as possible,
     for convenience.  */
  grub_machine_set_prefix ();
  grub_set_root_dev ();

  grub_register_core_commands ();

  grub_load_config ();
  grub_load_normal_mode ();
  grub_rescue_run ();
}
