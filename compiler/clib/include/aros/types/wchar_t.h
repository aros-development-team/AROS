#ifndef _AROS_TYPES_WCHAR_T_H
#define _AROS_TYPES_WCHAR_T_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: /aros/branches/ABI_V1/trunk-aroscsplit/AROS/compiler/arosstdc/include/aros/types/wchar_t.h 36769 2011-01-11T21:35:35.950252Z verhaegs  $
*/

#ifndef __cplusplus

#ifndef __GNUC__
typedef char wchar_t;
#else
#define __need_whar_t
#include <stddef.h>
#endif

#endif

#endif /* _AROS_TYPES_WCHAR_T_H */
