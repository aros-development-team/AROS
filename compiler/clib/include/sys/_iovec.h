#ifndef _SYS__IOVEC_H_
#define _SYS__IOVEC_H_
/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/systypes.h>

#ifdef  _AROS_SIZE_T_
typedef _AROS_SIZE_T_       size_t;
#undef  _AROS_SIZE_T_
#endif

struct iovec
{
    void        *iov_base;
    size_t       iov_len;
};

#endif /* _SYS__IOVEC_H_ */
