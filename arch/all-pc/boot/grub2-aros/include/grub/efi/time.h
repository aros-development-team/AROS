/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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

#ifndef GRUB_EFI_TIME_HEADER
#define GRUB_EFI_TIME_HEADER	1

#include <grub/symbol.h>

/* This is destined to overflow when one hour passes by.  */
#define GRUB_TICKS_PER_SECOND	((1UL << 31) / 60 / 60 * 2)

/* Return the real time in ticks.  */
grub_uint32_t EXPORT_FUNC (grub_get_rtc) (void);

#endif /* ! GRUB_EFI_TIME_HEADER */
