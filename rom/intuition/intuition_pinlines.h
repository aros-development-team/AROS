#ifndef _INTUITION_PINLINES_H
#define _INTUITION_PINLINES_H
/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Private inlines for intuition.library
    Lang: english
*/

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef INTUI_BASE_NAME
#define INTUI_BASE_NAME IntuitionBase
#endif

#define LateIntuiInit(data) \
    LP1( , BOOL, LateIntuiInit, \
	APTR, data, a0, \
	INTUI_BASE_NAME )

#endif _INTUITION_PINLINES_H
