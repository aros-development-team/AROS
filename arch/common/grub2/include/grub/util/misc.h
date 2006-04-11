/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005  Free Software Foundation, Inc.
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

#ifndef GRUB_UTIL_MISC_HEADER
#define GRUB_UTIL_MISC_HEADER	1

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

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

#endif /* ! GRUB_UTIL_MISC_HEADER */
