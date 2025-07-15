/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.

    Desc: Floating point rounding support
*/

#define STDC_NOINLINE_FLOAT

#include <fenv.h>
#include <float.h>

int __flt_rounds()
{
	switch (fegetround()) {
#ifdef FE_TOWARDZERO
	case FE_TOWARDZERO: return 0;
#endif
	case FE_TONEAREST: return 1;
#ifdef FE_UPWARD
	case FE_UPWARD: return 2;
#endif
#ifdef FE_DOWNWARD
	case FE_DOWNWARD: return 3;
#endif
	}
	return -1;
}
