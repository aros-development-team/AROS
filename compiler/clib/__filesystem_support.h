/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos support functions for internal use
*/

#ifndef CLIB_AROSSUPPORT_PROTOS_H
#   include <proto/arossupport.h>
#endif

#ifdef AROS_DOS_PACKETS

#define InitIOFS(x,y,z) do { kprintf("InitIOFS! %s:%s:%d", __FILE__, __FUNCTION__, __LINE__); for(;;); } while(0)
#define DoIOFS(a,b,c,d) kprintf("DoIOFS! %s:%s:%d", __FILE__, __FUNCTION__, __LINE__)

#else

void InitIOFS(struct IOFileSys *iofs, ULONG type,
              struct DosLibrary *DOSBase);
LONG DoIOFS(struct IOFileSys *iofs, struct DevProc *dvp, CONST_STRPTR name,
            struct DosLibrary *DOSBase);
#endif
            
CONST_STRPTR StripVolume(CONST_STRPTR name);

