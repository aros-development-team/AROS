/* $Id$ */
/* $Log: messages.c $
 * Revision 1.10  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 1.9  1999/03/09  10:31:35  Michiel
 * 00110: release number define
 *
 * Revision 1.8  1998/10/02  07:22:45  Michiel
 * final release 4.2 version
 *
 * Revision 1.7  1998/09/27  11:26:37  Michiel
 * ANODE_ERROR and ANODE_INIT added
 *
 * Revision 1.6  1998/09/03  07:12:14  Michiel
 * versie 17.4
 * bugfixes 118, 121, 123 and superindexblocks and td64 support
 *
 * Revision 1.5  1998/05/31  16:30:38  Michiel
 * version number
 *
 * Revision 1.4  1998/05/30  21:52:34  Michiel
 * Overdue addition
 *
 * Revision 1.3  1998/05/27  20:16:13  Michiel
 * AFS --> PFS2
 *
 * Revision 1.2  1997/11/19  22:14:15  mark
 * Copyright transferred from FLD to GREED
 * Changed name from AFS to PFS-II
 *
 * Revision 1.1  1997/03/03  22:04:04  Michiel
 * Initial revision
 * */

#include <exec/types.h>
#include "versionhistory.doc"

UBYTE AFS_WARNING_MEMORY_MASK[]         = "WARNING:\nAllocated memory doesn't match memorymask";

UBYTE AFS_ERROR_DNV_ALLOC_INFO[]        = "ALERT:\nAllocation info not found";
UBYTE AFS_ERROR_DNV_ALLOC_BLOCK[]       = "ALERT:\nAllocation block not found";
UBYTE AFS_ERROR_DNV_WRONG_ANID[]        = "ALERT:\nWrong ablock id";
UBYTE AFS_ERROR_DNV_WRONG_DIRID[]       = "ALERT:\nWrong dirblock id";
UBYTE AFS_ERROR_DNV_LOAD_DIRBLOCK[]     = "ALERT:\nCould not read directoryblock";
UBYTE AFS_ERROR_DNV_WRONG_BMID[]        = "ALERT:\nWrong bitmap block id";
UBYTE AFS_ERROR_DNV_WRONG_INDID[]       = "ALERT:\nWrong index block id";
UBYTE AFS_ERROR_CACHE_INCONSISTENCY[]   = "Cache inconsistency detected\nFinish all disk activity";
UBYTE AFS_ERROR_OUT_OF_BUFFERS[]        = "Out of buffers";
UBYTE AFS_ERROR_MEMORY_POOL[]           = "Couldn't allocate memorypool";
UBYTE AFS_ERROR_PLEASE_FREE_MEM[]       = "Please free some memory";
UBYTE AFS_ERROR_LIBRARY_PROBLEM[]       = "Couldn't open library!";
UBYTE AFS_ERROR_INIT_FAILED[]           = "Initialization failure";
UBYTE AFS_ERROR_READ_OUTSIDE[]          = "Read attempt outside partition!";
UBYTE AFS_ERROR_WRITE_OUTSIDE[]         = "Write attempt outside partition!";
UBYTE AFS_ERROR_SEEK_OUTSIDE[]          = "Seek attempt outside partition!";
UBYTE AFS_ERROR_READ_ERROR[]            = "Read Error %ld on block %ld\n"
                                          "Make sure disk is inserted";
UBYTE AFS_ERROR_WRITE_ERROR[]           = "Write Error %ld on block %ld\n"
                                          "Make sure disk is inserted";
UBYTE AFS_ERROR_READ_DELDIR[]           = "Could not read deldir";
UBYTE AFS_ERROR_DELDIR_INVALID[]        = "Deldir invalid";
UBYTE AFS_ERROR_EXNEXT_FAIL[]           = "ExamineNext failed";
UBYTE AFS_ERROR_DOSLIST_ADD[]           = "DosList add error.\nPlease remove volume";
UBYTE AFS_ERROR_EX_NEXT_FAIL[]          = "ExamineNext failed";
UBYTE AFS_ERROR_NEWDIR_ADDLISTENTRY[]   = "Newdir addlistentry failure";
UBYTE AFS_ERROR_LOAD_DIRBLOCK_FAIL[]    = "Couldn't load dirblock!!";
UBYTE AFS_ERROR_LRU_UPDATE_FAIL[]       = "LRU update failed";
UBYTE AFS_ERROR_UPDATE_FAIL[]           = "Disk update failed";
UBYTE AFS_ERROR_UNSLEEP[]               = "Unsleep error";
UBYTE AFS_ERROR_DISK_TOO_LARGE[]		= "Disk too large for this version of PFS3.\nPlease install TD64 or direct-scsi version";
UBYTE AFS_ERROR_ANODE_ERROR[]			= "Anode index invalid";
UBYTE AFS_ERROR_ANODE_INIT[]            = "Anode initialisation failure";
UBYTE AFS_ERROR_32BIT_ACCESS_ERROR[]    = "TD32 and Direct SCSI access modes failed!\nCan't read block %ld (<4G)\n%s:%ld";

#if VERSION23
UBYTE AFS_ERROR_READ_EXTENSION[]        = "Could not read rootblock extension";
UBYTE AFS_ERROR_EXTENSION_INVALID[]     = "Rootblock extension invalid";
#endif

#ifdef BETAVERSION
/* beta messages */
UBYTE AFS_BETA_WARNING_1[]              = "BETA WARNING NR 1";
UBYTE AFS_BETA_WARNING_2[]              = "BETA WARNING NR 2";
#endif

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

