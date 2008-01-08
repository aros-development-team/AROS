/* multiboot2.c - boot a multiboot 2 OS image. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007  Free Software Foundation, Inc.
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

#include <multiboot2.h>
#include <grub/loader.h>
#include <grub/machine/loader.h>
#include <grub/multiboot2.h>
#include <grub/elfload.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/rescue.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/gzio.h>

static grub_addr_t entry;
extern grub_dl_t my_mod;

static char *grub_mb2_tags;
static char *grub_mb2_tags_pos;
static grub_size_t grub_mb2_tags_len;
static int grub_mb2_tags_count;

static void
grub_mb2_tags_free (void)
{
  grub_dprintf ("loader", "Freeing all tags...\n");
  grub_free (grub_mb2_tags);
  grub_mb2_tags = 0;
  grub_mb2_tags_pos = 0;
  grub_mb2_tags_len = 0;
  grub_mb2_tags_count = 0;
}

grub_err_t
grub_mb2_tag_alloc (grub_addr_t *addr, int key, grub_size_t len)
{
  struct multiboot_tag_header *tag;
  grub_size_t used;
  grub_size_t needed;

  grub_dprintf ("loader", "Allocating tag: key 0x%x, size 0x%lx.\n",
		key, (unsigned long) len);

  used = grub_mb2_tags_pos - grub_mb2_tags;
  len = ALIGN_UP (len, sizeof (multiboot_word));

  needed = used + len;

  if (needed > grub_mb2_tags_len)
    {
      /* Allocate new buffer.  */
      grub_size_t newsize = needed * 2;
      char *newarea;

      grub_dprintf ("loader", "Reallocating tag buffer (new size 0x%lx).\n",
		    (unsigned long) newsize);

      newarea = grub_malloc (newsize);
      if (! newarea)
	return grub_errno;
      grub_memcpy (newarea, grub_mb2_tags, grub_mb2_tags_len);
      grub_free (grub_mb2_tags);

      grub_mb2_tags_len = newsize;
      grub_mb2_tags = newarea;
      grub_mb2_tags_pos = newarea + used;
    }

  tag = (struct multiboot_tag_header *) grub_mb2_tags_pos;
  grub_mb2_tags_pos += len;

  tag->key = key;
  tag->len = len;

  if (addr)
    *addr = (grub_addr_t) tag;

  grub_mb2_tags_count++;

  grub_dprintf ("loader", "Allocated tag %u at %p.\n", grub_mb2_tags_count, tag);

  return 0;
}

static grub_err_t
grub_mb2_tag_start_create (void)
{
  return grub_mb2_tag_alloc (0, MULTIBOOT2_TAG_START,
			    sizeof (struct multiboot_tag_start));
}

static grub_err_t
grub_mb2_tag_name_create (void)
{
  struct multiboot_tag_name *name;
  grub_addr_t name_addr;
  grub_err_t err;
  const char *grub_version = PACKAGE_STRING;

  err = grub_mb2_tag_alloc (&name_addr, MULTIBOOT2_TAG_NAME,
			   sizeof (struct multiboot_tag_name) +
			   sizeof (grub_version) + 1);
  if (err)
    return err;

  name = (struct multiboot_tag_name *) name_addr;
  grub_strcpy (name->name, grub_version);

  return GRUB_ERR_NONE;
}

typedef grub_err_t (*tag_create_t) (void);
static tag_create_t grub_mb2_tag_creators[] = {
  grub_mb2_tag_start_create,
  grub_mb2_tag_name_create,
  grub_mb2_tags_arch_create,
  0,
};

static grub_err_t
grub_mb2_tags_create (void)
{
  tag_create_t *creator;
  grub_err_t err;

  for (creator = grub_mb2_tag_creators; *creator != 0; creator++)
    {
      err = (*creator) ();
      if (err)
	goto error;
    }

  return GRUB_ERR_NONE;

error:
  grub_error_push ();
  grub_mb2_tags_free ();
  grub_error_pop ();
  return err;
}

static grub_err_t
grub_mb2_tags_finish (void)
{
  struct multiboot_tag_start *start;
  grub_err_t err;

  /* Create the `end' tag.  */
  err = grub_mb2_tag_alloc (0, MULTIBOOT2_TAG_END,
			   sizeof (struct multiboot_tag_end));
  if (err)
    goto error;

  /* We created the `start' tag first.  Update it now.  */
  start = (struct multiboot_tag_start *) grub_mb2_tags;
  start->size = grub_mb2_tags_pos - grub_mb2_tags;
  return GRUB_ERR_NONE;

error:
  grub_error_push ();
  grub_mb2_tags_free ();
  grub_error_pop ();
  return err;
}

static grub_err_t
grub_mb2_boot (void)
{
  grub_mb2_tags_finish ();

  grub_dprintf ("loader", "Tags at %p\n", grub_mb2_tags);
  grub_mb2_arch_boot (entry, grub_mb2_tags);

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_mb2_unload (void)
{
  struct multiboot_tag_header *tag;
  struct multiboot_tag_header *tags =
    (struct multiboot_tag_header *) grub_mb2_tags;

  /* Free all module memory in the tag list.  */
  for_each_tag (tag, tags)
    {
      if (tag->key == MULTIBOOT2_TAG_MODULE)
	{
	  struct multiboot_tag_module *module =
	      (struct multiboot_tag_module *) tag;
	  grub_free ((void *) module->addr);
	}
    }

  /* Allow architecture to un-reserve memory.  */
  grub_mb2_arch_unload (tags);

  /* Free the tags themselves.  */
  grub_mb2_tags_free ();

  grub_dl_unref (my_mod);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_mb2_load_other (UNUSED grub_file_t file, UNUSED void *buffer)
{
  /* XXX Create module tag here.  */
  return grub_error (GRUB_ERR_UNKNOWN_OS, "currently only ELF is supported");
}

/* Create the tag containing the cmdline and the address of the module data.  */
static grub_err_t
grub_mb2_tag_module_create (grub_addr_t modaddr, grub_size_t modsize,
			    char *type, int key, int argc, char *argv[])
{
  struct multiboot_tag_module *module;
  grub_ssize_t argslen = 0;
  grub_err_t err;
  char *p;
  grub_addr_t module_addr;
  int i;

  /* Allocate enough space for the arguments and spaces between them.  */
  for (i = 0; i < argc; i++)
    argslen += grub_strlen (argv[i]) + 1;

  /* Note: includes implicit 1-byte cmdline.  */
  err = grub_mb2_tag_alloc (&module_addr, key,
			   sizeof (struct multiboot_tag_module) + argslen);
  if (err)
    return grub_errno;

  module = (struct multiboot_tag_module *) module_addr;
  module->addr = modaddr;
  module->size = modsize;
  grub_strcpy(module->type, type);

  /* Fill in the command line.  */
  p = module->cmdline;
  for (i = 0; i < argc; i++)
    {
      p = grub_stpcpy (p, argv[i]);
      *p++ = ' ';
    }
  module->cmdline[argslen] = '\0';

  return GRUB_ERR_NONE;
}

/* Load ELF32 or ELF64.  */
static grub_err_t
grub_mb2_load_elf (grub_elf_t elf, int argc, char *argv[])
{
  grub_addr_t kern_base;
  grub_size_t kern_size;
  grub_err_t err;

  if (grub_elf_is_elf32 (elf))
    {
      entry = elf->ehdr.ehdr32.e_entry;
      err = grub_elf32_load (elf, grub_mb2_arch_elf32_hook, &kern_base,
			     &kern_size);
    }
  else if (grub_elf_is_elf64 (elf))
    {
      entry = elf->ehdr.ehdr64.e_entry;
      err = grub_elf64_load (elf, grub_mb2_arch_elf64_hook, &kern_base,
			     &kern_size);
    }
  else
    err = grub_error (GRUB_ERR_UNKNOWN_OS, "unknown ELF class");

  if (err)
    goto fail;

  grub_dprintf ("loader", "Entry point is 0x%lx.\n", (unsigned long) entry);

  grub_mb2_tag_module_create (kern_base, kern_size, "kernel",
			     MULTIBOOT2_TAG_MODULE, argc, argv);

fail:
  return err;
}

void
grub_multiboot2 (int argc, char *argv[])
{
  char *buffer;
  grub_file_t file = 0;
  grub_elf_t elf = 0;
  struct multiboot_header *header = 0;
  char *p;
  grub_ssize_t len;
  grub_err_t err;
  int header_found = 0;

  grub_loader_unset ();

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "No kernel specified");
      goto fail;
    }

  file = grub_gzfile_open (argv[0], 1);
  if (! file)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "Couldn't open file");
      goto fail;
    }

  buffer = grub_malloc (MULTIBOOT2_HEADER_SEARCH);
  if (! buffer)
    return;

  len = grub_file_read (file, buffer, MULTIBOOT2_HEADER_SEARCH);
  if (len < 32)
    {
      grub_error (GRUB_ERR_BAD_OS, "File too small");
      goto fail;
    }

  /* Look for the multiboot header in the buffer.  The header should
     be at least 12 bytes and aligned on a 4-byte boundary.  */
  for (p = buffer; p <= buffer + len - 12; p += 4)
    {
      header = (struct multiboot_header *) p;
      if (header->magic == MULTIBOOT2_HEADER_MAGIC)
	{
	  header_found = 1;
	  break;
	}
    }

  if (! header_found)
    grub_dprintf ("loader", "No multiboot 2 header found.\n");


  /* Create the basic tags.  */
  grub_dprintf ("loader", "Creating multiboot 2 tags\n");
  grub_mb2_tags_create ();

  /* Load the kernel and create its tag.  */
  elf = grub_elf_file (file);
  if (elf)
    {
      grub_dprintf ("loader", "Loading ELF multiboot 2 file.\n");
      err = grub_mb2_load_elf (elf, argc-1, &argv[1]);
      grub_elf_close (elf);
    }
  else
    {
      grub_dprintf ("loader", "Loading non-ELF multiboot 2 file.\n");

      if (header)
	err = grub_mb2_load_other (file, header);
      else
	err = grub_error (GRUB_ERR_BAD_OS,
			  "Need multiboot 2 header to load non-ELF files.");
      grub_file_close (file);
    }

  grub_free (buffer);

  if (err)
    goto fail;

  /* Good to go.  */
  grub_loader_set (grub_mb2_boot, grub_mb2_unload, 1);
  return;

fail:
  grub_mb2_tags_free ();
  grub_dl_unref (my_mod);
}

void
grub_module2 (int argc, char *argv[])
{
  grub_file_t file;
  grub_addr_t modaddr = 0;
  grub_ssize_t modsize = 0;
  grub_err_t err;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "No module specified");
      return;
    }

  if (argc == 1)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "No module type specified");
      return;
    }

  if (entry == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT,
		  "You need to load the multiboot kernel first");
      return;
    }

  /* Load module data.  */
  file = grub_gzfile_open (argv[0], 1);
  if (! file)
    goto out;

  modsize = grub_file_size (file);
  err = grub_mb2_arch_module_alloc (modsize, &modaddr);
  if (err)
    goto out;

  grub_dprintf ("loader", "Loading module at 0x%x - 0x%x\n", modaddr,
		modaddr + modsize);
  if (grub_file_read (file, (char *) modaddr, modsize) != modsize)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto out;
    }

  /* Create the module tag.  */
  err = grub_mb2_tag_module_create (modaddr, modsize,
				   argv[1], MULTIBOOT2_TAG_MODULE,
				   argc-2, &argv[2]);
  if (err)
    goto out;

out:
  grub_error_push ();

  if (file)
    grub_file_close (file);

  if (modaddr)
    grub_mb2_arch_module_free (modaddr, modsize);

  grub_error_pop ();
}
