/*
    Copyright © 2002-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _FLOATTEXT_PRIVATE_H_
#define _FLOATTEXT_PRIVATE_H_

#include <exec/types.h>
#include <utility/hooks.h>

struct Floattext_DATA
{
    STRPTR       text;
    BOOL         justify;
    BOOL         typesetting;
    STRPTR       skipchars;
    ULONG        tabsize;
    ULONG        oldwidth;
};

#endif /* _FLOATTEXT_PRIVATE_H_ */
