/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005  Free Software Foundation, Inc.
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

#ifndef GRUB_BIOSDISK_MACHINE_HEADER
#define GRUB_BIOSDISK_MACHINE_HEADER	1

#define GRUB_BIOSDISK_FLAG_LBA	1

struct grub_biosdisk_data
{
  int drive;
  unsigned long cylinders;
  unsigned long heads;
  unsigned long sectors;
  unsigned long flags;
};

int grub_biosdisk_rw_int13_extensions (int ah, int drive, void *dap);
int grub_biosdisk_rw_standard (int ah, int drive, int coff, int hoff,
			       int soff, int nsec, int segment);
int grub_biosdisk_check_int13_extensions (int drive);
int grub_biosdisk_get_diskinfo_int13_extensions (int drive, void *drp);
int grub_biosdisk_get_diskinfo_standard (int drive,
					 unsigned long *cylinders,
					 unsigned long *heads,
					 unsigned long *sectors);
int grub_biosdisk_get_num_floppies (void);

void grub_biosdisk_init (void);
void grub_biosdisk_fini (void);

#endif /* ! GRUB_BIOSDISK_MACHINE_HEADER */
