/* getroot.c - Get root device */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <config.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <grub/util/misc.h>

#ifdef __GNU__
#include <hurd.h>
#include <hurd/lookup.h>
#include <hurd/fs.h>
#include <sys/mman.h>
#endif

#ifdef __linux__
# include <sys/types.h>
# include <sys/wait.h>
#endif

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
# include <grub/util/libzfs.h>
# include <grub/util/libnvpair.h>
#endif

#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/emu/misc.h>
#include <grub/emu/hostdisk.h>
#include <grub/emu/getroot.h>

static void
strip_extra_slashes (char *dir)
{
  char *p = dir;

  while ((p = strchr (p, '/')) != 0)
    {
      if (p[1] == '/')
	{
	  memmove (p, p + 1, strlen (p));
	  continue;
	}
      else if (p[1] == '\0')
	{
	  if (p > dir)
	    p[0] = '\0';
	  break;
	}

      p++;
    }
}

static char *
xgetcwd (void)
{
  size_t size = 10;
  char *path;

  path = xmalloc (size);
  while (! getcwd (path, size))
    {
      size <<= 1;
      path = xrealloc (path, size);
    }

  return path;
}

#ifdef __linux__

/* Statting something on a btrfs filesystem always returns a virtual device
   major/minor pair rather than the real underlying device, because btrfs
   can span multiple underlying devices (and even if it's currently only
   using a single device it can be dynamically extended onto another).  We
   can't deal with the multiple-device case yet, but in the meantime, we can
   at least cope with the single-device case by scanning
   /proc/self/mountinfo.  */
static char *
find_root_device_from_mountinfo (const char *dir)
{
  FILE *fp;
  char *buf = NULL;
  size_t len = 0;
  char *ret = NULL;

  fp = fopen ("/proc/self/mountinfo", "r");
  if (! fp)
    return NULL; /* fall through to other methods */

  while (getline (&buf, &len, fp) > 0)
    {
      int mnt_id, parent_mnt_id;
      unsigned int major, minor;
      char enc_root[PATH_MAX], enc_path[PATH_MAX];
      int count;
      size_t enc_path_len;
      const char *sep;
      char fstype[PATH_MAX], device[PATH_MAX];
      struct stat st;

      if (sscanf (buf, "%d %d %u:%u %s %s%n",
		  &mnt_id, &parent_mnt_id, &major, &minor, enc_root, enc_path,
		  &count) < 6)
	continue;

      if (strcmp (enc_root, "/") != 0)
	continue; /* only a subtree is mounted */

      enc_path_len = strlen (enc_path);
      /* Check that enc_path is a prefix of dir.  The prefix must either be
         the entire string, or end with a slash, or be immediately followed
         by a slash.  */
      if (strncmp (dir, enc_path, enc_path_len) != 0 ||
	  (enc_path_len && dir[enc_path_len - 1] != '/' &&
	   dir[enc_path_len] && dir[enc_path_len] != '/'))
	continue;

      /* This is a parent of the requested directory.  /proc/self/mountinfo
	 is in mount order, so it must be the closest parent we've
	 encountered so far.  If it's virtual, return its device node;
	 otherwise, carry on to try to find something closer.  */

      free (ret);
      ret = NULL;

      if (major != 0)
	continue; /* not a virtual device */

      sep = strstr (buf + count, " - ");
      if (!sep)
	continue;

      sep += sizeof (" - ") - 1;
      if (sscanf (sep, "%s %s", fstype, device) != 2)
	continue;

      if (stat (device, &st) < 0)
	continue;

      if (!S_ISBLK (st.st_mode))
	continue; /* not a block device */

      ret = strdup (device);
    }

  free (buf);
  fclose (fp);
  return ret;
}

#endif /* __linux__ */

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
static char *
find_root_device_from_libzfs (const char *dir)
{
  char *device;
  char *poolname;
  char *poolfs;

  grub_find_zpool_from_dir (dir, &poolname, &poolfs);
  if (! poolname)
    return NULL;

  {
    zpool_handle_t *zpool;
    libzfs_handle_t *libzfs;
    nvlist_t *config, *vdev_tree;
    nvlist_t **children, **path;
    unsigned int nvlist_count;
    unsigned int i;

    libzfs = grub_get_libzfs_handle ();
    if (! libzfs)
      return NULL;

    zpool = zpool_open (libzfs, poolname);
    config = zpool_get_config (zpool, NULL);

    if (nvlist_lookup_nvlist (config, "vdev_tree", &vdev_tree) != 0)
      error (1, errno, "nvlist_lookup_nvlist (\"vdev_tree\")");

    if (nvlist_lookup_nvlist_array (vdev_tree, "children", &children, &nvlist_count) != 0)
      error (1, errno, "nvlist_lookup_nvlist_array (\"children\")");
    assert (nvlist_count > 0);

    while (nvlist_lookup_nvlist_array (children[0], "children",
				       &children, &nvlist_count) == 0)
      assert (nvlist_count > 0);

    for (i = 0; i < nvlist_count; i++)
      {
	if (nvlist_lookup_string (children[i], "path", &device) != 0)
	  error (1, errno, "nvlist_lookup_string (\"path\")");

	struct stat st;
	if (stat (device, &st) == 0)
	  break;

	device = NULL;
      }

    zpool_close (zpool);
  }

  free (poolname);
  if (poolfs)
    free (poolfs);

  return device;
}
#endif

#ifdef __MINGW32__

char *
grub_find_device (const char *dir __attribute__ ((unused)),
                  dev_t dev __attribute__ ((unused)))
{
  return 0;
}

#elif ! defined(__CYGWIN__)

char *
grub_find_device (const char *dir, dev_t dev)
{
  DIR *dp;
  char *saved_cwd;
  struct dirent *ent;

  if (! dir)
    {
#ifdef __CYGWIN__
      return NULL;
#else
      dir = "/dev";
#endif
    }

  dp = opendir (dir);
  if (! dp)
    return 0;

  saved_cwd = xgetcwd ();

  grub_util_info ("changing current directory to %s", dir);
  if (chdir (dir) < 0)
    {
      free (saved_cwd);
      closedir (dp);
      return 0;
    }

  while ((ent = readdir (dp)) != 0)
    {
      struct stat st;

      /* Avoid:
	 - dotfiles (like "/dev/.tmp.md0") since they could be duplicates.
	 - dotdirs (like "/dev/.static") since they could contain duplicates.  */
      if (ent->d_name[0] == '.')
	continue;

      if (lstat (ent->d_name, &st) < 0)
	/* Ignore any error.  */
	continue;

      if (S_ISLNK (st.st_mode)) {
#ifdef __linux__
	if (strcmp (dir, "mapper") == 0) {
	  /* Follow symbolic links under /dev/mapper/; the canonical name
	     may be something like /dev/dm-0, but the names under
	     /dev/mapper/ are more human-readable and so we prefer them if
	     we can get them.  */
	  if (stat (ent->d_name, &st) < 0)
	    continue;
	} else
#endif /* __linux__ */
	/* Don't follow other symbolic links.  */
	continue;
      }

      if (S_ISDIR (st.st_mode))
	{
	  /* Find it recursively.  */
	  char *res;

	  res = grub_find_device (ent->d_name, dev);

	  if (res)
	    {
	      if (chdir (saved_cwd) < 0)
		grub_util_error ("cannot restore the original directory");

	      free (saved_cwd);
	      closedir (dp);
	      return res;
	    }
	}

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__)
      if (S_ISCHR (st.st_mode) && st.st_rdev == dev)
#else
      if (S_ISBLK (st.st_mode) && st.st_rdev == dev)
#endif
	{
#ifdef __linux__
	  /* Skip device names like /dev/dm-0, which are short-hand aliases
	     to more descriptive device names, e.g. those under /dev/mapper */
	  if (ent->d_name[0] == 'd' &&
	      ent->d_name[1] == 'm' &&
	      ent->d_name[2] == '-' &&
	      ent->d_name[3] >= '0' &&
	      ent->d_name[3] <= '9')
	    continue;
#endif

	  /* Found!  */
	  char *res;
	  char *cwd;
#if defined(__NetBSD__)
	  /* Convert this block device to its character (raw) device.  */
	  const char *template = "%s/r%s";
#else
	  /* Keep the device name as it is.  */
	  const char *template = "%s/%s";
#endif

	  cwd = xgetcwd ();
	  res = xmalloc (strlen (cwd) + strlen (ent->d_name) + 3);
	  sprintf (res, template, cwd, ent->d_name);
	  strip_extra_slashes (res);
	  free (cwd);

	  /* /dev/root is not a real block device keep looking, takes care
	     of situation where root filesystem is on the same partition as
	     grub files */

	  if (strcmp(res, "/dev/root") == 0)
		continue;

	  if (chdir (saved_cwd) < 0)
	    grub_util_error ("cannot restore the original directory");

	  free (saved_cwd);
	  closedir (dp);
	  return res;
	}
    }

  if (chdir (saved_cwd) < 0)
    grub_util_error ("cannot restore the original directory");

  free (saved_cwd);
  closedir (dp);
  return 0;
}

#else /* __CYGWIN__ */

/* Read drive/partition serial number from mbr/boot sector,
   return 0 on read error, ~0 on unknown serial.  */
static unsigned
get_bootsec_serial (const char *os_dev, int mbr)
{
  /* Read boot sector.  */
  int fd = open (os_dev, O_RDONLY);
  if (fd < 0)
    return 0;
  unsigned char buf[0x200];
  int n = read (fd, buf, sizeof (buf));
  close (fd);
  if (n != sizeof(buf))
    return 0;

  /* Check signature.  */
  if (!(buf[0x1fe] == 0x55 && buf[0x1ff] == 0xaa))
    return ~0;

  /* Serial number offset depends on boot sector type.  */
  if (mbr)
    n = 0x1b8;
  else if (memcmp (buf + 0x03, "NTFS", 4) == 0)
    n = 0x048;
  else if (memcmp (buf + 0x52, "FAT32", 5) == 0)
    n = 0x043;
  else if (memcmp (buf + 0x36, "FAT", 3) == 0)
    n = 0x027;
  else
    return ~0;

  unsigned serial = *(unsigned *)(buf + n);
  if (serial == 0)
    return ~0;
  return serial;
}

char *
grub_find_device (const char *path, dev_t dev)
{
  /* No root device for /cygdrive.  */
  if (dev == (DEV_CYGDRIVE_MAJOR << 16))
    return 0;

  /* Convert to full POSIX and Win32 path.  */
  char fullpath[PATH_MAX], winpath[PATH_MAX];
  cygwin_conv_to_full_posix_path (path, fullpath);
  cygwin_conv_to_full_win32_path (fullpath, winpath);

  /* If identical, this is no real filesystem path.  */
  if (strcmp (fullpath, winpath) == 0)
    return 0;

  /* Check for floppy drive letter.  */
  if (winpath[0] && winpath[1] == ':' && strchr ("AaBb", winpath[0]))
    return xstrdup (strchr ("Aa", winpath[0]) ? "/dev/fd0" : "/dev/fd1");

  /* Cygwin returns the partition serial number in stat.st_dev.
     This is never identical to the device number of the emulated
     /dev/sdXN device, so above grub_find_device () does not work.
     Search the partition with the same serial in boot sector instead.  */
  char devpath[sizeof ("/dev/sda15") + 13]; /* Size + Paranoia.  */
  int d;
  for (d = 'a'; d <= 'z'; d++)
    {
      sprintf (devpath, "/dev/sd%c", d);
      if (get_bootsec_serial (devpath, 1) == 0)
	continue;
      int p;
      for (p = 1; p <= 15; p++)
	{
	  sprintf (devpath, "/dev/sd%c%d", d, p);
	  unsigned ser = get_bootsec_serial (devpath, 0);
	  if (ser == 0)
	    break;
	  if (ser != (unsigned)~0 && dev == (dev_t)ser)
	    return xstrdup (devpath);
	}
    }
  return 0;
}

#endif /* __CYGWIN__ */

char *
grub_guess_root_device (const char *dir)
{
  char *os_dev;
#ifdef __GNU__
  file_t file;
  mach_port_t *ports;
  int *ints;
  loff_t *offsets;
  char *data;
  error_t err;
  mach_msg_type_number_t num_ports = 0, num_ints = 0, num_offsets = 0, data_len = 0;
  size_t name_len;

  file = file_name_lookup (dir, 0, 0);
  if (file == MACH_PORT_NULL)
    return 0;

  err = file_get_storage_info (file,
			       &ports, &num_ports,
			       &ints, &num_ints,
			       &offsets, &num_offsets,
			       &data, &data_len);

  if (num_ints < 1)
    grub_util_error ("Storage info for `%s' does not include type", dir);
  if (ints[0] != STORAGE_DEVICE)
    grub_util_error ("Filesystem of `%s' is not stored on local disk", dir);

  if (num_ints < 5)
    grub_util_error ("Storage info for `%s' does not include name", dir);
  name_len = ints[4];
  if (name_len < data_len)
    grub_util_error ("Bogus name length for storage info for `%s'", dir);
  if (data[name_len - 1] != '\0')
    grub_util_error ("Storage name for `%s' not NUL-terminated", dir);

  os_dev = xmalloc (strlen ("/dev/") + data_len);
  memcpy (os_dev, "/dev/", strlen ("/dev/"));
  memcpy (os_dev + strlen ("/dev/"), data, data_len);

  if (ports && num_ports > 0)
    {
      mach_msg_type_number_t i;
      for (i = 0; i < num_ports; i++)
        {
	  mach_port_t port = ports[i];
	  if (port != MACH_PORT_NULL)
	    mach_port_deallocate (mach_task_self(), port);
        }
      munmap ((caddr_t) ports, num_ports * sizeof (*ports));
    }

  if (ints && num_ints > 0)
    munmap ((caddr_t) ints, num_ints * sizeof (*ints));
  if (offsets && num_offsets > 0)
    munmap ((caddr_t) offsets, num_offsets * sizeof (*offsets));
  if (data && data_len > 0)
    munmap (data, data_len);
  mach_port_deallocate (mach_task_self (), file);
#else /* !__GNU__ */
  struct stat st;

#ifdef __linux__
  os_dev = find_root_device_from_mountinfo (dir);
  if (os_dev)
    return os_dev;
#endif /* __linux__ */

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
  os_dev = find_root_device_from_libzfs (dir);
  if (os_dev)
    return os_dev;
#endif

  if (stat (dir, &st) < 0)
    grub_util_error ("cannot stat `%s'", dir);

#ifdef __CYGWIN__
  /* Cygwin specific function.  */
  os_dev = grub_find_device (dir, st.st_dev);

#else

  /* This might be truly slow, but is there any better way?  */
  os_dev = grub_find_device ("/dev", st.st_dev);
#endif
#endif /* !__GNU__ */

  return os_dev;
}

static int
grub_util_is_dmraid (const char *os_dev)
{
  if (! strncmp (os_dev, "/dev/mapper/nvidia_", 19))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/isw_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/hpt37x_", 19))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/hpt45x_", 19))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/via_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/lsi_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/pdc_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/jmicron_", 20))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/asr_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/sil_", 16))
    return 1;
  else if (! strncmp (os_dev, "/dev/mapper/ddf1_", 17))
    return 1;

  return 0;
}

int
grub_util_get_dev_abstraction (const char *os_dev __attribute__((unused)))
{
#ifdef __linux__
  /* User explicitly claims that this drive is visible by BIOS.  */
  if (grub_util_biosdisk_is_present (os_dev))
    return GRUB_DEV_ABSTRACTION_NONE;

  /* Check for LVM.  */
  if (!strncmp (os_dev, "/dev/mapper/", 12)
      && ! grub_util_is_dmraid (os_dev)
      && strncmp (os_dev, "/dev/mapper/mpath", 17) != 0)
    return GRUB_DEV_ABSTRACTION_LVM;

  /* Check for RAID.  */
  if (!strncmp (os_dev, "/dev/md", 7))
    return GRUB_DEV_ABSTRACTION_RAID;
#endif

  /* No abstraction found.  */
  return GRUB_DEV_ABSTRACTION_NONE;
}

#ifdef __linux__
static char *
get_mdadm_name (const char *os_dev)
{
  int mdadm_pipe[2];
  pid_t mdadm_pid;
  char *name = NULL;

  if (pipe (mdadm_pipe) < 0)
    {
      grub_util_warn ("Unable to create pipe for mdadm: %s", strerror (errno));
      return NULL;
    }

  mdadm_pid = fork ();
  if (mdadm_pid < 0)
    grub_util_warn ("Unable to fork mdadm: %s", strerror (errno));
  else if (mdadm_pid == 0)
    {
      /* Child.  */
      char *argv[5];

      close (mdadm_pipe[0]);
      dup2 (mdadm_pipe[1], STDOUT_FILENO);
      close (mdadm_pipe[1]);

      /* execvp has inconvenient types, hence the casts.  None of these
         strings will actually be modified.  */
      argv[0] = (char *) "mdadm";
      argv[1] = (char *) "--detail";
      argv[2] = (char *) "--export";
      argv[3] = (char *) os_dev;
      argv[4] = NULL;
      execvp ("mdadm", argv);
      exit (127);
    }
  else
    {
      /* Parent.  Read mdadm's output.  */
      FILE *mdadm;
      char *buf = NULL;
      size_t len = 0;

      close (mdadm_pipe[1]);
      mdadm = fdopen (mdadm_pipe[0], "r");
      if (! mdadm)
	{
	  grub_util_warn ("Unable to open stream from mdadm: %s",
			  strerror (errno));
	  goto out;
	}

      while (getline (&buf, &len, mdadm) > 0)
	{
	  if (strncmp (buf, "MD_NAME=", sizeof ("MD_NAME=") - 1) == 0)
	    {
	      char *name_start, *colon;
	      size_t name_len;

	      free (name);
	      name_start = buf + sizeof ("MD_NAME=") - 1;
	      /* Strip off the homehost if present.  */
	      colon = strchr (name_start, ':');
	      name = strdup (colon ? colon + 1 : name_start);
	      name_len = strlen (name);
	      if (name[name_len - 1] == '\n')
		name[name_len - 1] = '\0';
	    }
	}

out:
      close (mdadm_pipe[0]);
      waitpid (mdadm_pid, NULL, 0);
    }

  return name;
}
#endif /* __linux__ */

char *
grub_util_get_grub_dev (const char *os_dev)
{
  char *grub_dev = NULL;

  switch (grub_util_get_dev_abstraction (os_dev))
    {
    case GRUB_DEV_ABSTRACTION_LVM:

      {
	unsigned short i, len;
	grub_size_t offset = sizeof ("/dev/mapper/") - 1;

	len = strlen (os_dev) - offset + 1;
	grub_dev = xmalloc (len);

	for (i = 0; i < len; i++, offset++)
	  {
	    grub_dev[i] = os_dev[offset];
	    if (os_dev[offset] == '-' && os_dev[offset + 1] == '-')
	      offset++;
	  }
      }

      break;

    case GRUB_DEV_ABSTRACTION_RAID:

      if (os_dev[7] == '_' && os_dev[8] == 'd')
	{
	  /* This a partitionable RAID device of the form /dev/md_dNNpMM. */

	  char *p, *q;

	  p = strdup (os_dev + sizeof ("/dev/md_d") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] == '/' && os_dev[8] == 'd')
	{
	  /* This a partitionable RAID device of the form /dev/md/dNNpMM. */

	  char *p, *q;

	  p = strdup (os_dev + sizeof ("/dev/md/d") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] >= '0' && os_dev[7] <= '9')
	{
	  char *p , *q;

	  p = strdup (os_dev + sizeof ("/dev/md") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] == '/' && os_dev[8] >= '0' && os_dev[8] <= '9')
	{
	  char *p , *q;

	  p = strdup (os_dev + sizeof ("/dev/md/") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md%s", p);
	  free (p);
	}
      else if (os_dev[7] == '/')
	{
	  /* mdraid 1.x with a free name.  */
	  char *p , *q;

	  p = strdup (os_dev + sizeof ("/dev/md/") - 1);

	  q = strchr (p, 'p');
	  if (q)
	    *q = ',';

	  grub_dev = xasprintf ("md/%s", p);
	  free (p);
	}
      else
	grub_util_error ("unknown kind of RAID device `%s'", os_dev);

#ifdef __linux__
      {
	char *mdadm_name = get_mdadm_name (os_dev);
	struct stat st;

	if (mdadm_name)
	  {
	    char *newname;
	    newname = xasprintf ("/dev/md/%s", mdadm_name);
	    if (stat (newname, &st) == 0)
	      {
		free (grub_dev);
		grub_dev = xasprintf ("md/%s", mdadm_name);
	      }
	    free (newname);
	    free (mdadm_name);
	  }
      }
#endif /* __linux__ */

      break;

    default:  /* GRUB_DEV_ABSTRACTION_NONE */
      grub_dev = grub_util_biosdisk_get_grub_dev (os_dev);
    }

  return grub_dev;
}

const char *
grub_util_check_block_device (const char *blk_dev)
{
  struct stat st;

  if (stat (blk_dev, &st) < 0)
    grub_util_error ("cannot stat `%s'", blk_dev);

  if (S_ISBLK (st.st_mode))
    return (blk_dev);
  else
    return 0;
}

const char *
grub_util_check_char_device (const char *blk_dev)
{
  struct stat st;

  if (stat (blk_dev, &st) < 0)
    grub_util_error ("cannot stat `%s'", blk_dev);

  if (S_ISCHR (st.st_mode))
    return (blk_dev);
  else
    return 0;
}
