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

struct Screen *family = NULL,
	      *current = IntuitionBase->FirstScreen,
	      *previous = NULL,
	      *prefamily = NULL;

    if ( reserved != NULL )
	return;

    /* Find the screen in the list and check for family */
    while ( current && current!=screen )
    {
	if ( flags & SDEPTH_INFAMILY )
	{
	    /* Check if this is the first child in a family */
	    if ( !family && (GetPrivScreen(current)->SpecialFlags & SF_IsChild) )
	    {
		family = current;
		prefamily = previous;
	    }
	    /* Check if this screen is a child so next one belongs to current family */
	    if ( family && !(GetPrivScreen(current)->SpecialFlags & SF_IsChild) )
	    {
		family = NULL;
		prefamily = NULL;
	    }
	}
	previous = current;
	current = current->NextScreen;
    }

    if ( current )
    {
	if ( flags & SDEPTH_TOFRONT )
	{
	    if ( previous ) /* I'm not the very first screen */
	    {
		if ( flags & SDEPTH_INFAMILY )
		{
		    if ( GetPrivScreen(current)->SpecialFlags & SF_IsChild )
		    { /* Move me in the front of my family */
			if ( family ) /* I'm not the first one in my family */
			{
			    previous->NextScreen = current->NextScreen;
			    current->NextScreen = family;
			    if ( prefamily )
			    {
				prefamily->NextScreen = current;
			    }
			    else
			    {
				IntuitionBase->FirstScreen = current;
			    }
			}
		    }
		    else if ( GetPrivScreen(current)->SpecialFlags & SF_IsParent )
		    { /* Move whole family */
			if ( prefamily ) /* We are not the first family */
			{
			    prefamily->NextScreen = current->NextScreen;
			    current->NextScreen = IntuitionBase->FirstScreen;
			    IntuitionBase->FirstScreen = family;
			}
		    }
		    else
		    { /* I have no family */
			previous->NextScreen = current->NextScreen;
			current->NextScreen = IntuitionBase->FirstScreen;
			IntuitionBase->FirstScreen = current;
		    }
		}
		else /* ! SDEPTH_INFAMILY */
		{
		    if ( GetPrivScreen(current)->SpecialFlags & (SF_IsChild|SF_IsParent) )
		    { /* Move my whole family */
			if ( !family )
			{
			    prefamily = previous;
			    family = current;
			}
			if ( prefamily )
			{ /* We are not the first family */
			    while ( !(GetPrivScreen(current)->SpecialFlags & SF_IsParent) )
			    {
				current = current->NextScreen;
			    }
			    prefamily->NextScreen = current->NextScreen;
			    current->NextScreen = IntuitionBase->FirstScreen;
			    IntuitionBase->FirstScreen = family;
			}
		    }
		    else
		    { /* I have no family */
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
		if ( GetPrivScreen(current)->SpecialFlags & SF_IsChild )
		{
		    /* Go to last screen of this family */
		    while ( !GetPrivScreen(current->NextScreen)->SpecialFlags & SF_IsParent )
		    {
			current = current->NextScreen;
		    }
		    if ( current != screen ) /* I'm not the last screen of my family */
		    {
			if ( previous )
			{
			    previous->NextScreen = screen->NextScreen;
			}
			else
			{
			    IntuitionBase->FirstScreen = screen->NextScreen;
			}
			screen->NextScreen = current->NextScreen;
			current->NextScreen = screen;
		    }
		}
		else if ( GetPrivScreen(current)->SpecialFlags & SF_IsParent )
		{
		    if ( current->NextScreen ) /* I'm not the last screen */
		    {
			while ( current->NextScreen )
			{
			    current = current->NextScreen;
			}
			if ( prefamily )
			{
			    prefamily->NextScreen = screen->NextScreen;
			}
			else
			{
			    IntuitionBase->FirstScreen = screen->NextScreen;
			}
			if ( family )
			{
			    current->NextScreen = family;
			}
			else
			{
			    current->NextScreen = screen;
			}
			screen->NextScreen = NULL;
		    }
		}
		else
		{
		    if ( current->NextScreen ) /* I'm not the last screen */
		    {
			while ( current->NextScreen )
			{
			    current = current->NextScreen;
			}
			if ( previous )
			{
			    previous->NextScreen = screen->NextScreen;
			}
			else
			{
			    IntuitionBase->FirstScreen = screen->NextScreen;
			}
			current->NextScreen = screen;
			screen->NextScreen = NULL;
		    }
		}
	    }
	    else /* ! SDEPTH_INFAMILY */
	    {
struct Screen *last;
		if ( GetPrivScreen(current)->SpecialFlags & (SF_IsChild|SF_IsParent) )
		{
		    if ( !family )
		    {
			family = current;
			prefamily = previous;
		    }
		    /* Go to last screen of this family */
		    while ( !GetPrivScreen(current)->SpecialFlags & SF_IsParent )
		    {
			current = current->NextScreen;
		    }
		    if ( current->NextScreen ) /* We are not the last family */
		    {
			last = current->NextScreen;
			while ( last->NextScreen )
			{
			    last = last->NextScreen;
			}
			if ( prefamily )
			{
			    prefamily->NextScreen = current->NextScreen;
			}
			else
			{
			    IntuitionBase->FirstScreen = current->NextScreen;
			}
			last->NextScreen = family;
			current->NextScreen = NULL;
		    }
		}
		else
		{
		    if ( current->NextScreen ) /* I'm not the last screen */
		    {
			while ( current->NextScreen )
			{
			    current = current->NextScreen;
			}
			if ( previous )
			{
			    previous->NextScreen = screen->NextScreen;
			}
			else
			{
			    IntuitionBase->FirstScreen = screen->NextScreen;
			}
			current->NextScreen = screen;
			screen->NextScreen = NULL;
		    }
		}
	    }
	}
    }
    
    SetFrontBitMap(IntuitionBase->FirstScreen->RastPort.BitMap, TRUE);

    AROS_LIBFUNC_EXIT
} /* ScreenDepth */
