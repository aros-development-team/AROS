#ifndef _STDC_STDCNOTIMPL_H_
#define _STDC_STDCNOTIMPL_H_

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Macros for exposing stdc function that are not yet implemented.
*/


#if !defined(STDC_INCLUDE_NOTIMPL)
# define STDC_NOTIMPL(x)
# if !defined(STDC_INCLUDE_STDLIB_NOTIMPL)
# define STDC_STLDLIB_NOTIMPL STDC_NOTIMPL
# else
# define STDC_STLDLIB_NOTIMPL(x) x
# endif
# if !defined(STDC_INCLUDE_MATH_NOTIMPL)
#  define STDC_MATH_NOTIMPL STDC_NOTIMPL
# else
#  define STDC_MATH_NOTIMPL(x) x
# endif
# if !defined(STDC_INCLUDE_STRING_NOTIMPL)
#  define STDC_STRING_NOTIMPL STDC_NOTIMPL
# else
#  define STDC_STRING_NOTIMPL(x) x
# endif
# if !defined(STDC_INCLUDE_WCHAR_NOTIMPL)
#  define STDC_WCHAR_NOTIMPL STDC_NOTIMPL
# else
#  define STDC_WCHAR_NOTIMPL(x) x
# endif
# if !defined(STDC_INCLUDE_WCTYPE_NOTIMPL)
#  define STDC_WCTYPE_NOTIMPL STDC_NOTIMPL
# else
#  define STDC_WCTYPE_NOTIMPL(x) x
# endif
#else
# define STDC_NOTIMPL(x) x
# define STDC_STLDLIB_NOTIMPL STDC_NOTIMPL
# define STDC_MATH_NOTIMPL STDC_NOTIMPL
# define STDC_STRING_NOTIMPL STDC_NOTIMPL
# define STDC_WCHAR_NOTIMPL STDC_NOTIMPL
# define STDC_WCTYPE_NOTIMPL STDC_NOTIMPL
#endif

#endif /* _STDC_STDCNOTIMPL_H_ */
