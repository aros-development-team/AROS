/* halt.c - command to halt the computer.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2009  Free Software Foundation, Inc.
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
#include <grub/extcmd.h>
#include <grub/i18n.h>
#include <grub/machine/int.h>
#include <grub/acpi.h>

GRUB_MOD_LICENSE ("GPLv3+");

static const struct grub_arg_option options[] =
  {
    {"no-apm", 'n', 0, N_("Do not use APM to halt the computer."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static inline void __attribute__ ((noreturn))
stop (void)
{
  while (1)
    {
      asm volatile ("hlt");
    }
}
/*
 * Halt the system, using APM if possible. If NO_APM is true, don't use
 * APM even if it is available.
 */
void  __attribute__ ((noreturn))
grub_halt (int no_apm)
{
  struct grub_bios_int_registers regs;

  grub_acpi_halt ();

  if (no_apm)
    stop ();

  /* detect APM */
  regs.eax = 0x5300;
  regs.ebx = 0;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x15, &regs);
  
  if (regs.flags & GRUB_CPU_INT_FLAGS_CARRY)
    stop ();

  /* disconnect APM first */
  regs.eax = 0x5304;
  regs.ebx = 0;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x15, &regs);

  /* connect APM */
  regs.eax = 0x5301;
  regs.ebx = 0;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x15, &regs);
  if (regs.flags & GRUB_CPU_INT_FLAGS_CARRY)
    stop ();

  /* set APM protocol level - 1.1 or bust. (this covers APM 1.2 also) */
  regs.eax = 0x530E;
  regs.ebx = 0;
  regs.ecx = 0x0101;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x15, &regs);
  if (regs.flags & GRUB_CPU_INT_FLAGS_CARRY)
    stop ();

  /* set the power state to off */
  regs.eax = 0x5307;
  regs.ebx = 1;
  regs.ecx = 3;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x15, &regs);

  /* shouldn't reach here */
  stop ();
}

static grub_err_t __attribute__ ((noreturn))
grub_cmd_halt (grub_extcmd_context_t ctxt,
	       int argc __attribute__ ((unused)),
	       char **args __attribute__ ((unused)))

{
  struct grub_arg_list *state = ctxt->state;
  int no_apm = 0;

  if (state[0].set)
    no_apm = 1;
  grub_halt (no_apm);
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(halt)
{
  cmd = grub_register_extcmd ("halt", grub_cmd_halt, 0, "[-n]",
			      N_("Halt the system, if possible using APM."),
			      options);
}

GRUB_MOD_FINI(halt)
{
  grub_unregister_extcmd (cmd);
}
