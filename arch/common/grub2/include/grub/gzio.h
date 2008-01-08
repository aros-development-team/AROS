/* gzio.h - prototypes for gzio */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_GZIO_H
#define GRUB_GZIO_H	1

#include <grub/file.h>

grub_file_t grub_gzio_open (grub_file_t io, int transparent);
grub_file_t grub_gzfile_open (const char *name, int transparent);

#endif /* ! GRUB_GZIO_H */
