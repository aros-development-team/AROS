#ifndef _AROS_STDDEF_MAX_ALIGN_T_H
#define _AROS_STDDEF_MAX_ALIGN_T_H

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    max_align_t type definition
*/

#include <aros/cpu.h>  // for __WORDSIZE

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    long long __ll;
    long double __ld;
#if __WORDSIZE == 64
	void *__p; // ensure 8-byte or higher alignment
#endif
} max_align_t;

#ifdef __cplusplus
}
#endif

#endif /* _AROS_STDDEF_MAX_ALIGN_T_H */
