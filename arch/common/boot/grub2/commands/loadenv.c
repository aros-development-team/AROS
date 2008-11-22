/* loadenv.c - command to load/save environment variable.  */
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

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/arg.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/partition.h>
#include <grub/lib/envblk.h>

static const struct grub_arg_option options[] =
  {
    {"file", 'f', 0, "specify filename", 0, ARG_TYPE_PATHNAME},
    {0, 0, 0, 0, 0, 0}
  };

char buffer[GRUB_ENVBLK_MAXLEN];
grub_envblk_t envblk;

static grub_file_t
read_envblk_file (char *filename, void NESTED_FUNC_ATTR read_hook (grub_disk_addr_t sector, unsigned offset, unsigned length))
{
  char *buf = 0;
  grub_file_t file;

  if (! filename)
    {
      char *prefix;

      prefix = grub_env_get ("prefix");
      if (prefix)
        {
          int len;

          len = grub_strlen (prefix);
          buf = grub_malloc (len + 1 + sizeof (GRUB_ENVBLK_DEFCFG));
          grub_strcpy (buf, prefix);
          buf[len] = '/';
          grub_strcpy (buf + len + 1, GRUB_ENVBLK_DEFCFG);
          filename = buf;
        }
      else
        {
          grub_error (GRUB_ERR_FILE_NOT_FOUND, "prefix is not found");
          return 0;
        }
    }

  file = grub_file_open (filename);
  grub_free (buf);
  if (! file)
    return 0;

  if (read_hook)
    {
      if (! file->device->disk)
        {
          grub_file_close (file);
          grub_error (GRUB_ERR_BAD_DEVICE,
                      "this command is available only for disk devices.");
          return 0;
        }
      file->read_hook = read_hook;
    }

  if (grub_file_read (file, buffer, GRUB_ENVBLK_MAXLEN) != GRUB_ENVBLK_MAXLEN)
    {
      grub_file_close (file);
      grub_error (GRUB_ERR_BAD_FILE_TYPE, "file too short");
      return 0;
    }

  envblk = grub_envblk_find (buffer);
  if (! envblk)
    {
      grub_file_close (file);
      grub_error (GRUB_ERR_BAD_FILE_TYPE, "environment block not found");
      return 0;
    }

  return file;
}

static grub_err_t
grub_cmd_load_env (struct grub_arg_list *state,
                   int argc __attribute__ ((unused)), char **args __attribute__ ((unused)))

{
  grub_file_t file;

  auto int hook (char *name, char *value);
  int hook (char *name, char *value)
    {
      grub_env_set (name, value);

      return 0;
    }

  file = read_envblk_file ((state[0].set) ? state[0].arg : 0, 0);
  if (! file)
    return grub_errno;

  grub_file_close (file);

  grub_envblk_iterate (envblk, hook);

  return grub_errno;
}

static grub_err_t
grub_cmd_list_env (struct grub_arg_list *state,
                   int argc __attribute__ ((unused)), char **args __attribute__ ((unused)))
{
  grub_file_t file;

  auto int hook (char *name, char *value);
  int hook (char *name, char *value)
    {
      grub_printf ("%s=%s\n", name, value);

      return 0;
    }

  file = read_envblk_file ((state[0].set) ? state[0].arg : 0, 0);
  if (! file)
    return grub_errno;

  grub_file_close (file);

  grub_envblk_iterate (envblk, hook);

  return grub_errno;
}

static grub_err_t
grub_cmd_save_env (struct grub_arg_list *state, int argc, char **args)
{
  grub_file_t file;
  grub_disk_t disk;
  grub_disk_addr_t addr[GRUB_ENVBLK_MAXLEN >> GRUB_DISK_SECTOR_BITS];
  char buf[GRUB_DISK_SECTOR_SIZE];
  grub_disk_addr_t part_start = 0;
  int num = 0;

  auto void NESTED_FUNC_ATTR hook (grub_disk_addr_t sector, unsigned offset,
                                   unsigned length);

  void NESTED_FUNC_ATTR hook (grub_disk_addr_t sector,
                              unsigned offset, unsigned length)
    {
      if ((offset != 0) || (length != GRUB_DISK_SECTOR_SIZE))
        return;

      if (num < (GRUB_ENVBLK_MAXLEN >> GRUB_DISK_SECTOR_BITS))
        addr[num++] = sector;
    }

  if (! argc)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "No variable is specified");

  file = read_envblk_file ((state[0].set) ? state[0].arg : 0, hook);
  if (! file)
    return grub_errno;

  file->read_hook = 0;

  if (num != GRUB_ENVBLK_MAXLEN >> GRUB_DISK_SECTOR_BITS)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "invalid blocklist");
      goto quit;
    }

  disk = file->device->disk;
  if (disk->partition)
    part_start = grub_partition_get_start (disk->partition);

  for (num = 0; num < (GRUB_ENVBLK_MAXLEN >> GRUB_DISK_SECTOR_BITS); num++)
    {
      if (grub_disk_read (disk, addr[num] - part_start, 0,
                          GRUB_DISK_SECTOR_SIZE,  buf))
        goto quit;

      if (grub_memcmp (&buffer[num << GRUB_DISK_SECTOR_BITS], buf,
                       GRUB_DISK_SECTOR_SIZE))
        {
          grub_error (GRUB_ERR_BAD_DEVICE, "invalid blocklist");
          goto quit;
        }
    }

  while (argc)
    {
      char *value;

      value = grub_env_get (args[0]);
      if (value)
        {
          if (grub_envblk_insert (envblk, args[0], value))
            {
              grub_error (GRUB_ERR_BAD_ARGUMENT, "environment block too small");
              goto quit;
            }
        }

      argc--;
      args++;
    }

  for (num = 0; num < (GRUB_ENVBLK_MAXLEN >> GRUB_DISK_SECTOR_BITS); num++)
    if (grub_disk_write (disk, addr[num] - part_start, 0,
                         GRUB_DISK_SECTOR_SIZE,
                         &buffer[num << GRUB_DISK_SECTOR_BITS]))
      goto quit;

quit:
  grub_file_close (file);

  return grub_errno;
}

GRUB_MOD_INIT(loadenv)
{
  (void) mod;
  grub_register_command ("load_env", grub_cmd_load_env, GRUB_COMMAND_FLAG_BOTH,
			 "load_env [-f FILE]", "Load variables from environment block file.", options);
  grub_register_command ("list_env", grub_cmd_list_env, GRUB_COMMAND_FLAG_BOTH,
			 "list_env [-f FILE]", "List variables from environment block file.", options);
  grub_register_command ("save_env", grub_cmd_save_env, GRUB_COMMAND_FLAG_BOTH,
			 "save_env [-f FILE] variable_name [...]", "Save variables to environment block file.", options);
}

GRUB_MOD_FINI(loadenv)
{
  grub_unregister_command ("load_env");
  grub_unregister_command ("list_env");
  grub_unregister_command ("save_env");
}
