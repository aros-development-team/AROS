/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.5  2000/05/21 08:47:46  SDuvan
    Added script errors

    Revision 1.4  1999/10/25 13:58:19  SDuvan
    Added more strings -- not only errors anymore

    Revision 1.3  1998/10/20 16:44:34  hkiel
    Amiga Research OS

    Revision 1.2  1996/08/01 17:40:50  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

struct EString EString[]=
{
{ ERROR_NO_FREE_STORE,		"No free store" },
{ ERROR_TASK_TABLE_FULL,	"Task table full" },
{ ERROR_BAD_TEMPLATE,		"Bad template" },
{ ERROR_BAD_NUMBER,		"Bad number" },
{ ERROR_REQUIRED_ARG_MISSING,	"Required argument missing" },
{ ERROR_KEY_NEEDS_ARG,		"Keyword needs argument" },
{ ERROR_TOO_MANY_ARGS,		"Too many arguments" },
{ ERROR_UNMATCHED_QUOTES,	"Unmatched quoted" },
{ ERROR_LINE_TOO_LONG,		"Line too long" },
{ ERROR_FILE_NOT_OBJECT,	"File not object" },
{ ERROR_INVALID_RESIDENT_LIBRARY,"Invalid resident library" },
{ ERROR_NO_DEFAULT_DIR, 	"No default dir" },
{ ERROR_OBJECT_IN_USE,		"Object is in use" },
{ ERROR_OBJECT_EXISTS,		"Object exists already" },
{ ERROR_DIR_NOT_FOUND,		"Directory not found" },
{ ERROR_OBJECT_NOT_FOUND,	"Object not found" },
{ ERROR_BAD_STREAM_NAME,	"Bad stream name" },
{ ERROR_OBJECT_TOO_LARGE,	"Object too large" },
{ ERROR_ACTION_NOT_KNOWN,	"Action not known" },
{ ERROR_INVALID_COMPONENT_NAME, "Invalid component name" },
{ ERROR_INVALID_LOCK,		"Invalid lock" },
{ ERROR_OBJECT_WRONG_TYPE,	"Object is of wrong type" },
{ ERROR_DISK_NOT_VALIDATED,	"Disk is not validated" },
{ ERROR_DISK_WRITE_PROTECTED,	"Disk is write protected" },
{ ERROR_RENAME_ACROSS_DEVICES,	"Rename across devices attempted" },
{ ERROR_DIRECTORY_NOT_EMPTY,	"Directory isn't empty" },
{ ERROR_TOO_MANY_LEVELS,	"Too many levels" },
{ ERROR_DEVICE_NOT_MOUNTED,	"Device not mounted" },
{ ERROR_SEEK_ERROR,		"Seek error" },
{ ERROR_COMMENT_TOO_BIG,	"Comment too big" },
{ ERROR_DISK_FULL,		"Disk is full" },
{ ERROR_DELETE_PROTECTED,	"Object is delete protected" },
{ ERROR_WRITE_PROTECTED,	"Object is write protected" },
{ ERROR_READ_PROTECTED, 	"Object is read protected" },
{ ERROR_NOT_A_DOS_DISK, 	"Not a DOS disk" },
{ ERROR_NO_DISK,		"No disk in drive" },
{ ERROR_NO_MORE_ENTRIES,	"No more entries in directory" },
{ ERROR_IS_SOFT_LINK,		"Object is soft link" },
{ ERROR_OBJECT_LINKED,		"Object is linked" },
{ ERROR_BAD_HUNK,		"Bad hunk in loadfile" },
{ ERROR_NOT_IMPLEMENTED,	"Action not implemented" },
{ ERROR_RECORD_NOT_LOCKED,	"Record not locked" },
{ ERROR_LOCK_COLLISION, 	"Lock collision" },
{ ERROR_LOCK_TIMEOUT,		"Lock timed out" },
{ ERROR_UNLOCK_ERROR,		"Unlock error" },
{ ERROR_BUFFER_OVERFLOW,	"Buffer overflow" },
{ ERROR_BREAK,			"Break" },
{ ERROR_NOT_EXECUTABLE, 	"File is not executable" },

{ STRING_INSERT_VOLUME,         "Please insert volume\n%s\nin any drive" },
{ STRING_VOLUME_FULL,           "The volume %s is full" },
{ STRING_NO_DISK,               "No disk in drive %s" },
{ STRING_NO_DOS_DISK,           "Not a DOS disk in drive %s" },
{ STRING_MUST_REPLACE,          "You MUST replace volume\n%s\nin drive %s" },

{ STRING_RETRY,                 "Retry" },
{ STRING_CANCEL,                "Cancel" },
{ STRING_REQUESTTITLE,          "System requester" },


{ ERROR_NO_MATCHING_ELSEENDIF,  "No matching Else or EndIf"},
{ ERROR_SCRIPT_ONLY,            "This command is supposed to be used in "
                                "command files only" },
{ ERROR_NUMBER_OF_ARGUMENTS,    "Wrong number of arguments" },

{ 0,				"Undefined error" }
};
