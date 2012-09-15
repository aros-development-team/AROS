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

#include <grub/cpu/io.h>
#include <grub/misc.h>
#include <grub/acpi.h>
#include <grub/i18n.h>

const char bochs_shutdown[] = "Shutdown";

/*
 *  This call is special...  it never returns...  in fact it should simply
 *  hang at this point!
 */
static inline void  __attribute__ ((noreturn))
stop (void)
{
  asm volatile ("cli");
  while (1)
    {
      asm volatile ("hlt");
    }
}

void
grub_halt (void)
{
  unsigned int i;

#if defined (GRUB_MACHINE_COREBOOT) || defined (GRUB_MACHINE_MULTIBOOT)
  grub_acpi_halt ();
#endif

  /* Disable interrupts.  */
  __asm__ __volatile__ ("cli");

  /* Bochs, QEMU, etc.  */
  for (i = 0; i < sizeof (bochs_shutdown) - 1; i++)
    grub_outb (bochs_shutdown[i], 0x8900);

  grub_puts_ (N_("GRUB doesn't know how to halt this machine yet!"));

  /* In order to return we'd have to check what the previous status of IF
     flag was.  But user most likely doesn't want to return anyway ...  */
  stop ();
}
