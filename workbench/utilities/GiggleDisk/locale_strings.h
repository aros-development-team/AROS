#ifndef LOCALE_STRINGS_H
#define LOCALE_STRINGS_H 1

/* Locale Catalog Source File
 *
 * Automatically created by SimpleCat V3
 * Do NOT edit by hand!
 *
 * SimpleCat (C)1992-2007 Guido Mersmann
 *
 */

#define CATCOMP_NUMBERS 1


/****************************************************************************/


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifdef CATCOMP_ARRAY
#undef CATCOMP_NUMBERS
#undef CATCOMP_STRINGS
#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#endif

#ifdef CATCOMP_BLOCK
#undef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif



/****************************************************************************/


#ifdef CATCOMP_NUMBERS

#define MSG_Requester_Title 1
#define MSG_Requester_Ok 2
#define MSG_Error_UnableToOpenLibrary 3
#define MSG_ERROR_UnableToCreateTargetDir 4
#define MSG_Error_UnableToOpen 5
#define MSG_ERROR_UnableToOpenDevice 6
#define MSG_ERROR_DeviceIsNotTrackDiskCompatible 7
#define MSG_ERROR_NotEnoughMemory 8
#define MSG_ERROR_UnableToReadPartitionTable 9
#define MSG_ERROR_PartitionTableIsInvalid 10
#define MSG_ERROR_UnableToLocateDrawer 11
#define MSG_ERROR_RDBFileSystemHeaderIsInvalid 12
#define MSG_ERROR_RDBPartitionIsInvalid 13
#define MSG_ERROR_UnableToReadBlock 14
#define MSG_ERROR_NothingUseful 15

#define CATCOMP_LASTID 15

#endif /* CATCOMP_NUMBERS */


/****************************************************************************/


#ifdef CATCOMP_STRINGS

#define MSG_Requester_Title_STR "GiggleDisk Request"
#define MSG_Requester_Ok_STR "OK"
#define MSG_Error_UnableToOpenLibrary_STR "Unable to open '%s' version %ld or bigger!"
#define MSG_ERROR_UnableToCreateTargetDir_STR "Unable to create target directory"
#define MSG_Error_UnableToOpen_STR "Unable to open '%s'!"
#define MSG_ERROR_UnableToOpenDevice_STR "Unable to open '%s' unit %ld!"
#define MSG_ERROR_DeviceIsNotTrackDiskCompatible_STR "Device is not a trackdisk compatible device!"
#define MSG_ERROR_NotEnoughMemory_STR "Not enough memory for operation!"
#define MSG_ERROR_UnableToReadPartitionTable_STR "Unable to read Partition Table!"
#define MSG_ERROR_PartitionTableIsInvalid_STR "Partitiontable is invalid!"
#define MSG_ERROR_UnableToLocateDrawer_STR "Unable to locate drawer!"
#define MSG_ERROR_RDBFileSystemHeaderIsInvalid_STR "RDB filesystem header is invalid!"
#define MSG_ERROR_RDBPartitionIsInvalid_STR "RDB partition is invalid!"
#define MSG_ERROR_UnableToReadBlock_STR "Unable to read block %ld (size: 0x%lx)!"
#define MSG_ERROR_NothingUseful_STR "Unable to find any sign of a partition!\n"

#endif /* CATCOMP_STRINGS */


/****************************************************************************/


#ifdef CATCOMP_BLOCK

const char CatCompBlock[] =
{
    "\x00\x00\x00\x01\x00\x14"
    MSG_Requester_Title_STR "\x00\x00"
    "\x00\x00\x00\x02\x00\x04"
    MSG_Requester_Ok_STR "\x00\x00"
    "\x00\x00\x00\x03\x00\x2C"
    MSG_Error_UnableToOpenLibrary_STR "\x00\x00"
    "\x00\x00\x00\x04\x00\x22"
    MSG_ERROR_UnableToCreateTargetDir_STR "\x00"
    "\x00\x00\x00\x05\x00\x16"
    MSG_Error_UnableToOpen_STR "\x00\x00"
    "\x00\x00\x00\x06\x00\x1E"
    MSG_ERROR_UnableToOpenDevice_STR "\x00"
    "\x00\x00\x00\x07\x00\x2E"
    MSG_ERROR_DeviceIsNotTrackDiskCompatible_STR "\x00\x00"
    "\x00\x00\x00\x08\x00\x22"
    MSG_ERROR_NotEnoughMemory_STR "\x00\x00"
    "\x00\x00\x00\x09\x00\x20"
    MSG_ERROR_UnableToReadPartitionTable_STR "\x00"
    "\x00\x00\x00\x0A\x00\x1C"
    MSG_ERROR_PartitionTableIsInvalid_STR "\x00\x00"
    "\x00\x00\x00\x0B\x00\x1A"
    MSG_ERROR_UnableToLocateDrawer_STR "\x00\x00"
    "\x00\x00\x00\x0C\x00\x22"
    MSG_ERROR_RDBFileSystemHeaderIsInvalid_STR "\x00"
    "\x00\x00\x00\x0D\x00\x1A"
    MSG_ERROR_RDBPartitionIsInvalid_STR "\x00"
    "\x00\x00\x00\x0E\x00\x28"
    MSG_ERROR_UnableToReadBlock_STR "\x00"
    "\x00\x00\x00\x0F\x00\x2A"
    MSG_ERROR_NothingUseful_STR "\x00\x00"
};

#endif /* CATCOMP_BLOCK */


/****************************************************************************/



#endif /* LOCALE_STRINGS_H */

