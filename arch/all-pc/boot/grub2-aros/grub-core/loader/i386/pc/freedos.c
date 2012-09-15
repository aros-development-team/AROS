/* chainloader.c - boot another boot loader */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2004,2007,2009,2010  Free Software Foundation, Inc.
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
#include <grub/file.h>
#include <grub/err.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/partition.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/machine/biosnum.h>
#include <grub/i18n.h>
#include <grub/video.h>
#include <grub/mm.h>
#include <grub/cpu/relocator.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_dl_t my_mod;
static struct grub_relocator *rel;
static grub_uint32_t ebx = 0xffffffff;

#define GRUB_FREEDOS_SEGMENT         0x60
#define GRUB_FREEDOS_STACK_SEGMENT         0x1fe0
#define GRUB_FREEDOS_STACK_POINTER         0x8000

static grub_err_t
grub_freedos_boot (void)
{
  struct grub_relocator16_state state = { 
    .cs = GRUB_FREEDOS_SEGMENT,
    .ip = 0,
    .ds = 0,
    .es = 0,
    .fs = 0,
    .gs = 0,
    .ss = GRUB_FREEDOS_STACK_SEGMENT,
    .sp = GRUB_FREEDOS_STACK_POINTER,
    .ebx = ebx,
    .edx = 0,
    .a20 = 1
  };
  grub_video_set_mode ("text", 0, 0);

  return grub_relocator16_boot (rel, state);
}

static grub_err_t
grub_freedos_unload (void)
{
  grub_relocator_unload (rel);
  rel = NULL;
  grub_dl_unref (my_mod);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_freedos (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_err_t err;
  void *kernelsys;
  grub_size_t kernelsyssize;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  grub_dl_ref (my_mod);

  rel = grub_relocator_new ();
  if (!rel)
    goto fail;

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  ebx = grub_get_root_biosnumber ();

  kernelsyssize = grub_file_size (file);
  {
    grub_relocator_chunk_t ch;
    err = grub_relocator_alloc_chunk_addr (rel, &ch, GRUB_FREEDOS_SEGMENT << 4,
					   kernelsyssize);
    if (err)
      goto fail;
    kernelsys = get_virtual_current_address (ch);
  }

  if (grub_file_read (file, kernelsys, kernelsyssize)
      != (grub_ssize_t) kernelsyssize)
    goto fail;
 
  grub_loader_set (grub_freedos_boot, grub_freedos_unload, 1);
  return GRUB_ERR_NONE;

 fail:

  if (file)
    grub_file_close (file);

  grub_freedos_unload ();

  return grub_errno;
}

static grub_command_t cmd;

GRUB_MOD_INIT(freedos)
{
  cmd = grub_register_command ("freedos", grub_cmd_freedos,
			       0, N_("Load FreeDOS kernel.sys."));
  my_mod = mod;
}

GRUB_MOD_FINI(freedos)
{
  grub_unregister_command (cmd);
}
