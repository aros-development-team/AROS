#ifndef DEVICES_CD_H
#define DEVICES_CD_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for cd.device and CD drivers
    Lang: english
*/

/* CD device error codes */

#define CDERR_OPENFAIL      (-1)
#define CDERR_ABORTED       (-2)
#define CDERR_NOCMD         (-3)
#define CDERR_BADLENGTH     (-4)
#define CDERR_BADADDRESS    (-5)
#define CDERR_UNITBUSY      (-6)
#define CDERR_SELFTEST      (-7)

#define CDERR_NotSpecified      20
#define CDERR_NoSecHdr          21
#define CDERR_BadSecPreamble    22
#define CDERR_BadSecID          23
#define CDERR_BadHdrSum         24
#define CDERR_BadSecSum         25
#define CDERR_TooFewSecs        26
#define CDERR_BadSecHdr         27
#define CDERR_WriteProt         28
#define CDERR_NoDisk            29
#define CDERR_SeekError         30
#define CDERR_NoMem             31
#define CDERR_BadUnitNum        32
#define CDERR_BadDriveType      33
#define CDERR_DriveInUse        34
#define CDERR_PostReset         35
#define CDERR_BadDataType       36
#define CDERR_InvalidState      37

#define CDERR_Phase             42
#define CDERR_NoBoard           50

#endif /* DEVICES_CD_H */
