/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#include <config.h>
#include <grub/dl.h>
#include <grub/env.h>
#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/emu/misc.h>
#include <grub/disk.h>

void
grub_register_exported_symbols (void)
{
}

grub_err_t
grub_arch_dl_check_header (void *ehdr)
{
  (void) ehdr;
  return GRUB_ERR_BAD_MODULE;
}

grub_err_t
grub_arch_dl_relocate_symbols (grub_dl_t mod, void *ehdr)
{
  (void) mod;
  (void) ehdr;
  return GRUB_ERR_BAD_MODULE;
}

void
grub_emu_init (void)
{
  grub_no_autoload = 1;
}

#if defined (__ia64__) || defined (__powerpc__)
void grub_arch_dl_get_tramp_got_size (const void *ehdr __attribute__ ((unused)),
				      grub_size_t *tramp, grub_size_t *got)
{
  *tramp = 0;
  *got = 0;
}
#endif

#ifdef GRUB_LINKER_HAVE_INIT
void
grub_arch_dl_init_linker (void)
{
}
#endif

void
grub_emu_post_init (void)
{
}
