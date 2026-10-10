/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    envoyfs_imports.h - Envoy export discovery and login hashing, lifted
    from envoy/prefs/imports (re/spec/efs-protocol.md §1.1, §1.6).
*/

#ifndef _NETPREFS_ENVOYFS_IMPORTS_H_
#define _NETPREFS_ENVOYFS_IMPORTS_H_

#include <exec/types.h>
#include <exec/lists.h>

#define IMP_NAMELEN     64          /* an export name record of command 4 */
#define IMP_HOSTLEN     128
#define IMP_FIELDLEN    80          /* the handler reads each Unit field up to 79 characters */

struct ImpLogin
{
    char    User[32];
    char    Hash[16];               /* "$" + the 11-character ECrypt form, never the clear password */
};

/* Ask HOST's filesystem service for the exports USER may mount.  On success
   the names are appended to LIST as nodes (ln_Name); returns 0 or an Envoy
   error code.  Needs NIPCBase + ServicesBase (ENVOYERR_NORESOURCES without). */
ULONG Imp_ListExports(CONST_STRPTR host, CONST_STRPTR user, CONST_STRPTR password, struct List *list);
void  Imp_FreeList(struct List *list);

/* "$" + ECrypt(password, user); FALSE without AccountsBase */
BOOL  Imp_MakeLogin(struct ImpLogin *login, CONST_STRPTR user, CONST_STRPTR password);

/* The device (and mount file) name: <host>-<export>, realm prefix dropped,
   one trailing ':' of the export removed and any other ':' turned into '_' */
void  Imp_DeviceName(STRPTR buf, ULONG size, CONST_STRPTR host, CONST_STRPTR export);

/* A sentence for an Envoy error code */
CONST_STRPTR Imp_ErrorText(ULONG err, STRPTR buf, ULONG size);

#endif /* _NETPREFS_ENVOYFS_IMPORTS_H_ */
