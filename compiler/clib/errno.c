/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Global variable errno
    Lang: english
*/


int * __get_errno_ptr(void)
{
#ifndef  _CLIB_KERNEL_
    static int __errno;

    return &__errno;
#else
    GETUSER;

    return clib_userdata->errnoptr;
#endif
}

