/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Beep
    Lang: english
*/

/*****************************************************************************

    NAME
	Beep

    FORMAT
	Beep

    SYNOPSIS

    LOCATION
	Workbench:c

    FUNCTION
	BEEP produces a beep via Intuition DisplayBeep(NULL).

    EXAMPLE
	
    SEE ALSO

**************************************************************************/

#include <proto/exec.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/intuition.h>
#include <intuition/screens.h>

static const char version[] = "$VER: Beep 1.0 (30.12.2000)";

int main()
{
struct IntuitionBase *IntuitionBase;

  IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",0);
  if ( IntuitionBase == NULL )
  {
    printf ( "Cannot open intuition.library!\n" );
  }

  DisplayBeep ( NULL );

  CloseLibrary ( (struct Library *)IntuitionBase );

return 0;
}	    
