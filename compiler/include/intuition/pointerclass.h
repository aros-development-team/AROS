#ifndef INTUITION_POINTERCLASS_H
#define INTUITION_POINTERCLASS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Headerfile for Intuitions' pointer classes.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define POINTERA_Dummy	        (TAG_USER + 0x39000)

#define POINTERA_BitMap		(POINTERA_Dummy + 0x01)
#define POINTERA_XOffset	(POINTERA_Dummy + 0x02)
#define POINTERA_YOffset	(POINTERA_Dummy + 0x03)
#define POINTERA_WordWidth	(POINTERA_Dummy + 0x04)
#define POINTERA_XResolution	(POINTERA_Dummy + 0x05)
#define POINTERA_YResolution	(POINTERA_Dummy + 0x06)

#define POINTERXRESN_DEFAULT	0
#define POINTERXRESN_140NS	1
#define POINTERXRESN_70NS	2
#define POINTERXRESN_35NS	3
#define POINTERXRESN_SCREENRES	4
#define POINTERXRESN_LORES	5
#define POINTERXRESN_HIRES	6

#define POINTERYRESN_DEFAULT         0
#define POINTERYRESN_HIGH            2
#define POINTERYRESN_HIGHASPECT	     3
#define POINTERYRESN_SCREENRES	     4
#define POINTERYRESN_SCREENRESASPECT 5

#endif
