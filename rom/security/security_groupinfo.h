/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library private group information
*/
#ifndef _SECURITY_GROUPINFO_H
#define _SECURITY_GROUPINFO_H

#include <libraries/security.h>

/* Private Group Information: a sub class of the public structure */
struct secPrivGroupInfo
{
    struct secGroupInfo Pub;            /* The public part          */
    STRPTR              Pattern;        /* Pattern matching temp    */
    ULONG               Count;          /* last info                */
};

#endif /* _SECURITY_GROUPINFO_H */
