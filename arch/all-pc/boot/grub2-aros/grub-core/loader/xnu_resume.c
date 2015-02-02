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

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/xnu.h>
#include <grub/cpu/xnu.h>
#include <grub/mm.h>
#include <grub/loader.h>
#include <grub/i18n.h>

static void *grub_xnu_hibernate_image;

static grub_err_t
grub_xnu_resume_unload (void)
{
  /* Free loaded image */
  if (grub_xnu_hibernate_image)
    grub_free (grub_xnu_hibernate_image);
  grub_xnu_hibernate_image = 0;
  grub_xnu_unlock ();
  return GRUB_ERR_NONE;
}

grub_err_t
grub_xnu_resume (char *imagename)
{
  grub_file_t file;
  grub_size_t total_header_size;
  struct grub_xnu_hibernate_header hibhead;
  void *code;
  void *image;
  grub_uint32_t codedest;
  grub_uint32_t codesize;
  grub_addr_t target_image;
  grub_err_t err;

  grub_file_filter_disable_compression ();
  file = grub_file_open (imagename);
  if (! file)
    return 0;

  /* Read the header. */
  if (grub_file_read (file, &hibhead, sizeof (hibhead))
      != sizeof (hibhead))
    {
      grub_file_close (file);
      if (!grub_errno)
	grub_error (GRUB_ERR_READ_ERROR,
		    N_("premature end of file %s"), imagename);
      return grub_errno;
    }

  /* Check the header. */
  if (hibhead.magic != GRUB_XNU_HIBERNATE_MAGIC)
    {
      grub_file_close (file);
      return grub_error (GRUB_ERR_BAD_OS,
			 "hibernate header has incorrect magic number");
    }
  if (hibhead.encoffset)
    {
      grub_file_close (file);
      return grub_error (GRUB_ERR_BAD_OS,
			 "encrypted images aren't supported yet");
    }

  if (hibhead.image_size == 0)
    {
      grub_file_close (file);
      return grub_error (GRUB_ERR_BAD_OS,
			 "hibernate image is empty");
    }

  codedest = hibhead.launchcode_target_page;
  codedest *= GRUB_XNU_PAGESIZE;
  codesize = hibhead.launchcode_numpages;
  codesize *= GRUB_XNU_PAGESIZE;

  /* FIXME: check that codedest..codedest+codesize is available. */

  /* Calculate total size before pages to copy. */
  total_header_size = hibhead.extmapsize + sizeof (hibhead);

  /* Unload image if any. */
  if (grub_xnu_hibernate_image)
    grub_free (grub_xnu_hibernate_image);

  /* Try to allocate necessary space.
     FIXME: mm isn't good enough yet to handle huge allocations.
   */
  grub_xnu_relocator = grub_relocator_new ();
  if (!grub_xnu_relocator)
    {
      grub_file_close (file);
      return grub_errno;
    }

  {
    grub_relocator_chunk_t ch;
    err = grub_relocator_alloc_chunk_addr (grub_xnu_relocator, &ch, codedest,
					   codesize + GRUB_XNU_PAGESIZE);
    if (err)
      {
	grub_file_close (file);
	return err;
      }
    code = get_virtual_current_address (ch);
  }

  {
    grub_relocator_chunk_t ch;
    err = grub_relocator_alloc_chunk_align (grub_xnu_relocator, &ch, 0,
					    (0xffffffff - hibhead.image_size) + 1,
					    hibhead.image_size,
					    GRUB_XNU_PAGESIZE,
					    GRUB_RELOCATOR_PREFERENCE_NONE, 0);
    if (err)
      {
	grub_file_close (file);
	return err;
      }
    image = get_virtual_current_address (ch);
    target_image = get_physical_target_address (ch);
  }

  /* Read code part. */
  if (grub_file_seek (file, total_header_size) == (grub_off_t) -1
      || grub_file_read (file, code, codesize)
      != (grub_ssize_t) codesize)
    {
      grub_file_close (file);
      if (!grub_errno)
	grub_error (GRUB_ERR_READ_ERROR,
		    N_("premature end of file %s"), imagename);
      return grub_errno;
    }

  /* Read image. */
  if (grub_file_seek (file, 0) == (grub_off_t) -1
      || grub_file_read (file, image, hibhead.image_size)
      != (grub_ssize_t) hibhead.image_size)
    {
      grub_file_close (file);
      if (!grub_errno)
	grub_error (GRUB_ERR_READ_ERROR,
		    N_("premature end of file %s"), imagename);
      return grub_errno;
    }
  grub_file_close (file);

  /* Setup variables needed by asm helper. */
  grub_xnu_heap_target_start = codedest;
  grub_xnu_heap_size = target_image + hibhead.image_size - codedest;
  grub_xnu_stack = (codedest + hibhead.stack);
  grub_xnu_entry_point = (codedest + hibhead.entry_point);
  grub_xnu_arg1 = target_image;

  grub_dprintf ("xnu", "entry point 0x%x\n", codedest + hibhead.entry_point);
  grub_dprintf ("xnu", "image at 0x%x\n",
		codedest + codesize + GRUB_XNU_PAGESIZE);

  /* We're ready now. */
  grub_loader_set (grub_xnu_boot_resume,
		   grub_xnu_resume_unload, 0);

  /* Prevent module from unloading. */
  grub_xnu_lock ();
  return GRUB_ERR_NONE;
}
