/* rock.c:
 *
 * Support for the Rock Ridge filing system.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 * 
 * 08-Dec-10 neil    Do not mark files/dirs as unwritable/undeletable.
 * 06-Mar-09 error   - Removed madness, fixed insanity. Cleanup started
 * 18-Aug-07 sonic   Fixed reading CL and PL fields on little-endian machines
 * 04-Jun-07 sonic   Fixed endianess check in Is_A_Symbolic_Link()
 * 05-May-07 sonic   Added support for RockRidge protection bits and file comments
 * 07-Jul-02 sheutlin  various changes when porting to AROS
 *                     - global variables are now in a struct Globals *global
 *                     - replaced (unsigned) long by (U)LONG
 * 05-Feb-94   fmu   Added support for relocated directories.
 * 16-Oct-93   fmu   Adapted to new VOLUME structure.
 */

#include <proto/exec.h>
#include <exec/memory.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "rock.h"
#include "globals.h"
#include "aros_stuff.h"
#include "clib_stuff.h"

#define VOL(vol,tag) (((t_iso_vol_info *)(vol->vol_info))->tag)
#define OBJ(obj,tag) (((t_iso_obj_info *)(obj->obj_info))->tag)

/* Check whether the given volume uses the Rock Ridge Interchange Protocol.
 * The protocol is identified by the sequence
 *            'S' 'P' 7 1 0xbe 0xef
 * in the system use field of the (00) directory in the root directory of
 * the volume.
 *
 * Returns 1 iff the RR protocol is used; 0 otherwise.
 * If the RR protocol is used, *p_skip will be set to the skip length
 * specified in the SP system use field.
 */

t_bool Uses_Rock_Ridge_Protocol(VOLUME *p_volume, int *p_skip) {
ULONG loc = VOL(p_volume,pvd).root.extent_loc;
directory_record *dir;
int system_use_pos;
unsigned char *sys;

	if (!(dir = Get_Directory_Record(p_volume, loc, 0)))
		return 0;

	system_use_pos = 33 + dir->file_id_length;
	if (system_use_pos & 1)
		system_use_pos++;

	if (system_use_pos >= dir->length)
		return 0;

	sys = (unsigned char *) dir + system_use_pos;
	if (
			sys[0] == 'S' &&
			sys[1] == 'P' &&
			sys[2] == 7 &&
			sys[3] == 1 &&
			sys[4] == 0xbe &&
			sys[5] == 0xef)
	{
		*p_skip = sys[6];
		return 1;
	}
	else
		return 0;
}

/* Searches for the system use field with name p_name in the directory record
 * p_dir and fills the buffer p_buf (with length p_buf_len) with the information
 * contained in the system use field.
 *
 * p_index is the ordinal number of the system use field (if more than one
 * system use field with the same name is recorded.) 0=first occurrence,
 * 1=second occurrence, and so on.
 *
 * 1 is returned if the system use field has been found; otherwise 0
 * is returned.
 */

int Get_System_Use_Field
	(
		VOLUME *p_volume, directory_record *p_dir,
		char *p_name,
		char *p_buf,
		int p_buf_len,
		int p_index
	)
{
int system_use_pos;
int slen, len;
ULONG length = p_dir->length;
unsigned char *buf = (unsigned char *) p_dir;

	system_use_pos = 33 + p_dir->file_id_length;
	if (system_use_pos & 1)
		system_use_pos++;
	system_use_pos += VOL(p_volume,skip);

	/* the system use field must be at least 4 bytes long */
	while (system_use_pos + 3 < length)
	{
		slen = buf[system_use_pos+2];
		if (
				buf[system_use_pos] == p_name[0] &&
				buf[system_use_pos+1] == p_name[1]
			)
		{
			if (p_index)
				p_index--;
			else
			{
				len = (slen < p_buf_len) ? slen : p_buf_len;
				CopyMem(buf + system_use_pos, p_buf, len);
				return 1;
			}
		}
		/* look for continuation area: */
		if (
				buf[system_use_pos] == 'C' &&
				buf[system_use_pos+1] == 'E'
			)
		{
		ULONG newloc, offset;
			CopyMem(buf + system_use_pos + 8, &newloc, 4);
			CopyMem(buf + system_use_pos + 16, &offset, 4);
			CopyMem(buf + system_use_pos + 24, &length, 4);
			if (!Read_Chunk(p_volume->cd, newloc))
				return 0;
			buf = p_volume->cd->buffer;
			system_use_pos = offset;
			continue;
		}

		/* look for system use field terminator: */
		if (
				buf[system_use_pos] == 'S' &&
				buf[system_use_pos+1] == 'T'
			)
			return 0;

		system_use_pos += slen;
	}
	return 0;
}

/* Determines the Rock Ridge file name of the CDROM object p_obj.
 * The file name will be stored in the buffer p_buf (with length p_buf_len).
 * The file name will NOT be null-terminated. The number of characters in
 * the file name is returned. If there is no Rock Ridge file name for
 * p_obj, then -1 is returned.
 */

int Get_RR_File_Name
	(VOLUME *p_volume, directory_record *p_dir, char *p_buf, int p_buf_len)
{
struct nm_system_use_field {
	char	  id[2];
	unsigned char length;
	unsigned char version;
	unsigned char flags;
	char          name[210];
} nm;
int len, slen;
int index = 0;
int total = 0;

	for (;;)
	{
		if (
				!Get_System_Use_Field
					(p_volume,p_dir,"NM",(char *)&nm,sizeof(nm),index)
			)
			return -1;

		slen = nm.length-5;
		len = (p_buf_len < slen) ? p_buf_len : slen;
		if (len)
			CopyMem(nm.name, p_buf, len);

		total += len;
		if (!(nm.flags & 1))
			return total;

		p_buf += len;
		p_buf_len -= len;
		index++;
	}
}

/* Determines the Rock Ridge Amiga protection bits and file comment of the CDROM object p_obj.
 * Protection bits will be stored in the p_prot.
 * The file comment will be stored in the buffer p_buf (with length p_buf_len).
 * The file comment will NOT be null-terminated. The number of characters in
 * the file name is returned. If there is no Rock Ridge file comment for
 * p_obj, then -1 is returned.
 */

int Get_RR_File_Comment(VOLUME *p_volume, directory_record *p_dir, uint32_t *p_prot, char *p_buf, int p_buf_len)
{

#define AS_PROTECTION	    0x01
#define AS_COMMENT	    0x02
#define AS_COMMENT_CONTINUE 0x04
	
        struct as_system_use_field
        {
		char	      id[2];
		unsigned char length;
		unsigned char version;
		unsigned char flags;
		char          data[210];
	} as;
	char *ptr;
	int len, slen;
	int index = 0;
	int total = 0;
	uint32_t pbuf;

	for (;;)
	{
		if (!Get_System_Use_Field(p_volume,p_dir,"AS",(char *)&as,sizeof(as),index))
			return -1;

		ptr = as.data;
		if (as.flags & AS_PROTECTION) {
			CopyMem(ptr, &pbuf, 4);
			ptr += 4;
			*p_prot = AROS_BE2LONG(pbuf) & ~(FIBF_WRITE | FIBF_DELETE);
		}
		if (!(as.flags & AS_COMMENT))
			return -1;
		slen = *ptr++ - 1;
		len = (p_buf_len < slen) ? p_buf_len : slen;
		if (len)
			CopyMem(ptr, p_buf, len);

		total += len;
		if (!(as.flags & AS_COMMENT_CONTINUE))
			return total;

		p_buf += len;
		p_buf_len -= len;
		index++;
	}
}

/* Returns 1 if the PX system use field indicates a symbolic link.
 * amiga_mode is a pointer to storage area for AmigaOS protection bits.
 */

int Is_A_Symbolic_Link(VOLUME *p_volume, directory_record *p_dir, uint32_t *amiga_mode)
{
	struct px_system_use_field {
		char	  id[2];
		unsigned char length;
		unsigned char version;
		ULONG mode_i;
		ULONG mode_m;
		ULONG links_i;
		ULONG links_m;
		ULONG user_id_i;
		ULONG user_id_m;
		ULONG group_id_i;
		ULONG group_id_m;
	} px;

#if AROS_BIG_ENDIAN
#define mode mode_m
#else
#define mode mode_i
#endif

	if (!Get_System_Use_Field(p_volume,p_dir,"PX",(char *)&px,sizeof(px),0))
		return 0;
	*amiga_mode = 0;
	if (!(px.mode & S_IXUSR))
		*amiga_mode |= FIBF_EXECUTE;
	if (!(px.mode & S_IRUSR))
		*amiga_mode |= FIBF_READ;
	if (px.mode & S_IXGRP)
		*amiga_mode |= FIBF_GRP_EXECUTE;
	if (px.mode & S_IRGRP)
		*amiga_mode |= FIBF_GRP_READ;
	if (px.mode & S_IXOTH)
		*amiga_mode |= FIBF_OTR_EXECUTE;
	if (px.mode & S_IROTH)
		*amiga_mode |= FIBF_OTR_READ;
	/*
		0120000 is the POSIX code for symbolic links:
	*/
	return (px.mode & 0770000) == 0120000;
}

/* Read content of SL system use field.
 * A full path name (starting with ":" or "sys:") will always be returned.
 */

t_bool Get_Link_Name(CDROM_OBJ *p_obj, char *p_buf, int p_buf_len) {
unsigned char buf[256];
char out[530];
int index = 0;
int len;
int offs;
char c;

	out[0] = 0;
	for (;;)
	{
		if (
				!Get_System_Use_Field
					(p_obj->volume,OBJ(p_obj,dir),"SL",(char *)buf,sizeof(buf),index)
			)
		{
			return (index == 0) ? 0 : 1;
		}

		offs = 5;
		for (;;)
		{

			if (StrLen (out) > 256)
				return 0;

			if (index == 0 && offs == 5)
			{
				/* handle the first component record: */

				if (buf[5] & 4) /* parent directory */
				{
				CDROM_OBJ *parent1 = Find_Parent(p_obj);
				CDROM_OBJ *parent2;
				char fullpath[256];
					if (!parent1)
						return 0;
					parent2 = Find_Parent(parent1);
					if (!parent2)
						return 0;
					if (!Full_Path_Name(parent2, fullpath, sizeof (fullpath)))
						return 0;
					Close_Object(parent1);
					Close_Object(parent2);
					StrCat (out, fullpath);
					if (out[1] != 0)
						StrCat (out, "/");
				}
				else if (buf[5] & 8) /* root */
					StrCat (out, "sys:");
				else if (buf[5] & 16) /* volume root */
					StrCat (out, ":");
				else
				{
				/* current directory */
				CDROM_OBJ *parent = Find_Parent(p_obj);
				char fullpath[256];
					if (!parent)
						return 0;
					if (!Full_Path_Name(parent, fullpath, sizeof (fullpath)))
						return 0;
					Close_Object(parent);
					StrCat (out, fullpath);
					if (out[1] != 0)
						StrCat (out, "/");
				}
			}

			if (out[0] && (c = out[StrLen(out)-1]) != ':' && c != '/')
				StrCat (out, "/");

			if (buf[offs] & 32) /* host name */
				StrCat (out, "AMIGA");

			len = StrLen (out);
			CopyMem(buf + offs, out + len, buf[offs+1]);
			out[len + buf[offs+1]] = 0;

			offs += 2 + buf[offs+1];
			if (offs >= buf[2])
			break;
		}
		if (!(buf[4] & 1)) /* continue flag */
			break;
		index++;
	}
	CopyMem(out, p_buf, p_buf_len-1);
	p_buf[p_buf_len-1] = 0;
	return 1;
}

/* Check whether a system use field is present: */

int Has_System_Use_Field
	(VOLUME *p_volume, directory_record *p_dir, char *p_name)
{
	return Get_System_Use_Field(p_volume, p_dir, p_name, NULL, 0, 0);
}

/* Return content of "CL" system use field, or -1, if no such system use field
 * is present.
 */

LONG RR_Child_Link(VOLUME *p_volume, directory_record *p_dir) {
struct cl {
	char    	name[2];
	char    	length[1];
	char    	version[1];
	LONG	pos_i;
	LONG	pos_m;
} buf;

	if (!Get_System_Use_Field(p_volume, p_dir,"CL",(char *)&buf,sizeof(buf),0))
		return -1;
	else
#if AROS_BIG_ENDIAN
		return buf.pos_m;
#else
		return buf.pos_i;
#endif
}

/* Return content of "PL" system use field, or -1, if no such system use field
 * is present.
 */

LONG RR_Parent_Link(VOLUME *p_volume, directory_record *p_dir) {
struct pl {
	char    	name[2];
	char    	length[1];
	char    	version[1];
	LONG	pos_i;
	LONG	pos_m;
} buf;

	if (!Get_System_Use_Field(p_volume,p_dir,"PL",(char *)&buf,sizeof(buf),0))
		return -1;
	else
#if AROS_BIG_ENDIAN
		return buf.pos_m;
#else
		return buf.pos_i;
#endif
}
