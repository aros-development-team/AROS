/* main.c:
 *
 * Interactive test program for the ISO- and Rock-Ridge-support
 * routines.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 *
 * 28-Dec-12  neil   Adapted to new Read_TOC API.
 * 06-Mar-09  error  - Removed madness, fixed insanity. Cleanup started
 * 18-Aug-07  sonic  - Now builds on AROS.
 * 06-May-07  sonic  - Added separate "g" option for listing directories in Joliet format.
 *                   - Added support for protection bits and file comments.
 * 08-Mar-07  sonic  - Removed redundant "TRACKDISK" option.
 *                   - Removed unneeded dealing with block length.
 * 31-Mar-07  sonic  Added support for Joliet and character set translation.
 * 17-Feb-94   fmu   Fixed typo.
 * 28-Nov-93   fmu   Improved "cdrom d" command.
 * 12-Oct-93   fmu   "Show path table" function removed.
 * 09-Oct-93   fmu   Open utility.library.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dos/var.h>
#include <devices/trackdisk.h>

#include <clib/alib_protos.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include "cdrom.h"
#include "iso9660.h"
#include "joliet.h"
#include "rock.h"
#include "hfs.h"
#include "charset.h"
#include "globals.h"

#ifdef LATTICE
#include <pragmas/dos_pragmas.h>
#include <pragmas/exec_pragmas.h>
extern struct Library *SysBase, *DOSBase;
#endif

#undef DOSBase

#define STD_BUFFERS 20

#ifdef DEBUG_SECTORS
void dbprintf (char *p_dummy, ...)
{
}
#endif

struct CDVDBase glob;
struct CDVDBase *global = &glob;

void Cleanup (void)
{
  if (global->g_cd)
    Cleanup_CDROM (global->g_cd);

  if (CodesetsBase)
    CloseLibrary(CodesetsBase);
  if (UtilityBase)
    CloseLibrary ((struct Library*) UtilityBase);
}

void Usage (void)
{
  fprintf (stderr,
    "Usage: cdrom command [parameters]\n"
    "Commands:\n"
    "  a                Show information on CDROM drive\n"
    "  b                Read table of contents\n"
    "  c name           Show contents of file 'name'\n"
    "  d[rl] dir        Show contents of directory 'dir' (use ISO names)\n"
    "                   r=also show subdirectories, l=show additional information\n"
    "  e[r[l|L]] dir    Show contents of directory 'dir' (use Rock Ridge names)\n"
    "                   r=also show subdirectories, l=show system use field names\n"
    "                   L=show names and contents of system use fields\n"
    "  g[rl] dir        Show contents of girectory 'dir' (use Joliet names)\n"
    "                   r=also show subdirectories, l=show additional information\n"
    "  f dir name       Change to directory 'dir' and try to find object 'name'\n"
    "  i                Check which protocol is used\n"
    "  j [01]           0=start audio, 1=stop motor\n"
    "  l                Find offset of last session\n"
    "  m num            Read catalog node 'num' (MacHFS only)\n"
    "  o name           Try to open object 'name'\n"
    "  r                Read contents of root directory\n"
    "  s num [cnt]      Read 'cnt' sectors, starting at sector 'num'\n"
    "  t name           Try to open parent of object 'name'\n"
    "  v                Read primary volume descriptor\n"
    "  x dens [length]  Select mode: dens=density code,\n"
    "                   length=block length (default: 2048)\n"
    "  z                Send test unit ready command\n"
    "  T                Test trackdisk device\n"
    "Use \":\" as the name of the root directory\n"
    );
  exit (1);
}

char *MKSTR (char *p_in, int p_length, char *p_out)
{
  char *res = p_out;
  int len = p_length;
  int i;
  
  while (len && p_in[len-1] == ' ')
    len--;

  for (i=0; i<len; i++)
    *p_out++ = *p_in++;
    
  *p_out = 0;
  
  return res;
}

void Show_Flags (unsigned char p_flags)
{
  if (p_flags & 1)
    printf ("existence ");
  if (p_flags & 2)
    printf ("directory ");
  if (p_flags & 4)
    printf ("associated ");
  if (p_flags & 8)
    printf ("record ");
  if (p_flags & 16)
    printf ("protection ");
  if (p_flags & 128)
    printf ("multi-extent ");
}

void Show_Directory_Record (directory_record *p_dir)
{
  char buf[256];

  printf ("Extended Attr Record Length: %d\n", (int) p_dir->ext_attr_length);
  printf ("Location of Extent:          %lu\n", (unsigned long)p_dir->extent_loc);
  printf ("Data Length:                 %lu\n", (unsigned long)p_dir->data_length);
  printf ("Recording Date and Time:     %02d.%02d.19%02d %02d:%02d:%02d %+d\n",
  	  (int) p_dir->day, (int) p_dir->month, (int) p_dir->year,
	  (int) p_dir->hour, (int) p_dir->minute, (int) p_dir->second,
	  (int) p_dir->tz);
  printf ("Flags:                       %d (", (int) p_dir->flags);
  Show_Flags (p_dir->flags);
  printf (")\n");
  printf ("File Unit Size:              %d\n", (int) p_dir->file_unit_size);
  printf ("Gap Size:                    %d\n", (int) p_dir->gap_size);
  printf ("Volume Sequence Number:      %hu\n", p_dir->sequence);
  printf ("File Identifier:             ");
  if (p_dir->file_id[0] == 0)
    printf ("(00)\n");
  else if (p_dir->file_id[0] == 1)
    printf ("(01)\n");
  else
    printf ("%s\n", MKSTR (p_dir->file_id, p_dir->file_id_length, buf));
}

void Find_Block_Starting_With (CDROM *p_cd, int p_val)
{
  uint32_t sec = 0;
  int cmp;
  int i;
  
  for (;;) {
    if (!Read_Chunk (p_cd, sec)) {
      fprintf (stderr, "cannot read sector 16\n");
      exit (1);
    }
    for (i=0; i<4; i++) {
      cmp = p_cd->buffer[i<<9] * 256 + p_cd->buffer[(i<<9)+1];
      if (cmp == p_val)
	printf ("sector %lu, block %d\n", (unsigned long)sec, i);
    }
    sec++;
  }
}

void Show_Primary_Volume_Descriptor (CDROM *p_cd)
{
  prim_vol_desc *pvd;
  char buf[129];
  int blk;
  t_mdb mdb;
  t_hdr_node hdr;
  int skip;
  t_ulong offset, svd_offset;
  int protocol;
  t_bool hfs;
  
  hfs = Uses_HFS_Protocol (p_cd, &skip);
  protocol = Which_Protocol (p_cd, TRUE, TRUE, &skip, &offset, &svd_offset);
  
  if (protocol == PRO_UNKNOWN) {
    printf ("Unknown protocol\n");
    return;
  }
  
  if (protocol == PRO_HIGH_SIERRA) {
    printf ("High sierra protocol (not supported)\n");
  }
  
  if ((protocol == PRO_ROCK || protocol == PRO_ISO) && hfs)
    printf ("Multi-platform disk: HFS & ISO\n");

  if (protocol == PRO_ROCK)
    printf ("Rock Ridge extensions available, skip size = %d\n", skip);
  
  if (protocol == PRO_ISO || protocol == PRO_JOLIET)
    printf ("Data track offset = %lu\n", (unsigned long)offset);

  if (protocol == PRO_JOLIET)
    printf ("Joliet SVD offset = %lu\n", (unsigned long)svd_offset);

  if (protocol == PRO_ISO || protocol == PRO_ROCK || protocol == PRO_JOLIET) {

    if (!Read_Chunk (p_cd, 16 + offset)) {
      fprintf (stderr, "cannot read sector %lu\n", 16 + (unsigned long)offset);
      exit (1);
    }

    pvd = (prim_vol_desc *) p_cd->buffer;
    printf ("--- ISO-9660: ---\n");
    printf ("Volume Descriptor Type:          %d\n", (int) pvd->type);
    printf ("Standard Identifier:             %s\n", MKSTR (pvd->id,5,buf));
    printf ("Volume Descriptor Version:       %d\n", (int) pvd->version);
    printf ("System Identifier:               %s\n", MKSTR (pvd->system_id,32,buf));
    printf ("Volume Identifier:               %s\n", MKSTR (pvd->volume_id,32,buf));
    printf ("Volume Space Size:               %lu\n", (unsigned long) pvd->space_size);
    printf ("Volume Set Size:                 %hu\n", pvd->set_size);
    printf ("Volume Sequence Number:          %hu\n", pvd->sequence);
    printf ("Logical Block Size:              %hu\n", pvd->block_size);
    printf ("Path Table Size:                 %lu\n", (unsigned long) pvd->path_size);
    printf ("Location of Occ of M Path Table: %lu\n", (unsigned long) pvd->table);
    printf ("Location of Occ of Opt M Path T: %lu\n", (unsigned long) pvd->opt_table);
    printf ("Volume Set Identifier:           %s\n",
  					MKSTR (pvd->volume_set_id,128,buf));  
    printf ("Publisher Identifier:            %s\n",
  					MKSTR (pvd->publisher_id,128,buf)); 
    printf ("Data Preparer Identifier:        %s\n",
  					MKSTR (pvd->data_preparer,128,buf));
    printf ("Application Identifier:          %s\n",
  					MKSTR (pvd->application_id,128,buf));
    printf ("Copyright File Identifier:       %s\n",
  					MKSTR (pvd->copyright,37,buf));
    printf ("Abstract File Identifier:        %s\n",
  					MKSTR (pvd->abstract_file_id,37,buf));
    printf ("Bibliographic File Identifier:   %s\n",
  					MKSTR (pvd->bibliographic_id,37,buf));
    printf ("File Structure Version:          %d\n",
  					(int) pvd->file_structure_version);
    printf ("ROOT DIRECTORY RECORD:\n");
    Show_Directory_Record (&pvd->root);
  }
  
  if (hfs) {
    if ((blk = HFS_Find_Master_Directory_Block (p_cd, &mdb)) < 0) {
      printf ("No master directory block found\n");
      exit (1);
    }
    printf ("--- MacHFS: ---\n");
    printf ("Master directory block located at block %d\n", blk);
    printf ("Volume signature:               0x%hx\n", mdb.SigWord);
    printf ("Date/Time of creation:          %lu\n", (unsigned long)mdb.CrDate);
    printf ("Date/Time of last modification: %lu\n", (unsigned long)mdb.LsMod);
    printf ("Volume attributes:              0x%hx\n", mdb.Atrb);
    printf ("Number of files in root dir:    %u\n", mdb.NmFls);
    printf ("Allocation block size:          %lu bytes\n", (unsigned long)mdb.AlBlkSiz);
    printf ("Loc of first allocation block:  %u\n", mdb.AlBlSt);
    printf ("Volume name:                    %s\n",
    				MKSTR ((char *) mdb.VolName, mdb.VolNameLen,buf));
    printf ("Number of files in volume:      %lu\n", (unsigned long)mdb.FilCnt);
    printf ("Number of dirs in volume:       %lu\n", (unsigned long)mdb.DirCnt);
    printf ("Size of catalog file:           %lu allocation blocks\n",
    						(unsigned long)mdb.CTFlSize);
    printf ("Extent record for catalog file:\n");
    printf ("  1. allocation block: %10lu   length: %10lu\n",
    				(unsigned long)mdb.CTExtRec[0].StABN, (unsigned long)mdb.CTExtRec[0].NumABlks);
    printf ("  2. allocation block: %10lu   length: %10lu\n",
    				(unsigned long)mdb.CTExtRec[1].StABN, (unsigned long)mdb.CTExtRec[1].NumABlks);
    printf ("  3. allocation block: %10lu   length: %10lu\n",
    				(unsigned long)mdb.CTExtRec[2].StABN, (unsigned long)mdb.CTExtRec[2].NumABlks);
    if (!HFS_Get_Header_Node (p_cd, blk, &mdb, &hdr))
      printf ("*** Cannot read header node!!!\n");
    printf ("Header node:\n");
    printf ("Depth of tree:                  %u\n", hdr.Depth);
    printf ("Number of root node:            %lu\n", (unsigned long)hdr.Root);
    printf ("Number of leaf records in tree: %lu\n", (unsigned long)hdr.NRecs);
    printf ("Number of first leaf node:      %lu\n", (unsigned long)hdr.FNode);
    printf ("Number of last leaf node:       %lu\n", (unsigned long)hdr.LNode);
  }
}

void Show_Directory (CDROM *p_cd, uint32_t p_location, uint32_t p_length)
{
  int cnt = 0;
  int pos = 0;
  
  if (!Read_Chunk (p_cd, p_location)) {
    fprintf (stderr, "cannot read sector %lu\n", (unsigned long)p_location);
    exit (1);
  }

  while (cnt < p_length) {
    directory_record *dir = (directory_record *) (p_cd->buffer + pos);
    
    if (dir->length == 0)
      break;
    Show_Directory_Record (dir);
    cnt += dir->length;
    pos += dir->length;
    if (cnt < p_length) {
      printf ("------------------------------------------------------------\n");
      if (pos >= 2048) {
	if (!Read_Chunk (p_cd, ++p_location)) {
	  fprintf (stderr, "cannot read sector %lu\n", (unsigned long)p_location);
	  exit (1);
	}
	pos = 0;
      }
    }
  }
}

void Show_Root_Directory (CDROM *p_cd)
{
  prim_vol_desc *pvd;
  
  if (!Read_Chunk (p_cd, 16)) {
    fprintf (stderr, "cannot read sector 16\n");
    exit (1);
  }

  pvd = (prim_vol_desc *) p_cd->buffer;

  Show_Directory (p_cd, pvd->root.extent_loc, pvd->root.data_length);
}

void Check_Protocol (CDROM *p_cd)
{
  int skip;
  t_ulong offset, svd_offset;

  switch (Which_Protocol (p_cd, TRUE, TRUE, &skip, &offset, &svd_offset)) {
  case PRO_ROCK:
    printf ("Rock Ridge protocol, skip length = %d\n", skip);
    break;
  case PRO_ISO:
    printf ("ISO-9660 protocol, offset = %lu\n", (unsigned long)offset);
    break;
  case PRO_JOLIET:
    printf ("Joliet protocol, offset = %lu SVD offset = %lu\n", (unsigned long)offset, (unsigned long)svd_offset);
    break;
  case PRO_HFS:
    printf ("Macintosh HFS protocol\n");
    break;
  case PRO_HIGH_SIERRA:
    printf ("High Sierra protocol\n");
    break;
  default:
    printf ("Unknown protocol, iso_errno = %d\n", global->iso_errno);
    break;
  }
}

void Try_To_Open (CDROM *p_cd, char *p_directory, char *p_name)
{
  VOLUME *vol;
  CDROM_OBJ *top = NULL;
  CDROM_OBJ *home;
  CDROM_OBJ *obj;
  CDROM_OBJ *parent;
  char pathname[256];

  if (!(vol = Open_Volume (p_cd, 1, 1))) {
    fprintf (stderr, "cannot open volume; iso_errno = %d\n", global->iso_errno);
    exit (1);
  }

  if (p_directory && p_directory != (char *) -1) {
    if (!(top = Open_Top_Level_Directory (vol))) {
      fprintf (stderr, "cannot open top level directory\n");
      Close_Volume (vol);
      exit (1);
    }
    
    if (!(home = Open_Object (top, p_directory))) {
      fprintf (stderr, "cannot open top level directory\n");
      Close_Object (top);
      Close_Volume (vol);
      exit (1);
    }
  } else {
    if (!(home = Open_Top_Level_Directory (vol))) {
      fprintf (stderr, "cannot open home directory;"
		       " iso_errno = %d\n", global->iso_errno);
      Close_Volume (vol);
      exit (1);
    }
  }
  obj = Open_Object (home, p_name);
  if (obj) {
    CDROM_INFO info;
    printf ("%s '%s' found, location = %lu\n",
	    obj->symlink_f ? "Symbolic link" :
	    obj->directory_f ? "Directory" : "File",
    	    p_name, (unsigned long)Location (obj));
    printf ("Protection bits: 0x%08lX\n", (unsigned long)obj->protection);
    if (obj->symlink_f) {
      char linkname[256];
      printf ("Link to ");
      if (Get_Link_Name (obj, linkname, sizeof (linkname)))
	printf ("'%s'\n", linkname);
      else
	printf ("unknown file or directory\n");
    }
    if (Full_Path_Name (obj, pathname, sizeof (pathname)))
      printf ("Full path name: %s\n", pathname);
    else
      printf ("Full path name unknown\n");
    if (CDROM_Info (obj, &info)) {
      printf ("INFO Name = ");
      fwrite (info.name, info.name_length, 1, stdout);
      printf ("\n");
      printf ("INFO Comment = ");
      fwrite (info.comment, info.comment_length, 1, stdout);
      printf ("\n");
    } else
      printf ("CANNOT FIND INFO FOR OBJECT!\n");
    if (p_directory == (char *) -1) {
      parent = Find_Parent (obj);
      if (parent) {
	printf ("parent found, location = %lu\n",
	        (unsigned long)Location (parent));
	Close_Object (parent);
      } else
	printf ("parent not found, iso_errno = %d\n", global->iso_errno);
    }
    Close_Object (obj);
  } else {
    if (global->iso_errno == ISOERR_NOT_FOUND)
      printf ("Object '%s' not found\n", p_name);
    else
      printf ("Object '%s': iso_errno = %d\n", p_name, global->iso_errno);
  }

  if (top)
    Close_Object (top);
  Close_Object (home);
  Close_Volume (vol);
}

void Show_File_Contents (CDROM *p_cd, char *p_name)
{
  VOLUME *vol;
  CDROM_OBJ *home;
  CDROM_OBJ *obj;
#define THEBUFSIZE 99
  char buffer[THEBUFSIZE];
  int cnt;

  if (!(vol = Open_Volume (p_cd, 1, 1))) {
    fprintf (stderr, "cannot open volume; iso_errno = %d\n", global->iso_errno);
    exit (1);
  }

  if (!(home = Open_Top_Level_Directory (vol))) {
    fprintf (stderr, "cannot open top level directory;"
		     " iso_errno = %d\n", global->iso_errno);
    Close_Volume (vol);
    exit (1);
  }
  obj = Open_Object (home, p_name);
  if (obj) {
    for (;;) {
      cnt = Read_From_File (obj, buffer, THEBUFSIZE);
      if (cnt == -1) {
	fprintf (stderr, "cannot read from file!\n");
	break;
      }
      if (cnt == 0)
	break;
      fwrite (buffer, cnt, 1, stdout);
    }

    putchar ('\n');
    Close_Object (obj);
  } else {
    fprintf (stderr, "Object '%s': iso_errno = %d\n", p_name, global->iso_errno);
  }

  Close_Object (home);
  Close_Volume (vol);
  
}

void Print_System_Use_Fields (CDROM *p_cd, directory_record *p_dir,
			      int p_skip_size, int p_long)
{
  int system_use_pos;
  int slen;
  uint32_t length = p_dir->length;
  unsigned char *buf = (unsigned char *) p_dir;

  printf ("   system use fields: ");
  if (p_long)
    putchar ('\n');

  system_use_pos = 33 + p_dir->file_id_length;
  if (system_use_pos & 1)
    system_use_pos++;
  system_use_pos += p_skip_size;

  /* the system use field must be at least 4 bytes long */
  while (system_use_pos + 3 < length) {
    slen = buf[system_use_pos+2];
    /* look for continuation area: */
    if (buf[system_use_pos] == 'C' &&
	buf[system_use_pos+1] == 'E') {
      uint32_t newloc, offset;
      printf ("/ ");
      memcpy (&newloc, buf + system_use_pos + 8, 4);
      memcpy (&offset, buf + system_use_pos + 16, 4);
      memcpy (&length, buf + system_use_pos + 24, 4);
      if (!Read_Chunk (p_cd, newloc))
	return;
      buf = p_cd->buffer;
      system_use_pos = offset;
      continue;
    /* look for system use field terminator: */
    } else if (buf[system_use_pos] == 'S' &&
	buf[system_use_pos+1] == 'T') {
      printf ("ST");
      break;
    } else if  (buf[system_use_pos] == 'S' &&
      /* special handling for SL field: */
      buf[system_use_pos+1] == 'L') {
      printf ("SL(%s%d,", (buf[system_use_pos+4] & 1)? "cont " : "",
      			  (int) buf[system_use_pos+5]);
      fwrite (buf + system_use_pos + 7, buf[system_use_pos+6], 1, stdout);
      printf (") ");
    } else {
      if (p_long)
	printf ("   ");
      putchar (buf[system_use_pos]);
      putchar (buf[system_use_pos+1]);
      putchar (' ');
      if (p_long) {
	int i;
	for (i=0; i < slen-2; i++) {
	  if (i && (i & 15) == 0)
	    printf ("\n      ");
	  printf ("%2.2x ", (int) buf[system_use_pos+i+2]);
	}
      }
    }

    if (p_long)
      putchar ('\n');

    system_use_pos += slen;
  }
  
  putchar ('\n');
}

void Show_Subdirectory (CDROM_OBJ *p_home, char *p_name, int p_long_info,
			int p_recursive)
{
  CDROM_OBJ *obj;
  CDROM_INFO info;
  VOLUME *vol = p_home->volume;
  obj = Open_Object (p_home, p_name);
  if (obj) {
    uint32_t offset = 0;

    while (Examine_Next (obj, &info, &offset)) {
     directory_record *dir = info.suppl_info;
      fwrite (info.name, info.name_length, 1, stdout);
      if (info.symlink_f)
	printf (" (symbolic link)");
      else if (info.directory_f)
	printf (" (dir)");
      printf ("\n");
      if (p_long_info && dir) {
	printf ("   %02d.%02d.%02d %02d:%02d:%02d  ",
		(int) dir->day,
		(int) dir->month,
		(int) dir->year,
		(int) dir->hour,
		(int) dir->minute,
		(int) dir->second);
	printf ("%lu  ", (unsigned long)dir->data_length);
	printf ("loc=%lu  ", (unsigned long)dir->extent_loc);
	Show_Flags (dir->flags);
	putchar ('\n');
	if (dir->ext_attr_length)
	  printf ("   contains extended attribute record\n");
	if (dir->gap_size)
	  printf ("   is recorded in interleaved mode\n");
	if (vol->protocol == PRO_ROCK)
	  Print_System_Use_Fields (vol->cd, dir,
	  			   ((t_iso_vol_info *) vol->vol_info)->skip,
				   p_long_info == 2);
      }
    }

    Close_Object (obj);
  } else {
    fprintf (stderr, "Object '%s': iso_errno = %d\n", p_name, global->iso_errno);
    return;
  }

  if (p_recursive) {
    obj = Open_Object (p_home, p_name);
    if (obj) {
      uint32_t offset = 0;
    
      while (Examine_Next (obj, &info, &offset)) {
	if (info.directory_f) {
	  char *name = malloc (strlen (p_name) + info.name_length + 2);
	  int len;
	  if (!name) {
	    fprintf (stderr, "out of memory\n");
	    exit (1);
	  }
	  if (Is_Top_Level_Object (obj))
	    name[0] = 0;
	  else
	    sprintf (name, "%s/", p_name);
	  len = strlen (name) + info.name_length;
	  memcpy (name + strlen (name), info.name, info.name_length);
	  name[len] = 0;
	  printf ("\n%s:\n", name);
	  Show_Subdirectory (p_home, name, p_long_info, TRUE);
	  free (name);
	}
      }
      Close_Object (obj);
    } else {
      fprintf (stderr, "Object '%s': iso_errno = %d\n", p_name, global->iso_errno);
    }
  }

}

void Show_Dir_Contents (CDROM *p_cd, char *p_name, int p_rock_ridge,
			int p_joliet, char *p_options)
{
  VOLUME *vol;
  CDROM_OBJ *home;
  int long_info = strchr (p_options, 'l') ? 1 :
  		  strchr (p_options, 'L') ? 2 : 0;
  t_bool recursive = (strchr (p_options, 'r') != NULL);

  if (!(vol = Open_Volume (p_cd, p_rock_ridge, p_joliet))) {
    fprintf (stderr, "cannot open volume; iso_errno = %d\n", global->iso_errno);
    exit (1);
  }

  if (!(home = Open_Top_Level_Directory (vol))) {
    fprintf (stderr, "cannot open top level directory;"
		     " iso_errno = %d\n", global->iso_errno);
    Close_Volume (vol);
    exit (1);
  }

  Show_Subdirectory (home, p_name, long_info, recursive);

  Close_Object (home);
  Close_Volume (vol);
}

void Send_Test_Unit_Ready (CDROM *p_cd)
{
  printf ("result = %d\n", Test_Unit_Ready (p_cd));
}

void Show_Sectors (CDROM *p_cd, int p_sector, int p_cnt)
{
  int i, j, s;
  int off;
  
  if (p_sector < 0)
    return;
  
  for (s=0; s<p_cnt; s++) {
    if (!Read_Chunk (p_cd, p_sector + s)) {
      fprintf (stderr, "cannot read sector %d\n", p_sector + s);
      exit (1);
    }
    for (off=0, i=0; i<128; i++) {
      printf ("%02x:%03x0: ", s, i);
      for (j=0; j<16; j++)
	printf ("%02x ", (int) p_cd->buffer[off++]);
      off -= 16;
      putchar (' ');
      for (j=0; j<16; j++) {
	unsigned char c = p_cd->buffer[off++];
	if (32<=c && c<=127)
	  putchar (c);
	else
	  putchar ('.');
      }
      putchar ('\n');
    }
  }
}

int Get_Device_And_Unit (void)
{
  int len;
  char buf[10];
  
  len = GetVar ((UBYTE *) "CDROM_DEVICE", (UBYTE *) global->g_device,
  		sizeof (global->g_device), 0);
  if (len < 0)
    return 0;
  if (len >= sizeof (global->g_device)) {
    fprintf (stderr, "CDROM_DEVICE too long\n");
    exit (1);
  }
  global->g_device[len] = 0;
  
  len = GetVar ((UBYTE *) "CDROM_UNIT", (UBYTE *) buf,
  		sizeof (buf), 0);
  if (len < 0)
    return 0;
  if (len >= sizeof (buf)) {
    fprintf (stderr, "CDROM_UNIT too long\n");
    exit (1);
  }
  buf[len] = 0;
  global->g_unit = atoi (buf);

  if (GetVar ((UBYTE *) "CDROM_FASTMEM", (UBYTE *) buf,
      sizeof (buf), 0) > 0) {
    fprintf (stderr, "using fastmem\n");
    global->g_memory_type = MEMF_FAST;
  }

  return 1;
}

void Select_Mode (CDROM *p_cd, int p_mode, int p_block_length)
{
  if (!Mode_Select (p_cd, p_mode, p_block_length))
    fprintf (stderr, "mode select command failed!\n");
}

void Show_Drive_Information (CDROM *p_cd)
{
  t_inquiry_data data;
  
  if (!Inquire (p_cd, &data)) {
    fprintf (stderr, "cannot access CDROM drive\n");
    return;
  }

  if ((data.peripheral_type & 0x1f) != 5) {
    if ((data.peripheral_type & 0x1f) == 8)
      printf ("Drive type: medium changer\n");
    else
      printf ("WARNING: this is not a CDROM drive!\n");
  }

  printf ("Vendor      : ");
  fwrite (data.vendor, sizeof (data.vendor), 1, stdout);
  printf ("\nProduct     : ");
  fwrite (data.product, sizeof (data.product), 1, stdout);
  printf ("\nRevision    : ");
  fwrite (data.revision, sizeof (data.revision), 1, stdout);
  putchar ('\n');
  printf ("Conforms to : SCSI-%d\n", (int) (data.version & 0x7));
}

void Show_Table_Of_Contents (CDROM *p_cd)
{
  uint32_t toc_len;
  t_toc_data *toc;
  short i, len;
  
  if (!(toc = Read_TOC (p_cd, &toc_len))) {
    fprintf (stderr, "cannot read table of contents\n");
    return;
  }
  
  len = toc_len / 8;
  for (i=0; i<len; i++) {
    if (toc[i].track_number == 0xAA)
      printf ("Lead-out track at address %lu\n", (unsigned long)toc[i].address);
    else
      printf ("Track %d: %s track, start address %lu\n",
    	      (int) toc[i].track_number,
	      (toc[i].flags & 4) ? "data" : "audio",
	      (unsigned long)toc[i].address);
  }
}

void Show_Catalog_Node (CDROM *p_cd, t_ulong p_node)
{
  t_mdb mdb;
  t_node_descr *node;
  t_hdr_node *hdr;
  t_idx_record *idx;
  t_leaf_record *leaf;
  t_file_record *file;
  t_dir_thread_record *thread;
  char *cp;
  int blk;
  int i;
  char buf[100];

  blk = HFS_Find_Master_Directory_Block (p_cd, &mdb);
  if (blk < 0) {
    fprintf (stderr, "cannot find master directory block\n");
    return;
  }

  node = HFS_Get_Node (p_cd, blk, &mdb, p_node);
  if (!node) {
    fprintf (stderr, "cannot find node %lu\n", (unsigned long)p_node);
    return;
  }
  
  switch (node->Type) {
  case 0:
    printf ("Index node:\n");
    printf ("Fwd=%lu Bwd=%lu Level=%d\n", (unsigned long)node->FLink, (unsigned long)node->BLink,
    	    (int) node->NHeight);
    idx = (t_idx_record *) ((char *) node + 0xe);
    for (i=0; i<node->NRecs; i++, idx++) {
      printf ("Parent ID = 0x%08lx, '", (unsigned long)idx->parent_id);
      fwrite (idx->name, idx->name_length, 1, stdout);
      printf ("', pointer = %lu\n", (unsigned long)idx->pointer);
    }
    break;
  case 1:
    printf ("Header node:\n");
    printf ("Fwd=%lu Bwd=%lu Level=%d\n", (unsigned long)node->FLink, (unsigned long)node->BLink,
    	    (int) node->NHeight);
    hdr = (t_hdr_node *) node;
    printf ("Depth of tree:                  %u\n", (unsigned)hdr->Depth);
    printf ("Number of root node:            %lu\n", (unsigned long)hdr->Root);
    printf ("Number of leaf records in tree: %lu\n", (unsigned long)hdr->NRecs);
    printf ("Number of first leaf node:      %lu\n", (unsigned long)hdr->FNode);
    printf ("Number of last leaf node:       %lu\n", (unsigned long)hdr->LNode);
    break;
  case 2:
    printf ("Map node:\n");
    printf ("Fwd=%lu Bwd=%lu Level=%d\n", (unsigned long)node->FLink, (unsigned long)node->BLink,
    	    (int) node->NHeight);
    break;
  case 0xff:
    printf ("Leaf node:\n");
    printf ("Fwd=%lu Bwd=%lu Level=%d\n", (unsigned long)node->FLink, (unsigned long)node->BLink,
    	    (int) node->NHeight);
    for (i=0; i<node->NRecs; i++) {
      leaf = (t_leaf_record *) ((char *) node + ((short *) node)[255-i]);
      cp = (char *) leaf + leaf->length + 1;
      if ((leaf->length & 1) == 0)
	cp++;
      printf ("Parent ID = 0x%08lx, '", (unsigned long)leaf->parent_id);
      memcpy (buf, leaf->name, leaf->name_length);
      Convert_Mac_Characters (buf, leaf->name_length);
      fwrite (buf, leaf->name_length, 1, stdout);
      printf ("'  (");
      switch (*cp) {
      case 1:
	printf ("directory 0x%08lx)\n", (unsigned long)*(t_ulong *)(cp+6));
	break;
      case 2:
	file = (t_file_record *) cp;
	printf ("file 0x%08lx)\n\tdata length %lu, "
		"data extents %u-%u/%u-%u/%u-%u\n",
		(unsigned long)file->FlNum,
		(unsigned long)file->LgLen,
		(unsigned)file->ExtRec[0].StABN, (unsigned)file->ExtRec[0].NumABlks,
		(unsigned)file->ExtRec[1].StABN, (unsigned)file->ExtRec[1].NumABlks,
		(unsigned)file->ExtRec[2].StABN, (unsigned)file->ExtRec[2].NumABlks);
	printf ("\tresource length %lu, resource extents %u-%u/%u-%u/%u-%u\n",
		(unsigned long)file->RLgLen,
		(unsigned) file->RExtRec[0].StABN, (unsigned) file->RExtRec[0].NumABlks,
		(unsigned) file->RExtRec[1].StABN, (unsigned) file->RExtRec[1].NumABlks,
		(unsigned) file->RExtRec[2].StABN, (unsigned) file->RExtRec[2].NumABlks);
	printf ("\tfirst alloc blk for data fork: %u\n", file->StBlk);
	printf ("\tfirst alloc blk for resource fork: %u\n", file->RStBlk);
	break;
      case 3:
	thread = (t_dir_thread_record *) cp;
	printf ("directory thread 0x%08lu '", (unsigned long)thread->ParID);
	fwrite (thread->CName, thread->CNameLen, 1, stdout);
	printf ("')\n");
	break;
      case 4:
	printf ("file thread)\n");
	break;
      default:
	printf ("unknown)\n");
	break;
      }
    }
    break;
  default:
    printf ("unknown node type\n");
    return;
  }

}

void Play_Audio (CDROM *p_cd, int p_stop)
{
  int result;

  if (p_stop)
    result = Stop_Play_Audio (p_cd);
  else
    result = Start_Play_Audio (p_cd);
  
  if (!result)
    fprintf (stderr, "cannot perform operation\n");
}

void Find_Offset_Of_Last_Session (CDROM *p_cd)
{
  uint32_t last;
  
  if (!Find_Last_Session (p_cd, &last)) {
    fprintf (stderr, "cannot determine offset of last session\n");
  } else {
    printf ("offset of last session: %lu\n", (unsigned long)last);
  }
}

void Test_Trackdisk_Device (CDROM *p_cd)
{
  int state, num;
  
  for (;;) {
    p_cd->scsireq->io_Flags = IOF_QUICK;
    p_cd->scsireq->io_Command = TD_CHANGESTATE;
    BeginIO ((struct IORequest*) p_cd->scsireq);
    state = p_cd->scsireq->io_Actual;
    p_cd->scsireq->io_Flags = IOF_QUICK;
    p_cd->scsireq->io_Command = TD_CHANGENUM;
    BeginIO ((struct IORequest*) p_cd->scsireq);
    num = p_cd->scsireq->io_Actual;
    printf ("%s, changenum = %d\n", state ? "no disk present" : "disk present",
    	    num);
    Delay (50);
  }
}

int main (int argc, char *argv[])
{
  global->g_cd = NULL;
  global->g_memory_type = MEMF_CHIP;
  atexit (Cleanup);

  if (argc < 2)
    Usage ();

  if (!(UtilityBase = (APTR)OpenLibrary("utility.library", 37))) {
    fprintf (stderr, "cannot open utility.library\n");
    exit (1);
  }

  if (!Get_Device_And_Unit ()) {
    fprintf (stderr,
      "Please set the following environment variables:\n"
      "  CDROM_DEVICE    name of SCSI device\n"
      "  CDROM_UNIT      unit number of CDROM drive\n"
      "e.g.\n"
      "  setenv CDROM_DEVICE scsi.device\n"
      "  setenv CDROM_UNIT 2\n"
      "Set the variable CDROM_FASTMEM to any value if you\n"
      "want to use fast memory for SCSI buffers (does not work\n"
      "with all SCSI devices!)\n"
      "Set the variable CDROM_UNICODETABLE to file name of external\n"
      "Unicode conversion table file if you want to see file names\n"
      "with national characters on Joliet volumes\n"
      );
    exit (1);
  }

  CodesetsBase = NULL;
  InitCharset(global);

  global->g_cd = Open_CDROM (global, global->g_device, global->g_unit, global->g_memory_type,
  		   STD_BUFFERS, 0);
  if (!global->g_cd) {
    fprintf (stderr, "cannot open CDROM, error code = %d\n", global->g_cdrom_errno);
    exit (1);
  }

  if (argv[1][0] == 'a' && argc == 2)
    Show_Drive_Information (global->g_cd);
  else if (argv[1][0] == 'b' && argc == 2)
    Show_Table_Of_Contents (global->g_cd);
  else if (argv[1][0] == 'c' && argc == 3)
    Show_File_Contents (global->g_cd, argv[2]);
  else if (argv[1][0] == 'd' && argc == 3)
    Show_Dir_Contents (global->g_cd, argv[2], 0, 0, argv[1]+1);
  else if (argv[1][0] == 'e' && argc == 3)
    Show_Dir_Contents (global->g_cd, argv[2], 1, 0, argv[1]+1);
  else if (argv[1][0] == 'g' && argc == 3)
    Show_Dir_Contents (global->g_cd, argv[2], 0, 1, argv[1]+1);
  else if (argv[1][0] == 'f' && argc == 4)
    Try_To_Open (global->g_cd, argv[2], argv[3]);
  else if (argv[1][0] == 'i')
    Check_Protocol (global->g_cd);
  else if (argv[1][0] == 'j' && argc == 3)
    Play_Audio (global->g_cd, atoi (argv[2]));
  else if (argv[1][0] == 'l' && argc == 2)
    Find_Offset_Of_Last_Session (global->g_cd);
  else if (argv[1][0] == 'm' && argc == 3)
    Show_Catalog_Node (global->g_cd, atoi (argv[2]));
  else if (argv[1][0] == 'o' && argc == 3)
    Try_To_Open (global->g_cd, NULL, argv[2]);
  else if (argv[1][0] == 'r')
    Show_Root_Directory (global->g_cd);
  else if (argv[1][0] == 's' && argc == 3)
    Show_Sectors (global->g_cd, atoi (argv[2]), 1);
  else if (argv[1][0] == 's' && argc == 4)
    Show_Sectors (global->g_cd, atoi (argv[2]), atoi (argv[3]));
  else if (argv[1][0] == 't' && argc == 3)
    Try_To_Open (global->g_cd, (char *) -1, argv[2]);
  else if (argv[1][0] == 'v')
    Show_Primary_Volume_Descriptor (global->g_cd);
  else if (argv[1][0] == 'x' && argc == 3)
    Select_Mode (global->g_cd, atoi (argv[2]), 2048);
  else if (argv[1][0] == 'x' && argc == 4)
    Select_Mode (global->g_cd, atoi (argv[2]), atoi (argv[3]));
  else if (argv[1][0] == 'y' && argc == 3)
    Find_Block_Starting_With (global->g_cd, atoi (argv[2]));
  else if (argv[1][0] == 'z')
    Send_Test_Unit_Ready (global->g_cd);
  else if (argv[1][0] == 'T')
    Test_Trackdisk_Device (global->g_cd);
  else
    Usage ();

  exit (0);
}

