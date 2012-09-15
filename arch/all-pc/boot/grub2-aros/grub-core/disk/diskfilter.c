/* diskfilter.c - module to read RAID arrays.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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
#include <grub/diskfilter.h>
#include <grub/partition.h>
#ifdef GRUB_UTIL
#include <grub/i18n.h>
#include <grub/util/misc.h>
#endif

GRUB_MOD_LICENSE ("GPLv3+");

/* Linked list of DISKFILTER arrays. */
static struct grub_diskfilter_vg *array_list;
grub_raid5_recover_func_t grub_raid5_recover_func;
grub_raid6_recover_func_t grub_raid6_recover_func;
grub_diskfilter_t grub_diskfilter_list;
static int inscnt = 0;
static int lv_num = 0;

static struct grub_diskfilter_lv *
find_lv (const char *name);
static int is_lv_readable (struct grub_diskfilter_lv *lv, int easily);



static grub_err_t
is_node_readable (const struct grub_diskfilter_node *node, int easily)
{
  /* Check whether we actually know the physical volume we want to
     read from.  */
  if (node->pv)
    return !!(node->pv->disk);
  if (node->lv)
    return is_lv_readable (node->lv, easily);
  return 0;
}

static int
is_lv_readable (struct grub_diskfilter_lv *lv, int easily)
{
  unsigned i, j;
  if (!lv)
    return 0;
  for (i = 0; i < lv->segment_count; i++)
    {
      int need = lv->segments[i].node_count, have = 0;
      switch (lv->segments[i].type)
	{
	case GRUB_DISKFILTER_RAID6:
	  if (!easily)
	    need--;
	case GRUB_DISKFILTER_RAID4:
	case GRUB_DISKFILTER_RAID5:
	  if (!easily)
	    need--;
	case GRUB_DISKFILTER_STRIPED:
	  break;

	case GRUB_DISKFILTER_MIRROR:
	  need = 1;
	  break;

	case GRUB_DISKFILTER_RAID10:
	  {
	    unsigned int n;
	    n = lv->segments[i].layout & 0xFF;
	    if (n == 1)
	      n = (lv->segments[i].layout >> 8) & 0xFF;
	    need = lv->segments[i].node_count - n + 1;
	  }
	  break;
	}
	for (j = 0; j < lv->segments[i].node_count; j++)
	  {
	    if (is_node_readable (lv->segments[i].nodes + j, easily))
	      have++;
	    if (have >= need)
	      break;
	  }
	if (have < need)
	  return 0;
    }

  return 1;
}

static grub_err_t
insert_array (grub_disk_t disk, const struct grub_diskfilter_pv_id *id,
	      struct grub_diskfilter_vg *array,
              grub_disk_addr_t start_sector,
	      grub_diskfilter_t diskfilter __attribute__ ((unused)));

static int
is_valid_diskfilter_name (const char *name)
{
  return (grub_memcmp (name, "md", sizeof ("md") - 1) == 0
	  || grub_memcmp (name, "lvm/", sizeof ("lvm/") - 1) == 0
	  || grub_memcmp (name, "ldm/", sizeof ("ldm/") - 1) == 0);
}

static int
scan_disk (const char *name, int accept_diskfilter)
{
  auto int hook (grub_disk_t disk, grub_partition_t p);
  int hook (grub_disk_t disk, grub_partition_t p)
    {
      struct grub_diskfilter_vg *arr;
      grub_disk_addr_t start_sector;
      struct grub_diskfilter_pv_id id;
      grub_diskfilter_t diskfilter;

      grub_dprintf ("diskfilter", "Scanning for DISKFILTER devices on disk %s\n",
		    name);
#ifdef GRUB_UTIL
      grub_util_info ("Scanning for DISKFILTER devices on disk %s", name);
#endif

      disk->partition = p;
      
      for (arr = array_list; arr != NULL; arr = arr->next)
	{
	  struct grub_diskfilter_pv *m;
	  for (m = arr->pvs; m; m = m->next)
	    if (m->disk && m->disk->id == disk->id
		&& m->disk->dev->id == disk->dev->id
		&& m->part_start == grub_partition_get_start (disk->partition)
		&& m->part_size == grub_disk_get_size (disk))
	      return 0;
	}

      for (diskfilter = grub_diskfilter_list; diskfilter; diskfilter = diskfilter->next)
	{
#ifdef GRUB_UTIL
	  grub_util_info ("Scanning for %s devices on disk %s", 
			  diskfilter->name, name);
#endif
	  id.uuid = 0;
	  id.uuidlen = 0;
	  arr = diskfilter->detect (disk, &id, &start_sector);
	  if (arr &&
	      (! insert_array (disk, &id, arr, start_sector, diskfilter)))
	    {
	      if (id.uuidlen)
		grub_free (id.uuid);
	      return 0;
	    }
	  if (arr && id.uuidlen)
	    grub_free (id.uuid);

	  /* This error usually means it's not diskfilter, no need to display
	     it.  */
	  if (grub_errno != GRUB_ERR_OUT_OF_RANGE)
	    grub_print_error ();

	  grub_errno = GRUB_ERR_NONE;
	}

      return 0;
    }
  grub_disk_t disk;
  static int scan_depth = 0;

  if (!accept_diskfilter && is_valid_diskfilter_name (name))
    return 0;

  if (scan_depth > 100)
    return 0;

  scan_depth++;
  disk = grub_disk_open (name);
  if (!disk)
    {
      grub_errno = GRUB_ERR_NONE;
      scan_depth--;
      return 0;
    }
  if (hook (disk, 0))
    {
      scan_depth--;
      return 1;
    }
  if (grub_partition_iterate (disk, hook))
    {
      scan_depth--;
      return 1;
    }
  grub_disk_close (disk);
  scan_depth--;
  return 0;
}

static int
scan_disk_hook (const char *name)
{
  return scan_disk (name, 0);
}

static void
scan_devices (const char *arname)
{
  grub_disk_dev_t p;
  grub_disk_pull_t pull;
  struct grub_diskfilter_vg *vg;
  struct grub_diskfilter_lv *lv = NULL;

  for (pull = 0; pull < GRUB_DISK_PULL_MAX; pull++)
    for (p = grub_disk_dev_list; p; p = p->next)
      if (p->id != GRUB_DISK_DEVICE_DISKFILTER_ID
	  && p->iterate)
	{
	  if ((p->iterate) (scan_disk_hook, pull))
	    return;
	  if (arname && is_lv_readable (find_lv (arname), 1))
	    return;
	}

  for (vg = array_list; vg; vg = vg->next)
    {
      if (vg->lvs)
	for (lv = vg->lvs; lv; lv = lv->next)
	  if (!lv->scanned && lv->fullname && lv->became_readable_at)
	    {
	      scan_disk (lv->fullname, 1);
	      lv->scanned = 1;
	    }
    }
}

static int
grub_diskfilter_iterate (int (*hook) (const char *name),
		   grub_disk_pull_t pull)
{
  struct grub_diskfilter_vg *array;
  int islcnt = 0;

  if (pull == GRUB_DISK_PULL_RESCAN)
    {
      islcnt = inscnt + 1;
      scan_devices (NULL);
    }

  if (pull != GRUB_DISK_PULL_NONE && pull != GRUB_DISK_PULL_RESCAN)
    return 0;

  for (array = array_list; array; array = array->next)
    {
      struct grub_diskfilter_lv *lv;
      if (array->lvs)
	for (lv = array->lvs; lv; lv = lv->next)
	  if (lv->visible && lv->fullname && lv->became_readable_at >= islcnt)
	    {
	      if (hook (lv->fullname))
		return 1;
	    }
    }

  return 0;
}

#ifdef GRUB_UTIL
static grub_disk_memberlist_t
grub_diskfilter_memberlist (grub_disk_t disk)
{
  struct grub_diskfilter_lv *lv = disk->data;
  grub_disk_memberlist_t list = NULL, tmp;
  struct grub_diskfilter_pv *pv;
  grub_disk_pull_t pull;
  grub_disk_dev_t p;
  struct grub_diskfilter_vg *vg;
  struct grub_diskfilter_lv *lv2 = NULL;

  if (!lv->vg->pvs)
    return NULL;

  pv = lv->vg->pvs;
  while (pv && pv->disk)
    pv = pv->next;

  for (pull = 0; pv && pull < GRUB_DISK_PULL_MAX; pull++)
    for (p = grub_disk_dev_list; pv && p; p = p->next)
      if (p->id != GRUB_DISK_DEVICE_DISKFILTER_ID
	  && p->iterate)
	{
	  (p->iterate) (scan_disk_hook, pull);
	  while (pv && pv->disk)
	    pv = pv->next;
	}

  for (vg = array_list; pv && vg; vg = vg->next)
    {
      if (vg->lvs)
	for (lv2 = vg->lvs; pv && lv2; lv2 = lv2->next)
	  if (!lv2->scanned && lv2->fullname && lv2->became_readable_at)
	    {
	      scan_disk (lv2->fullname, 1);
	      lv2->scanned = 1;
	      while (pv && pv->disk)
		pv = pv->next;
	    }
    }

  for (pv = lv->vg->pvs; pv; pv = pv->next)
    {
      if (!pv->disk)
	{
	  /* TRANSLATORS: This message kicks in during the detection of
	     which modules needs to be included in core image. This happens
	     in the case of degraded RAID and means that autodetection may
	     fail to include some of modules. It's an installation time
	     message, not runtime message.  */
	  grub_util_warn (_("Couldn't find physical volume `%s'."
			    " Some modules may be missing from core image."),
			  pv->name);
	  continue;
	}
      tmp = grub_malloc (sizeof (*tmp));
      tmp->disk = pv->disk;
      tmp->next = list;
      list = tmp;
    }

  return list;
}

void
grub_diskfilter_print_partmap (grub_disk_t disk)
{
  struct grub_diskfilter_lv *lv = disk->data;
  struct grub_diskfilter_pv *pv;

  if (lv->vg->pvs)
    for (pv = lv->vg->pvs; pv; pv = pv->next)
      {
	grub_size_t s;
	if (!pv->disk)
	  {
	    /* TRANSLATORS: This message kicks in during the detection of
	       which modules needs to be included in core image. This happens
	       in the case of degraded RAID and means that autodetection may
	       fail to include some of modules. It's an installation time
	       message, not runtime message.  */
	    grub_util_warn (_("Couldn't find physical volume `%s'."
			      " Some modules may be missing from core image."),
			    pv->name);
	    continue;
	  }
	for (s = 0; pv->partmaps[s]; s++)
	  grub_printf ("%s ", pv->partmaps[s]);
      }
}

static const char *
grub_diskfilter_getname (struct grub_disk *disk)
{
  struct grub_diskfilter_lv *array = disk->data;

  return array->vg->driver->name;
}
#endif

static inline int
ascii2hex (char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return 0;
}

static struct grub_diskfilter_lv *
find_lv (const char *name)
{
  struct grub_diskfilter_vg *vg;
  struct grub_diskfilter_lv *lv = NULL;

  if (grub_memcmp (name, "mduuid/", sizeof ("mduuid/") - 1) == 0)
    {
      const char *uuidstr = name + sizeof ("mduuid/") - 1;
      grub_size_t uuid_len = grub_strlen (uuidstr) / 2;
      grub_uint8_t uuidbin[uuid_len];
      unsigned i;
      for (i = 0; i < uuid_len; i++)
	uuidbin[i] = ascii2hex (uuidstr[2 * i + 1])
	  | (ascii2hex (uuidstr[2 * i]) << 4);

      for (vg = array_list; vg; vg = vg->next)      
	{
	  if (uuid_len == vg->uuid_len
	      && grub_memcmp (uuidbin, vg->uuid, uuid_len) == 0)
	    if (is_lv_readable (vg->lvs, 0))
	      return vg->lvs;
	}
    }

  for (vg = array_list; vg; vg = vg->next)
    {
      if (vg->lvs)
	for (lv = vg->lvs; lv; lv = lv->next)
	  if (lv->fullname && grub_strcmp (lv->fullname, name) == 0
	      && is_lv_readable (lv, 0))
	    return lv;
    }
  return NULL;
}

static grub_err_t
grub_diskfilter_open (const char *name, grub_disk_t disk)
{
  struct grub_diskfilter_lv *lv;

  if (grub_memcmp (name, "md", sizeof ("md") - 1) != 0
      && grub_memcmp (name, "lvm/", sizeof ("lvm/") - 1) != 0
      && grub_memcmp (name, "ldm/", sizeof ("ldm/") - 1) != 0)
     return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "unknown DISKFILTER device %s",
			name);

  lv = find_lv (name);

  if (! lv)
    {
      scan_devices (name);
      if (grub_errno)
	{
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;
	}
      lv = find_lv (name);
    }

  if (!lv)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "unknown DISKFILTER device %s",
                       name);

  disk->id = lv->number;
  disk->data = lv;

  disk->total_sectors = lv->size;
  return 0;
}

static void
grub_diskfilter_close (grub_disk_t disk __attribute ((unused)))
{
  return;
}

static grub_err_t
read_lv (struct grub_diskfilter_lv *lv, grub_disk_addr_t sector,
	 grub_size_t size, char *buf);

grub_err_t
grub_diskfilter_read_node (const struct grub_diskfilter_node *node,
			   grub_disk_addr_t sector,
			   grub_size_t size, char *buf)
{
  /* Check whether we actually know the physical volume we want to
     read from.  */
  if (node->pv)
    {
      if (node->pv->disk)
	return grub_disk_read (node->pv->disk, sector + node->start
			       + node->pv->start_sector,
			       0, size << GRUB_DISK_SECTOR_BITS, buf);
      else
	return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
			   N_("physical volume %s not found"), node->pv->name);

    }
  if (node->lv)
    return read_lv (node->lv, sector + node->start, size, buf);
  return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "unknown node '%s'", node->name);
}

static grub_err_t
read_segment (struct grub_diskfilter_segment *seg, grub_disk_addr_t sector,
	      grub_size_t size, char *buf)
{
  grub_err_t err;
  switch (seg->type)
    {
    case GRUB_DISKFILTER_STRIPED:
      if (seg->node_count == 1)
	return grub_diskfilter_read_node (&seg->nodes[0],
					  sector, size, buf);
    case GRUB_DISKFILTER_MIRROR:
    case GRUB_DISKFILTER_RAID10:
      {
	grub_disk_addr_t read_sector, far_ofs;
	grub_uint64_t disknr, b, near, far, ofs;
	unsigned int i, j;
	    
	read_sector = grub_divmod64 (sector, seg->stripe_size, &b);
	far = ofs = near = 1;
	far_ofs = 0;

	if (seg->type == 1)
	  near = seg->node_count;
	else if (seg->type == 10)
	  {
	    near = seg->layout & 0xFF;
	    far = (seg->layout >> 8) & 0xFF;
	    if (seg->layout >> 16)
	      {
		ofs = far;
		far_ofs = 1;
	      }
	    else
	      far_ofs = grub_divmod64 (seg->raid_member_size,
				       far * seg->stripe_size, 0);
		
	    far_ofs *= seg->stripe_size;
	  }

	read_sector = grub_divmod64 (read_sector * near, 
				     seg->node_count,
				     &disknr);

	ofs *= seg->stripe_size;
	read_sector *= ofs;
	
	while (1)
	  {
	    grub_size_t read_size;

	    read_size = seg->stripe_size - b;
	    if (read_size > size)
	      read_size = size;

	    err = 0;
	    for (i = 0; i < near; i++)
	      {
		unsigned int k;

		k = disknr;
		err = 0;
		for (j = 0; j < far; j++)
		  {
		    if (grub_errno == GRUB_ERR_READ_ERROR
			|| grub_errno == GRUB_ERR_UNKNOWN_DEVICE)
		      grub_errno = GRUB_ERR_NONE;

		    err = grub_diskfilter_read_node (&seg->nodes[k],
						     read_sector
						     + j * far_ofs + b,
						     read_size,
						     buf);
		    if (! err)
		      break;
		    else if (err != GRUB_ERR_READ_ERROR
			     && err != GRUB_ERR_UNKNOWN_DEVICE)
		      return err;
		    k++;
		    if (k == seg->node_count)
		      k = 0;
		  }

		if (! err)
		  break;

		disknr++;
		if (disknr == seg->node_count)
		  {
		    disknr = 0;
		    read_sector += ofs;
		  }
	      }

	    if (err)
	      return err;

	    buf += read_size << GRUB_DISK_SECTOR_BITS;
	    size -= read_size;
	    if (! size)
	      return GRUB_ERR_NONE;
	    
	    b = 0;
	    disknr += (near - i);
	    while (disknr >= seg->node_count)
	      {
		disknr -= seg->node_count;
		read_sector += ofs;
	      }
	  }
      }

    case GRUB_DISKFILTER_RAID4:
    case GRUB_DISKFILTER_RAID5:
    case GRUB_DISKFILTER_RAID6:
      {
	grub_disk_addr_t read_sector;
	grub_uint64_t b, p, n, disknr, e;

	/* n = 1 for level 4 and 5, 2 for level 6.  */
	n = seg->type / 3;

	/* Find the first sector to read. */
	read_sector = grub_divmod64 (sector, seg->stripe_size, &b);
	read_sector = grub_divmod64 (read_sector, seg->node_count - n,
				     &disknr);
	if (seg->type >= 5)
	  {
	    grub_divmod64 (read_sector, seg->node_count, &p);

	    if (! (seg->layout & GRUB_RAID_LAYOUT_RIGHT_MASK))
	      p = seg->node_count - 1 - p;

	    if (seg->layout & GRUB_RAID_LAYOUT_SYMMETRIC_MASK)
	      {
		disknr += p + n;
	      }
	    else
	      {
		grub_uint32_t q;

		q = p + (n - 1);
		if (q >= seg->node_count)
		  q -= seg->node_count;

		if (disknr >= p)
		  disknr += n;
		else if (disknr >= q)
		  disknr += q + 1;
	      }

	    if (disknr >= seg->node_count)
	      disknr -= seg->node_count;
	  }
	else
	  p = seg->node_count - n;
	read_sector *= seg->stripe_size;

	while (1)
	  {
	    grub_size_t read_size;
	    int next_level;
	    
	    read_size = seg->stripe_size - b;
	    if (read_size > size)
	      read_size = size;

	    e = 0;
	    /* Reset read error.  */
	    if (grub_errno == GRUB_ERR_READ_ERROR
		|| grub_errno == GRUB_ERR_UNKNOWN_DEVICE)
	      grub_errno = GRUB_ERR_NONE;

	    err = grub_diskfilter_read_node (&seg->nodes[disknr],
					     read_sector + b,
					     read_size,
					     buf);

	    if ((err) && (err != GRUB_ERR_READ_ERROR
			  && err != GRUB_ERR_UNKNOWN_DEVICE))
	      return err;
	    e++;

	    if (err)
	      {
		grub_errno = GRUB_ERR_NONE;
		if (seg->type == GRUB_DISKFILTER_RAID6)
		  {
		    err = ((grub_raid6_recover_func) ?
			   (*grub_raid6_recover_func) (seg, disknr, p,
						       buf, read_sector + b,
						       read_size) :
			   grub_error (GRUB_ERR_BAD_DEVICE,
				       N_("module `%s' isn't loaded"),
				       "raid6rec"));
		  }
		else
		  {
		    err = ((grub_raid5_recover_func) ?
			   (*grub_raid5_recover_func) (seg, disknr,
						       buf, read_sector + b,
						       read_size) :
			   grub_error (GRUB_ERR_BAD_DEVICE,
				       N_("module `%s' isn't loaded"),
				       "raid5rec"));
		  }

		if (err)
		  return err;
	      }

	    buf += read_size << GRUB_DISK_SECTOR_BITS;
	    size -= read_size;
	    sector += read_size;
	    if (! size)
	      break;

	    b = 0;
	    disknr++;

	    if (seg->layout & GRUB_RAID_LAYOUT_SYMMETRIC_MASK)
	      {
		if (disknr == seg->node_count)
		  disknr = 0;

		next_level = (disknr == p);
	      }
	    else
	      {
		if (disknr == p)
		  disknr += n;

		next_level = (disknr >= seg->node_count);
	      }

	    if (next_level)
	      {
		read_sector += seg->stripe_size;

		if (seg->type >= 5)
		  {
		    if (seg->layout & GRUB_RAID_LAYOUT_RIGHT_MASK)
		      p = (p == seg->node_count - 1) ? 0 : p + 1;
		    else
		      p = (p == 0) ? seg->node_count - 1 : p - 1;

		    if (seg->layout & GRUB_RAID_LAYOUT_SYMMETRIC_MASK)
		      {
			disknr = p + n;
			if (disknr >= seg->node_count)
			  disknr -= seg->node_count;
		      }
		    else
		      {
			disknr -= seg->node_count;
			if ((disknr >= p && disknr < p + n)
			    || (disknr + seg->node_count >= p
				&& disknr + seg->node_count < p + n))
			  disknr = p + n;
			if (disknr >= seg->node_count)
			  disknr -= seg->node_count;
		      }
		  }
		else
		  disknr = 0;
	      }
	  }
      }   
      return GRUB_ERR_NONE;
    default:
      return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			 "unsupported RAID level %d", seg->type);
    }
}

static grub_err_t
read_lv (struct grub_diskfilter_lv *lv, grub_disk_addr_t sector,
	 grub_size_t size, char *buf)
{
  if (!lv)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "unknown volume");

  while (size)
    {
      grub_err_t err = 0;
      struct grub_diskfilter_vg *vg = lv->vg;
      struct grub_diskfilter_segment *seg = lv->segments;
      grub_uint64_t extent;
      grub_uint64_t to_read;

      extent = grub_divmod64 (sector, vg->extent_size, NULL);
      
      /* Find the right segment.  */
      {
	unsigned int i;
	for (i = 0; i < lv->segment_count; i++)
	  {
	    if ((seg->start_extent <= extent)
		&& ((seg->start_extent + seg->extent_count) > extent))
	      break;
	    seg++;
	  }
	if (i == lv->segment_count)
	  return grub_error (GRUB_ERR_READ_ERROR, "incorrect segment");
      }
      to_read = ((seg->start_extent + seg->extent_count)
		 * vg->extent_size) - sector;
      if (to_read > size)
	to_read = size;

      err = read_segment (seg, sector - seg->start_extent * vg->extent_size,
			  to_read, buf);
      if (err)
	return err;

      size -= to_read;
      sector += to_read;
      buf += to_read << GRUB_DISK_SECTOR_BITS;
    }
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_diskfilter_read (grub_disk_t disk, grub_disk_addr_t sector,
		      grub_size_t size, char *buf)
{
  return read_lv (disk->data, sector, size, buf);
}

static grub_err_t
grub_diskfilter_write (grub_disk_t disk __attribute ((unused)),
		 grub_disk_addr_t sector __attribute ((unused)),
		 grub_size_t size __attribute ((unused)),
		 const char *buf __attribute ((unused)))
{
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
}

struct grub_diskfilter_vg *
grub_diskfilter_get_vg_by_uuid (grub_size_t uuidlen, char *uuid)
{
  struct grub_diskfilter_vg *p;

  for (p = array_list; p != NULL; p = p->next)
    if ((p->uuid_len == uuidlen) &&
        (! grub_memcmp (p->uuid, uuid, p->uuid_len)))
      return p;
  return NULL;
}

grub_err_t
grub_diskfilter_vg_register (struct grub_diskfilter_vg *vg)
{
  struct grub_diskfilter_lv *lv, *p;
  struct grub_diskfilter_vg *vgp;
  unsigned i;

  grub_dprintf ("diskfilter", "Found array %s\n", vg->name);
#ifdef GRUB_UTIL
  grub_util_info ("Found array %s", vg->name);
#endif

  for (lv = vg->lvs; lv; lv = lv->next)
    {
      lv->number = lv_num++;

      if (lv->fullname)
	{
	  grub_size_t len;
	  int max_used_number = 0, need_new_name = 0;
	  len = grub_strlen (lv->fullname);
	  for (vgp = array_list; vgp; vgp = vgp->next)
	    for (p = vgp->lvs; p; p = p->next)
	      {
		int cur_num;
		char *num, *end;
		if (!p->fullname)
		  continue;
		if (grub_strncmp (p->fullname, lv->fullname, len) != 0)
		  continue;
		if (p->fullname[len] == 0)
		  {
		    need_new_name = 1;
		    continue;
		  }
		num = p->fullname + len + 1;
		if (!grub_isdigit (num[0]))
		  continue;
		cur_num = grub_strtoul (num, &end, 10);
		if (end[0])
		  continue;
		if (cur_num > max_used_number)
		  max_used_number = cur_num;
	      }
	  if (need_new_name)
	    {
	      char *tmp;
	      tmp = grub_xasprintf ("%s_%d", lv->fullname, max_used_number + 1);
	      if (!tmp)
		return grub_errno;
	      grub_free (lv->fullname);
	      lv->fullname = tmp;
	    }
	}
      /* RAID 1 doesn't use a chunksize but code assumes one so set
	 one. */
      for (i = 0; i < lv->segment_count; i++)
	if (lv->segments[i].type == 1)
	  lv->segments[i].stripe_size = 64;
      lv->vg = vg;
    }
  /* Add our new array to the list.  */
  vg->next = array_list;
  array_list = vg;
  return GRUB_ERR_NONE;
}

struct grub_diskfilter_vg *
grub_diskfilter_make_raid (grub_size_t uuidlen, char *uuid, int nmemb,
			   char *name, grub_uint64_t disk_size,
			   grub_uint64_t stripe_size,
			   int layout, int level)
{
  struct grub_diskfilter_vg *array;
  int i;
  grub_uint64_t totsize;
  struct grub_diskfilter_pv *pv;
  grub_err_t err;

  switch (level)
    {
    case 1:
      totsize = disk_size;
      break;

    case 10:
      {
	int n;
	n = layout & 0xFF;
	if (n == 1)
	  n = (layout >> 8) & 0xFF;

	totsize = grub_divmod64 (nmemb * disk_size, n, 0);
      }
      break;

    case 0:
    case 4:
    case 5:
    case 6:
      totsize = (nmemb - level / 3) * disk_size;
      break;

    default:
      return NULL;
    }

  array = grub_diskfilter_get_vg_by_uuid (uuidlen, uuid);
  if (array)
    {
      if (array->lvs && array->lvs->size < totsize)
	{
	  array->lvs->size = totsize;
	  if (array->lvs->segments)
	    array->lvs->segments->extent_count = totsize;
	}

      if (array->lvs->segments
	  && array->lvs->segments->raid_member_size > disk_size)
	array->lvs->segments->raid_member_size = disk_size;

      grub_free (uuid);
      return array;
    }
  array = grub_zalloc (sizeof (*array));
  if (!array)
    return NULL;
  array->uuid = uuid;
  array->uuid_len = uuidlen;
  if (name)
    {
      /* Strip off the homehost if present.  */
      char *colon = grub_strchr (name, ':');
      char *new_name = grub_xasprintf ("md/%s",
				       colon ? colon + 1 : name);

      if (! new_name)
	goto fail;

      array->name = new_name;
    }
  array->extent_size = 1;
  array->lvs = grub_zalloc (sizeof (*array->lvs));
  if (!array->lvs)
    goto fail;
  array->lvs->segment_count = 1;
  array->lvs->visible = 1;
  array->lvs->name = array->name;
  array->lvs->fullname = array->name;

  array->lvs->size = totsize;

  array->lvs->segments = grub_zalloc (sizeof (*array->lvs->segments));
  if (!array->lvs->segments)
    goto fail;
  array->lvs->segments->stripe_size = stripe_size;
  array->lvs->segments->layout = layout;
  array->lvs->segments->start_extent = 0;
  array->lvs->segments->extent_count = totsize;
  array->lvs->segments->type = level;
  array->lvs->segments->node_count = nmemb;
  array->lvs->segments->raid_member_size = disk_size;
  array->lvs->segments->nodes
    = grub_zalloc (nmemb * sizeof (array->lvs->segments->nodes[0]));
  array->lvs->segments->stripe_size = stripe_size;
  for (i = 0; i < nmemb; i++)
    {
      pv = grub_zalloc (sizeof (*pv));
      if (!pv)
	goto fail;
      pv->id.uuidlen = 0;
      pv->id.id = i;
      pv->next = array->pvs;
      array->pvs = pv;
      array->lvs->segments->nodes[i].pv = pv;
    }

  err = grub_diskfilter_vg_register (array);
  if (err)
    goto fail;

  return array;

 fail:
  grub_free (array->lvs);
  while (array->pvs)
    {
      pv = array->pvs->next;
      grub_free (array->pvs);
      array->pvs = pv;
    }
  grub_free (array);
  return NULL;
}

static grub_err_t
insert_array (grub_disk_t disk, const struct grub_diskfilter_pv_id *id,
	      struct grub_diskfilter_vg *array,
              grub_disk_addr_t start_sector,
	      grub_diskfilter_t diskfilter __attribute__ ((unused)))
{
  struct grub_diskfilter_pv *pv;

  grub_dprintf ("diskfilter", "Inserting %s into %s (%s)\n", disk->name,
		array->name, diskfilter->name);
#ifdef GRUB_UTIL
  grub_util_info ("Inserting %s into %s (%s)\n", disk->name,
		  array->name, diskfilter->name);
  array->driver = diskfilter;
#endif

  for (pv = array->pvs; pv; pv = pv->next)
    if (id->uuidlen == pv->id.uuidlen
	&& id->uuidlen 
	? (grub_memcmp (pv->id.uuid, id->uuid, id->uuidlen) == 0) 
	: (pv->id.id == id->id))
      {
	struct grub_diskfilter_lv *lv;
	/* FIXME: Check whether the update time of the superblocks are
	   the same.  */
	if (pv->disk && grub_disk_get_size (disk) >= pv->part_size)
	  return GRUB_ERR_NONE;
	pv->disk = grub_disk_open (disk->name);
	if (!pv->disk)
	  return grub_errno;
	/* This could happen to LVM on RAID, pv->disk points to the
	   raid device, we shouldn't change it.  */
	pv->start_sector -= pv->part_start;
	pv->part_start = grub_partition_get_start (disk->partition);
	pv->part_size = grub_disk_get_size (disk);

#ifdef GRUB_UTIL
	{
	  grub_size_t s = 1;
	  grub_partition_t p;
	  for (p = disk->partition; p; p = p->parent)
	    s++;
	  pv->partmaps = xmalloc (s * sizeof (pv->partmaps[0]));
	  s = 0;
	  for (p = disk->partition; p; p = p->parent)
	    pv->partmaps[s++] = xstrdup (p->partmap->name);
	  pv->partmaps[s++] = 0;
	}
#endif
	if (start_sector != (grub_uint64_t)-1)
	  pv->start_sector = start_sector;
	pv->start_sector += pv->part_start;
	/* Add the device to the array. */
	for (lv = array->lvs; lv; lv = lv->next)
	  if (!lv->became_readable_at && lv->fullname && is_lv_readable (lv, 0))
	    {
	      lv->became_readable_at = ++inscnt;
	      if (is_lv_readable (lv, 1))
		{
		  scan_disk (lv->fullname, 1);
		  lv->scanned = 1;
		}
	    }
	break;
      }

  return 0;
}

static void
free_array (void)
{
  while (array_list)
    {
      struct grub_diskfilter_vg *vg;
      struct grub_diskfilter_pv *pv;
      struct grub_diskfilter_lv *lv;

      vg = array_list;
      array_list = array_list->next;

      while ((pv = vg->pvs))
	{
	  vg->pvs = pv->next;
	  grub_free (pv->name);
	  if (pv->disk)
	    grub_disk_close (pv->disk);
	  if (pv->id.uuidlen)
	    grub_free (pv->id.uuid);
	  grub_free (pv->internal_id);
	  grub_free (pv);
	}

      while ((lv = vg->lvs))
	{
	  unsigned i;
	  vg->lvs = lv->next;
	  if (lv->name != lv->fullname)
	    grub_free (lv->fullname);
	  if (lv->name != vg->name)
	    grub_free (lv->name);
	  for (i = 0; i < lv->segment_count; i++)
	    grub_free (lv->segments[i].nodes);
	  grub_free (lv->segments);
	  grub_free (lv->internal_id);
	  grub_free (lv);
	}

      grub_free (vg->uuid);
      grub_free (vg->name);
      grub_free (vg);
    }

  array_list = 0;
}

#ifdef GRUB_UTIL
struct grub_diskfilter_pv *
grub_diskfilter_get_pv_from_disk (grub_disk_t disk,
				  struct grub_diskfilter_vg **vg_out)
{
  struct grub_diskfilter_pv *pv;
  struct grub_diskfilter_vg *vg;

  scan_disk (disk->name, 1);
  for (vg = array_list; vg; vg = vg->next)
    for (pv = vg->pvs; pv; pv = pv->next)
      {
	if (pv->disk && pv->disk->id == disk->id
	    && pv->disk->dev->id == disk->dev->id
	    && pv->part_start == grub_partition_get_start (disk->partition)
	    && pv->part_size == grub_disk_get_size (disk))
	  {
	    if (vg_out)
	      *vg_out = vg;
	    return pv;
	  }
      }
  return NULL;
}
#endif

static struct grub_disk_dev grub_diskfilter_dev =
  {
    .name = "diskfilter",
    .id = GRUB_DISK_DEVICE_DISKFILTER_ID,
    .iterate = grub_diskfilter_iterate,
    .open = grub_diskfilter_open,
    .close = grub_diskfilter_close,
    .read = grub_diskfilter_read,
    .write = grub_diskfilter_write,
#ifdef GRUB_UTIL
    .memberlist = grub_diskfilter_memberlist,
    .raidname = grub_diskfilter_getname,
#endif
    .next = 0
  };


GRUB_MOD_INIT(diskfilter)
{
  grub_disk_dev_register (&grub_diskfilter_dev);
}

GRUB_MOD_FINI(diskfilter)
{
  grub_disk_dev_unregister (&grub_diskfilter_dev);
  free_array ();
}
