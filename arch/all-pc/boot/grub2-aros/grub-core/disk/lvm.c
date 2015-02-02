/* lvm.c - module to read Logical Volumes.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009,2011  Free Software Foundation, Inc.
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
#include <grub/partition.h>
#include <grub/i18n.h>

#ifdef GRUB_UTIL
#include <grub/emu/misc.h>
#include <grub/emu/hostdisk.h>
#endif

GRUB_MOD_LICENSE ("GPLv3+");


/* Go the string STR and return the number after STR.  *P will point
   at the number.  In case STR is not found, *P will be NULL and the
   return value will be 0.  */
static grub_uint64_t
grub_lvm_getvalue (char **p, const char *str)
{
  *p = grub_strstr (*p, str);
  if (! *p)
    return 0;
  *p += grub_strlen (str);
  return grub_strtoull (*p, p, 10);
}

#if 0
static int
grub_lvm_checkvalue (char **p, char *str, char *tmpl)
{
  int tmpllen = grub_strlen (tmpl);
  *p = grub_strstr (*p, str);
  if (! *p)
    return 0;
  *p += grub_strlen (str);
  if (**p != '"')
    return 0;
  return (grub_memcmp (*p + 1, tmpl, tmpllen) == 0 && (*p)[tmpllen + 1] == '"');
}
#endif

static int
grub_lvm_check_flag (char *p, const char *str, const char *flag)
{
  grub_size_t len_str = grub_strlen (str), len_flag = grub_strlen (flag);
  while (1)
    {
      char *q;
      p = grub_strstr (p, str);
      if (! p)
	return 0;
      p += len_str;
      if (grub_memcmp (p, " = [", sizeof (" = [") - 1) != 0)
	continue;
      q = p + sizeof (" = [") - 1;
      while (1)
	{
	  while (grub_isspace (*q))
	    q++;
	  if (*q != '"')
	    return 0;
	  q++;
	  if (grub_memcmp (q, flag, len_flag) == 0 && q[len_flag] == '"')
	    return 1;
	  while (*q != '"')
	    q++;
	  q++;
	  if (*q == ']')
	    return 0;
	  q++;
	}
    }
}

static struct grub_diskfilter_vg * 
grub_lvm_detect (grub_disk_t disk,
		 struct grub_diskfilter_pv_id *id,
		 grub_disk_addr_t *start_sector)
{
  grub_err_t err;
  grub_uint64_t mda_offset, mda_size;
  char buf[GRUB_LVM_LABEL_SIZE];
  char vg_id[GRUB_LVM_ID_STRLEN+1];
  char pv_id[GRUB_LVM_ID_STRLEN+1];
  char *metadatabuf, *p, *q, *vgname;
  struct grub_lvm_label_header *lh = (struct grub_lvm_label_header *) buf;
  struct grub_lvm_pv_header *pvh;
  struct grub_lvm_disk_locn *dlocn;
  struct grub_lvm_mda_header *mdah;
  struct grub_lvm_raw_locn *rlocn;
  unsigned int i, j;
  grub_size_t vgname_len;
  struct grub_diskfilter_vg *vg;
  struct grub_diskfilter_pv *pv;

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
    {
#ifdef GRUB_UTIL
      grub_util_info ("no LVM signature found");
#endif
      goto fail;
    }

  pvh = (struct grub_lvm_pv_header *) (buf + grub_le_to_cpu32(lh->offset_xl));

  for (i = 0, j = 0; i < GRUB_LVM_ID_LEN; i++)
    {
      pv_id[j++] = pvh->pv_uuid[i];
      if ((i != 1) && (i != 29) && (i % 4 == 1))
	pv_id[j++] = '-';
    }
  pv_id[j] = '\0';

  dlocn = pvh->disk_areas_xl;

  dlocn++;
  /* Is it possible to have multiple data/metadata areas? I haven't
     seen devices that have it. */
  if (dlocn->offset)
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "we don't support multiple LVM data areas");

#ifdef GRUB_UTIL
      grub_util_info ("we don't support multiple LVM data areas\n");
#endif
      goto fail;
    }

  dlocn++;
  mda_offset = grub_le_to_cpu64 (dlocn->offset);
  mda_size = grub_le_to_cpu64 (dlocn->size);

  /* It's possible to have multiple copies of metadata areas, we just use the
     first one.  */

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
		  "unknown LVM metadata header");
#ifdef GRUB_UTIL
      grub_util_info ("unknown LVM metadata header\n");
#endif
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
    {
#ifdef GRUB_UTIL
      grub_util_info ("error parsing metadata\n");
#endif
      goto fail2;
    }

  vgname_len = q - p;
  vgname = grub_malloc (vgname_len + 1);
  if (!vgname)
    goto fail2;

  grub_memcpy (vgname, p, vgname_len);
  vgname[vgname_len] = '\0';

  p = grub_strstr (q, "id = \"");
  if (p == NULL)
    {
#ifdef GRUB_UTIL
      grub_util_info ("couldn't find ID\n");
#endif
      goto fail3;
    }
  p += sizeof ("id = \"") - 1;
  grub_memcpy (vg_id, p, GRUB_LVM_ID_STRLEN);
  vg_id[GRUB_LVM_ID_STRLEN] = '\0';

  vg = grub_diskfilter_get_vg_by_uuid (GRUB_LVM_ID_STRLEN, vg_id);

  if (! vg)
    {
      /* First time we see this volume group. We've to create the
	 whole volume group structure. */
      vg = grub_malloc (sizeof (*vg));
      if (! vg)
	goto fail3;
      vg->name = vgname;
      vg->uuid = grub_malloc (GRUB_LVM_ID_STRLEN);
      if (! vg->uuid)
	goto fail3;
      grub_memcpy (vg->uuid, vg_id, GRUB_LVM_ID_STRLEN);
      vg->uuid_len = GRUB_LVM_ID_STRLEN;

      vg->extent_size = grub_lvm_getvalue (&p, "extent_size = ");
      if (p == NULL)
	{
#ifdef GRUB_UTIL
	  grub_util_info ("unknown extent size\n");
#endif
	  goto fail4;
	}

      vg->lvs = NULL;
      vg->pvs = NULL;

      p = grub_strstr (p, "physical_volumes {");
      if (p)
	{
	  p += sizeof ("physical_volumes {") - 1;

	  /* Add all the pvs to the volume group. */
	  while (1)
	    {
	      grub_ssize_t s;
	      while (grub_isspace (*p))
		p++;

	      if (*p == '}')
		break;

	      pv = grub_zalloc (sizeof (*pv));
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

	      pv->id.uuid = grub_malloc (GRUB_LVM_ID_STRLEN);
	      if (!pv->id.uuid)
		goto pvs_fail;
	      grub_memcpy (pv->id.uuid, p, GRUB_LVM_ID_STRLEN);
	      pv->id.uuidlen = GRUB_LVM_ID_STRLEN;

	      pv->start_sector = grub_lvm_getvalue (&p, "pe_start = ");
	      if (p == NULL)
		{
#ifdef GRUB_UTIL
		  grub_util_info ("unknown pe_start\n");
#endif
		  goto pvs_fail;
		}

	      p = grub_strchr (p, '}');
	      if (p == NULL)
		{
#ifdef GRUB_UTIL
		  grub_util_info ("error parsing pe_start\n");
#endif
		  goto pvs_fail;
		}
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

      p = grub_strstr (p, "logical_volumes {");
      if (p)
	{
	  p += sizeof ("logical_volumes {") - 1;

	  /* And add all the lvs to the volume group. */
	  while (1)
	    {
	      grub_ssize_t s;
	      int skip_lv = 0;
	      struct grub_diskfilter_lv *lv;
	      struct grub_diskfilter_segment *seg;
	      int is_pvmove;

	      while (grub_isspace (*p))
		p++;

	      if (*p == '}')
		break;

	      lv = grub_zalloc (sizeof (*lv));

	      q = p;
	      while (*q != ' ')
		q++;

	      s = q - p;
	      lv->name = grub_strndup (p, s);
	      if (!lv->name)
		goto lvs_fail;

	      {
		const char *iptr;
		char *optr;
		lv->fullname = grub_malloc (sizeof ("lvm/") - 1 + 2 * vgname_len
					    + 1 + 2 * s + 1);
		if (!lv->fullname)
		  goto lvs_fail;

		grub_memcpy (lv->fullname, "lvm/", sizeof ("lvm/") - 1);
		optr = lv->fullname + sizeof ("lvm/") - 1;
		for (iptr = vgname; iptr < vgname + vgname_len; iptr++)
		  {
		    *optr++ = *iptr;
		    if (*iptr == '-')
		      *optr++ = '-';
		  }
		*optr++ = '-';
		for (iptr = p; iptr < p + s; iptr++)
		  {
		    *optr++ = *iptr;
		    if (*iptr == '-')
		      *optr++ = '-';
		  }
		*optr++ = 0;
		lv->idname = grub_malloc (sizeof ("lvmid/")
					  + 2 * GRUB_LVM_ID_STRLEN + 1);
		if (!lv->idname)
		  goto lvs_fail;
		grub_memcpy (lv->idname, "lvmid/",
			     sizeof ("lvmid/") - 1);
		grub_memcpy (lv->idname + sizeof ("lvmid/") - 1,
			     vg_id, GRUB_LVM_ID_STRLEN);
		lv->idname[sizeof ("lvmid/") - 1 + GRUB_LVM_ID_STRLEN] = '/';

		p = grub_strstr (q, "id = \"");
		if (p == NULL)
		  {
#ifdef GRUB_UTIL
		    grub_util_info ("couldn't find ID\n");
#endif
		    goto lvs_fail;
		  }
		p += sizeof ("id = \"") - 1;
		grub_memcpy (lv->idname + sizeof ("lvmid/") - 1
			     + GRUB_LVM_ID_STRLEN + 1,
			     p, GRUB_LVM_ID_STRLEN);
		lv->idname[sizeof ("lvmid/") - 1 + 2 * GRUB_LVM_ID_STRLEN + 1] = '\0';
	      }

	      lv->size = 0;

	      lv->visible = grub_lvm_check_flag (p, "status", "VISIBLE");
	      is_pvmove = grub_lvm_check_flag (p, "status", "PVMOVE");

	      lv->segment_count = grub_lvm_getvalue (&p, "segment_count = ");
	      if (p == NULL)
		{
#ifdef GRUB_UTIL
		  grub_util_info ("unknown segment_count\n");
#endif
		  goto lvs_fail;
		}
	      lv->segments = grub_malloc (sizeof (*seg) * lv->segment_count);
	      seg = lv->segments;

	      for (i = 0; i < lv->segment_count; i++)
		{

		  p = grub_strstr (p, "segment");
		  if (p == NULL)
		    {
#ifdef GRUB_UTIL
		      grub_util_info ("unknown segment\n");
#endif
		      goto lvs_segment_fail;
		    }

		  seg->start_extent = grub_lvm_getvalue (&p, "start_extent = ");
		  if (p == NULL)
		    {
#ifdef GRUB_UTIL
		      grub_util_info ("unknown start_extent\n");
#endif
		      goto lvs_segment_fail;
		    }
		  seg->extent_count = grub_lvm_getvalue (&p, "extent_count = ");
		  if (p == NULL)
		    {
#ifdef GRUB_UTIL
		      grub_util_info ("unknown extent_count\n");
#endif
		      goto lvs_segment_fail;
		    }

		  p = grub_strstr (p, "type = \"");
		  if (p == NULL)
		    goto lvs_segment_fail;
		  p += sizeof("type = \"") - 1;

		  lv->size += seg->extent_count * vg->extent_size;

		  if (grub_memcmp (p, "striped\"",
				   sizeof ("striped\"") - 1) == 0)
		    {
		      struct grub_diskfilter_node *stripe;

		      seg->type = GRUB_DISKFILTER_STRIPED;
		      seg->node_count = grub_lvm_getvalue (&p, "stripe_count = ");
		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown stripe_count\n");
#endif
			  goto lvs_segment_fail;
			}

		      if (seg->node_count != 1)
			seg->stripe_size = grub_lvm_getvalue (&p, "stripe_size = ");

		      seg->nodes = grub_zalloc (sizeof (*stripe)
						* seg->node_count);
		      stripe = seg->nodes;

		      p = grub_strstr (p, "stripes = [");
		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown stripes\n");
#endif
			  goto lvs_segment_fail2;
			}
		      p += sizeof("stripes = [") - 1;

		      for (j = 0; j < seg->node_count; j++)
			{
			  p = grub_strchr (p, '"');
			  if (p == NULL)
			    continue;
			  q = ++p;
			  while (*q != '"')
			    q++;

			  s = q - p;

			  stripe->name = grub_malloc (s + 1);
			  if (stripe->name == NULL)
			    goto lvs_segment_fail2;

			  grub_memcpy (stripe->name, p, s);
			  stripe->name[s] = '\0';

			  p = q + 1;

			  stripe->start = grub_lvm_getvalue (&p, ",")
			    * vg->extent_size;
			  if (p == NULL)
			    continue;

			  stripe++;
			}
		    }
		  else if (grub_memcmp (p, "mirror\"", sizeof ("mirror\"") - 1)
			   == 0)
		    {
		      seg->type = GRUB_DISKFILTER_MIRROR;
		      seg->node_count = grub_lvm_getvalue (&p, "mirror_count = ");
		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown mirror_count\n");
#endif
			  goto lvs_segment_fail;
			}

		      seg->nodes = grub_zalloc (sizeof (seg->nodes[0])
						* seg->node_count);

		      p = grub_strstr (p, "mirrors = [");
		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown mirrors\n");
#endif
			  goto lvs_segment_fail2;
			}
		      p += sizeof("mirrors = [") - 1;

		      for (j = 0; j < seg->node_count; j++)
			{
			  char *lvname;

			  p = grub_strchr (p, '"');
			  if (p == NULL)
			    continue;
			  q = ++p;
			  while (*q != '"')
			    q++;

			  s = q - p;

			  lvname = grub_malloc (s + 1);
			  if (lvname == NULL)
			    goto lvs_segment_fail2;

			  grub_memcpy (lvname, p, s);
			  lvname[s] = '\0';
			  seg->nodes[j].name = lvname;
			  p = q + 1;
			}
		      /* Only first (original) is ok with in progress pvmove.  */
		      if (is_pvmove)
			seg->node_count = 1;
		    }
		  else if (grub_memcmp (p, "raid", sizeof ("raid") - 1)
			   == 0 && (p[sizeof ("raid") - 1] >= '4'
				    && p[sizeof ("raid") - 1] <= '6')
			   && p[sizeof ("raidX") - 1] == '"')
		    {
		      switch (p[sizeof ("raid") - 1])
			{
			case '4':
			  seg->type = GRUB_DISKFILTER_RAID4;
			  seg->layout = GRUB_RAID_LAYOUT_LEFT_ASYMMETRIC;
			  break;
			case '5':
			  seg->type = GRUB_DISKFILTER_RAID5;
			  seg->layout = GRUB_RAID_LAYOUT_LEFT_SYMMETRIC;
			  break;
			case '6':
			  seg->type = GRUB_DISKFILTER_RAID6;
			  seg->layout = (GRUB_RAID_LAYOUT_RIGHT_ASYMMETRIC
					 | GRUB_RAID_LAYOUT_MUL_FROM_POS);
			  break;
			}
		      seg->node_count = grub_lvm_getvalue (&p, "device_count = ");

		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown device_count\n");
#endif
			  goto lvs_segment_fail;
			}

		      seg->stripe_size = grub_lvm_getvalue (&p, "stripe_size = ");
		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown stripe_size\n");
#endif
			  goto lvs_segment_fail;
			}


		      seg->nodes = grub_zalloc (sizeof (seg->nodes[0])
						* seg->node_count);

		      p = grub_strstr (p, "raids = [");
		      if (p == NULL)
			{
#ifdef GRUB_UTIL
			  grub_util_info ("unknown mirrors\n");
#endif
			  goto lvs_segment_fail2;
			}
		      p += sizeof("raids = [") - 1;

		      for (j = 0; j < seg->node_count; j++)
			{
			  char *lvname;

			  p = grub_strchr (p, '"');
			  p = p ? grub_strchr (p + 1, '"') : 0;
			  p = p ? grub_strchr (p + 1, '"') : 0;
			  if (p == NULL)
			    continue;
			  q = ++p;
			  while (*q != '"')
			    q++;

			  s = q - p;

			  lvname = grub_malloc (s + 1);
			  if (lvname == NULL)
			    goto lvs_segment_fail2;

			  grub_memcpy (lvname, p, s);
			  lvname[s] = '\0';
			  seg->nodes[j].name = lvname;
			  p = q + 1;
			}
		      if (seg->type == GRUB_DISKFILTER_RAID4)
			{
			  char *tmp;
			  tmp = seg->nodes[0].name;
			  grub_memmove (seg->nodes, seg->nodes + 1,
					sizeof (seg->nodes[0])
					* (seg->node_count - 1));
			  seg->nodes[seg->node_count - 1].name = tmp;
			}
		    }
		  else
		    {
#ifdef GRUB_UTIL
		      char *p2;
		      p2 = grub_strchr (p, '"');
		      if (p2)
			*p2 = 0;
		      grub_util_info ("unknown LVM type %s\n", p);
		      if (p2)
			*p2 ='"';
#endif
		      /* Found a non-supported type, give up and move on. */
		      skip_lv = 1;
		      break;
		    }

		  seg++;

		  continue;
		lvs_segment_fail2:
		  grub_free (seg->nodes);
		lvs_segment_fail:
		  goto fail4;
		}

	      if (p != NULL)
		p = grub_strchr (p, '}');
	      if (p == NULL)
		goto lvs_fail;
	      p += 3;

	      if (skip_lv)
		{
		  grub_free (lv->name);
		  grub_free (lv);
		  continue;
		}

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

      /* Match lvs.  */
      {
	struct grub_diskfilter_lv *lv1;
	struct grub_diskfilter_lv *lv2;
	for (lv1 = vg->lvs; lv1; lv1 = lv1->next)
	  for (i = 0; i < lv1->segment_count; i++)
	    for (j = 0; j < lv1->segments[i].node_count; j++)
	      {
		if (vg->pvs)
		  for (pv = vg->pvs; pv; pv = pv->next)
		    {
		      if (! grub_strcmp (pv->name,
					 lv1->segments[i].nodes[j].name))
			{
			  lv1->segments[i].nodes[j].pv = pv;
			  break;
			}
		    }
		if (lv1->segments[i].nodes[j].pv == NULL)
		  for (lv2 = vg->lvs; lv2; lv2 = lv2->next)
		    if (grub_strcmp (lv2->name,
				     lv1->segments[i].nodes[j].name) == 0)
		      lv1->segments[i].nodes[j].lv = lv2;
	      }
	
      }
      if (grub_diskfilter_vg_register (vg))
	goto fail4;
    }
  else
    {
      grub_free (vgname);
    }

  id->uuid = grub_malloc (GRUB_LVM_ID_STRLEN);
  if (!id->uuid)
    goto fail4;
  grub_memcpy (id->uuid, pv_id, GRUB_LVM_ID_STRLEN);
  id->uuidlen = GRUB_LVM_ID_STRLEN;
  grub_free (metadatabuf);
  *start_sector = -1;
  return vg;

  /* Failure path.  */
 fail4:
  grub_free (vg);
 fail3:
  grub_free (vgname);

 fail2:
  grub_free (metadatabuf);
 fail:
  return NULL;
}



static struct grub_diskfilter grub_lvm_dev = {
  .name = "lvm",
  .detect = grub_lvm_detect,
  .next = 0
};

GRUB_MOD_INIT (lvm)
{
  grub_diskfilter_register_back (&grub_lvm_dev);
}

GRUB_MOD_FINI (lvm)
{
  grub_diskfilter_unregister (&grub_lvm_dev);
}
