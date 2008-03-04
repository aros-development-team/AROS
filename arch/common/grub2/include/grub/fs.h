/* fs.h - filesystem manager */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2007,2008  Free Software Foundation, Inc.
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

#ifndef GRUB_FS_HEADER
#define GRUB_FS_HEADER	1

#include <grub/device.h>
#include <grub/symbol.h>
#include <grub/types.h>

/* Forward declaration is required, because of mutual reference.  */
struct grub_file;

/* Filesystem descriptor.  */
struct grub_fs
{
  /* My name.  */
  const char *name;

  /* Call HOOK with each file under DIR.  */
  grub_err_t (*dir) (grub_device_t device, const char *path,
		     int (*hook) (const char *filename, int dir));
  
  /* Open a file named NAME and initialize FILE.  */
  grub_err_t (*open) (struct grub_file *file, const char *name);
  
  /* Read LEN bytes data from FILE into BUF.  */
  grub_ssize_t (*read) (struct grub_file *file, char *buf, grub_size_t len);
  
  /* Close the file FILE.  */
  grub_err_t (*close) (struct grub_file *file);
  
  /* Return the label of the device DEVICE in LABEL.  The label is
     returned in a grub_malloc'ed buffer and should be freed by the
     caller.  */
  grub_err_t (*label) (grub_device_t device, char **label);

  /* The next filesystem.  */
  struct grub_fs *next;
};
typedef struct grub_fs *grub_fs_t;

/* This is special, because block lists are not files in usual sense.  */
extern struct grub_fs grub_fs_blocklist;

/* This hook is used to automatically load filesystem modules.
   If this hook loads a module, return non-zero. Otherwise return zero.
   The newly loaded filesystem is assumed to be inserted into the head of
   the linked list GRUB_FS_LIST through the function grub_fs_register.  */
typedef int (*grub_fs_autoload_hook_t) (void);
extern grub_fs_autoload_hook_t EXPORT_VAR(grub_fs_autoload_hook);

void EXPORT_FUNC(grub_fs_register) (grub_fs_t fs);
void EXPORT_FUNC(grub_fs_unregister) (grub_fs_t fs);
void EXPORT_FUNC(grub_fs_iterate) (int (*hook) (const grub_fs_t fs));
grub_fs_t EXPORT_FUNC(grub_fs_probe) (grub_device_t device);

#endif /* ! GRUB_FS_HEADER */
