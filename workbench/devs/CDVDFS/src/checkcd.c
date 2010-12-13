/* Checkcd.c:
 *
 * Performs consistency checks on a ISO 9660 CD.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1994 by Frank Munkert.
 *              (C) Copyright 2007 by Pavel Fedin.
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 * 
 * 06-Mar-09 error     - Removed madness, fixed insanity. Cleanup started
 * 18-Aug-07 sonic     - Now builds on AROS.
 * 08-Apr-07 sonic     - Removed "TRACKDISK" option.
 *                     - Removed dealing with block length.
 * 31-Mar-07 sonic     Added support for Joliet and character set translation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dos/var.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "cdrom.h"
#include "iso9660.h"
#include "rock.h"
#include "charset.h"
#include "globals.h"

#ifdef LATTICE
#include <pragmas/dos_pragmas.h>
#include <pragmas/exec_pragmas.h>
#include <pragmas/utility_pragmas.h>
extern struct Library *SysBase, *DOSBase;
#endif

#define STD_BUFFERS 20
#define FILE_BUFFERS 2

t_ulong g_check_sector;
char *g_check_name;
prim_vol_desc g_pvd;
int g_path_table_records = 0;

struct Globals glob;
struct Globals *global = &glob;

#ifdef __MORPHOS__
struct Library *UtilityBase;
#else
struct UtilityBase *UtilityBase;
#endif

#define TU(x,o)  (t_ulong)(((unsigned char *)(x))[o])
#if AROS_BIG_ENDIAN
#define GET721(x) (TU(x,0) + (TU(x,1) << 8))
#define GET722(x) (TU(x,1) + (TU(x,0) << 8))
#define GET731(x) (TU(x,0) + (TU(x,1) << 8) + (TU(x,2) << 16) + (TU(x,3) << 24))
#define GET732(x) (TU(x,3) + (TU(x,2) << 8) + (TU(x,1) << 16) + (TU(x,0) << 24))
#else
#define GET721(x) (TU(x,1) + (TU(x,0) << 8))
#define GET722(x) (TU(x,0) + (TU(x,1) << 8))
#define GET731(x) (TU(x,3) + (TU(x,2) << 8) + (TU(x,1) << 16) + (TU(x,0) << 24))
#define GET732(x) (TU(x,0) + (TU(x,1) << 8) + (TU(x,2) << 16) + (TU(x,3) << 24))
#endif

/* Check consistency of a 7.2.3 field: */

void Check_723 (void *p_buf, int p_offset)
{
  t_uchar *buf = (t_uchar *) (p_buf) + (p_offset - 1);
  t_ulong l = buf[0] + (buf[1] << 8);
  t_ulong m = buf[3] + (buf[2] << 8);
  if (l != m) {
    printf ("ERROR: %s, sector %lu, offset %d - not recorded according to 7.2.3\n",
            g_check_name, g_check_sector, p_offset);
  }
}

/* Check consistency of a 7.3.3 field: */

void Check_733 (void *p_buf, int p_offset)
{
  t_uchar *buf = (t_uchar *) p_buf + (p_offset - 1);
  t_ulong l = buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);
  t_ulong m = buf[7] + (buf[6] << 8) + (buf[5] << 16) + (buf[4] << 24);
  if (l != m) {
    printf ("ERROR: %s, sector %lu, offset %d - not recorded according to 7.3.3\n",
            g_check_name, g_check_sector, p_offset);
  }
}

/* Check optional path tables: */

void Check_Optional_Path_Tables (void)
{
  t_ulong loc1, loc2;
  t_uchar *buf1;
  t_uchar *buf2;
  int i;
  
  for (i=0; i<=1; i++) {
  
    int remain = g_pvd.path_size;

    if (i == 0)
      loc1 = GET731((char*) &g_pvd.l_table),
      loc2 = GET731((char *) &g_pvd.opt_l_table);
    else
      loc1 = g_pvd.table, loc2 = g_pvd.opt_table;
    if (!loc2)
      continue;

    for (;;) {
    
      if (!Read_Chunk (global->g_cd, loc1)) {
        printf ("ERROR: illegal sector %lu\n", loc1);
        exit (1);
      }
      buf1 = global->g_cd->buffer;
    
      if (!Read_Chunk (global->g_cd, loc2)) {
        printf ("ERROR: illegal sector %lu\n", loc2);
        exit (1);
      }
      buf2 = global->g_cd->buffer;

      if (memcmp (buf1, buf2, remain > 2048 ? 2048 : remain) != 0) {
        printf ("ERROR: %c path table and optional %c path table differ"
		" in sectors %lu and %lu\n",
		i ? 'M' : 'L', i ? 'M' : 'L', loc1, loc2);
      }

      if (remain > 2048)
        remain -= 2048;
      else
        break;

      loc1++, loc2++;
    }
  }
}

/* Get the path table record in sector p_loc with offset *p_offset. */

void Get_Path_Table_Record (t_uchar *p_buf, t_ulong p_loc, t_ulong *p_offset)
{
  t_ulong sector = p_loc + (*p_offset >> 11);
  int len;

  if (!Read_Chunk (global->g_cd, sector)) {
    printf ("ERROR: illegal sector %lu\n", sector);
    exit (1);
  }
  len = global->g_cd->buffer[*p_offset & 2047] + 8;
  if (len & 1)
    len++;

  if (len + (*p_offset & 2047) > 2048) {
    int part1_len = 2048 - (*p_offset & 2047);
    memcpy (p_buf, global->g_cd->buffer + (*p_offset & 2047), part1_len);
    if (!Read_Chunk (global->g_cd, sector+1)) {
      printf ("ERROR: illegal sector %lu\n", sector+1);
      exit (1);
    }
    memcpy (p_buf + part1_len, global->g_cd->buffer, len - part1_len);
  } else
    memcpy (p_buf, global->g_cd->buffer + (*p_offset & 2047), len);
  
  *p_offset += len;
}

/* Test whether the L and the M path tables contain the same information: */

void Compare_L_And_M_Path_Table (void)
{
  t_ulong loc1 = GET731((char*) &g_pvd.l_table);
  t_ulong loc2 = g_pvd.table;
  t_ulong offset1 = 0;
  t_ulong offset2 = 0;
  static t_uchar buf1[256];
  static t_uchar buf2[256];

  while (offset1 < g_pvd.path_size) {

    t_ulong tmp = offset1;

    Get_Path_Table_Record (buf1, loc1, &offset1);
    Get_Path_Table_Record (buf2, loc2, &offset2);

    if (offset1 != offset2) {
      printf ("ERROR: Length mismatch in L and M path table at offset %lu\n",
      	      tmp);
      return;
    }

    if (buf1[1] != buf2[1])
      printf ("ERROR: Extended attribute record lengths differ in L and M"
              " path table at offset %lu\n", offset1);

    if (memcmp (buf1+8, buf2+8, buf1[0]) != 0)
      printf ("ERROR: directory identifiers differ in L and M path table "
              "at offset %lu\n", offset1);

    if (GET731 (buf1+2) != GET732 (buf2+2))
      printf ("ERROR: extent locations differ in L and M path table at"
              " offset %lu\n", offset1);

    if (GET721 (buf1+6) != GET722 (buf2+6))
      printf ("ERROR: parent directory numbers differ in L and M path table at"
              " offset %lu\n", offset1);

    g_path_table_records++;
  }
}

/* Check consistency of path table and directory records: */

void Compare_Path_Table_With_Directory_Records (void)
{
  t_ulong loc = g_pvd.table;
  t_ulong offset = 0;
  static t_uchar buf[256];
  int rec = 1;
  VOLUME *vol;
  CDROM_OBJ *obj;
  CDROM_INFO info;
  t_ulong pos;
  t_bool warn_case = 0;

  vol = Open_Volume (global->g_cd, 0, 0);
  if (!vol) {
    printf ("ERROR: cannot open volume\n");
    exit (10);
  }

  for (; offset < g_pvd.path_size; rec++) {
    t_ulong tmp = offset;

    if ((rec & 7) == 1) {
      printf ("\r   (%d of %d)",
      	      rec, g_path_table_records);
      fflush (stdout);
    }
    Get_Path_Table_Record (buf, loc, &offset);
    /* skip root record: */
    if (rec == 1)
      continue;
    pos = GET732 (buf+2);
    obj = Iso_Create_Directory_Obj (vol, pos);
    if (!obj || !CDROM_Info (obj, &info)) {
      printf ("\nERROR: cannot get information for directory at location %lu\n"
              "(Path table offset = %lu)\n", pos, tmp);
      exit (10);
    }
    if (info.name_length != buf[0] ||
        Strnicmp ((UBYTE*) info.name, (UBYTE*) buf+8, info.name_length) != 0) {
      printf ("\nERROR: names in path table and directory record differ for "
              "directory at location %lu\n", pos);
      printf ("Name in path table = ");
      fwrite (buf+8, 1, info.name_length, stdout);
      printf ("\nName in directory record = ");
      fwrite (info.name, 1, info.name_length, stdout);
      putchar ('\n');
    } else if (!warn_case && memcmp (info.name, buf+8, info.name_length) != 0) {
      printf ("\nWARNING: Directory names in path table and directory records "
              "differ in case.\n");
      warn_case = 1;
    }
    Close_Object (obj);
  }
  printf ("\r%75s\r", "");
  Close_Volume (vol);
}

/* Check optional path tables: */

void Check_Path_Tables (void)
{
  Check_Optional_Path_Tables ();
  Compare_L_And_M_Path_Table ();
}

/* Check primary volume descriptor: */

void Check_PVD (void)
{
  int loc = 16;
  int pvd_pos = 0;
  prim_vol_desc *pvd;

  do {
    if (!Read_Chunk (global->g_cd, loc)) {
      printf ("ERROR: illegal sector %d\n", loc);
      exit (1);
    }
    pvd = (prim_vol_desc *) global->g_cd->buffer;
    if (memcmp (pvd->id, "CD001", 5) != 0) {
      printf ("ERROR: missing standard identifier at sector %d\n", loc);
      exit (10);
    }
    if (pvd->type > 4 && pvd->type < 255)
      printf ("WARNING: unknown volume descriptor type at sector %d\n", loc);
    if (pvd->version != 1)
      printf ("WARNING: unknown volume descriptor version at sector %d\n", loc);
    if (pvd->type == 1 && !pvd_pos)
      pvd_pos = loc;
    loc++;
  } while (pvd->type != 255);

  if (!Read_Chunk (global->g_cd, pvd_pos)) {
    printf ("ERROR: illegal sector %d\n", loc);
    exit (1);
  }
  pvd = (prim_vol_desc *) global->g_cd->buffer;
  g_check_name = "primary volume descriptor";
  g_check_sector = pvd_pos;
  Check_733 (pvd, 81);
  Check_723 (pvd, 121);
  Check_723 (pvd, 125);
  Check_723 (pvd, 129);
  Check_733 (pvd, 133);
  memcpy (&g_pvd, pvd, sizeof (g_pvd));
}

void Check_Subdirectory (CDROM_OBJ *p_home, char *p_name)
{
  CDROM_OBJ *obj;
  CDROM_INFO info;
  /* VOLUME *vol = p_home->volume; */

  printf ("  %s\r", p_name);
  fflush (stdout);
  obj = Open_Object (p_home, p_name);
  if (obj) {
    uint32_t offset = 0;

    while (Examine_Next (obj, &info, &offset)) {
      directory_record *dir = info.suppl_info;
      g_check_name = "directory record";
      g_check_sector = offset;
      Check_733 (dir, 3);
      Check_733 (dir, 11);
      Check_723 (dir, 29);
    }

    Close_Object (obj);
  } else {
    printf ("ERROR: Object '%s': iso_errno = %d\n", p_name, global->iso_errno);
    return;
  }
  obj = Open_Object (p_home, p_name);
  if (obj) {
    uint32_t offset = 0;

    while (Examine_Next (obj, &info, &offset)) {
      if (info.directory_f) {
        char *name = malloc (strlen (p_name) + info.name_length + 2);
	int len;
	if (!name) {
	  fprintf (stderr, "out of memory\n");
	  exit (10);
	}
	if (Is_Top_Level_Object (obj))
	  name[0] = 0;
	else
	  sprintf (name, "%s/", p_name);
	len = strlen (name) + info.name_length;
	memcpy (name + strlen (name), info.name, info.name_length);
	name[len] = 0;
	Check_Subdirectory (p_home, name);
	free (name);
      }
    }
    Close_Object (obj);
  } else {
    printf ("ERROR: Object '%s': iso_errno = %d\n", p_name, global->iso_errno);
  }

  printf ("  %*s\r", strlen (p_name), "");
}

void Check_Directories (void)
{
  VOLUME *vol;
  CDROM_OBJ *home;

  if (!(vol = Open_Volume (global->g_cd, 1, 1))) {
    printf ("ERROR: cannot open volume; iso_errno = %d\n", global->iso_errno);
    exit (10);
  }

  if (!(home = Open_Top_Level_Directory (vol))) {
    printf ("ERROR: cannot open top level directory; iso_errno = %d\n", global->iso_errno);
    Close_Volume (vol);
    exit (1);
  }

  Check_Subdirectory (home, ":");

  Close_Object (home);
  Close_Volume (vol);
}

void Cleanup (void)
{
  if (global->g_cd)
    Cleanup_CDROM (global->g_cd);

  if (UtilityBase)
    CloseLibrary ((struct Library *)UtilityBase);
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

  len = GetVar ((UBYTE *) "CDROM_UNICODETABLE", (UBYTE *) global->g_unicodetable_name,
		sizeof (global->g_unicodetable_name), 0);
  if (len < 0)
    len = 0;
  if (len >= sizeof (global->g_unicodetable_name)) {
    fprintf (stderr, "CDROM_UNICODETABLE too long\n");
    exit (1);
  }
  global->g_unicodetable_name[len] = 0;

  return 1;
}

int main (int argc, char *argv[])
{
  global->g_cd = NULL;
  global->g_memory_type = MEMF_CHIP;
  global->SysBase = SysBase;
  atexit (Cleanup);

  if (!(UtilityBase = (struct UtilityBase *)
         OpenLibrary ("utility.library", 37))) {
    fprintf (stderr, "cannot open utility.library\n");
    exit (1);
  }
  global->UtilityBase = (struct UtilityBase *)UtilityBase;

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

  InitUnicodeTable();
  if (global->g_unicodetable_name[0])
    ReadUnicodeTable(global->g_unicodetable_name);

  global->g_cd = Open_CDROM (global->g_device, global->g_unit, global->g_memory_type,
  		   STD_BUFFERS, FILE_BUFFERS);
  if (!global->g_cd) {
    fprintf (stderr, "cannot open CDROM, error code = %d\n", global->g_cdrom_errno);
    exit (1);
  }

  printf ("Checking primary volume descriptor...\n");
  Check_PVD ();
  printf ("Checking path tables...\n");
  Check_Path_Tables ();
  printf ("Comparing path table with directory records...\n");
  Compare_Path_Table_With_Directory_Records ();

  printf ("Checking directories...\n");
  Check_Directories ();

  printf ("All checks completed.\n");

  exit (0);
}

