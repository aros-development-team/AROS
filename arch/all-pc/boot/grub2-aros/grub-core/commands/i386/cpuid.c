/* cpuid.c - test for CPU features */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006, 2007, 2009  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/command.h>
#include <grub/extcmd.h>
#include <grub/i386/cpuid.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

static const struct grub_arg_option options[] =
  {
    /* TRANSLATORS: "(default)" at the end means that this option is used if
       no argument is specified.  */
    {"long-mode", 'l', 0, N_("Check if CPU supports 64-bit (long) mode (default)."), 0, 0},
    {"pae", 'p', 0, N_("Check if CPU supports Physical Address Extension."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

enum
  {
    MODE_LM = 0,
    MODE_PAE = 1
  };

enum
  {
    bit_PAE = (1 << 6),
  };
enum
  {
    bit_LM = (1 << 29)
  };

unsigned char grub_cpuid_has_longmode = 0, grub_cpuid_has_pae = 0;

static grub_err_t
grub_cmd_cpuid (grub_extcmd_context_t ctxt,
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  int val = 0;
  if (ctxt->state[MODE_PAE].set)
    val = grub_cpuid_has_pae;
  else
    val = grub_cpuid_has_longmode;
  return val ? GRUB_ERR_NONE
    /* TRANSLATORS: it's a standalone boolean value,
       opposite of "true".  */
    : grub_error (GRUB_ERR_TEST_FAILURE, N_("false"));
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(cpuid)
{
#ifdef __x86_64__
  /* grub-emu */
  grub_cpuid_has_longmode = 1;
  grub_cpuid_has_pae = 1;
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
  grub_cpuid (0, eax, ebx, ecx, edx);
  /* We only look at the first four characters.  */
  max_level = eax;
  if (max_level == 0)
    goto done;

  if (max_level >= 1)
    {
      grub_cpuid (1, eax, ebx, ecx, edx);
      grub_cpuid_has_pae = !!(edx & bit_PAE);
    }

  grub_cpuid (0x80000000, eax, ebx, ecx, edx);
  ext_level = eax;
  if (ext_level < 0x80000000)
    goto done;

  grub_cpuid (0x80000001, eax, ebx, ecx, edx);
  grub_cpuid_has_longmode = !!(edx & bit_LM);
done:
#endif

  cmd = grub_register_extcmd ("cpuid", grub_cmd_cpuid, 0,
			      "[-l]", N_("Check for CPU features."), options);
}

GRUB_MOD_FINI(cpuid)
{
  grub_unregister_extcmd (cmd);
}
