/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.8  2001/03/10 22:44:41  stegerg
    update to make localized dos strings work.

    Revision 1.7  2001/03/09 23:45:48  stegerg
    catalog for dos.library strings. fixed most error strings
    (the name of the ERROR_ defines is sometimes misleading).
    removed ERROR_NUMBER_OF_ARGUMENTS #define (one must use
    ERROR_TOO_MANY_ARGS for this -> the string for which is
    "wrong number of arguments")

    Revision 1.6  2001/01/30 18:05:36  stegerg
    changed ERROR_BREAK string from "Break" to "***Break"

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

#include <proto/dos.h>

struct EString EString[]=
{
{ ERROR_NO_FREE_STORE,		"not enough memory available" },
{ ERROR_TASK_TABLE_FULL,	"process table full" },
{ ERROR_BAD_TEMPLATE,		"bad template" },
{ ERROR_BAD_NUMBER,		"bad number" },
{ ERROR_REQUIRED_ARG_MISSING,	"required argument missing" },
{ ERROR_KEY_NEEDS_ARG,		"value after keyword missing" },
{ ERROR_TOO_MANY_ARGS,		"wrong number of arguments" },
{ ERROR_UNMATCHED_QUOTES,	"unmatched quotes" },
{ ERROR_LINE_TOO_LONG,		"argument line invalid or too long" },
{ ERROR_FILE_NOT_OBJECT,	"file is not executable" },
{ ERROR_INVALID_RESIDENT_LIBRARY,"invalid resident library" },
{ ERROR_NO_DEFAULT_DIR, 	"Error 201" },
{ ERROR_OBJECT_IN_USE,		"object is in use" },
{ ERROR_OBJECT_EXISTS,		"object already exists" },
{ ERROR_DIR_NOT_FOUND,		"directory not found" },
{ ERROR_OBJECT_NOT_FOUND,	"object not found" },
{ ERROR_BAD_STREAM_NAME,	"invalid window description" },
{ ERROR_OBJECT_TOO_LARGE,	"object too large" },
{ ERROR_ACTION_NOT_KNOWN,	"filesystem action type unknown" }, /* was: packet request type unknown */
{ ERROR_INVALID_COMPONENT_NAME, "object name invalid" },
{ ERROR_INVALID_LOCK,		"invalid object lock" },
{ ERROR_OBJECT_WRONG_TYPE,	"object is not of required type" },
{ ERROR_DISK_NOT_VALIDATED,	"disk not validated" },
{ ERROR_DISK_WRITE_PROTECTED,	"disk is write-protected" },
{ ERROR_RENAME_ACROSS_DEVICES,	"rename across devices attempted" },
{ ERROR_DIRECTORY_NOT_EMPTY,	"directory not empty" },
{ ERROR_TOO_MANY_LEVELS,	"too many levels" },
{ ERROR_DEVICE_NOT_MOUNTED,	"device (or volume) is not mounted" },
{ ERROR_SEEK_ERROR,		"seek failure" },
{ ERROR_COMMENT_TOO_BIG,	"comment is too long" },
{ ERROR_DISK_FULL,		"disk is full" },
{ ERROR_DELETE_PROTECTED,	"object is protected from deletion" },
{ ERROR_WRITE_PROTECTED,	"file is write protected" },
{ ERROR_READ_PROTECTED, 	"file is read protected" },
{ ERROR_NOT_A_DOS_DISK, 	"not a valid DOS disk" },
{ ERROR_NO_DISK,		"no disk in drive" },
{ ERROR_NO_MORE_ENTRIES,	"no more entries in directory" },
{ ERROR_IS_SOFT_LINK,		"object is soft link" },
{ ERROR_OBJECT_LINKED,		"object is linked" },
{ ERROR_BAD_HUNK,		"bad loadfile hunk" },
{ ERROR_NOT_IMPLEMENTED,	"function not implemented" },
{ ERROR_RECORD_NOT_LOCKED,	"record not locked" },
{ ERROR_LOCK_COLLISION, 	"record lock collision" },
{ ERROR_LOCK_TIMEOUT,		"record lock timeout" },
{ ERROR_UNLOCK_ERROR,		"record unlock error" },
{ ERROR_BUFFER_OVERFLOW,	"buffer overflow" },
{ ERROR_BREAK,			"***Break" },
{ ERROR_NOT_EXECUTABLE, 	"file not executable" },

{ STRING_INSERT_VOLUME,         "Please insert volume\n%s\nin any drive" },
{ STRING_VOLUME_FULL,           "The volume %s is full" },
{ STRING_NO_DISK,               "No disk in drive %s" },
{ STRING_NO_DOS_DISK,           "Not a DOS disk in drive %s" },
{ STRING_MUST_REPLACE,          "You MUST replace volume\n%s\nin drive %s" },

{ STRING_RETRY,                 "Retry" },
{ STRING_CANCEL,                "Cancel" },
{ STRING_REQUESTTITLE,          "System requester" },


{ ERROR_NO_MATCHING_ELSEENDIF,  "no matching Else or EndIf"},
{ ERROR_SCRIPT_ONLY,            "this command is supposed to be used in "
                                "command files only" },
{ 0,				"undefined error" }
};

