/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "kernel_traps.h"

struct ExceptionTranslation Traps[] =
{
    {EXCEPTION_ACCESS_VIOLATION     ,  2, 13},
    {EXCEPTION_ARRAY_BOUNDS_EXCEEDED,  2,  5},
    {EXCEPTION_BREAKPOINT	    ,  4,  3},
    {EXCEPTION_DATATYPE_MISALIGNMENT,  3, 17},
    {EXCEPTION_FLT_DIVIDE_BY_ZERO   ,  5, 16},
    {EXCEPTION_GUARD_PAGE	    ,  2, 14},
    {EXCEPTION_ILLEGAL_INSTRUCTION  ,  4,  6},
    {EXCEPTION_IN_PAGE_ERROR	    ,  2, 14},
    {EXCEPTION_INT_DIVIDE_BY_ZERO   ,  5,  0},
    {EXCEPTION_PRIV_INSTRUCTION     ,  8, 13},
    {EXCEPTION_SINGLE_STEP	    ,  9,  1},
    {0				    , -1, -1}
};
