/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/efiemu/efiemu.h>
#include <grub/command.h>

#define cpuid(num,a,b,c,d) \
  asm volatile ("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1" \
		: "=a" (a), "=r" (b), "=c" (c), "=d" (d)  \
		: "0" (num))

#define bit_LM (1 << 29)

const char *
grub_efiemu_get_default_core_name (void)
{

  unsigned int eax, ebx, ecx, edx;
  unsigned int max_level;
  unsigned int ext_level;

  /* See if we can use cpuid.  */
  asm volatile ("pushfl; pushfl; popl %0; movl %0,%1; xorl %2,%0;"
		"pushl %0; popfl; pushfl; popl %0; popfl"
		: "=&r" (eax), "=&r" (ebx)
		: "i" (0x00200000));
  if (((eax ^ ebx) & 0x00200000) == 0)
    return "efiemu32.o";

  /* Check the highest input value for eax.  */
  cpuid (0, eax, ebx, ecx, edx);
  /* We only look at the first four characters.  */
  max_level = eax;
  if (max_level == 0)
    return "efiemu32.o";

  cpuid (0x80000000, eax, ebx, ecx, edx);
  ext_level = eax;
  if (ext_level < 0x80000000)
    return "efiemu32.o";

  cpuid (0x80000001, eax, ebx, ecx, edx);
  return (edx & bit_LM) ? "efiemu64.o" : "efiemu32.o";
}
