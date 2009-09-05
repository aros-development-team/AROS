/* minicmd.c - commands for the rescue mode */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2006,2007,2009  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/term.h>
#include <grub/loader.h>
#include <grub/command.h>

/* cat FILE */
static grub_err_t
grub_mini_cmd_cat (struct grub_command *cmd __attribute__ ((unused)),
		   int argc, char *argv[])
{
  grub_file_t file;
  char buf[GRUB_DISK_SECTOR_SIZE];
  grub_ssize_t size;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no file specified");

  file = grub_file_open (argv[0]);
  if (! file)
    return grub_errno;

  while ((size = grub_file_read (file, buf, sizeof (buf))) > 0)
    {
      int i;

      for (i = 0; i < size; i++)
	{
	  unsigned char c = buf[i];

	  if ((grub_isprint (c) || grub_isspace (c)) && c != '\r')
	    grub_putchar (c);
	  else
	    {
	      grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
	      grub_printf ("<%x>", (int) c);
	      grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
	    }
	}
    }

  grub_putchar ('\n');
  grub_refresh ();
  grub_file_close (file);

  return 0;
}

/* help */
static grub_err_t
grub_mini_cmd_help (struct grub_command *cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char *argv[] __attribute__ ((unused)))
{
  grub_command_t p;

  for (p = grub_command_list; p; p = p->next)
    grub_printf ("%s (%d%c)\t%s\n", p->name,
		 p->prio & GRUB_PRIO_LIST_PRIO_MASK,
		 (p->prio & GRUB_PRIO_LIST_FLAG_ACTIVE) ? '+' : '-',
		 p->description);

  return 0;
}

#if 0
static void
grub_rescue_cmd_info (void)
{
  extern void grub_disk_cache_get_performance (unsigned long *,
					       unsigned long *);
  unsigned long hits, misses;

  grub_disk_cache_get_performance (&hits, &misses);
  grub_printf ("Disk cache: hits = %u, misses = %u ", hits, misses);
  if (hits + misses)
    {
      unsigned long ratio = hits * 10000 / (hits + misses);
      grub_printf ("(%u.%u%%)\n", ratio / 100, ratio % 100);
    }
  else
    grub_printf ("(N/A)\n");
}
#endif

/* root [DEVICE] */
static grub_err_t
grub_mini_cmd_root (struct grub_command *cmd __attribute__ ((unused)),
		    int argc, char *argv[])
{
  grub_device_t dev;
  grub_fs_t fs;

  if (argc > 0)
    {
      char *device_name = grub_file_get_device_name (argv[0]);
      if (! device_name)
	return grub_errno;

      grub_env_set ("root", device_name);
      grub_free (device_name);
    }

  dev = grub_device_open (0);
  if (! dev)
    return grub_errno;

  fs = grub_fs_probe (dev);
  if (grub_errno == GRUB_ERR_UNKNOWN_FS)
    grub_errno = GRUB_ERR_NONE;

  grub_printf ("(%s): Filesystem is %s.\n",
	       grub_env_get ("root"), fs ? fs->name : "unknown");

  grub_device_close (dev);

  return 0;
}

#if 0
static void
grub_rescue_cmd_testload (int argc, char *argv[])
{
  grub_file_t file;
  char *buf;
  grub_ssize_t size;
  grub_ssize_t pos;
  auto void read_func (unsigned long sector, unsigned offset, unsigned len);

  void read_func (unsigned long sector __attribute__ ((unused)),
		  unsigned offset __attribute__ ((unused)),
		  unsigned len __attribute__ ((unused)))
    {
      grub_putchar ('.');
      grub_refresh ();
    }

  if (argc < 1)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no file specified");
      return;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    return;

  size = grub_file_size (file) & ~(GRUB_DISK_SECTOR_SIZE - 1);
  if (size == 0)
    {
      grub_file_close (file);
      return;
    }

  buf = grub_malloc (size);
  if (! buf)
    goto fail;

  grub_printf ("Reading %s sequentially", argv[0]);
  file->read_hook = read_func;
  if (grub_file_read (file, buf, size) != size)
    goto fail;
  grub_printf (" Done.\n");

  /* Read sequentially again.  */
  grub_printf ("Reading %s sequentially again", argv[0]);
  if (grub_file_seek (file, 0) < 0)
    goto fail;

  for (pos = 0; pos < size; pos += GRUB_DISK_SECTOR_SIZE)
    {
      char sector[GRUB_DISK_SECTOR_SIZE];

      if (grub_file_read (file, sector, GRUB_DISK_SECTOR_SIZE)
	  != GRUB_DISK_SECTOR_SIZE)
	goto fail;

      if (grub_memcmp (sector, buf + pos, GRUB_DISK_SECTOR_SIZE) != 0)
	{
	  grub_printf ("\nDiffers in %d\n", pos);
	  goto fail;
	}
    }
  grub_printf (" Done.\n");

  /* Read backwards and compare.  */
  grub_printf ("Reading %s backwards", argv[0]);
  pos = size;
  while (pos > 0)
    {
      char sector[GRUB_DISK_SECTOR_SIZE];

      pos -= GRUB_DISK_SECTOR_SIZE;

      if (grub_file_seek (file, pos) < 0)
	goto fail;

      if (grub_file_read (file, sector, GRUB_DISK_SECTOR_SIZE)
	  != GRUB_DISK_SECTOR_SIZE)
	goto fail;

      if (grub_memcmp (sector, buf + pos, GRUB_DISK_SECTOR_SIZE) != 0)
	{
	  int i;

	  grub_printf ("\nDiffers in %d\n", pos);

	  for (i = 0; i < GRUB_DISK_SECTOR_SIZE; i++)
	    grub_putchar (buf[pos + i]);

	  if (i)
	    grub_refresh ();

	  goto fail;
	}
    }
  grub_printf (" Done.\n");

 fail:

  grub_file_close (file);
  grub_free (buf);
}
#endif

/* dump ADDRESS [SIZE] */
static grub_err_t
grub_mini_cmd_dump (struct grub_command *cmd __attribute__ ((unused)),
		    int argc, char *argv[])
{
  grub_uint8_t *addr;
  grub_size_t size = 4;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no address specified");

  addr = (grub_uint8_t *) grub_strtoul (argv[0], 0, 0);
  if (grub_errno)
    return grub_errno;

  if (argc > 1)
    size = (grub_size_t) grub_strtoul (argv[1], 0, 0);

  while (size--)
    {
      grub_printf ("%x%x ", *addr >> 4, *addr & 0xf);
      addr++;
    }

  return 0;
}

/* rmmod MODULE */
static grub_err_t
grub_mini_cmd_rmmod (struct grub_command *cmd __attribute__ ((unused)),
		     int argc, char *argv[])
{
  grub_dl_t mod;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");

  mod = grub_dl_get (argv[0]);
  if (! mod)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no such module");

  if (grub_dl_unref (mod) <= 0)
    grub_dl_unload (mod);

  return 0;
}

/* lsmod */
static grub_err_t
grub_mini_cmd_lsmod (struct grub_command *cmd __attribute__ ((unused)),
		     int argc __attribute__ ((unused)),
		     char *argv[] __attribute__ ((unused)))
{
  auto int print_module (grub_dl_t mod);

  int print_module (grub_dl_t mod)
    {
      grub_dl_dep_t dep;

      grub_printf ("%s\t%d\t\t", mod->name, mod->ref_count);
      for (dep = mod->dep; dep; dep = dep->next)
	{
	  if (dep != mod->dep)
	    grub_putchar (',');

	  grub_printf ("%s", dep->mod->name);
	}
      grub_putchar ('\n');
      grub_refresh ();

      return 0;
    }

  grub_printf ("Name\tRef Count\tDependencies\n");
  grub_dl_iterate (print_module);

  return 0;
}

/* exit */
static grub_err_t
grub_mini_cmd_exit (struct grub_command *cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char *argv[] __attribute__ ((unused)))
{
  grub_exit ();
  return 0;
}

static grub_command_t cmd_cat, cmd_help, cmd_root;
static grub_command_t cmd_dump, cmd_rmmod, cmd_lsmod, cmd_exit;

GRUB_MOD_INIT(minicmd)
{
  cmd_cat =
    grub_register_command ("cat", grub_mini_cmd_cat,
			   "cat FILE", "show the contents of a file");
  cmd_help =
    grub_register_command ("help", grub_mini_cmd_help,
			   0, "show this message");
  cmd_root =
    grub_register_command ("root", grub_mini_cmd_root,
			   "root [DEVICE]", "set the root device");
  cmd_dump =
    grub_register_command ("dump", grub_mini_cmd_dump,
			   "dump ADDR", "dump memory");
  cmd_rmmod =
    grub_register_command ("rmmod", grub_mini_cmd_rmmod,
			   "rmmod MODULE", "remove a module");
  cmd_lsmod =
    grub_register_command ("lsmod", grub_mini_cmd_lsmod,
			   0, "show loaded modules");
  cmd_exit =
    grub_register_command ("exit", grub_mini_cmd_exit,
			   0, "exit from GRUB");
}

GRUB_MOD_FINI(minicmd)
{
  grub_unregister_command (cmd_cat);
  grub_unregister_command (cmd_help);
  grub_unregister_command (cmd_root);
  grub_unregister_command (cmd_dump);
  grub_unregister_command (cmd_rmmod);
  grub_unregister_command (cmd_lsmod);
  grub_unregister_command (cmd_exit);
}
