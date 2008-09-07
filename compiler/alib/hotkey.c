/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */

#include <proto/exec.h>
#include <proto/commodities.h>
#include <libraries/commodities.h>

extern struct Library *CxBase;

	CxObj *HotKey(

/*  SYNOPSIS */
        STRPTR          description,
	struct MsgPort *port,
	LONG            id
	             )

/*  FUNCTION
	A simple way to get a hotkey for your program. The program is
	posted a message when the user does the specified input action in
	'description' regardless of whether the program has input focus or
	not. The key combination event is swallowed, that is not sent any
	farther in the input system.

	It's recommended that the user should be able to specify a
	program's hotkey with tooltypes, for instance
	HOTKEY="alt shift f5".

    INPUTS
	description  --  commodities filter description (see
			 commodities.library/SetFilter())
	port         --  message port the hotkey messages will be sent to
	id           --  identifier (see CxSender()) 

    RESULT
	A pointer to a filter object which represents the HotKey.

    NOTES
	Commodities.library must be open at the time of the call.

    EXAMPLE

    BUGS

    SEE ALSO
	commodities.library/CxFilter(), commodities.library/CxTranslate(),
	commodities.library/CxSender(), commodities.library/SetFilter(),
	commodities.library/CxObjError()

    INTERNALS

    HISTORY

    26.04.98  SDuvan  implemented

*****************************************************************************/
{
    CxObj  *filter;		/* The objects making up the hotkey */
    CxObj  *sender;		/* functionality... */
    CxObj  *translator;

    if((filter = CxFilter(description)) == NULL)
	return NULL;

    if((sender = CxSender(port, id)) == NULL)
    {
	DeleteCxObj(filter);
	return NULL;
    }

    AttachCxObj(filter, sender);

    /* Create the commodities equivalent of NIL: */
    if((translator = CxTranslate(NULL)) == NULL)
    {
	DeleteCxObjAll(filter);
	return NULL;
    }

    AttachCxObj(filter, translator);

    return filter;

} /* HotKey */
