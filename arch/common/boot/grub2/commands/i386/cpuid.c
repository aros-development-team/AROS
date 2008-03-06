/* cpuid.c - test for CPU features */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006, 2007  Free Software Foundation, Inc.
 *  Based on gcc/gcc/config/i386/driver-i386.c
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

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/env.h>

#define cpuid(num,a,b,c,d) \
  asm volatile ("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1" \
		: "=a" (a), "=r" (b), "=c" (c), "=d" (d)  \
		: "0" (num))

#define bit_LM (1 << 29)

unsigned char has_longmode = 0;

static const struct grub_arg_option options[] =
  {
    {"long-mode", 'l', 0, "check for long mode flag (default)", 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static grub_err_t
grub_cmd_cpuid (struct grub_arg_list *state __attribute__ ((unused)),
	       int argc __attribute__ ((unused)),
	       char **args __attribute__ ((unused)))
{
  return !has_longmode;
}

GRUB_MOD_INIT(cpuid)
{
#ifdef __x86_64__
  /* grub-emu */
  has_longmode = 1;
#else
  unsigned int eax, ebx, ecx, edx;
  unsigned int max_level;
  unsigned int ext_level;

  /* See if we can use cpuid.  */
  asm volatile ("pushfl; pushfl; popl %0; movl %0,%1; xorl %2,%0;"
		"pushl %0; popfl; pushfl; popl %0; popfl"
		: "=&r" (eax), "=&r" (ebx)
		: "i" (0x00200000));
  if (((eax ^ ebx) & 0x00200000) == 0)
    goto done;

  /* Check the highest input value for eax.  */
  cpuid (0, eax, ebx, ecx, edx);
  /* We only look at the first four characters.  */
  max_level = eax;
  if (max_level == 0)
    goto done;

  cpuid (0x80000000, eax, ebx, ecx, edx);
  ext_level = eax;
  if (ext_level < 0x80000000)
    goto done;

  cpuid (0x80000001, eax, ebx, ecx, edx);
  has_longmode = !!(edx & bit_LM);
done:
#endif

  grub_register_command ("cpuid", grub_cmd_cpuid, GRUB_COMMAND_FLAG_CMDLINE,
			 "cpuid", "Check for CPU features", options);
}

GRUB_MOD_FINI(cpuid)
{
  grub_unregister_command ("cpuid");
}
