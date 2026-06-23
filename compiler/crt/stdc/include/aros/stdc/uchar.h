#ifndef _STDC_UCHAR_H_
#define _STDC_UCHAR_H_
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ISO C11 <uchar.h> - Unicode (UTF-16/UTF-32) character utilities.
*/

#include <aros/system.h>

#include <aros/types/size_t.h>
#include <aros/types/mbstate_t.h>

#include <stdint.h>

#ifndef __cplusplus
typedef uint_least16_t char16_t;
typedef uint_least32_t char32_t;
#endif

__BEGIN_DECLS

size_t mbrtoc16(char16_t * restrict pc16, const char * restrict s, size_t n,
	mbstate_t * restrict ps);
size_t c16rtomb(char * restrict s, char16_t c16, mbstate_t * restrict ps);
size_t mbrtoc32(char32_t * restrict pc32, const char * restrict s, size_t n,
	mbstate_t * restrict ps);
size_t c32rtomb(char * restrict s, char32_t c32, mbstate_t * restrict ps);

__END_DECLS

#endif /* _STDC_UCHAR_H_ */
