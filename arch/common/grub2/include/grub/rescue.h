/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_RESCUE_HEADER
#define GRUB_RESCUE_HEADER	1

#include <grub/symbol.h>

/* Enter rescue mode.  */
void grub_enter_rescue_mode (void);

/* Register a rescue mode command.  */
void EXPORT_FUNC(grub_rescue_register_command) (const char *name,
						void (*func) (int argc,
							      char *argv[]),
						const char *message);

/* Unregister a rescue mode command.  */
void EXPORT_FUNC(grub_rescue_unregister_command) (const char *name);

#endif /* ! GRUB_RESCUE_HEADER */
