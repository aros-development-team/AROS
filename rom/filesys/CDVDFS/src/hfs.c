/* hfs.c:
 *
 * Support for the Macintosh HFS filing system.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 *
 * 06-Mar-09 error   - Removed madness, fixed insanity. Cleanup started
 * 03-Nov-94   fmu   - Fixed bug in HFS_Read_From_File().
 *                   - Truncate file names to 30 characters.
 * 15-Apr-93   fmu   - Improved conversion routines.
 *                   - Fixed bug in HFS_Find_Parent().
 *                   - Fixed bug in date conversion routine.
 * 02-Jan-93   fmu   Improved search method for master directory block.
 * 03-Dec-93   fmu   - Fixed bug in HFS_Find_Leaf_Record().
 *                   - Convert ':' and '/' characters.
 *                   - Also convert volume names.
 * 29-Nov-93   fmu   New function HFS_Block_Size().
 * 15-Nov-93   fmu   Corrected some typing mistakes in the HFS->ISO conversion
 *                   table.
 * 07-Jul-02 sheutlin  various changes when porting to AROS
 *                     - global variables are now in a struct Globals *global
 *                     - moved some definitions into hfs.h
 */

#include <proto/exec.h>
#include <proto/utility.h>
#include <stdlib.h>
#include <string.h>

#include "cdrom.h"
#include "generic.h"
#include "hfs.h"
#include "params.h"
#include "globals.h"

#include "clib_stuff.h"

#define VOL(vol,tag) (((t_hfs_vol_info *)(vol->vol_info))->tag)
#define OBJ(obj,tag) (((t_hfs_obj_info *)(obj->obj_info))->tag)

/* Number of seconds betweem 01-Jan-1904 and 01-Jan-1978: */

#define TIME_DIFF ((74UL * 365 + 19) * 24 * 60 * 60)

static char const g_conv_table[128] = {
  '\xC4', '\xC5', '\xC7', '\xC9', '\xD1', '\xD6', '\xDC',
  '\xE1', '\xE0', '\xE2', '\xE4', '\xE3', '\xE5', '\xE7',
  '\xE9', '\xE8', '\xEA', '\xEB', '\xED', '\xEC', '\xEE',
  '\xEF', '\xF1', '\xF3', '\xF2', '\xF4', '\xF6', '\xF5',
  '\xFA', '\xF9', '\xFB', '\xFC', '\xBF', '\xB0', '\xA2',
  '\xA3', '\xA7', '\xB7', '\xB6', '\xDF', '\xAE', '\xA9',
  '\xBF', '\xB4', '\xA8', '\xBF', '\xC6', '\xD8', '\xBF',
  '\xB1', '\xBF', '\xBF', '\xA5', '\xB5', '\xF0', '\xBF',
  '\xBF', '\xBF', '\xBF', '\xAA', '\xBA', '\xBF', '\xE6',
  '\xF8', '\xBF', '\xA1', '\xAC', '\xBF', '\xBF', '\xBF',
  '\xBF', '\xAB', '\xBB', '\xBF', '\xA0', '\xC0', '\xC3',
  '\xD5', '\xBF', '\xBF', '\xAD', '-', '\"', '\"',
  '`', '\xB4', '\xF7', '\xBF', '\xFF', '\xBF', '/',
  '\xA4', '\xBF', '\xBF', '\xBF', '\xBF', '\xBF', '.',
  '\xB8', '\xBF', '\xBF', '\xC2', '\xCA', '\xC1', '\xCB',
  '\xC8', '\xCD', '\xCE', '\xCF', '\xCC', '\xD3', '\xD4',
  '\xBF', '\xD2', '\xDA', '\xDB', '\xD9', '\xBF', '^',
  '~', '\xAD', '\xBF', '\xB7', '\xB0', '\xBF', '\"',
  '.', '\xBF'
};

void Convert_Mac_Characters (char *p_buf, int p_buf_len)
{
  unsigned char *cp = (unsigned char *) p_buf;
  int i;

  for (i=0; i<p_buf_len; i++, cp++)
    if (*cp >= 128)
      *cp = g_conv_table[*cp-128];
    else if (*cp == ':')
      *cp = '.';
    else if (*cp == '/')
      *cp = '-';
    else if (*cp < 32)
      *cp = 0xBF /* ¿ */;
}

void Convert_HFS_Spaces (char *p_buf, int p_buf_len)
{
  unsigned char *cp = (unsigned char *) p_buf;
  int i;

  for (i=0; i<p_buf_len; i++, cp++)
    if (*cp == ' ' || *cp == 0xA0)
      *cp = '_';
}

t_uchar *Read_Block(CDROM *p_cdrom, t_ulong p_block) {
	if (!Read_Chunk(p_cdrom, p_block >> 2))
		return NULL;

	return p_cdrom->buffer + ((p_block & 3) << 9);
}

/* Convert allocation block number into 512 byte block number.
 */

t_ulong AB_to_Block (VOLUME *p_volume, t_ulong p_alloc_block)
{
  return p_alloc_block * (VOL(p_volume,mdb).AlBlkSiz >> 9) +
  	 VOL(p_volume,mdb).AlBlSt + VOL(p_volume,start_block);
}

/* HFS disk integers are big-endian and may only be word-aligned.
 * Decode into native values without modifying the shared sector cache. */
t_ushort HFS_Get16(const void *data)
{
    const t_uchar *p = data;
    return ((t_ushort)p[0] << 8) | p[1];
}

t_ulong HFS_Get32(const void *data)
{
    const t_uchar *p = data;
    return ((t_ulong)p[0] << 24) | ((t_ulong)p[1] << 16) |
        ((t_ulong)p[2] << 8) | p[3];
}

static void HFS_Decode_Extents(const t_uchar *p, t_extdatarec extents)
{
    int i;
    for (i = 0; i < 3; i++, p += 4)
    {
        extents[i].StABN = HFS_Get16(p);
        extents[i].NumABlks = HFS_Get16(p + 2);
    }
}

/* Decode the fields used by the reader and the CD-ROM diagnostic tool. */
static void HFS_Decode_MDB(const t_uchar *p, t_mdb *mdb)
{
    memset(mdb, 0, sizeof(*mdb));
    mdb->SigWord = HFS_Get16(p);
    mdb->CrDate = HFS_Get32(p + 2);
    mdb->LsMod = HFS_Get32(p + 6);
    mdb->Atrb = HFS_Get16(p + 10);
    mdb->NmFls = HFS_Get16(p + 12);
    mdb->NmAlBlks = HFS_Get16(p + 18);
    mdb->AlBlkSiz = HFS_Get32(p + 20);
    mdb->AlBlSt = HFS_Get16(p + 28);
    mdb->VolNameLen = p[36];
    CopyMem(p + 37, mdb->VolName, sizeof(mdb->VolName));
    mdb->FilCnt = HFS_Get32(p + 84);
    mdb->DirCnt = HFS_Get32(p + 88);
    mdb->CTFlSize = HFS_Get32(p + 146);
    HFS_Decode_Extents(p + 150, mdb->CTExtRec);
}

void HFS_Decode_Node(const t_uchar *p, t_node_descr *node)
{
    node->FLink = HFS_Get32(p);
    node->BLink = HFS_Get32(p + 4);
    node->Type = p[8];
    node->NHeight = p[9];
    node->NRecs = HFS_Get16(p + 10);
    node->Reserved = HFS_Get16(p + 12);
}

int HFS_Find_Master_Directory_Block(CDROM *p_cd, t_mdb *p_mdb) {
t_uchar *block;
int i, entries;
int result;

	block = Read_Block(p_cd, 1);
	if (!block || block[0] != 0x50 || block[1] != 0x4D)
		return -1;

	entries = HFS_Get32(block + 4);
	for (i=0; i<entries; i++)
	{
		block = Read_Block(p_cd, i+1);
		if (!block || block[0] != 0x50 || block[1] != 0x4D)
			return -1;
		if (
				MemCmp
					(
						block + 48,
						"Apple_HFS", 9
					) == 0
			)
		{
			result = HFS_Get32(block + 8) + 2;
			block = Read_Block(p_cd, result);
			if (!block || block[0] != 0x42 || block[1] != 0x44)
				return -1;
			else
			{
				HFS_Decode_MDB(block, p_mdb);
				return result;
			}
		}
	}

	return -1;
}

t_bool Uses_HFS_Protocol(CDROM *p_cd, int *p_skip) {
t_mdb mdb;
int blk;

	blk = HFS_Find_Master_Directory_Block(p_cd, &mdb);
	if (blk == -1)
		return FALSE;

	*p_skip = blk - 2; /* *p_skip holds the start block of the volume */
	return TRUE;
}

t_bool HFS_Get_Header_Node
	(CDROM *p_cd, t_ulong p_mdb_pos, t_mdb *p_mdb, t_hdr_node *p_hdr_node)
{
t_ulong pos =
	(
		p_mdb_pos - 2 + p_mdb->AlBlSt +
 		p_mdb->CTExtRec[0].StABN * (p_mdb->AlBlkSiz >> 9)
	);
t_uchar *hdr;

	hdr = Read_Block(p_cd, pos);
	if (!hdr)
		return FALSE;

	/* This reader addresses catalog nodes as 512-byte blocks. */
    if (HFS_Get16(hdr + 32) != 512)
        return FALSE;
    memset(p_hdr_node, 0, sizeof(*p_hdr_node));
    HFS_Decode_Node(hdr, &p_hdr_node->node_descr);
    p_hdr_node->Depth = HFS_Get16(hdr + 14);
    p_hdr_node->Root = HFS_Get32(hdr + 16);
    p_hdr_node->NRecs = HFS_Get32(hdr + 20);
    p_hdr_node->FNode = HFS_Get32(hdr + 24);
    p_hdr_node->LNode = HFS_Get32(hdr + 28);
    p_hdr_node->NodeSize = HFS_Get16(hdr + 32);
    p_hdr_node->KeyLen = HFS_Get16(hdr + 34);
    p_hdr_node->NNodes = HFS_Get32(hdr + 36);
    p_hdr_node->Free = HFS_Get32(hdr + 40);
	return TRUE;
}

t_uchar *HFS_Get_Node
	(CDROM *p_cd, t_ulong p_mdb_pos, t_mdb *p_mdb, t_ulong p_node)
{
t_ulong first_node;
t_ulong pos;
t_ulong max = 0;
t_ulong sub = 0;
int i;

	for (i=0; i<3; i++)
	{
		max += p_mdb->CTExtRec[i].NumABlks * (p_mdb->AlBlkSiz >> 9);
		if (p_node < max)
			break;
		sub = max;
	}

	if (i==3)
		return NULL;

	first_node = (p_mdb_pos - 2 + p_mdb->AlBlSt +
		p_mdb->CTExtRec[i].StABN * (p_mdb->AlBlkSiz >> 9));

	pos = first_node + (p_node - sub);

	return Read_Block(p_cd, pos);
}

t_uchar *HFS_Node(VOLUME *p_volume, t_ulong p_node) {
  return HFS_Get_Node (p_volume->cd, VOL(p_volume,start_block) + 2,
  		       &VOL(p_volume,mdb), p_node);
}

/* HFS catalog nodes are 512 bytes. Their trailing offset table contains
 * one entry per record plus the end of the final record. */
t_bool HFS_Record_Bounds(const t_uchar *node, int record, int *start, int *end)
{
    int count = HFS_Get16(node + 10);
    int table;

    if (count < 1 || count > (512 - 14) / 2 - 1 ||
        record < 0 || record >= count)
        return FALSE;
    table = 512 - 2 * (count + 1);
    *start = HFS_Get16(node + 510 - 2 * record);
    *end = HFS_Get16(node + 508 - 2 * record);
    return *start >= 14 && *start < *end && *end <= table;
}

t_bool HFS_Load_Leaf(t_leaf_record_pos *leaf, const t_uchar *node)
{
    int start, end, keylen, data, len;
    const t_uchar *bytes = node;

    if (node[8] != 0xff ||
        !HFS_Record_Bounds(node, leaf->record, &start, &end))
        return FALSE;
    keylen = bytes[start] + 1;
    if (keylen < 7 || keylen > end - start ||
        bytes[start + 6] > 31 || bytes[start + 6] > keylen - 7)
        return FALSE;
    data = (start + keylen + 1) & ~1;
    len = end - data;
    if (len < 2 || len > 102)
        return FALSE;
    /* Require the fields consumed by each supported catalog record type. */
    switch (bytes[data])
    {
        case 1: if (len < 22) return FALSE; break;
        case 2: if (len < 102) return FALSE; break;
        case 3: if (len < 46) return FALSE; break;
        case 4: if (len < 46) return FALSE; break;
        default: return FALSE;
    }
    memset(&leaf->cat_rec, 0, sizeof(leaf->cat_rec));
    memset(&leaf->leaf_rec, 0, sizeof(leaf->leaf_rec));
    HFS_Decode_Node(node, &leaf->node_descr);
    leaf->leaf_rec.length = bytes[start];
    leaf->leaf_rec.parent_id = HFS_Get32(bytes + start + 2);
    leaf->leaf_rec.name_length = bytes[start + 6];
    CopyMem(bytes + start + 7, leaf->leaf_rec.name, leaf->leaf_rec.name_length);
    bytes += data;
    leaf->cat_rec.d.type = bytes[0];
    switch (bytes[0])
    {
        case 1:
            leaf->cat_rec.d.DirID = HFS_Get32(bytes + 6);
            leaf->cat_rec.d.CrDat = HFS_Get32(bytes + 10);
            break;
        case 2:
            leaf->cat_rec.f.StBlk = HFS_Get16(bytes + 24);
            leaf->cat_rec.f.RStBlk = HFS_Get16(bytes + 34);
            leaf->cat_rec.f.FlNum = HFS_Get32(bytes + 20);
            leaf->cat_rec.f.LgLen = HFS_Get32(bytes + 26);
            leaf->cat_rec.f.RLgLen = HFS_Get32(bytes + 36);
            leaf->cat_rec.f.CrDat = HFS_Get32(bytes + 44);
            HFS_Decode_Extents(bytes + 74, leaf->cat_rec.f.ExtRec);
            HFS_Decode_Extents(bytes + 86, leaf->cat_rec.f.RExtRec);
            break;
        case 3:
        case 4:
            leaf->cat_rec.dt.ParID = HFS_Get32(bytes + 10);
            leaf->cat_rec.dt.CNameLen = bytes[14];
            CopyMem(bytes + 15, leaf->cat_rec.dt.CName, 31);
            break;
    }
    if ((leaf->cat_rec.d.type == 3 || leaf->cat_rec.d.type == 4) &&
        leaf->cat_rec.dt.CNameLen > 31)
        return FALSE;
    return TRUE;
}

t_bool HFS_Find_Next_Leaf(VOLUME *p_volume, t_leaf_record_pos *p_leaf)
{
    t_uchar *node = HFS_Node(p_volume, p_leaf->node);
    if (!node || p_leaf->record >= HFS_Get16(node + 10))
        return FALSE;
    p_leaf->record++;
    if (p_leaf->record == HFS_Get16(node + 10))
    {
        if (!HFS_Get32(node) || HFS_Get32(node) == p_leaf->node)
            return FALSE;
        p_leaf->node = HFS_Get32(node);
        node = HFS_Node(p_volume, p_leaf->node);
        if (!node)
            return FALSE;
        p_leaf->record = 0;
    }
    if (!HFS_Load_Leaf(p_leaf, node))
        return FALSE;
    if (p_leaf->leaf_rec.name_length > 30)
    {
        p_leaf->leaf_rec.name_length = 30;
        *((char *)&p_leaf->leaf_rec.name[0] + 30) = '\0';
    }
    return TRUE;
}

/* Find leaf record p_leaf->record in node p_leaf->node.
 * Store the result in *p_leaf.
 */

t_bool HFS_Find_This_Leaf(VOLUME *p_volume, t_leaf_record_pos *p_leaf)
{
    t_uchar *node = HFS_Node(p_volume, p_leaf->node);
    return node != NULL && HFS_Load_Leaf(p_leaf, node);
}

/* Find the first leaf record with parent ID p_parent_id.
 * If the leaf node is found, TRUE is returned and *p_cat_rec will be
 * loaded with the catalog record. Otherwise, FALSE is returned.
 */

t_bool HFS_Find_Leaf_Record
	(VOLUME *p_volume, t_leaf_record_pos *p_leaf, t_ulong p_parent_id)
{
struct CDVDBase *global = p_volume->global;
t_uchar *node;
int i;
t_ulong this_node = VOL(p_volume,root_node);
t_ulong next_node;

	node = HFS_Node(p_volume, VOL(p_volume,root_node));
	if (!node)
	{
		global->iso_errno = ISOERR_SCSI_ERROR;
		return FALSE;
	}

	for (;;)
	{
		if (node[8] == 0)
		{
			 /* index node */
			next_node = -1;
			for (i=0; i<HFS_Get16(node + 10); i++)
			{
                int start, end;
                const t_uchar *idx;
                if (!HFS_Record_Bounds(node, i, &start, &end))
                {
                    global->iso_errno = ISOERR_INTERNAL;
                    return FALSE;
                }
                idx = node + start;
                if (idx[0] != 0 && end - start < 42)
                {
                    global->iso_errno = ISOERR_INTERNAL;
                    return FALSE;
                }
				if (idx[0] != 0)
				{
					if (HFS_Get32(idx + 2) >= p_parent_id)
						break;
					next_node = HFS_Get32(idx + 38);
				}
			}
		}
		else if (node[8] == 0xff)
		{
			/* leaf node */
			break;
		}
		else
		{
			global->iso_errno = ISOERR_INTERNAL;
			return FALSE;
		}

		if (next_node == -1)
		{
			global->iso_errno = ISOERR_INTERNAL;
			return FALSE;
		}

		node = HFS_Node(p_volume, next_node);
		if (!node)
		{
			global->iso_errno = ISOERR_SCSI_ERROR;
			return FALSE;
		}
		this_node = next_node;
	}

	p_leaf->node = this_node;
	p_leaf->record = 0;
    if (!HFS_Load_Leaf(p_leaf, node))
    {
        global->iso_errno = ISOERR_INTERNAL;
        return FALSE;
    }

	/* walk forwards until the same key is found: */
	for (;;)
	{
		if (p_leaf->leaf_rec.parent_id == p_parent_id)
			break;
		if (p_leaf->leaf_rec.parent_id > p_parent_id)
		{
			global->iso_errno = ISOERR_INTERNAL;
			return FALSE;
		}

		if (!HFS_Find_Next_Leaf(p_volume, p_leaf))
		{
			global->iso_errno = ISOERR_INTERNAL;
			return FALSE;
		}

	}

	if (p_leaf->leaf_rec.name_length > 30)
	{
		p_leaf->leaf_rec.name_length = 30;
		*((char *)&p_leaf->leaf_rec.name[0] + 30) = '\0';
	}

	return TRUE;
}

static t_handler const g_hfs_handler;

t_bool HFS_Init_Vol_Info
	(VOLUME *p_volume, int p_start_block) {
struct CDVDBase *global = p_volume->global;
t_uchar *block;
t_hdr_node hdr;

	/*
		Here is a good place to check whether the values of the variables
		g_data_fork_extension and g_resource_fork_extension are OK.
	*/

	if (
			global->g_data_fork_extension[0] == 0 &&
			(global->g_resource_fork_extension[0] == 0)
		)
		StrCpy (global->g_resource_fork_extension, ".rsrc");

	/*
		Volume info initialization:
	*/

	p_volume->handler = &g_hfs_handler;

	p_volume->vol_info = AllocMem (sizeof (t_hfs_vol_info), MEMF_PUBLIC);
	if (!p_volume->vol_info)
	{
		global->iso_errno = ISOERR_NO_MEMORY;
		return FALSE;
	}

	VOL(p_volume,start_block) = p_start_block;

	if (!(block = Read_Block(p_volume->cd, p_start_block + 2)))
	{
		global->iso_errno = ISOERR_SCSI_ERROR;
		FreeMem (p_volume->vol_info, sizeof (t_hfs_vol_info));
		return FALSE;
	}

	HFS_Decode_MDB(block, &VOL(p_volume,mdb));

	if (
			!HFS_Get_Header_Node
				(p_volume->cd, p_start_block + 2, &VOL(p_volume,mdb), &hdr)
		)
	{
		global->iso_errno = ISOERR_SCSI_ERROR;
		FreeMem (p_volume->vol_info, sizeof (t_hfs_vol_info));
		return FALSE;
	}

	VOL(p_volume,root_node) = hdr.Root;

	p_volume->mixed_char_filenames = TRUE;

	return TRUE;
}

void HFS_Close_Vol_Info(VOLUME *p_volume)
{
	FreeMem (p_volume->vol_info, sizeof (t_hfs_vol_info));
}

CDROM_OBJ *HFS_Alloc_Obj(struct CDVDBase *global) {
CDROM_OBJ *obj = AllocMem (sizeof (CDROM_OBJ), MEMF_PUBLIC | MEMF_CLEAR);

	if (!obj)
	{
		global->iso_errno = ISOERR_NO_MEMORY;
		return NULL;
	}

	obj->global = global;
	obj->obj_info = AllocMem (sizeof (t_hfs_obj_info), MEMF_PUBLIC | MEMF_CLEAR);
	if (!obj->obj_info)
	{
		global->iso_errno = ISOERR_NO_MEMORY;
		FreeMem (obj, sizeof (CDROM_OBJ));
		return NULL;
	}

	obj->global = global;

	return obj;
}

CDROM_OBJ *HFS_Open_Top_Level_Directory(VOLUME *p_volume) {
struct CDVDBase *global = p_volume->global;
CDROM_OBJ *obj = HFS_Alloc_Obj(global);

  if (!obj)
    return NULL;

  obj->directory_f = TRUE;
  obj->volume = p_volume;

  OBJ(obj,parent_id) = 1;
  StrCpy (OBJ(obj,name), ":");
  OBJ(obj,cat_rec).d.DirID = 2;

  return obj;
}

CDROM_OBJ *HFS_Open_Object(VOLUME *p_volume, char *p_name, t_ulong p_parent) {
t_leaf_record_pos leaf;
struct CDVDBase *global = p_volume->global;
CDROM_OBJ *obj;
char name[50];
char type;
t_bool data_fork = FALSE;

	if (!HFS_Find_Leaf_Record(p_volume, &leaf, p_parent))
		return NULL;

	for (;;)
	{
		type = leaf.cat_rec.d.type;
		if (type == 1 || type == 2)
		{
 			if (leaf.leaf_rec.parent_id != p_parent)
			{
				global->iso_errno = ISOERR_NOT_FOUND;
				return NULL;
			}

			CopyMem(leaf.leaf_rec.name, name, leaf.leaf_rec.name_length);
			name[leaf.leaf_rec.name_length] = 0;

			if (global->g_convert_hfs_filenames)
				Convert_Mac_Characters (name, leaf.leaf_rec.name_length);
			if (global->g_convert_hfs_spaces)
				Convert_HFS_Spaces (name, leaf.leaf_rec.name_length);

			if (type == 1)
			{
				if (Stricmp ((UBYTE *) p_name, (UBYTE *) name) == 0)
				break;
			}
			else
			{
				StrCat (name, global->g_data_fork_extension);
				if (Stricmp ((UBYTE *) p_name, (UBYTE *) name) == 0)
				{
					data_fork = TRUE;
					break;
				}
				name[leaf.leaf_rec.name_length] = 0;
				StrCat (name, global->g_resource_fork_extension);
				if (Stricmp ((UBYTE *) p_name, (UBYTE *) name) == 0)
				{
					data_fork = FALSE;
					break;
				}
			}
		}
		if (!HFS_Find_Next_Leaf(p_volume, &leaf))
			return NULL;
	}

	obj = HFS_Alloc_Obj(global);
	if (!obj)
		return NULL;
	obj->directory_f = (type == 1);
	obj->volume = p_volume;

	OBJ(obj,data_fork) = data_fork;
	OBJ(obj,parent_id) = p_parent;
	StrCpy (OBJ(obj,name), name);
	CopyMem(&leaf.cat_rec, &OBJ(obj,cat_rec), sizeof (t_catalog_record));

	return obj;
}

/* Open the object with name p_name in the directory p_dir.
 * p_name must not contain '/' or ':' characters.
 */

CDROM_OBJ *HFS_Open_Obj_In_Directory(CDROM_OBJ *p_dir, char *p_name) {
	return HFS_Open_Object(p_dir->volume, p_name, OBJ(p_dir,cat_rec).d.DirID);
}

CDROM_OBJ *HFS_Find_Parent(CDROM_OBJ *p_obj) {
struct CDVDBase *global = p_obj->global;
t_leaf_record_pos leaf;

	if (OBJ(p_obj,parent_id) == 2)
		return HFS_Open_Top_Level_Directory(p_obj->volume);

	if (!HFS_Find_Leaf_Record(p_obj->volume, &leaf, OBJ(p_obj,parent_id)))
		return NULL;

	for (;;)
	{
		if (leaf.leaf_rec.parent_id != OBJ(p_obj,parent_id))
		{
			global->iso_errno = ISOERR_INTERNAL;
			return NULL;
		}
		if (leaf.cat_rec.d.type == 3)
		{
		char buf[50];
			CopyMem(leaf.cat_rec.dt.CName, buf, leaf.cat_rec.dt.CNameLen);
			buf[leaf.cat_rec.dt.CNameLen] = 0;
			if (global->g_convert_hfs_filenames)
				Convert_Mac_Characters (buf, leaf.cat_rec.dt.CNameLen);
			if (global->g_convert_hfs_spaces)
				Convert_HFS_Spaces (buf, leaf.cat_rec.dt.CNameLen);
			return HFS_Open_Object(p_obj->volume, buf, leaf.cat_rec.dt.ParID);
		}
		if (!HFS_Find_Next_Leaf(p_obj->volume, &leaf))
			return NULL;
	}
}

void HFS_Close_Obj(CDROM_OBJ *p_obj)
{
	FreeMem (p_obj->obj_info, sizeof (t_hfs_obj_info));
	FreeMem (p_obj, sizeof (CDROM_OBJ));
}

int HFS_Read_From_File(CDROM_OBJ *p_file, char *p_buffer, int p_buffer_length)
{
    struct CDVDBase *global = p_file->global;
    uint32_t block;
    int remain_block, remain_file, remain;
    int len;
    CDROM *cd;
    int pos;
    int buf_pos = 0;
    int todo;
    t_bool data_fork = OBJ(p_file,data_fork);
    t_ulong first_block;
    t_ulong data_length = (data_fork ? OBJ(p_file,cat_rec).f.LgLen :
	    OBJ(p_file,cat_rec).f.RLgLen);

    if (p_file->pos == data_length)
    {
	/* at end of file: */
	return 0;
    }

    cd = p_file->volume->cd;
    first_block = AB_to_Block
	(
	 p_file->volume,
	 data_fork ?
	 OBJ(p_file,cat_rec).f.ExtRec[0].StABN :
	 OBJ(p_file,cat_rec).f.RExtRec[0].StABN
	);
    block = first_block + (p_file->pos >> 9);
    todo = p_buffer_length;

    while (todo)
    {
	t_uchar *data;

	if (0 == Read_File_Chunk(cd, block >> 2))
	{
	    global->iso_errno = ISOERR_SCSI_ERROR;
	    return -1;
	}

	data = cd->buffer + ((block & 3) << 9);

	/*
	 * this is offset within first block
	 */
	pos = p_file->pos & 511;

	/*
	 * The available data extends to the end of the current cache slot.
	 */
	remain_block = SCSI_BUFSIZE * cd->buffer_sectors -
		((block << 9) & (SCSI_BUFSIZE * cd->buffer_sectors - 1)) - pos;

	/*
	 * how much data left in a file do we have?
	 */
	remain_file = data_length - p_file->pos;

	/*
	 * and how much can we read (file vs block)
	 */
	remain = (remain_block < remain_file) ? remain_block : remain_file;

	/*
	 * last: how much the *user* wants to read?
	 */
	len = (todo < remain) ? todo : remain;

	/*
	 * copy memory block
	 */
	CopyMem ((APTR) (data + pos), (APTR) (p_buffer + buf_pos), len);

	/*
	 * update position in user buffer
	 * and file offset
	 * reduce amount of bytes to be read
	 */
	buf_pos += len;
	p_file->pos += len;
	todo -= len;

	/*
	 * fin?
	 */
	if (p_file->pos >= data_length)
	    break;

	/*
	 * HFS uses four 512-byte blocks per cached 2048-byte sector.
	 * Advance to the next cache-slot boundary.
	 */
	block = (block + (cd->buffer_sectors << 2)) &
		~((cd->buffer_sectors << 2) - 1);
    }

    return buf_pos;
}

t_bool HFS_Cdrom_Info(CDROM_OBJ *p_obj, CDROM_INFO *p_info) {
    memset(p_info, 0, sizeof(*p_info));
	p_info->symlink_f = 0;
	p_info->directory_f = p_obj->directory_f;
	p_info->name_length = StrLen (OBJ(p_obj,name));
	CopyMem(OBJ(p_obj,name), p_info->name, p_info->name_length);
	if (p_info->directory_f)
	{
		p_info->file_length = 0;
		p_info->date = OBJ(p_obj,cat_rec).d.CrDat - TIME_DIFF;
	}
	else
	{
		if (OBJ(p_obj,data_fork))
			p_info->file_length = OBJ(p_obj,cat_rec).f.LgLen;
		else
			p_info->file_length = OBJ(p_obj,cat_rec).f.RLgLen;
		p_info->date = OBJ(p_obj,cat_rec).f.CrDat - TIME_DIFF;
	}

	return TRUE;
}

/*  The 'offset' is a long integer coded like this:
 *
 *  Bit:      |    31 ... 8  | 7   6 | 5   4   3   2   1   0
 *            |              |       |
 *  Contents: |  Node number |  Fork |    Record number
 *
 *  The "Fork" value is encoded as follows:
 *
 *   0 - data fork
 *   1 - resource fork
 *   2 - directory
 *   3 - illegal
 */

t_bool HFS_Examine_Next
	(CDROM_OBJ *p_obj, CDROM_INFO *p_info, uint32_t *p_offset)
{
struct CDVDBase *global = p_obj->global;
t_leaf_record_pos leaf;
short fork = 3;

    memset(p_info, 0, sizeof(*p_info));

	while (fork == 3)
	{
		if (*p_offset == 0)
		{
			if (
					!HFS_Find_Leaf_Record
						(p_obj->volume, &leaf, OBJ(p_obj,cat_rec).d.DirID)
				)
				return FALSE;
		}
		else
		{
			leaf.node = *p_offset >> 8;
			leaf.record = *p_offset & 63;
			fork = (*p_offset & 0xC0) >> 6;

			if (fork == 0)
			{
				if (!HFS_Find_This_Leaf(p_obj->volume, &leaf))
					return FALSE;
			}
			else
			{
				if (!HFS_Find_Next_Leaf(p_obj->volume, &leaf))
					return FALSE;
				if (leaf.leaf_rec.parent_id != OBJ(p_obj,cat_rec).d.DirID)
					return FALSE;
			}
		}
		if (leaf.cat_rec.d.type == 1)
			fork = 2;
		else if (leaf.cat_rec.d.type != 2)
			fork = 3;
		else if (fork != 0 && leaf.cat_rec.f.LgLen > 0)
			fork = 0;
		else if (leaf.cat_rec.f.RLgLen > 0)
			fork = 1;
		else
			fork = 3;

		*p_offset = ((leaf.node << 8) + (fork << 6) + leaf.record);

	}

	p_info->symlink_f = 0;
	p_info->directory_f = (leaf.cat_rec.d.type == 1);
	p_info->name_length = leaf.leaf_rec.name_length;
	CopyMem(leaf.leaf_rec.name, p_info->name, leaf.leaf_rec.name_length);
	if (global->g_convert_hfs_filenames)
		Convert_Mac_Characters (p_info->name, p_info->name_length);
	if (global->g_convert_hfs_spaces)
		Convert_HFS_Spaces (p_info->name, p_info->name_length);
	if (p_info->directory_f)
	{
		p_info->file_length = 0;
		p_info->date = leaf.cat_rec.d.CrDat - TIME_DIFF;
	}
	else
	{
		if (fork == 0)
		{
			CopyMem
				(
					global->g_data_fork_extension,
					p_info->name + p_info->name_length,
      	      StrLen (global->g_data_fork_extension)
				);
			p_info->name_length += StrLen (global->g_data_fork_extension);
			p_info->file_length = leaf.cat_rec.f.LgLen;
		}
		else
		{
			CopyMem
				(
					global->g_resource_fork_extension,
					p_info->name + p_info->name_length,
      	      StrLen (global->g_resource_fork_extension)
				);
			p_info->name_length += StrLen (global->g_resource_fork_extension);
			p_info->file_length = leaf.cat_rec.f.RLgLen;
		}
		p_info->date = leaf.cat_rec.f.CrDat - TIME_DIFF;
	}
	p_info->suppl_info = NULL;

	return TRUE;
}

void *HFS_Clone_Obj_Info(void *p_info) {
t_hfs_obj_info *info = (t_hfs_obj_info *) p_info;
t_hfs_obj_info *new;

	new = AllocMem (sizeof (t_hfs_obj_info), MEMF_PUBLIC);
	if (!new)
		return NULL;

	CopyMem(info, new, sizeof (t_hfs_obj_info));

	return new;
}

t_bool HFS_Is_Top_Level_Obj (CDROM_OBJ *p_obj)
{
  return OBJ(p_obj,parent_id) == 1;
}

t_bool HFS_Same_Objects (CDROM_OBJ *p_obj1, CDROM_OBJ *p_obj2)
{
  if (OBJ(p_obj1,cat_rec).d.type != OBJ(p_obj2,cat_rec).d.type)
    return FALSE;

  if (OBJ(p_obj1,cat_rec).d.type == 1)
    return OBJ(p_obj1,cat_rec).d.DirID == OBJ(p_obj2,cat_rec).d.DirID;
  else
    return OBJ(p_obj1,cat_rec).f.FlNum == OBJ(p_obj2,cat_rec).f.FlNum;
}

t_ulong HFS_Creation_Date(VOLUME *p_volume)
{
	return VOL(p_volume,mdb).CrDate - TIME_DIFF;
}

/* Return volume size in 2048 byte blocks:
 */

t_ulong HFS_Volume_Size (VOLUME *p_volume)
{
  return (VOL(p_volume,mdb).NmAlBlks * (VOL(p_volume,mdb).AlBlkSiz >> 9));
}

t_ulong HFS_Block_Size (VOLUME *p_volume)
{
  return 512;
}

void HFS_Volume_Id(VOLUME *p_volume, char *p_buf, int p_buf_length) {
struct CDVDBase *global = p_volume->global;
int len = p_buf_length - 1;

	if (len > VOL(p_volume,mdb).VolNameLen)
		len = VOL(p_volume,mdb).VolNameLen;

	CopyMem(VOL(p_volume,mdb).VolName, p_buf, len);
	p_buf[len] = 0;

	if (global->g_convert_hfs_filenames)
		Convert_Mac_Characters (p_buf, len);
	if (global->g_convert_hfs_spaces)
		Convert_HFS_Spaces (p_buf, len);
}

t_ulong HFS_Location (CDROM_OBJ *p_obj)
{
 if (p_obj->directory_f) {
   if (Is_Top_Level_Object (p_obj))
     return FALSE;
   return OBJ(p_obj,cat_rec).d.DirID;
 } else
   return OBJ(p_obj,cat_rec).f.FlNum;
}

t_ulong HFS_File_Length (CDROM_OBJ *p_obj)
{
  return (OBJ(p_obj,data_fork) ?
  	  OBJ(p_obj,cat_rec).f.LgLen :
	  OBJ(p_obj,cat_rec).f.RLgLen);
}

static t_handler const g_hfs_handler = {
  HFS_Close_Vol_Info,
  HFS_Open_Top_Level_Directory,
  HFS_Open_Obj_In_Directory,
  HFS_Find_Parent,
  HFS_Close_Obj,
  HFS_Read_From_File,
  HFS_Cdrom_Info,
  HFS_Examine_Next,
  HFS_Clone_Obj_Info,
  HFS_Is_Top_Level_Obj,
  HFS_Same_Objects,
  HFS_Creation_Date,
  HFS_Volume_Size,
  HFS_Volume_Id,
  HFS_Location,
  HFS_File_Length,
  HFS_Block_Size
};
