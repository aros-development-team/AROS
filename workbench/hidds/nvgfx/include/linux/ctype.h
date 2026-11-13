/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_CTYPE_H_
#define _LINUX_CTYPE_H_

#include <ctype.h>
#define isascii(c)      (((unsigned char)(c)) <= 0x7f)
#define toascii(c)      (((unsigned char)(c)) & 0x7f)

#endif /* _LINUX_CTYPE_H_ */
