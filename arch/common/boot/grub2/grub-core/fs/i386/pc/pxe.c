/* pxe.c - Driver to provide access to the pxe filesystem  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
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
#include <grub/env.h>

#include <grub/machine/pxe.h>
#include <grub/machine/int.h>
#include <grub/machine/memory.h>

#define SEGMENT(x)	((x) >> 4)
#define OFFSET(x)	((x) & 0xF)
#define SEGOFS(x)	((SEGMENT(x) << 16) + OFFSET(x))
#define LINEAR(x)	(void *) (((x >> 16) <<4) + (x & 0xFFFF))

struct grub_pxe_disk_data
{
  grub_uint32_t server_ip;
  grub_uint32_t gateway_ip;
};

struct grub_pxe_bangpxe *grub_pxe_pxenv;
static grub_uint32_t grub_pxe_your_ip;
static grub_uint32_t grub_pxe_default_server_ip;
static grub_uint32_t grub_pxe_default_gateway_ip;
static unsigned grub_pxe_blksize = GRUB_PXE_MIN_BLKSIZE;

static grub_file_t curr_file = 0;

struct grub_pxe_data
{
  grub_uint32_t packet_number;
  grub_uint32_t block_size;
  char filename[0];
};

static grub_uint32_t pxe_rm_entry = 0;

static struct grub_pxe_bangpxe *
grub_pxe_scan (void)
{
  struct grub_bios_int_registers regs;
  struct grub_pxenv *pxenv;
  struct grub_pxe_bangpxe *bangpxe;

  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;

  regs.ebx = 0;
  regs.ecx = 0;
  regs.eax = 0x5650;
  regs.es = 0;

  grub_bios_interrupt (0x1a, &regs);

  if ((regs.eax & 0xffff) != 0x564e)
    return NULL;

  pxenv = (struct grub_pxenv *) ((regs.es << 4) + (regs.ebx & 0xffff));
  if (grub_memcmp (pxenv->signature, GRUB_PXE_SIGNATURE,
		   sizeof (pxenv->signature))
      != 0)
    return NULL;

  if (pxenv->version < 0x201)
    return NULL;

  bangpxe = (void *) ((((pxenv->pxe_ptr & 0xffff0000) >> 16) << 4)
		      + (pxenv->pxe_ptr & 0xffff));

  if (!bangpxe)
    return NULL;

  if (grub_memcmp (bangpxe->signature, GRUB_PXE_BANGPXE_SIGNATURE,
		   sizeof (bangpxe->signature)) != 0)
    return NULL;

  pxe_rm_entry = bangpxe->rm_entry;

  return bangpxe;
}

static int
grub_pxe_iterate (int (*hook) (const char *name))
{
  if (hook ("pxe"))
    return 1;
  return 0;
}

static grub_err_t
parse_ip (const char *val, grub_uint32_t *ip, const char **rest)
{
  grub_uint32_t newip = 0;
  unsigned long t;
  int i;
  const char *ptr = val;

  for (i = 0; i < 4; i++)
    {
      t = grub_strtoul (ptr, (char **) &ptr, 0);
      if (grub_errno)
	return grub_errno;
      if (t & ~0xff)
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "Invalid IP.");
      newip >>= 8;
      newip |= (t << 24);
      if (i != 3 && *ptr != '.')
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "Invalid IP.");
      ptr++;
    }
  *ip = newip;
  if (rest)
    *rest = ptr - 1;
  return 0;
}

static grub_err_t
grub_pxe_open (const char *name, grub_disk_t disk)
{
  struct grub_pxe_disk_data *data;

  if (grub_strcmp (name, "pxe") != 0
      && grub_strncmp (name, "pxe:", sizeof ("pxe:") - 1) != 0)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a pxe disk");

  data = grub_malloc (sizeof (*data));
  if (!data)
    return grub_errno;

  if (grub_strncmp (name, "pxe:", sizeof ("pxe:") - 1) == 0)
    {
      const char *ptr;
      grub_err_t err;

      ptr = name + sizeof ("pxe:") - 1;
      err = parse_ip (ptr, &(data->server_ip), &ptr);
      if (err)
	return err;
      if (*ptr == ':')
	{
	  err = parse_ip (ptr + 1, &(data->gateway_ip), 0);
	  if (err)
	    return err;
	}
      else
	data->gateway_ip = grub_pxe_default_gateway_ip;
    }
  else
    {
      data->server_ip = grub_pxe_default_server_ip;
      data->gateway_ip = grub_pxe_default_gateway_ip;
    }

  disk->total_sectors = 0;
  disk->id = (unsigned long) data;

  disk->data = data;

  return GRUB_ERR_NONE;
}

static void
grub_pxe_close (grub_disk_t disk)
{
  grub_free (disk->data);
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
grub_pxefs_dir (grub_device_t device,
		const char *path  __attribute__ ((unused)),
		int (*hook) (const char *filename,
			     const struct grub_dirhook_info *info)
		__attribute__ ((unused)))
{
  if (device->disk->dev->id != GRUB_DISK_DEVICE_PXE_ID)
    return grub_error (GRUB_ERR_IO, "not a pxe disk");

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
  struct grub_pxe_disk_data *disk_data = file->device->disk->data;
  grub_file_t file_int, bufio;

  if (file->device->disk->dev->id != GRUB_DISK_DEVICE_PXE_ID)
    return grub_error (GRUB_ERR_IO, "not a pxe disk");

  if (curr_file != 0)
    {
      grub_pxe_call (GRUB_PXENV_TFTP_CLOSE, &c.c2, pxe_rm_entry);
      curr_file = 0;
    }

  c.c1.server_ip = disk_data->server_ip;
  c.c1.gateway_ip = disk_data->gateway_ip;
  grub_strcpy ((char *)&c.c1.filename[0], name);
  grub_pxe_call (GRUB_PXENV_TFTP_GET_FSIZE, &c.c1, pxe_rm_entry);
  if (c.c1.status)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");

  file->size = c.c1.file_size;

  c.c2.tftp_port = grub_cpu_to_be16 (GRUB_PXE_TFTP_PORT);
  c.c2.packet_size = grub_pxe_blksize;
  grub_pxe_call (GRUB_PXENV_TFTP_OPEN, &c.c2, pxe_rm_entry);
  if (c.c2.status)
    return grub_error (GRUB_ERR_BAD_FS, "open fails");

  data = grub_zalloc (sizeof (struct grub_pxe_data) + grub_strlen (name) + 1);
  if (! data)
    return grub_errno;

  data->block_size = c.c2.packet_size;
  grub_strcpy (data->filename, name);

  file_int = grub_malloc (sizeof (*file_int));
  if (! file_int)
    {
      grub_free (data);
      return grub_errno;
    }

  file->data = data;
  file->not_easly_seekable = 1;
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
  struct grub_pxe_disk_data *disk_data = file->device->disk->data;
  grub_uint32_t pn, r;

  data = file->data;

  pn = grub_divmod64 (file->offset, data->block_size, &r);
  if (r)
    {
      grub_error (GRUB_ERR_BAD_FS,
		  "read access must be aligned to packet size");
      return -1;
    }

  if ((curr_file != file) || (data->packet_number > pn))
    {
      struct grub_pxenv_tftp_open o;

      if (curr_file != 0)
        grub_pxe_call (GRUB_PXENV_TFTP_CLOSE, &o, pxe_rm_entry);

      o.server_ip = disk_data->server_ip;
      o.gateway_ip = disk_data->gateway_ip;
      grub_strcpy ((char *)&o.filename[0], data->filename);
      o.tftp_port = grub_cpu_to_be16 (GRUB_PXE_TFTP_PORT);
      o.packet_size = data->block_size;
      grub_pxe_call (GRUB_PXENV_TFTP_OPEN, &o, pxe_rm_entry);
      if (o.status)
	{
	  grub_error (GRUB_ERR_BAD_FS, "open fails");
	  return -1;
	}
      data->block_size = o.packet_size;
      data->packet_number = 0;
      curr_file = file;
    }

  c.buffer = SEGOFS (GRUB_MEMORY_MACHINE_SCRATCH_ADDR);
  while (pn >= data->packet_number)
    {
      c.buffer_size = data->block_size;
      grub_pxe_call (GRUB_PXENV_TFTP_READ, &c, pxe_rm_entry);
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

  if (curr_file == file)
    {
      grub_pxe_call (GRUB_PXENV_TFTP_CLOSE, &c, pxe_rm_entry);
      curr_file = 0;
    }

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

static char *
grub_env_write_readonly (struct grub_env_var *var __attribute__ ((unused)),
			 const char *val __attribute__ ((unused)))
{
  return NULL;
}

static void
set_mac_env (grub_uint8_t *mac_addr, grub_size_t mac_len)
{
  char buf[(sizeof ("XX:") - 1) * mac_len + 1];
  char *ptr = buf;
  unsigned i;

  for (i = 0; i < mac_len; i++)
    {
      grub_snprintf (ptr, sizeof (buf) - (ptr - buf),
		     "%02x:", mac_addr[i] & 0xff);
      ptr += (sizeof ("XX:") - 1);
    }
  if (mac_len)
    *(ptr - 1) = 0;
  else
    buf[0] = 0;

  grub_env_set ("net_pxe_mac", buf);
  /* XXX: Is it possible to change MAC in PXE?  */
  grub_register_variable_hook ("net_pxe_mac", 0, grub_env_write_readonly);
}

static void
set_env_limn_ro (const char *varname, char *value, grub_size_t len)
{
  char c;
  c = value[len];
  value[len] = 0;
  grub_env_set (varname, value);
  value[len] = c;
  grub_register_variable_hook (varname, 0, grub_env_write_readonly);
}

static void
parse_dhcp_vendor (void *vend, int limit)
{
  grub_uint8_t *ptr, *ptr0;

  ptr = ptr0 = vend;

  if (grub_be_to_cpu32 (*(grub_uint32_t *) ptr) != 0x63825363)
    return;
  ptr = ptr + sizeof (grub_uint32_t);
  while (ptr - ptr0 < limit)
    {
      grub_uint8_t tagtype;
      grub_uint8_t taglength;

      tagtype = *ptr++;

      /* Pad tag.  */
      if (tagtype == 0)
	continue;

      /* End tag.  */
      if (tagtype == 0xff)
	return;

      taglength = *ptr++;

      switch (tagtype)
	{
	case 12:
	  set_env_limn_ro ("net_pxe_hostname", (char *) ptr, taglength);
	  break;

	case 15:
	  set_env_limn_ro ("net_pxe_domain", (char *) ptr, taglength);
	  break;

	case 17:
	  set_env_limn_ro ("net_pxe_rootpath", (char *) ptr, taglength);
	  break;

	case 18:
	  set_env_limn_ro ("net_pxe_extensionspath", (char *) ptr, taglength);
	  break;

	  /* If you need any other options please contact GRUB
	     developpement team.  */
	}

      ptr += taglength;
    }
}

static void
grub_pxe_detect (void)
{
  struct grub_pxe_bangpxe *pxenv;
  struct grub_pxenv_get_cached_info ci;
  struct grub_pxenv_boot_player *bp;

  pxenv = grub_pxe_scan ();
  if (! pxenv)
    return;

  ci.packet_type = GRUB_PXENV_PACKET_TYPE_DHCP_ACK;
  ci.buffer = 0;
  ci.buffer_size = 0;
  grub_pxe_call (GRUB_PXENV_GET_CACHED_INFO, &ci, pxe_rm_entry);
  if (ci.status)
    return;

  bp = LINEAR (ci.buffer);

  grub_pxe_your_ip = bp->your_ip;
  grub_pxe_default_server_ip = bp->server_ip;
  grub_pxe_default_gateway_ip = bp->gateway_ip;
  set_mac_env (bp->mac_addr, bp->hw_len < sizeof (bp->mac_addr) ? bp->hw_len
	       : sizeof (bp->mac_addr));
  set_env_limn_ro ("net_pxe_boot_file", (char *) bp->boot_file,
		   sizeof (bp->boot_file));
  set_env_limn_ro ("net_pxe_dhcp_server_name", (char *) bp->server_name,
		   sizeof (bp->server_name));
  parse_dhcp_vendor (&bp->vendor, sizeof (bp->vendor));
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

static void
set_ip_env (char *varname, grub_uint32_t ip)
{
  char buf[sizeof ("XXX.XXX.XXX.XXX")];

  grub_snprintf (buf, sizeof (buf), "%d.%d.%d.%d", (ip & 0xff),
		 (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff);
  grub_env_set (varname, buf);
}

static char *
write_ip_env (grub_uint32_t *ip, const char *val)
{
  char *buf;
  grub_err_t err;
  grub_uint32_t newip;
  
  err = parse_ip (val, &newip, 0);
  if (err)
    return 0;

  /* Normalize the IP.  */
  buf = grub_xasprintf ("%d.%d.%d.%d", (newip & 0xff), (newip >> 8) & 0xff,
		       (newip >> 16) & 0xff, (newip >> 24) & 0xff);
  if (!buf)
    return 0;

  *ip = newip;

  return buf; 
}

static char *
grub_env_write_pxe_default_server (struct grub_env_var *var 
				   __attribute__ ((unused)),
				   const char *val)
{
  return write_ip_env (&grub_pxe_default_server_ip, val);
}

static char *
grub_env_write_pxe_default_gateway (struct grub_env_var *var
				    __attribute__ ((unused)),
				    const char *val)
{
  return write_ip_env (&grub_pxe_default_gateway_ip, val);
}

static char *
grub_env_write_pxe_blocksize (struct grub_env_var *var __attribute__ ((unused)),
			      const char *val)
{
  unsigned size;
  char *buf;

  size = grub_strtoul (val, 0, 0);
  if (grub_errno)
    return 0;

  if (size < GRUB_PXE_MIN_BLKSIZE)
    size = GRUB_PXE_MIN_BLKSIZE;
  else if (size > GRUB_PXE_MAX_BLKSIZE)
    size = GRUB_PXE_MAX_BLKSIZE;
  
  buf = grub_xasprintf ("%d", size);
  if (!buf)
    return 0;

  grub_pxe_blksize = size;
  
  return buf;
}


GRUB_MOD_INIT(pxe)
{
  grub_pxe_detect ();
  if (grub_pxe_pxenv)
    {
      char *buf;

      buf = grub_xasprintf ("%d", grub_pxe_blksize);
      if (buf)
	grub_env_set ("pxe_blksize", buf);
      grub_free (buf);

      set_ip_env ("pxe_default_server", grub_pxe_default_server_ip);
      set_ip_env ("pxe_default_gateway", grub_pxe_default_gateway_ip);
      set_ip_env ("net_pxe_ip", grub_pxe_your_ip);
      grub_register_variable_hook ("pxe_default_server", 0,
				   grub_env_write_pxe_default_server);
      grub_register_variable_hook ("pxe_default_gateway", 0,
				   grub_env_write_pxe_default_gateway);

      /* XXX: Is it possible to change IP in PXE?  */
      grub_register_variable_hook ("net_pxe_ip", 0,
				   grub_env_write_readonly);
      grub_register_variable_hook ("pxe_blksize", 0,
				   grub_env_write_pxe_blocksize);
      grub_disk_dev_register (&grub_pxe_dev);
      grub_fs_register (&grub_pxefs_fs);
    }
}

GRUB_MOD_FINI(pxe)
{
  grub_pxe_unload ();
}
