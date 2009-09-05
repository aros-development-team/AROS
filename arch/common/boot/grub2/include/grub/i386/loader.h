/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2007,2008,2009  Free Software Foundation, Inc.
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

#ifndef GRUB_LOADER_CPU_HEADER
#define GRUB_LOADER_CPU_HEADER	1

#include <grub/types.h>
#include <grub/err.h>
#include <grub/symbol.h>
#include <grub/machine/machine.h>

extern grub_addr_t EXPORT_VAR(grub_os_area_addr);
extern grub_size_t EXPORT_VAR(grub_os_area_size);

#ifdef GRUB_MACHINE_PCBIOS
extern grub_uint32_t EXPORT_VAR(grub_linux_prot_size);
extern char *EXPORT_VAR(grub_linux_tmp_addr);
extern char *EXPORT_VAR(grub_linux_real_addr);
extern grub_int32_t EXPORT_VAR(grub_linux_is_bzimage);
grub_err_t EXPORT_FUNC(grub_linux16_boot) (void);
#endif

#endif /* ! GRUB_LOADER_CPU_HEADER */
