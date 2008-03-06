/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2006,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_UTIL_MISC_HEADER
#define GRUB_UTIL_MISC_HEADER	1

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>

#ifdef __NetBSD__
/* NetBSD uses /boot for its boot block.  */
# define DEFAULT_DIRECTORY	"/grub"
#else
# define DEFAULT_DIRECTORY	"/boot/grub"
#endif

#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

extern char *progname;
extern int verbosity;
extern jmp_buf main_env;

void grub_util_info (const char *fmt, ...);
void grub_util_error (const char *fmt, ...) __attribute__ ((noreturn));

void *xmalloc (size_t size);
void *xrealloc (void *ptr, size_t size);
char *xstrdup (const char *str);

char *grub_util_get_path (const char *dir, const char *file);
size_t grub_util_get_fp_size (FILE *fp);
size_t grub_util_get_image_size (const char *path);
void grub_util_read_at (void *img, size_t len, off_t offset, FILE *fp);
char *grub_util_read_image (const char *path);
void grub_util_load_image (const char *path, char *buf);
void grub_util_write_image (const char *img, size_t size, FILE *out);
void grub_util_write_image_at (const void *img, size_t size, off_t offset,
			       FILE *out);
char *grub_util_get_disk_name (int disk, char *name);

#endif /* ! GRUB_UTIL_MISC_HEADER */
