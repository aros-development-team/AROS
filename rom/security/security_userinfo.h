/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library private user information
*/
#ifndef _SECURITY_USERINFO_H
#define _SECURITY_USERINFO_H

#include <libraries/security.h>

/* Private User Information: a sub class of the public structure */
struct secPrivUserInfo
{
    struct secUserInfo  Pub;            /* The public part                      */
    BOOL                Password;       /* TRUE if the User has a password      */
    STRPTR              Pattern;        /* Pattern matching temp                */
    ULONG               Count;          /* last info                            */
    UWORD               Tgid;           /* gid for secKeyType_gidNext           */
};

#endif /* _SECURITY_USERINFO_H */
