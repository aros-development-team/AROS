/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/memory.h>

GRUB_MOD_LICENSE ("GPLv3+");

static const char *names[] =
  {
    [GRUB_MEMORY_AVAILABLE] = N_("available RAM"),
    [GRUB_MEMORY_RESERVED] = N_("reserved RAM"),
    /* TRANSLATORS: this refers to memory where ACPI tables are stored
       and which can be used by OS once it loads ACPI tables.  */
    [GRUB_MEMORY_ACPI] = N_("ACPI reclaimable RAM"),
    /* TRANSLATORS: this refers to memory which ACPI-compliant OS
       is required to save accross hibernations.  */
    [GRUB_MEMORY_NVS] = N_("ACPI non-volatile storage RAM"),
    [GRUB_MEMORY_BADRAM] = N_("faulty RAM (BadRAM)"),
    [GRUB_MEMORY_CODE] = N_("RAM holding firmware code"),
    [GRUB_MEMORY_HOLE] = N_("Address range not associated with RAM")
  };

static grub_err_t
grub_cmd_lsmmap (grub_command_t cmd __attribute__ ((unused)),
		 int argc __attribute__ ((unused)),
		 char **args __attribute__ ((unused)))

{
  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t, grub_memory_type_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr, grub_uint64_t size, 
			     grub_memory_type_t type)
    {
      if (type < ARRAY_SIZE (names) && names[type])
	grub_printf_ (N_("base_addr = 0x%llx, length = 0x%llx, %s\n"),
		      (long long) addr, (long long) size, _(names[type]));
      else
	grub_printf_ (N_("base_addr = 0x%llx, length = 0x%llx, type = 0x%x\n"),
		      (long long) addr, (long long) size, type);
      return 0;
    }
#ifndef GRUB_MACHINE_EMU
  grub_machine_mmap_iterate (hook);
#endif

  return 0;
}

static grub_command_t cmd;

GRUB_MOD_INIT(lsmmap)
{
  cmd = grub_register_command ("lsmmap", grub_cmd_lsmmap,
			       0, N_("List memory map provided by firmware."));
}

GRUB_MOD_FINI(lsmmap)
{
  grub_unregister_command (cmd);
}
