/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Change order of screens.
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	AROS_LH3(void, ScreenDepth,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),
	AROS_LHA(ULONG          , flags, D0),
	AROS_LHA(APTR           , reserved, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 131, Intuition)

/*  FUNCTION
	Move the specified screen to the front or back, based on passed flag.
	If the screen is in a group, the screen will change its position in
	the group only. If the screen is the parent of a group, the whole
	group will be moved.

    INPUTS
	screen - Move this screen.
	flags - SDEPTH_TOFRONT or SDEPTH_TOBACK for bringing the screen to
		front or back.
		If the screen is a child of another screen you may specify
		SDEPTH_INFAMILY to move the screen within the family. If
		not specified the whle family will move.
	reserved - For future use. MUST be NULL by now.	

    RESULT
	None.

    NOTES
	Only the owner of the screen should use SDEPTH_INFAMILY.
	Intentionally commodities should not change the internal arrangement
	of screen families.

    EXAMPLE

    BUGS

    SEE ALSO
	ScreenToBack(), ScreenToFront()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

struct Screen *parent = NULL,
	      *current = IntuitionBase->FirstScreen,
	      *previous = NULL,
	      *preparent = NULL;

    if ( reserved != NULL )
	return;

    while ( current && current!=screen )
    {
//	if ( flags & SDEPTH_INFAMILY )
//	{
//	    /* Check if this screen belongs to current family */
//	    if ( parent && !(current->SPECIALFLAGS & SF_CHILD) )
//	    {
//		parent = NULL;
//	    }
//
//	    /* Check if this is a parent screen */
//	    if ( current->SPECIALFLAGS & SF_PARENT )
//	    {
//		parent = current;
//		preparent = previous;
//	    }
//	}
	previous = current;
	current = current->NextScreen;
    }

    if ( current )
    {
	if ( flags & SDEPTH_TOFRONT )
	{
	    if ( previous ) /* I'm not the first screen */
	    {
		if ( flags & SDEPTH_INFAMILY )
		{
//		    if ( current->SPECIALFLAGS & SF_CHILD )
//		    {
//			previous->NextScreen = current->NextScreen;
//			current->NextScreen = parent->NextScreen;
//			parent->NextScreen = current;
//		    }
//		    else if ( current->SPECIALFLAGS & SP_PARENT )
//		    {
//			/* Go to last screen of this family */
//			while ( current->NextScreen && (current->NextScreen->SPECIALFLAGS & SF_CHILD) )
//			    current = current->NextScreen;
//			previous->NextScreen = current->NextScreen;
//			current->NextScreen = IntuitionBase->FirstScreen;
//			IntuitionBase->FirstScreen = screen;
//		    }
//		    else
		    {
			previous->NextScreen = current->NextScreen;
			current->NextScreen = IntuitionBase->FirstScreen;
			IntuitionBase->FirstScreen = current;
		    }
		}
		else
		{
//		    if ( current->SPECIALFLAGS & SF_PARENT )
//		    {
//			/* Go to last screen of this family */
//			while ( current->NextScreen && (current->NextScreen->SPECIALFLAGS & SF_CHILD) )
//			    current = current->NextScreen;
//			previous->NextScreen = current->NextScreen;
//			current->NextScreen = IntuitionBase->FirstScreen;
//			IntuitionBase->FirstScreen = screen;
//		    }
//		    else if ( current->SPECIALFLAGS & SF_CHILD )
//		    {
//			if ( preparent ) /* I'm not in the first family */
//			{
//			    /* Go to last screen of this family */
//			    while ( current->NextScreen && (current->NextScreen->SPECIALFLAGS & SF_CHILD) )
//				current = current->NextScreen;
//			    preparent->NextScreen = current->NextScreen;
//			    current->NextScreen = IntuitionBase->FirstScreen;
//			    IntuitionBase->FirstScreen = parent;
//			}
//		    }
//		    else
		    {
			previous->NextScreen = current->NextScreen;
			current->NextScreen = IntuitionBase->FirstScreen;
			IntuitionBase->FirstScreen = current;
		    }
		}
	    }
	}
	else if ( flags & SDEPTH_TOBACK )
	{
	    if ( flags & SDEPTH_INFAMILY )
	    {
//		if ( current->SPECIALFLAGS & SF_CHILD )
//		{
//		    if ( screen->NextScreen && (screen->NextScreen->SPECIALFLAGS & SF_CHILD) ) /* I'm not the last screen in this family */
//		    {
//			/* Go to last screen of this family */
//			while ( current->NextScreen && (current->NextScreen->SPECIALFLAGS & SF_CHILD) )
//			    current = current->NextScreen;
//			previous->NextScreen = screen->NextScreen;
//			screen->NextScreen = current->NextScreen;
//			current->NextScreen = screen;
//		    }
//		}
//		else if ( current->SPECIALFLAGS & SP_PARENT )
//		{
//struct Screen *last;
//		    /* Go to last screen of this family */
//		    while ( current->NextScreen && (current->NextScreen->SPECIALFLAGS & SF_CHILD) )
//			current = current->NextScreen;
//		    if ( current->NextScreen ) /* I'm not the last family */
//		    {
//			if ( previous )
//			    previous->NextScreen = current->NextScreen;
//			else
//			    IntuitionBase->FirstScreen = current->NextScreen;
//			last = current;
//			while ( last->NextScreen )
//			    last = last->NextScreen;
//			last->NextScreen = screen;
//			current->NextScreen = NULL;
//		    }
//		}
//		else
		{
		    if ( current->NextScreen ) /* I'm not the last screen */
		    {
			while ( current->NextScreen )
			    current = current->NextScreen;
			if ( previous )
			    previous->NextScreen = screen->NextScreen;
			else
			    IntuitionBase->FirstScreen = screen->NextScreen;
			current->NextScreen = screen;
			screen->NextScreen = NULL;
		    }
		}
	    }
	    else
	    {
//		if ( current->SPECIALFLAGS & SF_PARENT )
//		{
//struct Screen *last;
//		    /* Go to last screen of this family */
//		    while ( current->NextScreen && (current->NextScreen->SPECIALFLAGS & SF_CHILD) )
//			current = current->NextScreen;
//		    if ( current->NextScreen ) /* I'm not the last family */
//		    {
//			if ( previous )
//			    previous->NextScreen = current->NextScreen;
//			else
//			    IntuitionBase->FirstScreen  = current->NextScreen;
//			last = current;
//			while ( last->NextScreen )
//			    last = last->NextScreen;
//			last->NextScreen = screen;
//			current->NextScreen = NULL;
//		    }
//		}
//		else if ( current->SPECIALFLAGS & SF_CHILD )
//		{
//struct Screen *last;
//		    /* Go to last screen of this family */
//		    while ( current->NextScreen && (current->NextScreen->SPECIALFLAGS & SF_CHILD) )
//			current = current->NextScreen;
//		    if ( current->NextScreen ) /* I'm not the last family */
//		    {
//			if ( prevparent )
//			    prevparent->NextScreen = current->NextScreen;
//			else
//			    IntuitionBase->FirstScreen  = current->NextScreen;
//			last = current;
//			while ( last->NextScreen )
//			    last = last->NextScreen;
//			last->NextScreen = parent;
//			current->NextScreen = NULL;
//		}
//		else
		{
		    if ( current->NextScreen ) /* I'm not the last screen */
		    {
			while ( current->NextScreen )
			    current = current->NextScreen;
			if ( previous )
			    previous->NextScreen = screen->NextScreen;
			else
			    IntuitionBase->FirstScreen = screen->NextScreen;
			current->NextScreen = screen;
			screen->NextScreen = NULL;
		    }
		}
	    }
	}
    }

#warning TODO: Refresh Display after changing order of screens

    AROS_LIBFUNC_EXIT
} /* ScreenDepth */
