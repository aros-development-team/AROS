#ifndef _AROS_TYPES_IOVEC_S_H
#define _AROS_TYPES_IOVEC_S_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: /aros/branches/ABI_V1/trunk-aroscsplit/AROS/compiler/arosnixc/include/aros/types/iovec_s.h 35917 2010-11-20T22:28:04.084725Z verhaegs  $
*/

#include <aros/types/size_t.h>

struct iovec
{
    void        *iov_base;
    size_t       iov_len;
};

#endif /* _AROS_TYPES_IOVEC_S_H */
