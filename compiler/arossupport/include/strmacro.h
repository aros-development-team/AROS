#ifndef AROS_STRMACRO_H
#define AROS_STRMACRO_H

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: STR helper macro(s) to stringify, and concatenate, strings.
    Lang: english
*/

#define ___AROS_STR(x) #x
#define __AROS_STR(x) ___AROS_STR(x)

#define __AROS_CAT(a, b) a##b
#define __AROS_CAT_EXPAND(a, b) __AROS_CAT(a, b)

#define __AROS_CAT3(a, b, c) a##b##c
#define __AROS_CAT3_EXPAND(a, b, c) __AROS_CAT3(a, b, c)

#endif /* AROS_STRMACRO_H */
