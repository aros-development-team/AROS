/* lvm.c - module to read Logical Volumes.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/lvm.h>

static struct grub_lvm_vg *vg_list;
static int lv_count;


/* Go the string STR and return the number after STR.  *P will point
   at the number.  In case STR is not found, *P will be NULL and the
   return value will be 0.  */
static int
grub_lvm_getvalue (char **p, char *str)
{
  *p = grub_strstr (*p, str);
  if (! *p)
    return 0;
  *p += grub_strlen (str);
  return grub_strtoul (*p, NULL, 10);
}

static int
grub_lvm_iterate (int (*hook) (const char *name))
{
  struct grub_lvm_vg *vg;
  for (vg = vg_list; vg; vg = vg->next)
    {
      struct grub_lvm_lv *lv;
      if (vg->lvs)
	for (lv = vg->lvs; lv; lv = lv->next)
	  if (hook (lv->name))
	    return 1;
    }

  return 0;
}

#ifdef GRUB_UTIL
static grub_disk_memberlist_t
grub_lvm_memberlist (grub_disk_t disk)
{
  struct grub_lvm_lv *lv = disk->data;
  grub_disk_memberlist_t list = NULL, tmp;
  struct grub_lvm_pv *pv;

  if (lv->vg->pvs)
    for (pv = lv->vg->pvs; pv; pv = pv->next)
      {
	tmp = grub_malloc (sizeof (*tmp));
	tmp->disk = pv->disk;
	tmp->next = list;
	list = tmp;
      }

  return list;
}
#endif

static grub_err_t
grub_lvm_open (const char *name, grub_disk_t disk)
{
  struct grub_lvm_vg *vg;
  struct grub_lvm_lv *lv = NULL;
  for (vg = vg_list; vg; vg = vg->next)
    {
      if (vg->lvs)
	for (lv = vg->lvs; lv; lv = lv->next)
	  if (! grub_strcmp (lv->name, name))
	    break;

      if (lv)
	break;
    }

  if (! lv)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Unknown LVM device %s", name);

  disk->has_partitions = 0;
  disk->id = lv->number;
  disk->data = lv;
  disk->total_sectors = lv->size;
  
  return 0;
}

static void
grub_lvm_close (grub_disk_t disk __attribute ((unused)))
{
  return;
}

static grub_err_t
grub_lvm_read (grub_disk_t disk, grub_disk_addr_t sector,
		grub_size_t size, char *buf)
{
  grub_err_t err = 0;
  struct grub_lvm_lv *lv = disk->data;
  struct grub_lvm_vg *vg = lv->vg;
  struct grub_lvm_segment *seg = lv->segments;
  struct grub_lvm_pv *pv;
  grub_uint64_t offset;
  grub_uint64_t extent;
  unsigned int i;

  extent = grub_divmod64 (sector, vg->extent_size, NULL);

  /* Find the right segment.  */
  for (i = 0; i < lv->segment_count; i++)
    {
      if ((seg->start_extent <= extent)
	  && ((seg->start_extent + seg->extent_count) > extent))
	{
	  break;
	}

      seg++;
    }

  if (seg->stripe_count == 1)
    {
      /* This segment is linear, so that's easy.  We just need to find
	 out the offset in the physical volume and read SIZE bytes
	 from that.  */
      struct grub_lvm_stripe *stripe = seg->stripes;
      grub_uint64_t seg_offset; /* Offset of the segment in PV device.  */

      pv = stripe->pv;
      seg_offset = ((grub_uint64_t) stripe->start
		    * (grub_uint64_t) vg->extent_size) + pv->start;

      offset = sector - ((grub_uint64_t) seg->start_extent
			 * (grub_uint64_t) vg->extent_size) + seg_offset;
    }
  else
    {
      /* This is a striped segment. We have to find the right PV
	 similar to RAID0. */
      struct grub_lvm_stripe *stripe = seg->stripes;
      grub_uint32_t a, b;
      grub_uint64_t seg_offset; /* Offset of the segment in PV device.  */
      unsigned int stripenr;

      offset = sector - ((grub_uint64_t) seg->start_extent
			 * (grub_uint64_t) vg->extent_size);

      a = grub_divmod64 (offset, seg->stripe_size, NULL);
      grub_divmod64 (a, seg->stripe_count, &stripenr);

      a = grub_divmod64 (offset, seg->stripe_size * seg->stripe_count, NULL);
      grub_divmod64 (offset, seg->stripe_size, &b);
      offset = a * seg->stripe_size + b;

      stripe += stripenr;
      pv = stripe->pv;
      
      seg_offset = ((grub_uint64_t) stripe->start
		    * (grub_uint64_t) vg->extent_size) + pv->start;

      offset += seg_offset;
    }

  /* Check whether we actually know the physical volume we want to
     read from.  */
  if (pv->disk)
    err = grub_disk_read (pv->disk, offset, 0,
			  size << GRUB_DISK_SECTOR_BITS, buf);
  else
    err = grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		      "Physical volume %s not found", pv->name);
  
  return err;
}

static grub_err_t
grub_lvm_write (grub_disk_t disk __attribute ((unused)),
		 grub_disk_addr_t sector __attribute ((unused)),
		 grub_size_t size __attribute ((unused)),
		 const char *buf __attribute ((unused)))
{
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
}

static int
grub_lvm_scan_device (const char *name)
{
  grub_err_t err;
  grub_disk_t disk;
  grub_uint64_t da_offset, da_size, mda_offset, mda_size;
  char buf[GRUB_LVM_LABEL_SIZE];
  char vg_id[GRUB_LVM_ID_STRLEN+1];
  char pv_id[GRUB_LVM_ID_STRLEN+1];
  char *metadatabuf, *p, *q, *vgname;
  struct grub_lvm_label_header *lh = (struct grub_lvm_label_header *) buf;
  struct grub_lvm_pv_header *pvh;
  struct grub_lvm_disk_locn *dlocn;
  struct grub_lvm_mda_header *mdah;
  struct grub_lvm_raw_locn *rlocn;
  unsigned int i, j, vgname_len;
  struct grub_lvm_vg *vg;
  struct grub_lvm_pv *pv;
  
  disk = grub_disk_open (name);
  if (!disk)
    return 0;

  /* Search for label. */
  for (i = 0; i < GRUB_LVM_LABEL_SCAN_SECTORS; i++)
    {
      err = grub_disk_read (disk, i, 0, sizeof(buf), buf);
      if (err)
	goto fail;
      
      if ((! grub_strncmp ((char *)lh->id, GRUB_LVM_LABEL_ID,
			   sizeof (lh->id)))
	  && (! grub_strncmp ((char *)lh->type, GRUB_LVM_LVM2_LABEL,
			      sizeof (lh->type))))
	break;
    }

  /* Return if we didn't find a label. */
  if (i == GRUB_LVM_LABEL_SCAN_SECTORS)
    goto fail;
  
  pvh = (struct grub_lvm_pv_header *) (buf + grub_le_to_cpu32(lh->offset_xl));

  for (i = 0, j = 0; i < GRUB_LVM_ID_LEN; i++)
    {
      pv_id[j++] = pvh->pv_uuid[i];
      if ((i != 1) && (i != 29) && (i % 4 == 1))
	pv_id[j++] = '-';
    }
  pv_id[j] = '\0';

  dlocn = pvh->disk_areas_xl;
  da_offset = grub_le_to_cpu64 (dlocn->offset);
  da_size = grub_le_to_cpu64 (dlocn->size);

  dlocn++;
  /* Is it possible to have multiple data/metadata areas? I haven't
     seen devices that have it. */
  if (dlocn->offset)
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "We don't support multiple data areas");
		  
      goto fail;
    }

  dlocn++;
  mda_offset = grub_le_to_cpu64 (dlocn->offset);
  mda_size = grub_le_to_cpu64 (dlocn->size);
  dlocn++;
  
  if (dlocn->offset)
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "We don't support multiple metadata areas");
		  
      goto fail;
    }

  /* Allocate buffer space for the circular worst-case scenario. */
  metadatabuf = grub_malloc (2 * mda_size);
  if (! metadatabuf)
    goto fail;

  err = grub_disk_read (disk, 0, mda_offset, mda_size, metadatabuf);
  if (err)
    goto fail2;

  mdah = (struct grub_lvm_mda_header *) metadatabuf;
  if ((grub_strncmp ((char *)mdah->magic, GRUB_LVM_FMTT_MAGIC,
		     sizeof (mdah->magic)))
      || (grub_le_to_cpu32 (mdah->version) != GRUB_LVM_FMTT_VERSION))
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "Unknown metadata header");
      goto fail2;
    }

  rlocn = mdah->raw_locns;
  if (grub_le_to_cpu64 (rlocn->offset) + grub_le_to_cpu64 (rlocn->size) >
      grub_le_to_cpu64 (mdah->size))
    {
      /* Metadata is circular. Copy the wrap in place. */
      grub_memcpy (metadatabuf + mda_size,
                   metadatabuf + GRUB_LVM_MDA_HEADER_SIZE,
                   grub_le_to_cpu64 (rlocn->offset) +
                   grub_le_to_cpu64 (rlocn->size) -
                   grub_le_to_cpu64 (mdah->size));
    }
  p = q = metadatabuf + grub_le_to_cpu64 (rlocn->offset);

  while (*q != ' ' && q < metadatabuf + mda_size)
    q++;

  if (q == metadatabuf + mda_size)
    goto fail2;

  vgname_len = q - p;
  vgname = grub_malloc (vgname_len + 1);
  if (!vgname)
    goto fail2;

  grub_memcpy (vgname, p, vgname_len);
  vgname[vgname_len] = '\0';

  p = grub_strstr (q, "id = \"");
  if (p == NULL)
    goto fail3;
  p += sizeof ("id = \"") - 1;
  grub_memcpy (vg_id, p, GRUB_LVM_ID_STRLEN);
  vg_id[GRUB_LVM_ID_STRLEN] = '\0';

  for (vg = vg_list; vg; vg = vg->next)
    {
      if (! grub_memcmp(vg_id, vg->id, GRUB_LVM_ID_STRLEN))
	break;
    }

  if (! vg)
    {
      /* First time we see this volume group. We've to create the
	 whole volume group structure. */
      vg = grub_malloc (sizeof (*vg));
      if (! vg)
	goto fail3;
      vg->name = vgname;
      grub_memcpy (vg->id, vg_id, GRUB_LVM_ID_STRLEN+1);

      vg->extent_size = grub_lvm_getvalue (&p, "extent_size = ");
      if (p == NULL)
	goto fail4;

      vg->lvs = NULL;
      vg->pvs = NULL;

      p = grub_strstr (p, "physical_volumes {");
      if (p)
	{
	  p += sizeof ("physical_volumes {") - 1;
	  
	  /* Add all the pvs to the volume group. */
	  while (1)
	    {
	      int s;
	      while (grub_isspace (*p))
		p++;
	      
	      if (*p == '}')
		break;
	      
	      pv = grub_malloc (sizeof (*pv));
	      q = p;
	      while (*q != ' ')
		q++;
	      
	      s = q - p;
	      pv->name = grub_malloc (s + 1);
	      grub_memcpy (pv->name, p, s);
	      pv->name[s] = '\0';
	      
	      p = grub_strstr (p, "id = \"");
	      if (p == NULL)
		goto pvs_fail;
	      p += sizeof("id = \"") - 1;
	      
	      grub_memcpy (pv->id, p, GRUB_LVM_ID_STRLEN);
	      pv->id[GRUB_LVM_ID_STRLEN] = '\0';
	      
	      pv->start = grub_lvm_getvalue (&p, "pe_start = ");
	      if (p == NULL)
		goto pvs_fail;
	      
	      p = grub_strchr (p, '}');
	      if (p == NULL)
		goto pvs_fail;
	      p++;

	      pv->disk = NULL;
	      pv->next = vg->pvs;
	      vg->pvs = pv;
	      
	      continue;
	    pvs_fail:
	      grub_free (pv->name);
	      grub_free (pv);
	      goto fail4;
	    }
	}

      p = grub_strstr (p, "logical_volumes");
      if (p)
	{
	  p += 18;
	  
	  /* And add all the lvs to the volume group. */
	  while (1)
	    {
	      int s;
	      struct grub_lvm_lv *lv;
	      struct grub_lvm_segment *seg;
	      
	      while (grub_isspace (*p))
		p++;
	      
	      if (*p == '}')
		break;
	      
	      lv = grub_malloc (sizeof (*lv));
	      
	      q = p;
	      while (*q != ' ')
		q++;
	      
	      s = q - p;
	      lv->name = grub_malloc (vgname_len + 1 + s + 1);
	      grub_memcpy (lv->name, vgname, vgname_len);
	      lv->name[vgname_len] = '-';
	      grub_memcpy (lv->name + vgname_len + 1, p, s);
	      lv->name[vgname_len + 1 + s] = '\0';
	      
	      lv->size = 0;
	      
	      lv->segment_count = grub_lvm_getvalue (&p, "segment_count = ");
	      if (p == NULL)
		goto lvs_fail;
	      lv->segments = grub_malloc (sizeof (*seg) * lv->segment_count);
	      seg = lv->segments;
	      
	      for (i = 0; i < lv->segment_count; i++)
		{
		  struct grub_lvm_stripe *stripe;
		  
		  p = grub_strstr (p, "segment");
		  if (p == NULL)
		    goto lvs_segment_fail;
		  
		  seg->start_extent = grub_lvm_getvalue (&p, "start_extent = ");
		  if (p == NULL)
		    goto lvs_segment_fail;
		  seg->extent_count = grub_lvm_getvalue (&p, "extent_count = ");
		  if (p == NULL)
		    goto lvs_segment_fail;
		  seg->stripe_count = grub_lvm_getvalue (&p, "stripe_count = ");
		  if (p == NULL)
		    goto lvs_segment_fail;
		  
		  lv->size += seg->extent_count * vg->extent_size;
		  
		  if (seg->stripe_count != 1)
		    seg->stripe_size = grub_lvm_getvalue (&p, "stripe_size = ");
		  
		  seg->stripes = grub_malloc (sizeof (*stripe)
					      * seg->stripe_count);
		  stripe = seg->stripes;
		  
		  p = grub_strstr (p, "stripes = [");
		  if (p == NULL)
		    goto lvs_segment_fail2;
		  p += sizeof("stripes = [") - 1;
		  
		  for (j = 0; j < seg->stripe_count; j++)
		    {
		      char *pvname;
		      
		      p = grub_strchr (p, '"');
		      if (p == NULL)
			continue;
		      q = ++p;
		      while (*q != '"')
			q++;

		      s = q - p;
		      
		      pvname = grub_malloc (s + 1);
                      if (pvname == NULL)
                        goto lvs_segment_fail2;
                      
		      grub_memcpy (pvname, p, s);
		      pvname[s] = '\0';
		      
		      if (vg->pvs)
			for (pv = vg->pvs; pv; pv = pv->next)
			  {
			    if (! grub_strcmp (pvname, pv->name))
			      {
				stripe->pv = pv;
				break;
			      }
			  }
		      
		      grub_free(pvname);
		      
		      stripe->start = grub_lvm_getvalue (&p, ",");
		      if (p == NULL)
			continue;
		      
		      stripe++;
		    }
		  
		  seg++;

		  continue;
		lvs_segment_fail2:
		  grub_free (seg->stripes);
		lvs_segment_fail:
		  goto fail4;
		}

	      p = grub_strchr (p, '}');
	      if (p == NULL)
		goto lvs_fail;
	      p += 3;
	      
	      lv->number = lv_count++;
	      lv->vg = vg;
	      lv->next = vg->lvs;
	      vg->lvs = lv;

	      continue;
	    lvs_fail:
	      grub_free (lv->name);
	      grub_free (lv);
	      goto fail4;
	    }
	}

	vg->next = vg_list;
	vg_list = vg;
    }
  else
    {
      grub_free (vgname);
    }

  /* Match the device we are currently reading from with the right
     PV. */
  if (vg->pvs)
    for (pv = vg->pvs; pv; pv = pv->next)
      {
	if (! grub_memcmp (pv->id, pv_id, GRUB_LVM_ID_STRLEN))
	  {
	    pv->disk = grub_disk_open (name);
	    break;
	  }
      }

  goto fail2;

  /* Failure path.  */
 fail4:
  grub_free (vg);
 fail3:
  grub_free (vgname);

  /* Normal exit path.  */
 fail2:
  grub_free (metadatabuf);
 fail:
  grub_disk_close (disk);
  return 0;
}

static struct grub_disk_dev grub_lvm_dev =
  {
    .name = "lvm",
    .id = GRUB_DISK_DEVICE_LVM_ID,
    .iterate = grub_lvm_iterate,
    .open = grub_lvm_open,
    .close = grub_lvm_close,
    .read = grub_lvm_read,
    .write = grub_lvm_write,
#ifdef GRUB_UTIL
    .memberlist = grub_lvm_memberlist,
#endif
    .next = 0
  };


GRUB_MOD_INIT(lvm)
{
  grub_device_iterate (&grub_lvm_scan_device);
  if (grub_errno)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
    }

  grub_disk_dev_register (&grub_lvm_dev);
}

GRUB_MOD_FINI(lvm)
{
  grub_disk_dev_unregister (&grub_lvm_dev);
  /* FIXME: free the lvm list. */
}
