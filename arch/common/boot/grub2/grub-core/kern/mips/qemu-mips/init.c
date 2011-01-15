#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/machine/kernel.h>
#include <grub/machine/memory.h>
#include <grub/cpu/kernel.h>

#define RAMSIZE (*(grub_uint32_t *) ((16 << 20) - 264))

grub_uint32_t
grub_get_rtc (void)
{
  static int calln = 0;
  return calln++;
}

void
grub_machine_init (void)
{
  grub_mm_init_region ((void *) GRUB_MACHINE_MEMORY_USABLE,
		       RAMSIZE - (GRUB_MACHINE_MEMORY_USABLE & 0x7fffffff));
  grub_install_get_time_ms (grub_rtc_get_time_ms);
}

void
grub_machine_fini (void)
{
}

void
grub_exit (void)
{
  while (1);
}

void
grub_halt (void)
{
  while (1);
}

void
grub_reboot (void)
{
  while (1);
}

grub_err_t 
grub_machine_mmap_iterate (grub_memory_hook_t hook)
{
  hook (0, RAMSIZE, GRUB_MEMORY_AVAILABLE);
  return GRUB_ERR_NONE;
}
