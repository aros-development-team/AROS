#ifndef CLIB_CAPSIMAGE_PROTOS_H
#define CLIB_CAPSIMAGE_PROTOS_H


/*
**	$VER: capsimage_protos.h 1.0 (30.03.2010)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 2010 
**	All Rights Reserved
*/

#include <exec/types.h>

CapsLong CAPSInit(void);
CapsLong CAPSExit(void);
CapsLong CAPSAddImage(void);
CapsLong CAPSRemImage(CapsLong id);
CapsLong CAPSLockImage(CapsLong id, CONST_STRPTR name);
CapsLong CAPSLockImageMemory(CapsLong id, CapsUByte * buffer, CapsULong length, CapsULong flag);
CapsLong CAPSUnlockImage(CapsLong id);
CapsLong CAPSLoadImage(CapsLong id, CapsULong flag);
CapsLong CAPSGetImageInfo(struct CapsImageInfo * pi, CapsLong id);
CapsLong CAPSLockTrack(struct CapsTrackInfo * pi, CapsLong id, CapsULong cylinder,
	CapsULong head, CapsULong flag);
CapsLong CAPSUnlockTrack(CapsLong id, CapsULong cylinder, CapsULong head);
CapsLong CAPSUnlockAllTracks(CapsLong id);
CONST_STRPTR CAPSGetPlatformName(CapsULong pid);
CapsLong CAPSGetVersionInfo(struct CapsVersionInfo * pi, CapsULong flag);

#endif	/*  CLIB_CAPSIMAGE_PROTOS_H  */
