/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos support functions for internal use
*/

void InitIOFS(struct IOFileSys *iofs, ULONG type,
              struct DosLibrary *DOSBase);

CONST_STRPTR StripVolume(CONST_STRPTR name);

LONG DoIOFS(struct IOFileSys *iofs, struct DevProc *dvp, CONST_STRPTR name,
            struct DosLibrary *DOSBase);
