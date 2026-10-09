#ifndef IMPORTS_H
#define IMPORTS_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Filesystem Imports - listing a host's exports and mounting one
          (re/spec/efs-protocol.md §1.1, §1.6).
*/

#include <exec/types.h>
#include <exec/lists.h>

#define IMP_NAMELEN     64          /* an export name record of command 4 */
#define IMP_HOSTLEN     128
#define IMP_FIELDLEN    80          /* the handler reads each Unit field up to 79 characters */

enum { LOC_TEMPORARY, LOC_PERMANENT, LOC_STORAGE };

struct ImpLogin
{
    char    User[32];
    char    Hash[16];               /* "$" + the 11-character ECrypt form, never the clear password */
};

/* Ask HOST's filesystem service for the exports USER may mount. On success the names
   are appended to LIST as nodes (ln_Name); returns 0 or an Envoy error code. */
ULONG Imp_ListExports(CONST_STRPTR host, CONST_STRPTR user, CONST_STRPTR password, struct List *list);
void  Imp_FreeList(struct List *list);

/* "$" + ECrypt(password, user) */
BOOL  Imp_MakeLogin(struct ImpLogin *login, CONST_STRPTR user, CONST_STRPTR password);

/* The device (and mount file) name: <host>-<volume>, realm prefix dropped, one trailing ':'
   of the export removed and any other ':' turned into '_' */
void  Imp_DeviceName(STRPTR buf, ULONG size, CONST_STRPTR host, CONST_STRPTR export);

/* Imp_Mount() result when the handler could not mount (refused, unknown export, wrong login) */
#define IMP_ERR_REFUSED 0x8001

/* Write the mount file (and its icon for Permanent and Storage), run C:Mount on it and check
   the device. path receives the mount file's name. Returns 0, IMP_ERR_REFUSED or a DOS error code. */
LONG  Imp_Mount(CONST_STRPTR host, CONST_STRPTR export, const struct ImpLogin *login, LONG location,
                STRPTR path, ULONG pathsize, STRPTR volname, ULONG volsize);

/* A sentence for an Envoy error code */
CONST_STRPTR Imp_ErrorText(ULONG err, STRPTR buf, ULONG size);

#endif
