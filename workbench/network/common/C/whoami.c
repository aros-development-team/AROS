/* $Id$
/*
 * whoami.c ---
 *
 * Author: ppessi <Pekka.Pessi@hut.fi>
 *         Pavel Fedin <sonic_amiga@rambler.ru>
 *
 * Copyright © 1994 AmiTCP/IP Group, <amitcp-group@hut.fi>,
 *		    Helsinki University of Technology, Finland.
 *		    All rights reserved.
 * Copyright © 2005 Pavel Fedin
 *
 * Created      : Wed Jan 12 01:40:38 1994 ppessi
 * Last modified: Thu Sep 29 22:29:20 2005 sonic
 */

//#include "whoami_rev.h"

static const char version[] = "$VER: whoami 1.3 (29.09.2005)\n"
  "Copyright © 1994 AmiTCP/IP Group, <amitcp-group@hut.fi>\n"
  "Helsinki University of Technology, Finland.\n"
  "Copyright © 2005 Pavel Fedin\n";

/****** utilities/whoami ***************************************************

   NAME
        whoami - prints effective current user id

   VERSION
	$Id$

   TEMPLATE
        whoami

   FUNCTION
        Whoami prints your effective user id.  It works even if you are
        su'd.

   RETURN VALUE
        Whoami return WARN, if the user id has got no user name associated.

   SEE ALSO
        id

*****************************************************************************
*
*/

#define USE_INLINE_STDARG

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#ifdef __SASC
#include <clib/exec_protos.h>
extern struct ExecBase* SysBase;
#include <pragmas/exec_sysbase_pragmas.h>
#else
#include <proto/exec.h>
#endif
#include <proto/dos.h>
#include <proto/usergroup.h>
#include <libraries/usergroup.h>

#include <devices/sana2.h>
#include <net/sana2errno.h>
#include <string.h>

#define USERGROUPVERSION 1	/* minimum version to use */

#ifndef MAXLINELENGTH
#define MAXLINELENGTH 1024
#endif

#ifdef __MORPHOS__
ULONG __abox__ = 1;
#endif

LONG whoami(void)
{
  LONG retval = 128;
  struct DosLibrary *DOSBase;
  struct ExecBase *SysBase;

  SysBase = *(struct ExecBase**)4;
  DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37L);

  if (DOSBase) {
    struct Library *UserGroupBase =
      OpenLibrary("usergroup.library", USERGROUPVERSION);

    if (UserGroupBase != NULL) {
      uid_t uid = geteuid();
      struct passwd *pw = getpwuid(uid);

      if (pw != NULL) {
	Printf("%s\n", (ULONG)pw->pw_name);
	retval = RETURN_OK;
      } else {
	Printf("whoami: no login associated with uid %lu.\n", uid);
	retval = RETURN_WARN;
      }

//	CloseLibrary(UserGroupBase);
    } else {
      Printf("whoami: cannot open usergroup.library\n");
    }

    CloseLibrary((struct Library *)DOSBase);
  }

  return retval;
}
