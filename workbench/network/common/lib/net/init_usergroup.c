/* $Id$
 *
 *      init_usergroup.c - SAS/C autoinitialization func. for usergroup.library
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <exec/types.h>
#include <exec/libraries.h>

#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <dos/dosextens.h>
#include <stdlib.h>

#include <libraries/usergroup.h>
#include <clib/usergroup_protos.h>
#include <pragmas/usergroup_pragmas.h>

struct Library *UserGroupBase = NULL;

#define USERGROUPVERSION 1	/* minimum version to use */

extern STRPTR _ProgramName;	/* SAS startup module defines this :-) */
extern int errno;		/* errno variable */

/****** net.lib/autoinit_usergroup.library ************************************

    NAME
        autoinit usergroup.library - SAS C Autoinitialization Functions

    SYNOPSIS
        error = _STI_200_openUserGroup()

        LONG _STI_200_openUserGroup(void)

        _STD_200_closeUserGroup()

        void _STD_200_closeUserGroup(void)

    FUNCTION
        These functions open and close the usergroup.library at the startup
        and exit of the program, respectively.  For a program to use these
        functions, it must be linked with netlib:usr.lib.

    NOTES
        _STI_200_openUserGroup() also checks that the system version is at
        least 37.  It puts up a requester if the usergroup.library is not
        found or is too old version.

        The autoinitialization and autotermination functions are features
        specific to the SAS C6.  However, these functions can be used with
        other (ANSI) C compilers, too.  Example follows:

        \* at start of main() *\

        atexit(_STD_200_closeUserGroup);
        if (_STI_200_openUserGroup() != 0)
	   exit(20);

    BUGS 
        The same autoinitialization won't work for both SAS C 6.3 and SAS C
        6.50 or latter.  Only way to terminate an initialization function is
        by exit() call with SAS C 6.3 binary.  If an autoinitialization
        function is terminated by exit() call with SAS C 6.50 binary, the
        autotermination functions won't be called.  Due this braindamage
        these compilers require separate net.lib libraries.

    SEE ALSO
        SAS/C 6 User's Guide p. 145 for details of autoinitialization and
        autotermination functions.

****************************************************************************** */

/* SAS C 6.50 kludge */
#if __VERSION__ > 6 || __REVISION__ >= 50
#define exit(x) return(x)
#endif

/*
 * Using __stdargs prevents creation of register arguments entry point.
 * If both stack args and reg. args entry points are created, this
 * function is called _twice_, which is not wanted.
 */
LONG __stdargs
_STI_200_openUserGroup(void)
{
  const UBYTE *errorStr = "Cannot open %s.";

  struct Process *me = (struct Process *)FindTask(NULL);
  /*
   * Check OS version
   */
  if ((*(struct Library **)4)->lib_Version < 37)
    exit(20);

  /*
   * Open bsdsocket.library
   */
  if (UserGroupBase = OpenLibrary(USERGROUPNAME, USERGROUPVERSION)) {
    if (ug_SetupContextTags(_ProgramName,
			    UGT_INTRMASK, SIGBREAKB_CTRL_C,
			    UGT_ERRNOPTR(sizeof(errno)), &errno,
			    TAG_END)
	== 0)
      return 0;
    errorStr = "Cannot initialize context in %s.";
  }

  /*
   * Post requester only if approved 
   */
  if (me->pr_WindowPtr != (APTR) -1) {
    struct Library *IntuitionBase;
    if (IntuitionBase = OpenLibrary("intuition.library", 36)) {
      struct EasyStruct libraryES;

      libraryES.es_StructSize = sizeof(libraryES);
      libraryES.es_Flags = 0;
      libraryES.es_Title = _ProgramName;
      libraryES.es_TextFormat = (STRPTR)errorStr;
      libraryES.es_GadgetFormat = "Exit %s";
      
      EasyRequest(NULL, &libraryES, NULL, 
		  USERGROUPNAME, 
		  _ProgramName);

      CloseLibrary(IntuitionBase);
    }
  }

  exit(RETURN_FAIL);
}

void __stdargs
_STD_200_closeUserGroup(void)
{
  if (UserGroupBase != NULL) {
    CloseLibrary(UserGroupBase), UserGroupBase = NULL;
  }
}
