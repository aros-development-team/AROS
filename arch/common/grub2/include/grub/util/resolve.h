/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_UTIL_RESOLVE_HEADER
#define GRUB_UTIL_RESOLVE_HEADER	1

struct grub_util_path_list
{
  const char *name;
  struct grub_util_path_list *next;
};

/* Resolve the dependencies of the modules MODULES using the information
   in the file DEP_LIST_FILE. The directory PREFIX is used to find files.  */
struct grub_util_path_list *
grub_util_resolve_dependencies (const char *prefix,
				const char *dep_list_file,
				char *modules[]);

#endif /* ! GRUB_UTIL_RESOLVE_HEADER */
