/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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
#include <grub/msdos_partition.h>
#include <grub/scsi.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/video.h>
#include <grub/mm.h>
#include <grub/cpu/relocator.h>
#include <grub/extcmd.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_dl_t my_mod;
static struct grub_relocator *rel;
static grub_uint32_t eip = 0xffffffff;

#define GRUB_PLAN9_TARGET            0x100000
#define GRUB_PLAN9_ALIGN             4096
#define GRUB_PLAN9_CONFIG_ADDR       0x001200
#define GRUB_PLAN9_CONFIG_PATH_SIZE  0x000040
#define GRUB_PLAN9_CONFIG_MAGIC      "ZORT 0\r\n"

static const struct grub_arg_option options[] =
  {
    {"map", 'm', GRUB_ARG_OPTION_REPEATABLE,
     /* TRANSLATORS: it's about guessing which GRUB disk
	is which Plan9 disk. If your language has no
	word "mapping" you can use another word which
	means that the GRUBDEVICE and PLAN9DEVICE are
	actually the same device, just named differently
	in OS and GRUB.  */
     N_("Override guessed mapping of Plan9 devices."), 
     N_("GRUBDEVICE=PLAN9DEVICE"),
     ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

struct grub_plan9_header
{
  grub_uint32_t magic;
#define GRUB_PLAN9_MAGIC 0x1eb
  grub_uint32_t text_size;
  grub_uint32_t data_size;
  grub_uint32_t bss_size;
  grub_uint32_t sectiona;
  grub_uint32_t entry_addr;
  grub_uint32_t zero;
  grub_uint32_t sectionb;
};

static grub_err_t
grub_plan9_boot (void)
{
  struct grub_relocator32_state state = { 
    .eax = 0,
    .eip = eip,
    .ebx = 0,
    .ecx = 0,
    .edx = 0,
    .edi = 0,
    .esp = 0,
    .ebp = 0,
    .esi = 0
  };
  grub_video_set_mode ("text", 0, 0);

  return grub_relocator32_boot (rel, state, 0);
}

static grub_err_t
grub_plan9_unload (void)
{
  grub_relocator_unload (rel);
  rel = NULL;
  grub_dl_unref (my_mod);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_plan9 (grub_extcmd_context_t ctxt, int argc, char *argv[])
{
  grub_file_t file = 0;
  void *mem;
  grub_size_t memsize, padsize;
  struct grub_plan9_header hdr;
  char *config, *configptr;
  grub_size_t configsize;
  char *pmap = NULL;
  grub_size_t pmapalloc = 256;
  grub_size_t pmapptr = 0;
  int noslash = 1;
  char prefixes[5][10] = {"dos", "plan9", "ntfs", "linux", "linuxswap"};
  int prefixescnt[5];
  char *bootdisk = NULL, *bootpart = NULL, *bootpath = NULL;

  auto int fill_partition (grub_disk_t disk,
			   const grub_partition_t partition);
  int fill_partition (grub_disk_t disk,
		      const grub_partition_t partition)
  {
    int file_disk = 0;
    int pstart, pend;
    if (!noslash)
      {
	if (grub_extend_alloc (pmapptr + 1, &pmapalloc, (void **) &pmap))
	  return 1;
	pmap[pmapptr++] = '/';
      }
    noslash = 0;

    file_disk = file->device->disk && disk->id == file->device->disk->id
      && disk->dev->id == file->device->disk->dev->id;

    pstart = pmapptr;
    if (grub_strcmp (partition->partmap->name, "plan") == 0)
      {
	unsigned ptr = partition->index + sizeof ("part ") - 1;
	grub_err_t err;
	disk->partition = partition->parent;
	do
	  {
	    if (grub_extend_alloc (pmapptr + 1, &pmapalloc, (void **) &pmap))
	      return 1;
	    err = grub_disk_read (disk, 1, ptr, 1, pmap + pmapptr);
	    if (err)
	      {
		disk->partition = 0;
		return err;
	      }
	    ptr++;
	    pmapptr++;
	  }
	while (grub_isalpha (pmap[pmapptr - 1])
	       || grub_isdigit (pmap[pmapptr - 1]));
	pmapptr--;
      }
    else
      {
	char name[50];
	int c = 0;
	if (grub_strcmp (partition->partmap->name, "msdos") == 0)
	  {
	    switch (partition->msdostype)
	      {
	      case GRUB_PC_PARTITION_TYPE_PLAN9:
		c = 1;
		break;
	      case GRUB_PC_PARTITION_TYPE_NTFS:
		c = 2;
		break;
	      case GRUB_PC_PARTITION_TYPE_MINIX:
	      case GRUB_PC_PARTITION_TYPE_LINUX_MINIX:
	      case GRUB_PC_PARTITION_TYPE_EXT2FS:
		c = 3;
		break;
	      case GRUB_PC_PARTITION_TYPE_LINUX_SWAP:
		c = 4;
		break;
	      }
	  }

	if (prefixescnt[c] == 0)
	  grub_strcpy (name, prefixes[c]);
	else
	  grub_snprintf (name, sizeof (name), "%s.%d", prefixes[c],
			 prefixescnt[c]);
	prefixescnt[c]++;
	if (grub_extend_alloc (pmapptr + grub_strlen (name) + 1,
			       &pmapalloc, (void **) &pmap))
	  return 1;
	grub_strcpy (pmap + pmapptr, name);
	pmapptr += grub_strlen (name);
      }
    pend = pmapptr;
    if (grub_extend_alloc (pmapptr + 2 + 25 + 5 + 25, &pmapalloc,
			   (void **) &pmap))
      return 1;
    pmap[pmapptr++] = ' ';
    grub_snprintf (pmap + pmapptr, 25 + 5 + 25,
		   "%" PRIuGRUB_UINT64_T " %" PRIuGRUB_UINT64_T,
		   grub_partition_get_start (partition),
		   grub_partition_get_start (partition)
		   + grub_partition_get_len (partition));
    if (file_disk && grub_partition_get_start (partition)
	== grub_partition_get_start (file->device->disk->partition)
	&& grub_partition_get_len (partition)
	== grub_partition_get_len (file->device->disk->partition))
      {
	grub_free (bootpart);
	bootpart = grub_strndup (pmap + pstart, pend - pstart);
      }

    pmapptr += grub_strlen (pmap + pmapptr);
    return 0;
  }

  auto int fill_disk (const char *name);
  int fill_disk (const char *name)
  {
    grub_device_t dev;
    char *plan9name = NULL;
    unsigned i;
    int file_disk = 0;

    dev = grub_device_open (name);
    if (!dev)
      {
	grub_print_error ();
	return 0;
      }
    if (!dev->disk)
      {
	grub_device_close (dev);
	return 0;
      }
    file_disk = file->device->disk && dev->disk->id == file->device->disk->id
      && dev->disk->dev->id == file->device->disk->dev->id;
    for (i = 0; ctxt->state[0].args && ctxt->state[0].args[i]; i++)
      if (grub_strncmp (name, ctxt->state[0].args[i], grub_strlen (name)) == 0
	  && ctxt->state[0].args[i][grub_strlen (name)] == '=')
	break;
    if (ctxt->state[0].args && ctxt->state[0].args[i])
      plan9name = grub_strdup (ctxt->state[0].args[i] + grub_strlen (name) + 1);
    else
      switch (dev->disk->dev->id)
	{
	case GRUB_DISK_DEVICE_BIOSDISK_ID:
	  if (dev->disk->id & 0x80)
	    plan9name = grub_xasprintf ("sdB%u",
					(unsigned) (dev->disk->id & 0x7f));
	  else
	    plan9name = grub_xasprintf ("fd%u",
					(unsigned) (dev->disk->id & 0x7f));
	  break;
	  /* Shouldn't happen as Plan9 doesn't work on these platforms.  */
	case GRUB_DISK_DEVICE_OFDISK_ID:
	case GRUB_DISK_DEVICE_EFIDISK_ID:

	  /* Plan9 doesn't see those.  */
	default:

	  /* Not sure how to handle those. */
	case GRUB_DISK_DEVICE_NAND_ID:
	  if (!file_disk)
	    {
	      grub_device_close (dev);
	      return 0;
	    }
	  
	  /* if it's the disk the kernel is loaded from we need to name
	     it nevertheless.  */
	  plan9name = grub_strdup ("sdZ0");
	  break;

	case GRUB_DISK_DEVICE_ATA_ID:
	  {
	    int unit;
	    if (grub_strlen (dev->disk->name) < sizeof ("ata0") - 1)
	      unit = 0;
	    else
	      unit = grub_strtoul (dev->disk->name + sizeof ("ata0") - 1, 0, 0);
	    plan9name = grub_xasprintf ("sd%c%d", 'C' + unit / 2, unit % 2);
	  }
	  break;
	case GRUB_DISK_DEVICE_SCSI_ID:
	  if (((dev->disk->id >> GRUB_SCSI_ID_SUBSYSTEM_SHIFT) & 0xff)
	      == GRUB_SCSI_SUBSYSTEM_PATA)
	    {
	      int unit;
	      if (grub_strlen (dev->disk->name) < sizeof ("ata0") - 1)
		unit = 0;
	      else
		unit = grub_strtoul (dev->disk->name + sizeof ("ata0") - 1,
				     0, 0);
	      plan9name = grub_xasprintf ("sd%c%d", 'C' + unit / 2, unit % 2);
	      break;
	    }
	  
	  /* FIXME: how does Plan9 number controllers?
	     We probably need save the SCSI devices and sort them  */
	  plan9name
	    = grub_xasprintf ("sd0%u", (unsigned)
			      ((dev->disk->id >> GRUB_SCSI_ID_BUS_SHIFT)
			       & 0xf));
	  break;
	}
    if (!plan9name)
      {
	grub_print_error ();
	return 0;
      }
    if (grub_extend_alloc (pmapptr + grub_strlen (plan9name)
			   + sizeof ("part="), &pmapalloc,
			   (void **) &pmap))
      {
	grub_free (plan9name);
	return 1;
      }
    grub_strcpy (pmap + pmapptr, plan9name);
    pmapptr += grub_strlen (plan9name);
    if (!file_disk)
      grub_free (plan9name);
    else
      {
	grub_free (bootdisk);
	bootdisk = plan9name;
      }
    grub_strcpy (pmap + pmapptr, "part=");
    pmapptr += sizeof ("part=") - 1;

    noslash = 1;
    grub_memset (prefixescnt, 0, sizeof (prefixescnt));
    if (grub_partition_iterate (dev->disk, fill_partition))
      return 1;
    if (grub_extend_alloc (pmapptr + 1, &pmapalloc, (void **) &pmap))
      return 1;
    pmap[pmapptr++] = '\n';

    return 0;
  }

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  grub_dl_ref (my_mod);

  rel = grub_relocator_new ();
  if (!rel)
    goto fail;

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  pmap = grub_malloc (pmapalloc);
  if (!pmap)
    goto fail;

  if (grub_disk_dev_iterate (fill_disk))
    goto fail;

  if (grub_extend_alloc (pmapptr + 1, &pmapalloc,
			 (void **) &pmap))
    goto fail;
  pmap[pmapptr] = 0;

  {
    char *file_name = grub_strchr (argv[0], ')');
    if (file_name)
      file_name++;
    else
      file_name = argv[0];
    if (*file_name)
      file_name++;

    if (bootpart)
      bootpath = grub_xasprintf ("%s!%s!%s", bootdisk, bootpart, file_name);
    else
      bootpath = grub_xasprintf ("%s!%s", bootdisk, file_name);
    grub_free (bootdisk);
    grub_free (bootpart);
  }
  if (!bootpath)
    goto fail;

  if (grub_file_read (file, &hdr, sizeof (hdr)) != (grub_ssize_t) sizeof (hdr))
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"),
		    argv[0]);
      goto fail;
    }

  if (grub_be_to_cpu32 (hdr.magic) != GRUB_PLAN9_MAGIC
      || hdr.zero)
    {
      grub_error (GRUB_ERR_BAD_OS, "unsupported Plan9");
      goto fail;
    }

  memsize = ALIGN_UP (grub_be_to_cpu32 (hdr.text_size) + sizeof (hdr),
		       GRUB_PLAN9_ALIGN);
  memsize += ALIGN_UP (grub_be_to_cpu32 (hdr.data_size), GRUB_PLAN9_ALIGN);
  memsize += ALIGN_UP(grub_be_to_cpu32 (hdr.bss_size), GRUB_PLAN9_ALIGN);
  eip = grub_be_to_cpu32 (hdr.entry_addr) & 0xfffffff;

  /* path */
  configsize = GRUB_PLAN9_CONFIG_PATH_SIZE;
  /* magic */
  configsize += sizeof (GRUB_PLAN9_CONFIG_MAGIC) - 1;
  {
    int i;
    for (i = 1; i < argc; i++)
      configsize += grub_strlen (argv[i]) + 1;
  }
  configsize += (sizeof ("bootfile=") - 1) + grub_strlen (bootpath) + 1;
  configsize += pmapptr;
  /* Terminating \0.  */
  configsize++;

  {
    grub_relocator_chunk_t ch;
    grub_err_t err;
    err = grub_relocator_alloc_chunk_addr (rel, &ch, GRUB_PLAN9_CONFIG_ADDR,
					   configsize);
    if (err)
      goto fail;
    config = get_virtual_current_address (ch);
  }

  grub_memset (config, 0, GRUB_PLAN9_CONFIG_PATH_SIZE);
  grub_strncpy (config, bootpath, GRUB_PLAN9_CONFIG_PATH_SIZE - 1);

  configptr = config + GRUB_PLAN9_CONFIG_PATH_SIZE;
  grub_memcpy (configptr, GRUB_PLAN9_CONFIG_MAGIC,
	       sizeof (GRUB_PLAN9_CONFIG_MAGIC) - 1);
  configptr += sizeof (GRUB_PLAN9_CONFIG_MAGIC) - 1;
  configptr = grub_stpcpy (configptr, "bootfile=");
  configptr = grub_stpcpy (configptr, bootpath);
  *configptr++ = '\n';
  {
    int i;
    for (i = 1; i < argc; i++)
      {
	configptr = grub_stpcpy (configptr, argv[i]);
	*configptr++ = '\n';
      }
  }
  configptr = grub_stpcpy (configptr, pmap);

  {
    grub_relocator_chunk_t ch;
    grub_err_t err;

    err = grub_relocator_alloc_chunk_addr (rel, &ch, GRUB_PLAN9_TARGET,
					   memsize);
    if (err)
      goto fail;
    mem = get_virtual_current_address (ch);
  }

  {
    grub_uint8_t *ptr;
    ptr = mem;
    grub_memcpy (ptr, &hdr, sizeof (hdr));
    ptr += sizeof (hdr);

    if (grub_file_read (file, ptr, grub_be_to_cpu32 (hdr.text_size))
	!= (grub_ssize_t) grub_be_to_cpu32 (hdr.text_size))
      {
	if (!grub_errno)
	  grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"),
		      argv[0]);
	goto fail;
      }
    ptr += grub_be_to_cpu32 (hdr.text_size);
    padsize = ALIGN_UP (grub_be_to_cpu32 (hdr.text_size) + sizeof (hdr),
			GRUB_PLAN9_ALIGN) - grub_be_to_cpu32 (hdr.text_size)
      - sizeof (hdr);

    grub_memset (ptr, 0, padsize);
    ptr += padsize;

    if (grub_file_read (file, ptr, grub_be_to_cpu32 (hdr.data_size))
	!= (grub_ssize_t) grub_be_to_cpu32 (hdr.data_size))
      {
	if (!grub_errno)
	  grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"),
		      argv[0]);
	goto fail;
      }
    ptr += grub_be_to_cpu32 (hdr.data_size);
    padsize = ALIGN_UP (grub_be_to_cpu32 (hdr.data_size), GRUB_PLAN9_ALIGN)
      - grub_be_to_cpu32 (hdr.data_size);

    grub_memset (ptr, 0, padsize);
    ptr += padsize;
    grub_memset (ptr, 0, ALIGN_UP(grub_be_to_cpu32 (hdr.bss_size),
				  GRUB_PLAN9_ALIGN));
  }
  grub_loader_set (grub_plan9_boot, grub_plan9_unload, 1);
  return GRUB_ERR_NONE;

 fail:
  grub_free (pmap);

  if (file)
    grub_file_close (file);

  grub_plan9_unload ();

  return grub_errno;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(plan9)
{
  cmd = grub_register_extcmd ("plan9", grub_cmd_plan9,
			      GRUB_COMMAND_OPTIONS_AT_START,
			      N_("KERNEL ARGS"), N_("Load Plan9 kernel."),
			      options);
  my_mod = mod;
}

GRUB_MOD_FINI(plan9)
{
  grub_unregister_extcmd (cmd);
}
