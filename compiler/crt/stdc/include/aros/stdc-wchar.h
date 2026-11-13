#ifndef _STDC_WCHAR_H
#define _STDC_WCHAR_H
/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#if defined(__WCHAR_MAX__) && __WCHAR_MAX__ > 256
#include <aros/stdc-wchar-utf8.h>
#else
#include <aros/stdc-wchar-char.h>
#endif

#endif /* _STDC_WCHAR_H */
