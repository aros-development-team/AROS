/* pxe.c - Driver to provide access to the pxe filesystem  */
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

#include <grub/dl.h>
#include <grub/fs.h>
#include <grub/mm.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/bufio.h>

#include <grub/machine/pxe.h>
#include <grub/machine/memory.h>

#define SEGMENT(x)	((x) >> 4)
#define OFFSET(x)	((x) & 0xF)
#define SEGOFS(x)	((SEGMENT(x) << 16) + OFFSET(x))
#define LINEAR(x)	(void *) (((x >> 16) <<4) + (x & 0xFFFF))

struct grub_pxenv *grub_pxe_pxenv;
grub_uint32_t grub_pxe_your_ip;
grub_uint32_t grub_pxe_server_ip;
grub_uint32_t grub_pxe_gateway_ip;
int grub_pxe_blksize = GRUB_PXE_MIN_BLKSIZE;

static grub_file_t curr_file = 0;

struct grub_pxe_data
{
  grub_uint32_t packet_number;
  grub_uint32_t block_size;
  char filename[0];
};

static int
grub_pxe_iterate (int (*hook) (const char *name))
{
  if (hook ("pxe"))
    return 1;
  return 0;
}

static grub_err_t
grub_pxe_open (const char *name, grub_disk_t disk)
{
  if (grub_strcmp (name, "pxe"))
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a pxe disk");

  disk->total_sectors = 0;
  disk->id = (unsigned long) "pxe";

  disk->has_partitions = 0;
  disk->data = 0;

  return GRUB_ERR_NONE;
}

static void
grub_pxe_close (grub_disk_t disk __attribute((unused)))
{
}

static grub_err_t
grub_pxe_read (grub_disk_t disk __attribute((unused)),
               grub_disk_addr_t sector __attribute((unused)),
               grub_size_t size __attribute((unused)),
               char *buf __attribute((unused)))
{
  return GRUB_ERR_OUT_OF_RANGE;
}

static grub_err_t
grub_pxe_write (grub_disk_t disk __attribute((unused)),
                grub_disk_addr_t sector __attribute((unused)),
                grub_size_t size __attribute((unused)),
                const char *buf __attribute((unused)))
{
  return GRUB_ERR_OUT_OF_RANGE;
}

static struct grub_disk_dev grub_pxe_dev =
  {
    .name = "pxe",
    .id = GRUB_DISK_DEVICE_PXE_ID,
    .iterate = grub_pxe_iterate,
    .open = grub_pxe_open,
    .close = grub_pxe_close,
    .read = grub_pxe_read,
    .write = grub_pxe_write,
    .next = 0
  };

static grub_err_t
grub_pxefs_dir (grub_device_t device __attribute((unused)),
                const char *path __attribute((unused)),
                int (*hook) (const char *filename, int dir) __attribute((unused)))
{
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_pxefs_open (struct grub_file *file, const char *name)
{
  union
    {
      struct grub_pxenv_tftp_get_fsize c1;
      struct grub_pxenv_tftp_open c2;
    } c;
  struct grub_pxe_data *data;
  grub_file_t file_int, bufio;

  c.c1.server_ip = grub_pxe_server_ip;
  c.c1.gateway_ip = grub_pxe_gateway_ip;
  grub_strcpy (c.c1.filename, name);
  grub_pxe_call (GRUB_PXENV_TFTP_GET_FSIZE, &c.c1);
  if (c.c1.status)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");

  file->size = c.c1.file_size;

  c.c2.tftp_port = grub_cpu_to_be16 (GRUB_PXE_TFTP_PORT);
  c.c2.packet_size = grub_pxe_blksize;
  grub_pxe_call (GRUB_PXENV_TFTP_OPEN, &c.c2);
  if (c.c2.status)
    return grub_error (GRUB_ERR_BAD_FS, "open fails");

  data = grub_malloc (sizeof (struct grub_pxe_data) + grub_strlen (name) + 1);
  if (! data)
    return grub_errno;

  data->packet_number = 0;
  data->block_size = grub_pxe_blksize;
  grub_strcpy (data->filename, name);

  file_int = grub_malloc (sizeof (*file_int));
  if (! file_int)
    {
      grub_free (data);
      return grub_errno;
    }

  file->data = data;
  grub_memcpy (file_int, file, sizeof (struct grub_file));
  curr_file = file_int;

  bufio = grub_bufio_open (file_int, data->block_size);
  if (! bufio)
    {
      grub_free (file_int);
      grub_free (data);
      return grub_errno;
    }

  grub_memcpy (file, bufio, sizeof (struct grub_file));

  return GRUB_ERR_NONE;
}

static grub_ssize_t
grub_pxefs_read (grub_file_t file, char *buf, grub_size_t len)
{
  struct grub_pxenv_tftp_read c;
  struct grub_pxe_data *data;
  grub_uint32_t pn, r;

  data = file->data;

  pn = grub_divmod64 (file->offset, data->block_size, &r);
  if (r)
    return grub_error (GRUB_ERR_BAD_FS,
                       "read access must be aligned to packet size");

  if ((curr_file != file) || (data->packet_number > pn))
    {
      struct grub_pxenv_tftp_open o;

      grub_pxe_call (GRUB_PXENV_TFTP_CLOSE, &o);

      o.server_ip = grub_pxe_server_ip;
      o.gateway_ip = grub_pxe_gateway_ip;
      grub_strcpy (o.filename, data->filename);
      o.tftp_port = grub_cpu_to_be16 (GRUB_PXE_TFTP_PORT);
      o.packet_size = data->block_size;
      grub_pxe_call (GRUB_PXENV_TFTP_OPEN, &o);
      if (o.status)
        return grub_error (GRUB_ERR_BAD_FS, "open fails");
      data->packet_number = 0;
      curr_file = file;
    }

  c.buffer = SEGOFS (GRUB_MEMORY_MACHINE_SCRATCH_ADDR);
  while (pn >= data->packet_number)
    {
      c.buffer_size = grub_pxe_blksize;
      grub_pxe_call (GRUB_PXENV_TFTP_READ, &c);
      if (c.status)
        {
          grub_error (GRUB_ERR_BAD_FS, "read fails");
          return -1;
        }
      data->packet_number++;
    }

  grub_memcpy (buf, (char *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR, len);

  return len;
}

static grub_err_t
grub_pxefs_close (grub_file_t file)
{
  struct grub_pxenv_tftp_close c;

  grub_pxe_call (GRUB_PXENV_TFTP_CLOSE, &c);
  grub_free (file->data);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_pxefs_label (grub_device_t device __attribute ((unused)),
		   char **label __attribute ((unused)))
{
  *label = 0;
  return GRUB_ERR_NONE;
}

static struct grub_fs grub_pxefs_fs =
  {
    .name = "pxefs",
    .dir = grub_pxefs_dir,
    .open = grub_pxefs_open,
    .read = grub_pxefs_read,
    .close = grub_pxefs_close,
    .label = grub_pxefs_label,
    .next = 0
  };

static void
grub_pxe_detect (void)
{
  struct grub_pxenv *pxenv;
  struct grub_pxenv_get_cached_info ci;
  struct grub_pxenv_boot_player *bp;

  pxenv = grub_pxe_scan ();
  if (! pxenv)
    return;

  ci.packet_type = GRUB_PXENV_PACKET_TYPE_DHCP_ACK;
  ci.buffer = 0;
  ci.buffer_size = 0;
  grub_pxe_call (GRUB_PXENV_GET_CACHED_INFO, &ci);
  if (ci.status)
    return;

  bp = LINEAR (ci.buffer);

  grub_pxe_your_ip = bp->your_ip;
  grub_pxe_server_ip = bp->server_ip;
  grub_pxe_gateway_ip = bp->gateway_ip;

  grub_pxe_pxenv = pxenv;
}

void
grub_pxe_unload (void)
{
  if (grub_pxe_pxenv)
    {
      grub_fs_unregister (&grub_pxefs_fs);
      grub_disk_dev_unregister (&grub_pxe_dev);

      grub_pxe_pxenv = 0;
    }
}

GRUB_MOD_INIT(pxe)
{
  (void) mod;			/* To stop warning. */

  grub_pxe_detect ();
  if (grub_pxe_pxenv)
    {
      grub_disk_dev_register (&grub_pxe_dev);
      grub_fs_register (&grub_pxefs_fs);
    }
}

GRUB_MOD_FINI(pxe)
{
  grub_pxe_unload ();
}
