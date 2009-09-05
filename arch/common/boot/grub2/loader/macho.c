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
#include <grub/gzio.h>
#include <grub/misc.h>
#include <grub/mm.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))

/* 32-bit. */

int
grub_macho_contains_macho32 (grub_macho_t macho)
{
  return macho->offset32 != -1;
}

static void
grub_macho_parse32 (grub_macho_t macho)
{
  struct grub_macho_header32 head;

  /* Is there any candidate at all? */
  if (macho->offset32 == -1)
    return;

  /* Read header and check magic*/
  if (grub_file_seek (macho->file, macho->offset32) == (grub_off_t) -1
      || grub_file_read (macho->file, &head, sizeof (head))
      != sizeof(head))
    {
      grub_error (GRUB_ERR_READ_ERROR, "Cannot read Mach-O header.");
      macho->offset32 = -1;
      return;
    }
  if (head.magic != GRUB_MACHO_MAGIC32)
    {
      grub_error (GRUB_ERR_BAD_OS, "Invalid Mach-O 32-bit header.");
      macho->offset32 = -1;
      return;
    }

  /* Read commands. */
  macho->ncmds32 = head.ncmds;
  macho->cmdsize32 = head.sizeofcmds;
  macho->cmds32 = grub_malloc(macho->cmdsize32);
  if (! macho->cmds32)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "not enough memory to read commands");
      return;
    }
  if (grub_file_read (macho->file, macho->cmds32,
		      (grub_size_t) macho->cmdsize32)
      != (grub_ssize_t) macho->cmdsize32)
    {
      grub_error (GRUB_ERR_READ_ERROR, "Cannot read Mach-O header.");
      macho->offset32 = -1;
    }
}

typedef int NESTED_FUNC_ATTR (*grub_macho_iter_hook_t)
(grub_macho_t , struct grub_macho_cmd *,
	       void *);

static grub_err_t
grub_macho32_cmds_iterate (grub_macho_t macho,
			   grub_macho_iter_hook_t hook,
			   void *hook_arg)
{
  grub_uint8_t *hdrs = macho->cmds32;
  int i;
  if (! macho->cmds32)
    return grub_error (GRUB_ERR_BAD_OS, "Couldn't find 32-bit Mach-O");
  for (i = 0; i < macho->ncmds32; i++)
    {
      struct grub_macho_cmd *hdr = (struct grub_macho_cmd *) hdrs;
      if (hook (macho, hdr, hook_arg))
	break;
      hdrs += hdr->cmdsize;
    }

  return grub_errno;
}

grub_size_t
grub_macho32_filesize (grub_macho_t macho)
{
  if (grub_macho_contains_macho32 (macho))
    return macho->end32 - macho->offset32;
  return 0;
}

grub_err_t
grub_macho32_readfile (grub_macho_t macho, void *dest)
{
  grub_ssize_t read;
  if (! grub_macho_contains_macho32 (macho))
    return grub_error (GRUB_ERR_BAD_OS,
		       "Couldn't read architecture-specific part");

  if (grub_file_seek (macho->file, macho->offset32) == (grub_off_t) -1)
    {
      grub_error_push ();
      return grub_error (GRUB_ERR_BAD_OS,
			 "Invalid offset in program header.");
    }

  read = grub_file_read (macho->file, dest,
			 macho->end32 - macho->offset32);
  if (read != (grub_ssize_t) (macho->end32 - macho->offset32))
    {
      grub_error_push ();
      return grub_error (GRUB_ERR_BAD_OS,
			 "Couldn't read architecture-specific part");
    }
  return GRUB_ERR_NONE;
}

/* Calculate the amount of memory spanned by the segments. */
grub_err_t
grub_macho32_size (grub_macho_t macho, grub_addr_t *segments_start,
		   grub_addr_t *segments_end, int flags)
{
  int nr_phdrs = 0;

  /* Run through the program headers to calculate the total memory size we
     should claim.  */
  auto int NESTED_FUNC_ATTR calcsize (grub_macho_t _macho,
				      struct grub_macho_cmd *phdr, void *_arg);
  int NESTED_FUNC_ATTR calcsize (grub_macho_t UNUSED _macho,
				 struct grub_macho_cmd *hdr0, void UNUSED *_arg)
    {
      struct grub_macho_segment32 *hdr = (struct grub_macho_segment32 *) hdr0;
      if (hdr->cmd != GRUB_MACHO_CMD_SEGMENT32)
	return 0;
      if (! hdr->filesize && (flags & GRUB_MACHO_NOBSS))
	return 0;

      nr_phdrs++;
      if (hdr->vmaddr < *segments_start)
	*segments_start = hdr->vmaddr;
      if (hdr->vmaddr + hdr->vmsize > *segments_end)
	*segments_end = hdr->vmaddr + hdr->vmsize;
      return 0;
    }

  *segments_start = (grub_uint32_t) -1;
  *segments_end = 0;

  grub_macho32_cmds_iterate (macho, calcsize, 0);

  if (nr_phdrs == 0)
    return grub_error (GRUB_ERR_BAD_OS, "No program headers present");

  if (*segments_end < *segments_start)
    /* Very bad addresses.  */
    return grub_error (GRUB_ERR_BAD_OS, "Bad program header load addresses");

  return GRUB_ERR_NONE;
}

/* Load every loadable segment into memory specified by `_load_hook'.  */
grub_err_t
grub_macho32_load (grub_macho_t macho, char *offset, int flags)
{
  grub_err_t err = 0;
  auto int NESTED_FUNC_ATTR do_load(grub_macho_t _macho,
			       struct grub_macho_cmd *hdr0,
			       void UNUSED *_arg);
  int NESTED_FUNC_ATTR do_load(grub_macho_t _macho,
			       struct grub_macho_cmd *hdr0,
			       void UNUSED *_arg)
  {
    struct grub_macho_segment32 *hdr = (struct grub_macho_segment32 *) hdr0;

    if (hdr->cmd != GRUB_MACHO_CMD_SEGMENT32)
      return 0;

    if (! hdr->filesize && (flags & GRUB_MACHO_NOBSS))
      return 0;
    if (! hdr->vmsize)
      return 0;

    if (grub_file_seek (_macho->file, hdr->fileoff
			+ _macho->offset32) == (grub_off_t) -1)
      {
	grub_error_push ();
	grub_error (GRUB_ERR_BAD_OS,
		    "Invalid offset in program header.");
	return 1;
      }

    if (hdr->filesize)
      {
	grub_ssize_t read;
	read = grub_file_read (_macho->file, offset + hdr->vmaddr,
				   min (hdr->filesize, hdr->vmsize));
	if (read != (grub_ssize_t) min (hdr->filesize, hdr->vmsize))
	  {
	    /* XXX How can we free memory from `load_hook'? */
	    grub_error_push ();
	    err=grub_error (GRUB_ERR_BAD_OS,
			    "Couldn't read segment from file: "
			    "wanted 0x%lx bytes; read 0x%lx bytes.",
			    hdr->filesize, read);
	    return 1;
	  }
      }

    if (hdr->filesize < hdr->vmsize)
      grub_memset (offset + hdr->vmaddr + hdr->filesize,
		   0, hdr->vmsize - hdr->filesize);
    return 0;
  }

  grub_macho32_cmds_iterate (macho, do_load, 0);

  return err;
}

grub_uint32_t
grub_macho32_get_entry_point (grub_macho_t macho)
{
  grub_uint32_t entry_point = 0;
  auto int NESTED_FUNC_ATTR hook(grub_macho_t _macho,
			       struct grub_macho_cmd *hdr,
			       void UNUSED *_arg);
  int NESTED_FUNC_ATTR hook(grub_macho_t UNUSED _macho,
			       struct grub_macho_cmd *hdr,
			       void UNUSED *_arg)
  {
    if (hdr->cmd == GRUB_MACHO_CMD_THREAD)
      entry_point = ((struct grub_macho_thread32 *) hdr)->entry_point;
    return 0;
  }
  grub_macho32_cmds_iterate (macho, hook, 0);
  return entry_point;
}


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
      grub_error (GRUB_ERR_READ_ERROR, "Cannot read Mach-O header.");
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
	  grub_error (GRUB_ERR_READ_ERROR, "Cannot read Mach-O header.");
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
  /* FIXME: implement 64-bit.*/
  /*  grub_macho_parse64 (macho); */

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

  file = grub_gzfile_open (name, 1);
  if (! file)
    return 0;

  macho = grub_macho_file (file);
  if (! macho)
    grub_file_close (file);

  return macho;
}
