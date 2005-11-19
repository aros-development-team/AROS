/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "datatypes_intern.h"
#include <proto/locale.h>


struct StringDef
{
    ULONG sd_StringID;
    UBYTE *sd_String;
};

struct StringDef __dtStrings[]=
{
    { DTERROR_UNKNOWN_DATATYPE,       "Unknown data type for %s" },
    { DTERROR_COULDNT_SAVE,           "Couldn't save %s" },
    { DTERROR_COULDNT_OPEN,           "Couldn't open %s" },
    { DTERROR_COULDNT_SEND_MESSAGE,   "Couldn't send message" },
    { DTERROR_COULDNT_OPEN_CLIPBOARD, "Couldn't open clipboard" },
    { DTERROR_Reserved,               "Unknown data type" },
    { DTERROR_UNKNOWN_COMPRESSION,    "Unknown compression type" },
    { DTERROR_NOT_ENOUGH_DATA,        "Not enough data" },
    { DTERROR_INVALID_DATA,           "Invalid data" },
    { DTMSG_TYPE_OFFSET + DTF_BINARY, "Binary" },
    { DTMSG_TYPE_OFFSET + DTF_ASCII,  "ASCII" },
    { DTMSG_TYPE_OFFSET + DTF_IFF,    "IFF" },
    { DTMSG_TYPE_OFFSET + DTF_MISC,   "Miscellaneous" },
    { GID_SYSTEM,                     "System" },
    { GID_TEXT,                       "Text" },
    { GID_DOCUMENT,                   "Document" },
    { GID_SOUND,                      "Sound" },
    { GID_INSTRUMENT,                 "Instrument" },
    { GID_MUSIC,                      "Music" },
    { GID_PICTURE,                    "Picture" },
    { GID_ANIMATION,                  "Animation" },
    { GID_MOVIE,                      "Movie" },
    { NULL,                           NULL }
};


/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH1(CONST_STRPTR, GetDTString,

/*  SYNOPSIS */
	AROS_LHA(ULONG, id, D0),

/*  LOCATION */
	struct Library *, DataTypesBase, 23, DataTypes)

/*  FUNCTION

    Get a pointer to a localized datatypes string.

    INPUTS

    id   --  ID of the string to get

    RESULT

    Pointer to a NULL terminated string.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    CONST_STRPTR      str = NULL;
    struct StringDef *sd;
   
    if((LocaleBase != NULL) && 
       (GPB(DataTypesBase)->dtb_LibsCatalog != NULL))
	str = GetCatalogStr(GPB(DataTypesBase)->dtb_LibsCatalog, id, NULL);
   
    if(str == NULL)
    {
	for(sd = __dtStrings; sd->sd_String; sd++)
	{
	    if (sd->sd_StringID == id)
	    {
		str = sd->sd_String;
		break;
	    }
	}
    }
    
    return str;

    AROS_LIBFUNC_EXIT
} /* GetDTString */
