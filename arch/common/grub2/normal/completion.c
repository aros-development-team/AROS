/* completion.c - complete a command, a disk, a partition or a file */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/misc.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/disk.h>
#include <grub/file.h>

/* The current word.  */
static char *current_word;

/* The matched string.  */
static char *match;

/* The count of candidates.  */
static int num_found;

/* The string to be appended.  */
static const char *suffix;

/* The callback function to print items.  */
static void (*print_func) (const char *, grub_completion_type_t, int);



/* Add a string to the list of possible completions. COMPLETION is the
   string that should be added. EXTRA will be appended if COMPLETION
   matches uniquely. The type TYPE specifies what kind of data is added.  */
static int
add_completion (const char *completion, const char *extra,
		grub_completion_type_t type)
{
  if (grub_strncmp (current_word, completion, grub_strlen (current_word)) == 0)
    {
      num_found++;

      switch (num_found)
	{
	case 1:
	  match = grub_strdup (completion);
	  if (! match)
	    return 1;
	  suffix = extra;
	  break;

	case 2:
	  if (print_func)
	    print_func (match, type, 0);
	  
	  /* Fall through.  */

	default:
	  {
	    char *s = match;
	    const char *t = completion;

	    if (print_func)
	      print_func (completion, type, num_found - 1);
			    
	    /* Detect the matched portion.  */
	    while (*s && *t && *s == *t)
	      {
		s++;
		t++;
	      }

	    *s = '\0';
	  }
	  break;
	}
    }
      
  return 0;
}

static int
iterate_partition (grub_disk_t disk, const grub_partition_t p)
{
  const char *disk_name = disk->name;
  char *partition_name = grub_partition_get_name (p);
  char *name;
  int ret;
  
  if (! partition_name)
    return 1;

  name = grub_malloc (grub_strlen (disk_name) + 1
		      + grub_strlen (partition_name) + 1);
  if (! name)
    {
      grub_free (partition_name);
      return 1;
    }

  grub_sprintf (name, "%s,%s", disk_name, partition_name);
  grub_free (partition_name);
  
  ret = add_completion (name, ")", GRUB_COMPLETION_TYPE_PARTITION);
  grub_free (name);
  return ret;
}

static int
iterate_dir (const char *filename, int dir)
{
  if (! dir)
    {
      if (add_completion (filename, " ", GRUB_COMPLETION_TYPE_FILE))
	return 1;
    }
  else
    {
      char fname[grub_strlen (filename) + 2];

      grub_sprintf (fname, "%s/", filename);
      if (add_completion (fname, "", GRUB_COMPLETION_TYPE_FILE))
	return 1;
    }
  
  return 0;
}

static int
iterate_dev (const char *devname)
{
  grub_device_t dev;
  
  /* Complete the partition part.  */
  dev = grub_device_open (devname);
  
  if (dev)
    {
      if (dev->disk && dev->disk->has_partitions)
	{
	  if (add_completion (devname, ",", GRUB_COMPLETION_TYPE_DEVICE))
	    return 1;
	}
      else
	{
	  if (add_completion (devname, ")", GRUB_COMPLETION_TYPE_DEVICE))
	    return 1;
	}
    }
  
  grub_errno = GRUB_ERR_NONE;
  return 0;
}

static int
iterate_command (grub_command_t cmd)
{
  if (grub_command_find (cmd->name))
    {
      if (cmd->flags & GRUB_COMMAND_FLAG_CMDLINE)
	{
	  if (add_completion (cmd->name, " ", GRUB_COMPLETION_TYPE_COMMAND))
	    return 1;
	}
    }
  
  return 0;
}

/* Complete a device.  */
static int
complete_device (void)
{
  /* Check if this is a device or a partition.  */
  char *p = grub_strchr (++current_word, ',');
  grub_device_t dev;
  
  if (! p)
    {
      /* Complete the disk part.  */
      if (grub_disk_dev_iterate (iterate_dev))
	return 1;
    }
  else
    {
      /* Complete the partition part.  */
      *p = '\0';
      dev = grub_device_open (current_word);
      *p = ',';
      grub_errno = GRUB_ERR_NONE;
      
      if (dev)
	{
	  if (dev->disk && dev->disk->has_partitions)
	    {
	      if (grub_partition_iterate (dev->disk, iterate_partition))
		{
		  grub_device_close (dev);
		  return 1;
		}
	    }
	  
	  grub_device_close (dev);
	}
      else
	return 1;
    }

  return 0;
}

/* Complete a file.  */
static int
complete_file (void)
{
  char *device;
  char *dir;
  char *last_dir;
  grub_fs_t fs;
  grub_device_t dev;
  int ret = 0;
  
  device = grub_file_get_device_name (current_word);
  if (grub_errno != GRUB_ERR_NONE)
    return 1;
  
  dev = grub_device_open (device);
  if (! dev)
    {
      ret = 1;
      goto fail;
    }
  
  fs = grub_fs_probe (dev);
  if (! fs)
    {
      ret = 1;
      goto fail;
    }

  dir = grub_strchr (current_word, '/');
  last_dir = grub_strrchr (current_word, '/');
  if (dir)
    {
      char *dirfile;
      
      current_word = last_dir + 1;
      
      dir = grub_strdup (dir);
      if (! dir)
	{
	  ret = 1;
	  goto fail;
	}
      
      /* Cut away the filename part.  */
      dirfile = grub_strrchr (dir, '/');
      dirfile[1] = '\0';
      
      /* Iterate the directory.  */
      (fs->dir) (dev, dir, iterate_dir);
      
      grub_free (dir);
      
      if (grub_errno)
	{
	  ret = 1;
	  goto fail;
	}
    }
  else
    {
      current_word += grub_strlen (current_word);
      match = grub_strdup ("/");
      if (! match)
	{
	  ret = 1;
	  goto fail;
	}
      
      suffix = "";
      num_found = 1;
    }

 fail:
  if (dev)
    grub_device_close (dev);
  grub_free (device);
  return ret;
}

/* Complete an argument.  */
static int
complete_arguments (char *command)
{
  grub_command_t cmd;
  const struct grub_arg_option *option;
  char shortarg[] = "- ";

  cmd = grub_command_find (command); 

  if (!cmd || !cmd->options)
    return 0;

  if (add_completion ("-u", " ", GRUB_COMPLETION_TYPE_ARGUMENT))
    return 1;

  /* Add the short arguments.  */
  for (option = cmd->options; option->doc; option++)
    {
      if (! option->shortarg)
	continue;

      shortarg[1] = option->shortarg;
      if (add_completion (shortarg, " ", GRUB_COMPLETION_TYPE_ARGUMENT))
	return 1;

    }

  /* First add the built-in arguments.  */
  if (add_completion ("--help", " ", GRUB_COMPLETION_TYPE_ARGUMENT))
    return 1;
  if (add_completion ("--usage", " ", GRUB_COMPLETION_TYPE_ARGUMENT))
    return 1;

  /* Add the long arguments.  */
  for (option = cmd->options; option->doc; option++)
    {
      char *longarg;
      if (!option->longarg)
	continue;

      longarg = grub_malloc (grub_strlen (option->longarg));
      grub_sprintf (longarg, "--%s", option->longarg);

      if (add_completion (longarg, " ", GRUB_COMPLETION_TYPE_ARGUMENT))
	{
	  grub_free (longarg);
	  return 1;
	}
      grub_free (longarg);
    }

  return 0;
}

/* Try to complete the string in BUF. Return the characters that
   should be added to the string.  This command outputs the possible
   completions by calling HOOK, in that case set RESTORE to 1 so the
   caller can restore the prompt.  */
char *
grub_normal_do_completion (char *buf, int *restore,
			   void (*hook) (const char *, grub_completion_type_t, int))
{
  char *first_word;

  /* Initialize variables.  */
  match = 0;
  num_found = 0;
  suffix = "";
  print_func = hook;

  *restore = 1;
  
  /* Find the first word.  */
  for (first_word = buf; *first_word == ' '; first_word++)
    ;

  /* Find the delimeter of the current word.  */
  for (current_word = first_word + grub_strlen (first_word);
       current_word > first_word;
       current_word--)
    if (*current_word == ' ' || *current_word == '=')
      break;
  
  if (current_word == first_word)
    {
      /* Complete a command.  */
      if (grub_iterate_commands (iterate_command))
	goto fail;
    }
  else
    {
      current_word++;
      
      if (*current_word == '-')
	{
	  if (complete_arguments (buf))
	    goto fail;
	}
      else if (*current_word == '(' && ! grub_strchr (current_word, ')'))
	{
	  /* Complete a device.  */
	  if (complete_device ())
	    goto fail;
	}
      else
	{
	  /* Complete a file.  */
	  if (complete_file ())
	    goto fail;
	}
    }

  /* If more than one match is found those matches will be printed and
     the prompt should be restored.  */
  if (num_found > 1)
    *restore = 1;
  else
    *restore = 0;

  /* Return the part that matches.  */
  if (match)
    {
      char *ret;
      int current_len;
      int match_len;

      current_len = grub_strlen (current_word);
      match_len = grub_strlen (match);
      ret = grub_malloc (match_len - current_len + grub_strlen (suffix) + 1);
      grub_strcpy (ret, match + current_len);
      if (num_found == 1)
	grub_strcat (ret, suffix);
      
      grub_free (match);

      if (*ret == '\0')
	{
	  grub_free (ret);
	  return 0;
	}
      
      return ret;
    }

 fail:
  grub_free (match);
  grub_errno = GRUB_ERR_NONE;

  return 0;
}
