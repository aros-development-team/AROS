#ifndef OPENURL_INTERFACE_DEF_H
#define OPENURL_INTERFACE_DEF_H
/*
**  $VER: openurl.i 7.3 (28.12.2007)
**
**  This file defines interface ASM structure for OS4.
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Versions 3.1 to 7.1 were developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Copyright (C) 2006-2007 OpenURL Team
*/ 

#include <exec/types.i>
#include <exec/exec.i>
#include <exec/interfaces.i>

STRUCTURE OpenURLIFace, InterfaceData_SIZE
	    FPTR IOpenURL_Obtain
	    FPTR IOpenURL_Release
	    FPTR IOpenURL_Expunge
	    FPTR IOpenURL_Clone
	    FPTR IOpenURL_URL_OpenA
	    FPTR IOpenURL_URL_Open
	    FPTR IOpenURL_Reserved1
	    FPTR IOpenURL_Reserved2
	    FPTR IOpenURL_Reserved3
	    FPTR IOpenURL_Reserved4
	    FPTR IOpenURL_Reserved5
	    FPTR IOpenURL_Reserved6
	    FPTR IOpenURL_URL_GetPrefsA
	    FPTR IOpenURL_URL_GetPrefs
	    FPTR IOpenURL_URL_FreePrefsA
	    FPTR IOpenURL_URL_FreePrefs
	    FPTR IOpenURL_URL_SetPrefsA
	    FPTR IOpenURL_URL_SetPrefs
	    FPTR IOpenURL_URL_LaunchPrefsAppA
	    FPTR IOpenURL_URL_LaunchPrefsApp
	    FPTR IOpenURL_URL_GetAttr
	LABEL OpenURLIFace_SIZE

#endif
