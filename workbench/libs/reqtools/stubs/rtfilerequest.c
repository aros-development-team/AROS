/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
*/

#define AROS_TAGRETURNTYPE APTR
#include <utility/tagitem.h>
#include <libraries/reqtools.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/reqtools.h>

extern struct ReqToolsBase * ReqToolsBase;

/*****************************************************************************

    NAME */
	APTR rtFileRequest (

/*  SYNOPSIS */
	struct rtFileRequester * filereq,
	char *file,
	const char *title,
	Tag tag1, 
	...)

/*  FUNCTION
        Request a filename, directory, or multiple file selections.

    INPUTS
        filereq - File requester allocated with rtAllocRequest().
        file    - Initial filename and destination for the selected name;
                  provide a buffer of at least 108 bytes.
        title   - Requester window title.
        tag1    - First requester tag; terminate with TAG_DONE.

    RESULT
        Result from rtFileRequestA(): nonzero for an accepted single
        selection, zero on cancellation, or an rtFileList pointer when
        multiple selection is enabled.

    NOTES
        The file list returned for multiple selection is released with
        rtFreeFileList(). The requester requires a process with DOS access.

    SEE ALSO
        rtFileRequestA(), rtAllocRequest(), rtFreeFileList()

***************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = rtFileRequestA(filereq, file, title, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
    
} /* rtFileRequest */
