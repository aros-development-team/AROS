/* rescue.c - rescue mode */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002, 2003  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/kernel.h>
#include <grub/rescue.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/loader.h>
#include <grub/dl.h>
#include <grub/partition.h>
#include <grub/env.h>

#define GRUB_RESCUE_BUF_SIZE	256
#define GRUB_RESCUE_MAX_ARGS	20

struct grub_rescue_command
{
  const char *name;
  void (*func) (int argc, char *argv[]);
  const char *message;
  struct grub_rescue_command *next;
};
typedef struct grub_rescue_command *grub_rescue_command_t;

static char linebuf[GRUB_RESCUE_BUF_SIZE];

static grub_rescue_command_t grub_rescue_command_list;

void
grub_rescue_register_command (const char *name,
			      void (*func) (int argc, char *argv[]),
			      const char *message)
{
  grub_rescue_command_t cmd;

  cmd = (grub_rescue_command_t) grub_malloc (sizeof (*cmd));
  if (! cmd)
    return;

  cmd->name = name;
  cmd->func = func;
  cmd->message = message;

  cmd->next = grub_rescue_command_list;
  grub_rescue_command_list = cmd;
}

void
grub_rescue_unregister_command (const char *name)
{
  grub_rescue_command_t *p, q;

  for (p = &grub_rescue_command_list, q = *p; q; p = &(q->next), q = q->next)
    if (grub_strcmp (name, q->name) == 0)
      {
	*p = q->next;
	grub_free (q);
	break;
      }
}

/* Prompt to input a command and read the line.  */
static void
grub_rescue_get_command_line (const char *prompt)
{
  int c;
  int pos = 0;
  
  grub_printf (prompt);
  grub_memset (linebuf, 0, GRUB_RESCUE_BUF_SIZE);
  
  while ((c = GRUB_TERM_ASCII_CHAR (grub_getkey ())) != '\n' && c != '\r')
    {
      if (grub_isprint (c))
	{
	  if (pos < GRUB_RESCUE_BUF_SIZE - 1)
	    {
	      linebuf[pos++] = c;
	      grub_putchar (c);
	    }
	}
      else if (c == '\b')
	{
	  if (pos > 0)
	    {
	      linebuf[--pos] = 0;
	      grub_putchar (c);
	      grub_putchar (' ');
	      grub_putchar (c);
	    }
	}
      grub_refresh ();
    }

  grub_putchar ('\n');
  grub_refresh ();
}

/* boot */
static void
grub_rescue_cmd_boot (int argc __attribute__ ((unused)),
		      char *argv[] __attribute__ ((unused)))
{
  grub_loader_boot ();
}

/* cat FILE */
static void
grub_rescue_cmd_cat (int argc, char *argv[])
{
  grub_file_t file;
  char buf[GRUB_DISK_SECTOR_SIZE];
  grub_ssize_t size;

  if (argc < 1)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no file specified");
      return;
    }
  
  file = grub_file_open (argv[0]);
  if (! file)
    return;

  while ((size = grub_file_read (file, buf, sizeof (buf))) > 0)
    {
      int i;

      for (i = 0; i < size; i++)
	{
	  unsigned char c = buf[i];

	  if (grub_isprint (c) || grub_isspace (c))
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
}

static int
grub_rescue_print_devices (const char *name)
{
  grub_printf ("(%s) ", name);
  
  return 0;
}

static int
grub_rescue_print_files (const char *filename, int dir)
{
  grub_printf ("%s%s ", filename, dir ? "/" : "");
  
  return 0;
}

/* ls [ARG] */
static void
grub_rescue_cmd_ls (int argc, char *argv[])
{
  if (argc < 1)
    {
      grub_device_iterate (grub_rescue_print_devices);
      grub_putchar ('\n');
      grub_refresh ();
    }
  else
    {
      char *device_name;
      grub_device_t dev;
      grub_fs_t fs;
      char *path;
      
      device_name = grub_file_get_device_name (argv[0]);
      dev = grub_device_open (device_name);
      if (! dev)
	goto fail;

      fs = grub_fs_probe (dev);
      path = grub_strchr (argv[0], ')');
      if (! path)
	path = argv[0];
      else
	path++;
      
      if (! path && ! device_name)
	{
	  grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid argument");
	  goto fail;
	}
      
      if (! path)
	{
	  if (grub_errno == GRUB_ERR_UNKNOWN_FS)
	    grub_errno = GRUB_ERR_NONE;
	  
	  grub_printf ("(%s): Filesystem is %s.\n",
		       device_name, fs ? fs->name : "unknown");
	}
      else if (fs)
	{
	  (fs->dir) (dev, path, grub_rescue_print_files);
	  grub_putchar ('\n');
	  grub_refresh ();
	}

    fail:
      if (dev)
	grub_device_close (dev);
      
      grub_free (device_name);
    }
}

/* help */
static void
grub_rescue_cmd_help (int argc __attribute__ ((unused)),
		      char *argv[] __attribute__ ((unused)))
{
  grub_rescue_command_t p, q;

  /* Sort the commands. This is not a good algorithm, but this is enough,
     because rescue mode has a small number of commands.  */
  for (p = grub_rescue_command_list; p; p = p->next)
    for (q = p->next; q; q = q->next)
      if (grub_strcmp (p->name, q->name) > 0)
	{
	  struct grub_rescue_command tmp;

	  tmp.name = p->name;
	  tmp.func = p->func;
	  tmp.message = p->message;

	  p->name = q->name;
	  p->func = q->func;
	  p->message = q->message;

	  q->name = tmp.name;
	  q->func = tmp.func;
	  q->message = tmp.message;
	}

  /* Print them.  */
  for (p = grub_rescue_command_list; p; p = p->next)
    grub_printf ("%s\t%s\n", p->name, p->message);
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
static void
grub_rescue_cmd_root (int argc, char *argv[])
{
  grub_device_t dev;
  grub_fs_t fs;

  if (argc > 0)
    {
      char *device_name = grub_file_get_device_name (argv[0]);
      if (! device_name)
	return;

      grub_env_set ("root", device_name);
      grub_free (device_name);
    }
  
  dev = grub_device_open (0);
  if (! dev)
    return;

  fs = grub_fs_probe (dev);
  if (grub_errno == GRUB_ERR_UNKNOWN_FS)
    grub_errno = GRUB_ERR_NONE;
  
  grub_printf ("(%s): Filesystem is %s.\n",
	       grub_env_get ("root"), fs ? fs->name : "unknown");
  
  grub_device_close (dev);
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
static void
grub_rescue_cmd_dump (int argc, char *argv[])
{
  grub_uint8_t *addr;
  grub_size_t size = 4;
  
  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no address specified");
      return;
    }

  addr = (grub_uint8_t *) grub_strtoul (argv[0], 0, 0);
  if (grub_errno)
    return;

  if (argc > 1)
    size = (grub_size_t) grub_strtoul (argv[1], 0, 0);

  while (size--)
    {
      grub_printf ("%x%x ", *addr >> 4, *addr & 0xf);
      addr++;
    }
}

/* insmod MODULE */
static void
grub_rescue_cmd_insmod (int argc, char *argv[])
{
  char *p;
  grub_dl_t mod;
  
  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");
      return;
    }

  p = grub_strchr (argv[0], '/');
  if (! p)
    mod = grub_dl_load (argv[0]);
  else
    mod = grub_dl_load_file (argv[0]);

  if (mod)
    grub_dl_ref (mod);
}

/* rmmod MODULE */
static void
grub_rescue_cmd_rmmod (int argc, char *argv[])
{
  grub_dl_t mod;
  
  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");
      return;
    }

  mod = grub_dl_get (argv[0]);
  if (! mod)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no such module");
      return;
    }

  if (grub_dl_unref (mod) <= 0)
    grub_dl_unload (mod);
}

/* lsmod */
static void
grub_rescue_cmd_lsmod (int argc __attribute__ ((unused)),
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
}

/* set ENVVAR=VALUE */
static void
grub_rescue_cmd_set (int argc, char *argv[])
{
  char *var;
  char *val;

  auto int print_env (struct grub_env_var *env);

  int print_env (struct grub_env_var *env)
    {
      grub_printf ("%s=%s\n", env->name, env->value);
      return 0;
    }

  if (argc < 1)
    {
      grub_env_iterate (print_env);
      return;
    }

  var = argv[0];
  val = grub_strchr (var, '=');
  if (! val)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "not an assignment");
      return;
    }

  val[0] = 0;
  grub_env_set (var, val + 1);
  val[0] = '=';
}

static void
grub_rescue_cmd_unset (int argc, char *argv[])
{
  if (argc < 1)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no environment variable specified");
      return;
    }

  grub_env_unset (argv[0]);
}

static void
attempt_normal_mode (void)
{
  grub_rescue_command_t cmd;

  for (cmd = grub_rescue_command_list; cmd; cmd = cmd->next)
    {
      if (grub_strcmp ("normal", cmd->name) == 0)
	{
	  (cmd->func) (0, 0);
	  break;
	}
    }
}

/* Enter the rescue mode.  */
void
grub_enter_rescue_mode (void)
{
  auto grub_err_t getline (char **line);
  
  grub_err_t getline (char **line)
    {
      grub_rescue_get_command_line ("> ");
      *line = linebuf;
      return 0;
    }

  /* First of all, attempt to execute the normal mode.  */
  attempt_normal_mode ();

  grub_printf ("Entering into rescue mode...\n");
  
  grub_rescue_register_command ("boot", grub_rescue_cmd_boot,
				"boot an operating system");
  grub_rescue_register_command ("cat", grub_rescue_cmd_cat,
				"show the contents of a file");
  grub_rescue_register_command ("help", grub_rescue_cmd_help,
				"show this message");
  grub_rescue_register_command ("ls", grub_rescue_cmd_ls,
				"list devices or files");
  grub_rescue_register_command ("root", grub_rescue_cmd_root,
				"set the root device");
  grub_rescue_register_command ("dump", grub_rescue_cmd_dump,
				"dump memory");
  grub_rescue_register_command ("insmod", grub_rescue_cmd_insmod,
				"insert a module");
  grub_rescue_register_command ("rmmod", grub_rescue_cmd_rmmod,
				"remove a module");
  grub_rescue_register_command ("lsmod", grub_rescue_cmd_lsmod,
				"show loaded modules");
  grub_rescue_register_command ("set", grub_rescue_cmd_set,
				"set an environment variable");
  grub_rescue_register_command ("unset", grub_rescue_cmd_unset,
				"remove an environment variable");
  
  while (1)
    {
      char *line = linebuf;
      char *name;
      int n;
      grub_rescue_command_t cmd;
      char **args;

      /* Print an error, if any.  */
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      /* Get a command line.  */
      grub_rescue_get_command_line ("grub rescue> ");

      if (grub_split_cmdline (line, getline, &n, &args) || n < 0)
	continue;

      /* In case of an assignment set the environment accordingly
	 instead of calling a function.  */
      if (n == 0 && grub_strchr (line, '='))
	{
	  char *val = grub_strchr (args[0], '=');
	  val[0] = 0;
	  grub_env_set (args[0], val + 1);
	  val[0] = '=';
	  continue;
	}

      /* Get the command name.  */
      name = args[0];

      /* If nothing is specified, restart.  */
      if (*name == '\0')
	continue;

      /* Find the command and execute it.  */
      for (cmd = grub_rescue_command_list; cmd; cmd = cmd->next)
	{
	  if (grub_strcmp (name, cmd->name) == 0)
	    {
	      (cmd->func) (n, &args[1]);
	      break;
	    }
	}
      
      /* If not found, print an error message.  */
      if (! cmd)
	{
	  grub_printf ("Unknown command `%s'\n", name);
	  grub_printf ("Try `help' for usage\n");
	}
    }
}
