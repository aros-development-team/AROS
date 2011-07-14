/* iso9660.c:
 *
 * Support for the ISO-9660 filing system.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 *              (C) Copyright 2007-2010 The AROS Development Team.
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 * 
 * 20-Apr-10 neil    - Added work-around for Rock Ridge discs that have
 *                     version numbers in file names.
 * 10-Feb-10 neil    - Removed error in fetching Rock Ridge names.
 * 06-Mar-09 error   - Removed madness, fixed insanity. Cleanup started
 * 16-Jun-08 sonic   - Skip files with "Associated" flag set. Helps to
 *                     read ISO discs created under MacOS.
 *                   - Supports "Hidden" flag.
 * 06-May-07 sonic   - Rewritten routines dealing with object names.
 *                   - Added support for RockRidge protection bits and file comments.
 * 08-Apr-07 sonic   - removed redundant "TRACKDISK" option
 * 31-Mar-07 sonic   - added Joliet support.
 *                   - removed static buffer in Get_Directory_Record().
 * 20-Aug-94   fmu   Uses function Find_Last_Session() before the traversal
 *                   of the TOC.
 * 17-Jul-94   fmu   Tries to cope with CDROMs which have an incorrect TOC.
 * 17-May-94   fmu   New option MAYBELOWERCASE (=ML).
 * 25-Apr-94   fmu   The extented attribute record length has to be
 *                   considered when reading file sections.
 * 17-Feb-94   fmu   Volume ID must not be longer than 30 characters.
 * 05-Feb-94   fmu   Added support for relocated directories.
 * 07-Jan-94   fmu   Support for drives which don't support the SCSI-2
 *                   READ TOC command.
 * 01-Jan-94   fmu   Added multisession support.
 * 11-Dec-93   fmu   Fixed bug in Iso_Find_Parent().
 * 02-Dec-93   fmu   Bugfix: a logical block of a file extent must not
 *                   necessarily start at a logical sector border.
 * 29-Nov-93   fmu   - New function Iso_Block_Size().
 *                   - Support for variable logical block sizes.
 * 15-Nov-93   fmu   Uses_High_Sierra_Protocol added.
 * 13-Nov-93   fmu   Bad iso_errno return value in Iso_Open_Obj_In_Directory
 *                   corrected.
 * 12-Oct-93   fmu   Adapted to new VOLUME and CDROM_OBJ structures.
 * 24-Sep-93   fmu   Two further bugs in Seek_Position fixed.
 * 16-Sep-93   fmu   Fixed bug in Seek_Position.
 * 16-Sep-93   fmu   Bugfix: Top level object recognition in CDROM_Info
 *                   had to be changed for Rock Ridge disks.
 * 07-Jul-02 sheutlin  various changes when porting to AROS
 *                     - global variables are now in a struct Globals *global
 */

#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/date.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "cdrom.h"
#include "iso9660.h"
#include "joliet.h"
#include "rock.h"
#include "globals.h"

#include "clib_stuff.h"

#ifndef FIBF_HIDDEN
#define FIBF_HIDDEN (1<<7)
#endif

t_bool Iso_Is_Top_Level_Object (CDROM_OBJ *);

#define VOL(vol,tag) (((t_iso_vol_info *)(vol->vol_info))->tag)
#define OBJ(obj,tag) (((t_iso_obj_info *)(obj->obj_info))->tag)

int Get_Volume_Name(VOLUME *p_volume, char *buf, int buflen)
{
    char *iso_name = VOL(p_volume,pvd).volume_id;

    D(bug("[CDVDFS]\tGet_Volume_Name()\n"));
    if (p_volume->protocol == PRO_JOLIET)
	return Get_Joliet_Name(p_volume->global, iso_name, buf, 32);
    else {
	CopyMem(iso_name, buf, 32);
	return 32;
    }
}

int Get_File_Name(VOLUME *volume, directory_record *dir, char *buf, int buflen)
{
    int len;
    
    D(bug("[CDVDFS]\tGet_File_Name()\n"));

    switch (volume->protocol) 
    {
	case PRO_JOLIET:
	    len = Get_Joliet_Name(volume->global, dir->file_id, buf, dir->file_id_length);
	    break;

	case PRO_ROCK:
	    len = Get_RR_File_Name(volume, dir, buf, buflen);
	    if (len > 0)
		break;

	    /* Fall through... */

	default:
	    len = dir->file_id_length;
	    CopyMem(dir->file_id, buf, len);
	    break;
    }
    buf[len] = 0;
    return len;
}

/* Check whether the given volume uses the ISO 9660 Protocol.
 * The protocol is identified by the sequence
 *            'C' 'D' '0' '0' '1'
 * in the 2nd..6th byte of sector 16 of a track.
 *
 * All data tracks on the disk are examined.
 *
 * Returns TRUE iff the ISO protocol is used; FALSE otherwise.
 */

t_bool Uses_Iso_Protocol(CDROM *p_cdrom, t_ulong *p_offset) 
{
    int i, len;
    t_ulong *buf;
    
    D(bug("[CDVDFS]\tUses_Iso_Protocol()\n"));

    /*
       If Data_Tracks() returns -1, then the drive probably doesn't support
       the SCSI-2 READ TOC command.
       */
    if ((len = Data_Tracks(p_cdrom, &buf)) < 0)
    {
	*p_offset = 0;
	if (!Read_Chunk(p_cdrom, 16))
	    return FALSE;  
	return StrNCmp((char *) p_cdrom->buffer + 1, "CD001", 5) == 0;
    }

    if (len == 0)
	return FALSE;

    /*
       Use a vendor-specific command to find the offset of the last
session:
*/
    if (
	    Find_Last_Session(p_cdrom, p_offset) &&
	    Read_Chunk(p_cdrom, 16 + *p_offset) &&
	    StrNCmp((char *) p_cdrom->buffer + 1, "CD001", 5) == 0
       )
    {
	FreeVec (buf);
	return TRUE;
    }

    /* Search all data tracks for valid primary volume descriptors: */
    for (i=len-1; i>=0; i--)
    {
	*p_offset = buf[i];
	if (!Read_Chunk(p_cdrom, 16 + *p_offset))
	    continue;
	if (StrNCmp((char *) p_cdrom->buffer + 1, "CD001", 5) == 0)
	{
	    FreeVec (buf);
	    return TRUE;
	}
    }

    FreeVec (buf);

    /*
       On some disks, the information in the TOC may not be valid. Therefore
       also check sector 16:
       */
    if (!Read_Chunk(p_cdrom, 16))
	return FALSE;
    if (StrNCmp((char *) p_cdrom->buffer + 1, "CD001", 5) == 0)
    {
	*p_offset = 0;
	return TRUE;
    }

    return FALSE;
}

/* Check whether the given volume uses the High Sierra Protocol.
 * The protocol is identified by the sequence
 *            'C' 'D' 'R' 'O' 'M'
 * in the 10th..14th byte of sector 16.
 *
 * Returns TRUE iff the High Sierra protocol is used; FALSE otherwise.
 */

t_bool Uses_High_Sierra_Protocol(CDROM *p_cdrom) 
{
    D(bug("[CDVDFS]\tUses_High_Sierra_Protocol()\n"));
    if (!Read_Chunk(p_cdrom, 16))
	return FALSE;

    return StrNCmp((char *) p_cdrom->buffer + 9, "CDROM", 5) == 0;
}

static t_handler const g_iso_handler;
t_bool Iso_Init_Vol_Info(VOLUME *p_volume, int p_skip, t_ulong p_offset, t_ulong p_svd_offset) 
{
    struct CDVDBase *global = p_volume->global;
    long loc = p_svd_offset + p_offset;
    
    D(bug("[CDVDFS]\tIso_Init_Vol_Info()\n"));

    p_volume->handler = &g_iso_handler;
    p_volume->vol_info = AllocMem (sizeof (t_iso_vol_info), MEMF_PUBLIC);
    if (!p_volume->vol_info)
    {
	global->iso_errno = ISOERR_NO_MEMORY;
	return FALSE;
    }

    for (;;)
    {
	if (!Read_Chunk(p_volume->cd, loc))
	{
	    global->iso_errno = ISOERR_SCSI_ERROR;
	    FreeMem (p_volume->vol_info, sizeof (t_iso_vol_info));
	    return FALSE;
	}

	if ((p_volume->cd->buffer[0] == 1) || (p_volume->cd->buffer[0] == 2))
	{
	    CopyMem
		(
		 p_volume->cd->buffer,
		 &VOL(p_volume,pvd),
		 sizeof (prim_vol_desc)
		);
	    break;
	}

	if (p_volume->cd->buffer[0] == 255 || loc > 1000)
	{
	    global->iso_errno = ISOERR_NO_PVD;
	    FreeMem (p_volume->vol_info, sizeof (t_iso_vol_info));
	    return FALSE;
	}

	loc++;
    }

    VOL(p_volume,skip) = p_skip;

    switch (VOL(p_volume,pvd).block_size)
    {
	case 512:
	    VOL(p_volume,blockshift) = 2;
	    break;
	case 1024:
	    VOL(p_volume,blockshift) = 1;
	    break;
	case 2048:
	default:
	    VOL(p_volume,blockshift) = 0;
	    break;
    }

    /*
       Look at the system ID to find out if the CD is supposed
       to feature proper file names. These are CDs made for use
       with the CDTV and the CD³² (both share the "CDTV" system ID)
       and the "Fresh Fish", "Frozen Fish" and "Gold Fish" CDs
       created by Mkisofs. If any of these IDs is found the
       file name to lower case conversion is temporarily
       disabled if the "ML=MAYBELOWERCASE" option has been selected.
       */

    if (
	    p_volume->protocol == PRO_ROCK || p_volume->protocol == PRO_JOLIET ||
	    !StrNCmp(VOL(p_volume,pvd).system_id,"CDTV",4) ||
	    !StrNCmp(VOL(p_volume,pvd).system_id,"AMIGA",5)
       )
	p_volume->mixed_char_filenames = TRUE;
    else
	p_volume->mixed_char_filenames = FALSE;

    return TRUE;
}

void Iso_Close_Vol_Info(VOLUME *p_volume)
{
    D(bug("[CDVDFS]\tIso_Close_Vol_Info()\n"));
    FreeMem (p_volume->vol_info, sizeof (t_iso_vol_info));
}

CDROM_OBJ *Iso_Alloc_Obj(struct CDVDBase *global, int p_length_of_dir_record) 
{
    CDROM_OBJ *obj;
    
    D(bug("[CDVDFS]\tIso_Alloc_Obj()\n"));

    obj = AllocMem (sizeof (CDROM_OBJ), MEMF_PUBLIC | MEMF_CLEAR);
    if (!obj)
    {
	global->iso_errno = ISOERR_NO_MEMORY;
	return NULL;
    }

    obj->global = global;
    obj->obj_info = AllocMem (sizeof (t_iso_obj_info), MEMF_PUBLIC);
    if (!obj->obj_info)
    {
	FreeMem (obj, sizeof (CDROM_OBJ));
	return NULL;
    }

    OBJ(obj,dir) = AllocMem (p_length_of_dir_record, MEMF_PUBLIC);
    if (!OBJ(obj,dir))
    {
	global->iso_errno = ISOERR_NO_MEMORY;
	FreeMem (obj->obj_info, sizeof (t_iso_obj_info));
	FreeMem (obj, sizeof (CDROM_OBJ));
	return NULL;
    }

    D(bug("[CDVDFS]\tIso_Alloc_Obj = %08lx\n", obj));

    return obj;
}

/* Get the "CDROM object" for the root directory of the volume.
 */

CDROM_OBJ *Iso_Open_Top_Level_Directory(VOLUME *p_volume) 
{
    CDROM_OBJ *obj;
    
    D(bug("[CDVDFS]\tIso_Open_Top_Level_Directory()\n"));

    obj = Iso_Alloc_Obj(p_volume->global, VOL(p_volume,pvd).root.length);
    if (!obj)
	return NULL;

    obj->directory_f = TRUE;
    obj->volume = p_volume;
    obj->pos = 0;
    CopyMem
	(
	 &VOL(p_volume,pvd).root,
	 OBJ(obj,dir),
	 VOL(p_volume,pvd).root.length
	);

    return obj;
}

/* Compare the name of the directory entry dir on the volume volume
 * with the C string p_name, and return 1 if both strings are equal.
 * NOTE: dir may contain a file name (with version number) or a directory
 *       name (without version number).
 */

int Names_Equal(VOLUME *volume, directory_record *dir, char *p_name)
{
    struct CDVDBase *global = volume->global;
    int pos, len;
    char buf[256];

    len = Get_File_Name(volume, dir, buf, sizeof(buf));

    D(bug("[CDVDFS]\tComparing names '%s' <=> '%s'\n", p_name, buf));

    if (Strnicmp(buf, p_name, len) == 0 && p_name[len] == 0)
    {
	D(bug("[CDVDFS]\t-> Equal\n"));
	return TRUE;
    }

    if (volume->protocol == PRO_ROCK
	&& !(buf[len-2] == ';' && buf[len-1] == '1'))
    {
	D(bug("[CDVDFS]\t-> Not Equal\n"));
	return FALSE;
    }

    /* compare without version number: */

    for (pos=len-1; pos>=0; pos--)
	if (buf[pos] == ';')
	    break;

    if (pos>=0)
    {
	D(bug("[CDVDFS]\t-> checking some more\n"));
	return (Strnicmp(buf, p_name, pos) == 0 && p_name[pos] == 0);
    }
    else
    {
	D(bug("[CDVDFS]\t-> Not Equal\n"));
	return FALSE;
    }
}

/* Get a record from a directory.
 * p_location is a LOGICAL BLOCK number.
 */
directory_record *Get_Directory_Record
	(VOLUME *p_volume, uint32_t p_location, uint32_t p_offset)
{
    struct CDVDBase *global = p_volume->global;
    int len;
    int loc;

    D(bug("[CDVDFS]\tGet_Directory_Record(%lu, %lu): blockshift = %lu\n", p_location, p_offset, VOL(p_volume, blockshift)));
    loc = (p_location >> VOL(p_volume,blockshift)) + (p_offset >> 11);
    D(bug("[CDVDFS]\tResult sector = %lu\n", loc));

    if (!Read_Chunk(p_volume->cd, loc))
    {
	D(bug("[CDVDFS]\tRead failed\n"));
	global->iso_errno = ISOERR_SCSI_ERROR;
	return NULL;
    }

    len = p_volume->cd->buffer[p_offset & 2047];
    if (len)
    {
	CopyMem(p_volume->cd->buffer + (p_offset & 2047), p_volume->buffer, len);
	D({
	    int x;
	    directory_record *dr = (directory_record*)p_volume->buffer;
	    bug("[CDVDFS]\tEntry: >");
	    for (x=0; x<dr->file_id_length; x++)
		bug("%c", dr->file_id[x]);
	    bug("<\n");
	 });
    }
    else
    {
	D(bug("[CDVDFS]\t== Last entry ==\n"));
	p_volume->buffer[0] = 0;  /* mark as last record */
    }

    return (directory_record *)p_volume->buffer;
}

/* Create a "CDROM object" for the directory which is located
 * at sector p_location.
 */

CDROM_OBJ *Iso_Create_Directory_Obj(VOLUME *p_volume, uint32_t p_location)
{
    struct CDVDBase *global = p_volume->global;
    directory_record *dir;
    uint32_t loc;
    int offset = 0;
    CDROM_OBJ *obj;
    uint32_t len;

    D(bug("[CDVDFS]\tIso_Create_Directory_Obj(%ld)\n", p_location));

    if (p_location == VOL(p_volume,pvd).root.extent_loc)
	return Iso_Open_Top_Level_Directory(p_volume);

    dir = Get_Directory_Record(p_volume, p_location, 0);
    if (!dir)
    {
	D(bug("[CDVDFS]\tNo directory record for '.'\n"));
	return NULL;
    }

    dir = Get_Directory_Record(p_volume, p_location, dir->length);
    if (!dir)
    {
	D(bug("[CDVDFS]\tNo directory record for '..'\n"));
	return NULL;
    }

    loc = dir->extent_loc;
    len = dir->data_length;
    for (;;)
    {
	if (offset >= len)
	{
	    D(bug("[CDVDFS]\tNo more elements.\n"));
	    return NULL;
	}

	dir = Get_Directory_Record(p_volume, loc, offset);
	if (!dir)
	{
	    D(bug("[CDVDFS]\tFailed to read next entry\n"));
	    return NULL;
	}

	if (!dir->length)
	{
	    D(bug("[CDVDFS]\tAdvancing to next sector\n"));
	    /* go to next logical sector: */
	    offset = (offset & 0xfffff800) + 2048;
	    continue;
	}
	if (dir->extent_loc == p_location)
	    break;
	offset += dir->length;
    }

    obj = Iso_Alloc_Obj(global, dir->length);
    if (!obj)
    {
	D(bug("[CDVDFS]\tFailed to allocate object\n"));
	return NULL;
    }

    obj->directory_f = TRUE;
    obj->volume = p_volume;
    obj->pos = 0;
    CopyMem(dir, OBJ(obj,dir), dir->length);
    D(bug("[CDVDFS]\tObject %08lx allocated and initialized\n", obj));

    return obj;
}


/* Open the object with name p_name in the directory p_dir.
 * p_name must not contain '/' or ':' characters.
 */

CDROM_OBJ *Iso_Open_Obj_In_Directory(CDROM_OBJ *p_dir, char *p_name)
{
    struct CDVDBase *global = p_dir->global;
    uint32_t loc = OBJ(p_dir,dir)->extent_loc + OBJ(p_dir,dir)->ext_attr_length;
    uint32_t len = OBJ(p_dir,dir)->data_length;
    directory_record *dir;
    int offset;
    CDROM_OBJ *obj;
    long cl;

    /* skip first two entries: */

    D(bug("[CDVDFS]\tIso_Open_Obj_In_Directory(%s)\n", p_name));
    dir = Get_Directory_Record(p_dir->volume, loc, 0);
    if (!dir)
    {
	D(bug("[CDVDFS]\tNo directory record for '.'\n"));
	return NULL;
    }

    offset = dir->length;
    dir = Get_Directory_Record(p_dir->volume, loc, offset);
    if (!dir)
    {
	D(bug("[CDVDFS]\tNo directory record for '..'\n"));
	return NULL;
    }

    offset += dir->length;
    for (;;)
    {
	if (offset >= len)
	{
	    D(bug("[CDVDFS]\tNo more directory records\n"));
	    global->iso_errno = ISOERR_NOT_FOUND;
	    return NULL;
	}

	dir = Get_Directory_Record(p_dir->volume, loc, offset);
	if (!dir)
	{
	    D(bug("[CDVDFS]\tFailed to read directory records\n"));
	    return NULL;
	}

	if (!dir->length)
	{
	    /* go to next logical sector: */
	    D(bug("[CDVDFS]\tRead dir advancing to next sector\n"));
	    offset = (offset & 0xfffff800) + 2048;
	    continue;
	}

	/* We skip files that are marked as associated */
	if (((dir->flags & FILE_FLAG_ASSOCIATED) == 0) &&
		Names_Equal(p_dir->volume, dir, p_name))
	    break;
	
	D(bug("[CDVDFS]\tRead dir advancing to next record\n"));
	offset += dir->length;
    }

    if (p_dir->volume->protocol == PRO_ROCK && (cl = RR_Child_Link(p_dir->volume, dir)) >= 0)
    {
	return Iso_Create_Directory_Obj(p_dir->volume, cl);
    }

    obj = Iso_Alloc_Obj(global, dir->length);
    if (!obj)
    {
	D(bug("[CDVDFS]\tFailed to create object\n"));
	return NULL;
    }

    obj->directory_f = (dir->flags & FILE_FLAG_DIRECTORY);
    obj->protection = (dir->flags & FILE_FLAG_HIDDEN) ? FIBF_HIDDEN : 0;
    if (p_dir->volume->protocol == PRO_ROCK && Is_A_Symbolic_Link(p_dir->volume, dir, &obj->protection))
    {
	obj->symlink_f = 1;
	obj->directory_f = 0;
    }
    CopyMem(dir, OBJ(obj,dir), dir->length);
    obj->volume = p_dir->volume;
    obj->pos = 0;
    if (!obj->directory_f)
	OBJ(obj,parent_loc) = loc;
    
    D(bug("[CDVDFS]\tObject %08lx created and initialized\n", obj));

    return obj;
}

/* Close a "CDROM object" and deallocate all associated resources.
 */

void Iso_Close_Obj(CDROM_OBJ *p_object) 
{
    D(bug("[CDVDFS]\tIso_Close_Obj(%08lx)\n", p_object));
    FreeMem (OBJ(p_object,dir), OBJ(p_object,dir)->length);
    FreeMem (p_object->obj_info, sizeof (t_iso_obj_info));
    FreeMem (p_object, sizeof (CDROM_OBJ));
    D(bug("[CDVDFS]\tObject disposed\n"));
}

/* Read bytes from a file.
 */

int Iso_Read_From_File(CDROM_OBJ *p_file, char *p_buffer, int p_buffer_length) 
{
    struct CDVDBase *global = p_file->global;
    uint32_t loc;
    int remain_block, remain_file, remain;
    int len;
    VOLUME *vol = p_file->volume;
    CDROM *cd = vol->cd;
    int buf_pos = 0;
    int todo;
    uint32_t ext_loc;
    short blockshift;
    int offset;
    uint32_t firstblock;

    D(bug("[CDVDFS]\tIso_Read_From_File()\n"));

    if (p_file->pos == OBJ(p_file,dir)->data_length)
    {
	/* at end of file: */
	return 0;
    }

    blockshift = VOL(vol,blockshift);
    /* 'firstblock' is the first logical block of the file section: */
    firstblock =
	OBJ(p_file,dir)->extent_loc + OBJ(p_file,dir)->ext_attr_length;
    /*
       'offset' is the offset of the first logical block of the file
       extent from the first logical (2048-byte-)sector.
       */
    if (blockshift)
	offset = ((firstblock & ((1<<blockshift)-1))<< (11-blockshift));
    else
	offset = 0;
    /*
       'ext_loc' is the first logical sector of the file extent.
       'loc' is the first logical sector to be read.
       */
    ext_loc = firstblock >> blockshift;
    loc = ext_loc + ((p_file->pos + offset) >> 11);
    todo = p_buffer_length;

    offset += p_file->pos;
    offset &= 2047; // this is the offset in first block;

    /*
     * how much data available in next read chunk?
     */
    remain_block = (SCSI_BUFSIZE << 4) - ((loc << 11) & ((SCSI_BUFSIZE << 4) - 1)) - offset;

    while (todo)
    {
	D(bug("[CDVDFS]\tRead: BPos: %6ld; FPos: %6ld; Ofst: %6ld; Blck: %6ld;\n", buf_pos, p_file->pos, offset, loc));

	if (!Read_Chunk(cd, loc))
	{
	    global->iso_errno = ISOERR_SCSI_ERROR;
	    return -1;
	}

	/*
	 * how much data left in a file?
	 */
	remain_file = OBJ(p_file,dir)->data_length - p_file->pos;

	/*
	 * how much data remain in file vs chunk?
	 */
	remain = (remain_block < remain_file) ? remain_block : remain_file;

	/*
	 * and how much user wants to read?
	 */
	len = (todo < remain) ? todo : remain;

	D(bug("[CDVDFS]\tRead: Chnk: %6ld; File: %6ld; Todo: %6ld; Read: %6ld;\n", remain_block, remain_file, todo, len));
	/*
	 * copy read data
	 */
	CopyMem ((APTR) (cd->buffer + offset), (APTR) (p_buffer + buf_pos), len);

	/*
	 * update positions, offsets, todo
	 */
	buf_pos += len;
	p_file->pos += len;
	todo -= len;

	/*
	 * if read it all - end
	 */
	if (p_file->pos >= OBJ(p_file,dir)->data_length)
	    break;

	remain_block = (SCSI_BUFSIZE << 4);
	offset = 0;

	loc = (loc + 16) &~ 15;
    }

    return buf_pos;
}

t_ulong Extract_Date(struct CDVDBase *global, directory_record *p_dir_record) 
{
    struct ClockData ClockData;

    D(bug("[CDVDFS]\tIso_Extract_Date\n"));
    ClockData.sec   = p_dir_record->second;
    ClockData.min   = p_dir_record->minute;
    ClockData.hour  = p_dir_record->hour;
    ClockData.mday  = p_dir_record->day;
    ClockData.wday  = 0; /* is ignored by CheckDate() and Date2Amiga() */
    ClockData.month = p_dir_record->month;
    ClockData.year  = p_dir_record->year + 1900;

    if (CheckDate (&ClockData))
	return Date2Amiga (&ClockData);
    else
	return 0;
}

/* Return information on a "CDROM object."
 */

int Iso_CDROM_Info(CDROM_OBJ *p_obj, CDROM_INFO *p_info) 
{
    int len;
    
    D(bug("[CDVDFS]\tIso_CDROM_Info\n"));

    if (Iso_Is_Top_Level_Object (p_obj))
    {
	p_info->name_length = 1;
	p_info->name[0] = ':';
	p_info->directory_f = TRUE;
	p_info->file_length = 0;
	p_info->date = Volume_Creation_Date(p_obj->volume);
	p_info->protection = 0;
	p_info->comment_length = 0;
    }
    else
    {
	directory_record *rec = OBJ(p_obj, dir);

	p_info->name_length = Get_File_Name(p_obj->volume, rec, p_info->name, sizeof(p_info->name));
	p_info->directory_f = p_obj->directory_f;
	p_info->symlink_f = p_obj->symlink_f;
	p_info->file_length = OBJ(p_obj,dir)->data_length;
	p_info->date = Extract_Date(p_obj->global, OBJ(p_obj,dir));
	p_info->protection = p_obj->protection;

	if (p_obj->volume->protocol == PRO_ROCK &&
		(len = Get_RR_File_Comment(p_obj->volume, rec, &p_info->protection, p_info->comment, sizeof(p_info->comment))) > 0)
	    p_info->comment_length = len;
	else
	    p_info->comment_length = 0;
    }

    return 1;
}

/* Browse all entries in a directory.
 */

int Iso_Examine_Next
	(CDROM_OBJ *p_dir, CDROM_INFO *p_info, uint32_t *p_offset)
{
    struct CDVDBase *global = p_dir->global;
    uint32_t offset;
    directory_record *rec;
    int len;

    D(bug("[CDVDFS]\tIso_Examine_Next()\n"));

    if (!p_dir->directory_f || p_dir->symlink_f)
    {
	global->iso_errno = ISOERR_BAD_ARGUMENTS;
	return 0;
    }

    if (*p_offset == 0)
    {
	/* skip first two directory entries: */

	rec = Get_Directory_Record
	    (
	     p_dir->volume,
	     OBJ(p_dir,dir)->extent_loc + OBJ(p_dir,dir)->ext_attr_length,
	     0
	    );
	if (!rec)
	    return 0;

	offset = rec->length;

	rec = Get_Directory_Record
	    (
	     p_dir->volume,
	     OBJ(p_dir,dir)->extent_loc + OBJ(p_dir,dir)->ext_attr_length,
	     offset
	    );
	if (!rec)
	    return 0;

	*p_offset = offset + rec->length;
    }

    do {
	for (;;)
	{
	    if (OBJ(p_dir,dir)->data_length <= *p_offset)
		return 0;

	    rec = Get_Directory_Record
		(
		 p_dir->volume,
		 OBJ(p_dir,dir)->extent_loc + OBJ(p_dir,dir)->ext_attr_length,
		 *p_offset
		);
	    if (!rec)
		return 0;

	    if (rec->length == 0)
	    {
		/* go to next logical sector: */
		*p_offset = (*p_offset & 0xfffff800) + 2048;
	    }
	    else
		break;
	}

	*p_offset += rec->length;
    } while (rec->flags & FILE_FLAG_ASSOCIATED);

    p_info->name_length = Get_File_Name(p_dir->volume, rec, p_info->name, sizeof(p_info->name));

    p_info->protection = (rec->flags & FILE_FLAG_HIDDEN) ? FIBF_HIDDEN : 0;

    if (p_dir->volume->protocol == PRO_ROCK && Is_A_Symbolic_Link(p_dir->volume, rec, &p_info->protection))
    {
	p_info->symlink_f = 1;
	p_info->directory_f = 0;
    }
    else if (p_dir->volume->protocol == PRO_ROCK &&	Has_System_Use_Field(p_dir->volume, rec, "CL"))
    {
	p_info->symlink_f = 0;
	p_info->directory_f = 1;
    }
    else
    {
	p_info->symlink_f = 0;
	p_info->directory_f = rec->flags & FILE_FLAG_DIRECTORY;
    }

    p_info->file_length = rec->data_length;
    p_info->date = Extract_Date(global, rec);

    if (p_dir->volume->protocol == PRO_ROCK &&
	    (len = Get_RR_File_Comment(p_dir->volume, rec, &p_info->protection, p_info->comment, sizeof(p_info->name))) > 0)
	p_info->comment_length = len;
    else
	p_info->comment_length = 0;

    p_info->suppl_info = rec;

    return 1;
}

/* Clone a "CDROM object info."
 */

void *Iso_Clone_Obj_Info(void *p_info) 
{
    t_iso_obj_info *info = (t_iso_obj_info *) p_info;
    t_iso_obj_info *new;

    D(bug("[CDVDFS]\tIso_Clone_Obj_Info\n"));

    new = AllocMem (sizeof (t_iso_obj_info), MEMF_PUBLIC);
    if (!new)
	return NULL;

    CopyMem(info, new, sizeof (t_iso_obj_info));

    new->dir = AllocMem (info->dir->length, MEMF_PUBLIC);
    if (!new->dir)
    {
	FreeMem (new, sizeof (t_iso_obj_info));
	return NULL;
    }
    CopyMem(info->dir, new->dir, info->dir->length);

    return new;
}

/* Find parent directory.
 */

CDROM_OBJ *Iso_Find_Parent(CDROM_OBJ *p_object) 
{
    directory_record *dir;
    uint32_t dir_loc;
    long pl;
    
    D(bug("[CDVDFS]\tIso_Find_Parent(%08lx)\n", p_object));

    if (p_object->directory_f)
	dir_loc =
	    OBJ(p_object,dir)->extent_loc + OBJ(p_object,dir)->ext_attr_length;
    else
	dir_loc = OBJ(p_object,parent_loc);

    dir = Get_Directory_Record(p_object->volume, dir_loc, 0);
    if (!dir)
	return NULL;

    if (p_object->directory_f)
    {
	dir = Get_Directory_Record(p_object->volume, dir_loc, dir->length);
	if (!dir)
	    return NULL;
	if (
		p_object->volume->protocol == PRO_ROCK &&
		(pl = RR_Parent_Link(p_object->volume, dir)) >= 0
	   )
	    return Iso_Create_Directory_Obj(p_object->volume, pl);
    }

    return Iso_Create_Directory_Obj(p_object->volume, dir->extent_loc);
}

/* Test if p_object is the root directory.
 */

t_bool Iso_Is_Top_Level_Object (CDROM_OBJ *p_object)
{
    return p_object->directory_f &&
	OBJ(p_object,dir)->extent_loc ==
	VOL(p_object->volume,pvd).root.extent_loc;
}

/* Test if two objects are equal.
 */

t_bool Iso_Same_Objects (CDROM_OBJ *p_obj1, CDROM_OBJ *p_obj2)
{
    return (OBJ(p_obj1,dir)->extent_loc ==
	    OBJ(p_obj2,dir)->extent_loc);
}

/*
 * Convert p_num digits into an integer value:
 */

int Digs_To_Int (char *p_digits, int p_num)
{
    int result = 0;
    int i;

    for (i=0; i<p_num; i++)
	result = result * 10 + p_digits[i] - '0';

    return result;
}

/*
 * Return volume creation date as number of seconds since 1-Jan-1978:
 */

t_ulong Iso_Creation_Date(VOLUME *p_volume) 
{
    struct CDVDBase *global = p_volume->global;
    struct ClockData ClockData;
    char *dt = VOL(p_volume,pvd).vol_creation;
    
    D(bug("[CDVDFS]\tIso_Creation_Date\n"));

    ClockData.sec   = Digs_To_Int (dt+12, 2);
    ClockData.min	  = Digs_To_Int (dt+10, 2);
    ClockData.hour  = Digs_To_Int (dt+8, 2);
    ClockData.mday  = Digs_To_Int (dt+6, 2);
    ClockData.wday  = 0; /* is ignored by CheckDate() and Date2Amiga() */
    ClockData.month = Digs_To_Int (dt+4, 2);
    ClockData.year  = Digs_To_Int (dt, 4);

    if (CheckDate (&ClockData))
	return Date2Amiga (&ClockData);
    else
	return 0;
}

t_ulong Iso_Volume_Size (VOLUME *p_volume)
{
    return VOL(p_volume,pvd).space_size;
}

t_ulong Iso_Block_Size (VOLUME *p_volume)
{
    return VOL(p_volume,pvd).block_size;
}

void Iso_Volume_ID(VOLUME *p_volume, char *p_buffer, int p_buf_length)
{
    int iso_len;
    int len;
    char buf[32];

    D(bug("[CDVDFS]\tIso_Volume_ID\n"));

    iso_len = Get_Volume_Name(p_volume, buf, sizeof(buf));
    for (; iso_len; iso_len--)
    {
	if (buf[iso_len-1] != ' ')
	    break;
    }
    len = (iso_len > p_buf_length-1) ? p_buf_length-1 : iso_len;
    if (len > 0)
	CopyMem(buf, p_buffer, len);
    p_buffer[len] = 0;
}

t_ulong Iso_Location (CDROM_OBJ *p_object)
{
    return OBJ(p_object,dir)->extent_loc;
}

t_ulong Iso_File_Length (CDROM_OBJ *p_obj)
{
    return OBJ(p_obj,dir)->data_length;
}

static t_handler const g_iso_handler = 
{
    Iso_Close_Vol_Info,
    Iso_Open_Top_Level_Directory,
    Iso_Open_Obj_In_Directory,
    Iso_Find_Parent,
    Iso_Close_Obj,
    Iso_Read_From_File,
    Iso_CDROM_Info,
    Iso_Examine_Next,
    Iso_Clone_Obj_Info,
    Iso_Is_Top_Level_Object,
    Iso_Same_Objects,
    Iso_Creation_Date,
    Iso_Volume_Size,
    Iso_Volume_ID,
    Iso_Location,
    Iso_File_Length,
    Iso_Block_Size
};

