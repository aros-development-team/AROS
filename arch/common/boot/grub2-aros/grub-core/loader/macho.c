/* macho.c - load Mach-O files. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

/* This Mach-O loader is incomplete and can load only non-relocatable segments.
   This is however enough to boot xnu (otool -l and Mach-O specs for more info).
*/

#include <grub/err.h>
#include <grub/macho.h>
#include <grub/cpu/macho.h>
#include <grub/machoload.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/mm.h>

grub_err_t
grub_macho_close (grub_macho_t macho)
{
  grub_file_t file = macho->file;

  grub_free (macho->cmds32);
  grub_free (macho->cmds64);

  grub_free (macho);

  if (file)
    grub_file_close (file);

  return grub_errno;
}

grub_macho_t
grub_macho_file (grub_file_t file)
{
  grub_macho_t macho;
  union grub_macho_filestart filestart;

  macho = grub_malloc (sizeof (*macho));
  if (! macho)
    return 0;

  macho->file = file;
  macho->offset32 = -1;
  macho->offset64 = -1;
  macho->end32 = -1;
  macho->end64 = -1;
  macho->cmds32 = 0;
  macho->cmds64 = 0;

  if (grub_file_seek (macho->file, 0) == (grub_off_t) -1)
    goto fail;

  if (grub_file_read (macho->file, &filestart, sizeof (filestart))
      != sizeof (filestart))
    {
      grub_error_push ();
      grub_error (GRUB_ERR_READ_ERROR, "cannot read Mach-O header");
      goto fail;
    }

  /* Is it a fat file? */
  if (filestart.fat.magic == grub_be_to_cpu32 (GRUB_MACHO_FAT_MAGIC))
    {
      struct grub_macho_fat_arch *archs;
      int i, narchs;

      /* Load architecture description. */
      narchs = grub_be_to_cpu32 (filestart.fat.nfat_arch);
      if (grub_file_seek (macho->file, sizeof (struct grub_macho_fat_header))
	  == (grub_off_t) -1)
	goto fail;
      archs = grub_malloc (sizeof (struct grub_macho_fat_arch) * narchs);
      if (!archs)
	goto fail;
      if (grub_file_read (macho->file, archs,
			  sizeof (struct grub_macho_fat_arch) * narchs)
	  != (grub_ssize_t)sizeof(struct grub_macho_fat_arch) * narchs)
	{
	  grub_free (archs);
	  grub_error_push ();
	  grub_error (GRUB_ERR_READ_ERROR, "cannot read Mach-O header");
	  goto fail;
	}

      for (i = 0; i < narchs; i++)
	{
	  if (GRUB_MACHO_CPUTYPE_IS_HOST32
	      (grub_be_to_cpu32 (archs[i].cputype)))
	    {
	      macho->offset32 = grub_be_to_cpu32 (archs[i].offset);
	      macho->end32 = grub_be_to_cpu32 (archs[i].offset)
		+ grub_be_to_cpu32 (archs[i].size);
	    }
	  if (GRUB_MACHO_CPUTYPE_IS_HOST64
	      (grub_be_to_cpu32 (archs[i].cputype)))
	    {
	      macho->offset64 = grub_be_to_cpu32 (archs[i].offset);
	      macho->end64 = grub_be_to_cpu32 (archs[i].offset)
		+ grub_be_to_cpu32 (archs[i].size);
	    }
	}
      grub_free (archs);
    }

  /* Is it a thin 32-bit file? */
  if (filestart.thin32.magic == GRUB_MACHO_MAGIC32)
    {
      macho->offset32 = 0;
      macho->end32 = grub_file_size (file);
    }

  /* Is it a thin 64-bit file? */
  if (filestart.thin64.magic == GRUB_MACHO_MAGIC64)
    {
      macho->offset64 = 0;
      macho->end64 = grub_file_size (file);
    }

  grub_macho_parse32 (macho);
  grub_macho_parse64 (macho);

  return macho;

fail:
  grub_macho_close (macho);
  return 0;
}

grub_macho_t
grub_macho_open (const char *name)
{
  grub_file_t file;
  grub_macho_t macho;

  file = grub_file_open (name);
  if (! file)
    return 0;

  macho = grub_macho_file (file);
  if (! macho)
    grub_file_close (file);

  return macho;
}
