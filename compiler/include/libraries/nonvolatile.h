#ifndef  LIBRARIES_NONVOLATILE_H
#define  LIBRARIES_NONVOLATILE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include  <exec/types.h>
#include  <exec/nodes.h>


struct NVInfo
{
    ULONG  nvi_MaxStorage;
    ULONG  nvi_FreeStorage;
};


struct NVEntry
{
    struct MinNode  nve_Node;
    STRPTR          nve_Name;
    ULONG           nve_Size;
    ULONG           nve_Protection;
};


// Bit definitions for the mask in SetNVProtection() and NVEntry.nve_Protection

#define  NVEB_DELETE   0
#define  NVEB_APPNAME  31

#define  NVEF_DELETE   (1 << NVEB_DELETE)
#define  NVEF_APPNAME  (1 << NVEB_APPNAME)


// Errors reported by StoreNV()

#define  NVERR_BADNAME    1
#define  NVERR_WRITEPROT  2
#define  NVERR_FAIL       3
#define  NVERR_FATAL      4


// The size of the data returned by this library

#define SizeNVData(d) ((((ULONG *)d)[-1]) - sizeof(ULONG))


#endif  // LIBRARIES_NONVOLATILE_H
