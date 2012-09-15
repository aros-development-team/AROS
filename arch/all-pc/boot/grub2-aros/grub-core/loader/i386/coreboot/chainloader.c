/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2011  Free Software Foundation, Inc.
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

#include <grub/loader.h>
#include <grub/memory.h>
#include <grub/i386/memory.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/elfload.h>
#include <grub/video.h>
#include <grub/relocator.h>
#include <grub/i386/relocator.h>
#include <grub/command.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_addr_t entry;
static struct grub_relocator *relocator = NULL;

static grub_err_t
grub_chain_boot (void)
{
  struct grub_relocator32_state state;

  grub_video_set_mode ("text", 0, 0);

  state.eip = entry;
  return grub_relocator32_boot (relocator, state, 0);
}

static grub_err_t
grub_chain_unload (void)
{
  grub_relocator_unload (relocator);
  relocator = NULL;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_chain_elf32_hook (Elf32_Phdr * phdr, grub_addr_t * addr, int *do_load)
{
  grub_err_t err;
  grub_relocator_chunk_t ch;

  if (phdr->p_type != PT_LOAD)
    {
      *do_load = 0;
      return 0;
    }

  *do_load = 1;
  err = grub_relocator_alloc_chunk_addr (relocator, &ch,
					 phdr->p_paddr, phdr->p_memsz);
  if (err)
    return err;

  *addr = (grub_addr_t) get_virtual_current_address (ch);

  return GRUB_ERR_NONE;
}


static grub_err_t
grub_cmd_chain (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_err_t err;
  grub_file_t file;
  grub_elf_t elf;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  grub_loader_unset ();

  file = grub_file_open (argv[0]);
  if (!file)
    return grub_errno;

  relocator = grub_relocator_new ();
  if (!relocator)
    {
      grub_file_close (file);
      return grub_errno;
    }

  elf = grub_elf_file (file, argv[0]);
  if (!elf)
    {
      grub_relocator_unload (relocator);
      relocator = 0;
      grub_file_close (file);
    }

  if (!grub_elf_is_elf32 (elf))
    {
      grub_relocator_unload (relocator);
      relocator = 0;
      grub_elf_close (elf);
    }

  entry = elf->ehdr.ehdr32.e_entry & 0xFFFFFF;
  
  err = grub_elf32_load (elf, argv[0], grub_chain_elf32_hook, 0, 0);

  grub_elf_close (elf);
  if (err)
    return err;

  grub_loader_set (grub_chain_boot, grub_chain_unload, 0);
  return GRUB_ERR_NONE;
}

static grub_command_t cmd_chain;

GRUB_MOD_INIT (chain)
{
  cmd_chain = grub_register_command ("chainloader", grub_cmd_chain,
				     N_("FILE"),
				     /* TRANSLATORS: "payload" is a term used
					by coreboot and must be translated in
					sync with coreboot. If unsure,
					let it untranslated.  */
				     N_("Load another coreboot payload"));
}

GRUB_MOD_FINI (chain)
{
  grub_unregister_command (cmd_chain);
  grub_chain_unload ();
}
