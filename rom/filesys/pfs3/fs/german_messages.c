/* $Id$ */
/* $Log: german_messages.c $
 * Revision 1.13  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 1.12  1999/03/09  10:31:35  Michiel
 * 00110: release number define
 *
 * Revision 1.11  1998/10/02  07:22:45  Michiel
 * final release 4.2 version
 *
 * Revision 1.10  1998/09/28  17:13:32  Michiel
 * translation upgrade
 *
 * Revision 1.8  1998/09/03  07:12:14  Michiel
 * versie 17.4
 * bugfixes 118, 121, 123 and superindexblocks and td64 support
 *
 * Revision 1.7  1998/05/31  16:30:38  Michiel
 * version number
 *
 * Revision 1.6  1998/05/30  21:52:34  Michiel
 * 1.1 to 4.1
 *
 * Revision 1.5  1998/05/27  20:16:13  Michiel
 * AFS --> PFS2
 *
 * Revision 1.4  1997/11/19  22:12:02  mark
 * Copyright transferred from FLD to GREED
 * Changed name from AFS to PFS-II
 *
 * Revision 1.3  1997/03/03  22:04:04  Michiel
 * Release 16.21
 *
 * Revision 1.2  1995/11/15  15:49:30  Michiel
 * updated to AFS 16.4
 *
 * Revision 1.1  1995/07/27  12:26:25  Michiel
 * Initial revision
 * */

#include "versionhistory.doc"
#include <exec/types.h>

UBYTE AFS_WARNING_MEMORY_MASK[]         = "Warnung:\nBenutzter Speicher ist nicht mit Mask vereinbar";

UBYTE AFS_ERROR_DNV_ALLOC_INFO[]        = "Alarm:\nAllocationinformation nicht gefunden";
UBYTE AFS_ERROR_DNV_ALLOC_BLOCK[]       = "Alarm:\nAllocationblock nicht gefunden";
UBYTE AFS_ERROR_DNV_WRONG_ANID[]        = "Alarm:\nFalsche Allocationblock ID";
UBYTE AFS_ERROR_DNV_WRONG_DIRID[]       = "Alarm:\nFalsche Verzeichnis ID";
UBYTE AFS_ERROR_DNV_LOAD_DIRBLOCK[]     = "Alarm:\nKonnte Verzeichnis nicht lesen";
UBYTE AFS_ERROR_DNV_WRONG_BMID[]        = "Alarm:\nFalsche Bitmapblock ID";
UBYTE AFS_ERROR_DNV_WRONG_INDID[]       = "Alarm:\nFalsche Indexblock ID";
UBYTE AFS_ERROR_CACHE_INCONSISTENCY[]   = "Puffer Fehler\nAlle Diskbenutzung beenden";
UBYTE AFS_ERROR_OUT_OF_BUFFERS[]        = "Puffertabelle ist voll";
UBYTE AFS_ERROR_MEMORY_POOL[]           = "Speicherplatzmangel";
UBYTE AFS_ERROR_PLEASE_FREE_MEM[]       = "Bitte etwas Speicher freimachen";
UBYTE AFS_ERROR_LIBRARY_PROBLEM[]       = "Konnte Library nicht öffnen";
UBYTE AFS_ERROR_INIT_FAILED[]           = "Initialisierungsfehler";
UBYTE AFS_ERROR_READ_OUTSIDE[]          = "Leseversuch außerhalb Partition!";
UBYTE AFS_ERROR_WRITE_OUTSIDE[]         = "Schreibversuch außerhalb Partition!";
UBYTE AFS_ERROR_SEEK_OUTSIDE[]          = "Leseversuch außerhalb Partition!";
UBYTE AFS_ERROR_READ_ERROR[]            = "Lesefehler %ld auf Block %ld";
UBYTE AFS_ERROR_WRITE_ERROR[]           = "Schreibfehler %ld auf Block %ld";
UBYTE AFS_ERROR_READ_DELDIR[]           = "Konnte Deldir nicht lesen";
UBYTE AFS_ERROR_DELDIR_INVALID[]        = "Deldir nicht gültig";
UBYTE AFS_ERROR_EXNEXT_FAIL[]           = "ExamineNext fehlgeslagen";
UBYTE AFS_ERROR_DOSLIST_ADD[]           = "DosList Fehler.\nBitte Disk entfernen";
UBYTE AFS_ERROR_EX_NEXT_FAIL[]          = "ExamineNext fehlgeslagen";
UBYTE AFS_ERROR_NEWDIR_ADDLISTENTRY[]   = "Neues Verzeichnis fehlgeslagen";
UBYTE AFS_ERROR_LOAD_DIRBLOCK_FAIL[]    = "Konnte Verzeichnis nicht lesen";
UBYTE AFS_ERROR_LRU_UPDATE_FAIL[]       = "Fehler bei Pufferbeischreibung";
UBYTE AFS_ERROR_UPDATE_FAIL[]           = "Fehler bei Diskbeschreibung";
UBYTE AFS_ERROR_UNSLEEP[]               = "Fehler bei Unsleep";
UBYTE AFS_ERROR_DISK_TOO_LARGE[]		= "Datenträger zu groß für PFS3.\nBitte installieren Sie TD64 or Direct-SCSI Version";
UBYTE AFS_ERROR_ANODE_ERROR[]			= "Anode Index unzulässig";
UBYTE AFS_ERROR_ANODE_INIT[]            = "Anode Initialisierungs Fehler";

#if VERSION23
UBYTE AFS_ERROR_READ_EXTENSION[]        = "Konnte Rootblockextension nicht lesen";
UBYTE AFS_ERROR_EXTENSION_INVALID[]     = "Rootblockextension nicht gültig";
#endif

/* beta messages */

UBYTE AFS_BETA_WARNING_1[]       = "BETA WARNING NR 1";
UBYTE AFS_BETA_WARNING_2[]       = "BETA WARNING NR 2";

/*
 * Message shown when formatting a disk
 */

	UBYTE FORMAT_MESSAGE[] =
	                "Professional File System 3 V" RELEASE "\n\n"
	                "      Open Source Version       \n\n"
	                "          based on PFS3         \n"
	                "            written by          \n"
					"           Michiel Pelt         \n\n"
	                "     Press mouse to continue    ";

#if DELDIR
UBYTE deldirname[] = "\007.DELDIR";
#endif
