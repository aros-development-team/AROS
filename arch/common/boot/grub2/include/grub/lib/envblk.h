/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#ifndef GRUB_ENVBLK_HEADER
#define GRUB_ENVBLK_HEADER	1

#define GRUB_ENVBLK_SIGNATURE	0x764e6547	/* GeNv  */

#define GRUB_ENVBLK_MAXLEN	8192

#define GRUB_ENVBLK_DEFCFG	"grubenv"

#ifndef ASM_FILE

struct grub_envblk
{
  grub_uint32_t signature;
  grub_uint16_t length;
  char data[0];
} __attribute__ ((packed));
typedef struct grub_envblk *grub_envblk_t;

grub_envblk_t grub_envblk_find (char *buf);
int grub_envblk_insert (grub_envblk_t envblk, char *name, char *value);
void grub_envblk_delete (grub_envblk_t envblk, char *name);
void grub_envblk_iterate (grub_envblk_t envblk, int hook (char *name, char *value));

#endif

#endif /* ! GRUB_ENVBLK_HEADER */
