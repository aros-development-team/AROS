/* init.c - initialize an ia64-based EFI system */
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

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/cache.h>
#include <grub/kernel.h>
#include <grub/efi/efi.h>

static grub_uint64_t divisor = 1;

static grub_uint64_t
get_itc (void)
{
  grub_uint64_t ret;
  asm volatile ("mov %0=ar.itc" : "=r" (ret));
  return ret;
}

grub_uint64_t
grub_rtc_get_time_ms (void)
{
  return get_itc () / divisor;
}

void
grub_machine_init (void)
{
  grub_efi_event_t event;
  grub_uint64_t before, after;
  grub_efi_uintn_t idx;
  grub_efi_init ();

  efi_call_5 (grub_efi_system_table->boot_services->create_event,
	      GRUB_EFI_EVT_TIMER, GRUB_EFI_TPL_CALLBACK, 0, 0, &event);

  before = get_itc ();
  efi_call_3 (grub_efi_system_table->boot_services->set_timer, event,
	      GRUB_EFI_TIMER_RELATIVE, 200000);
  efi_call_3 (grub_efi_system_table->boot_services->wait_for_event, 1,
	      &event, &idx);
  after = get_itc ();
  efi_call_1 (grub_efi_system_table->boot_services->close_event, event);
  divisor = (after - before + 5) / 20;
  if (divisor == 0)
    divisor = 800000;
  grub_install_get_time_ms (grub_rtc_get_time_ms);
}

void
grub_machine_fini (void)
{
  grub_efi_fini ();
}

void
grub_arch_sync_caches (void *address, grub_size_t len)
{
  /* Cache line length is at least 32.  */
  grub_uint64_t a = (grub_uint64_t)address & ~0x1f;

  /* Flush data.  */
  for (len = (len + 31) & ~0x1f; len > 0; len -= 0x20, a += 0x20)
    asm volatile ("fc.i %0" : : "r" (a));
  /* Sync and serialize.  Maybe extra.  */
  asm volatile (";; sync.i;; srlz.i;;");
}
