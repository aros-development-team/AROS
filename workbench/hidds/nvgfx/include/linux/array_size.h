/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_ARRAY_SIZE_H_
#define _LINUX_ARRAY_SIZE_H_

#ifndef ARRAY_SIZE /* Mesa util/macros.h has the same */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#endif /* _LINUX_ARRAY_SIZE_H_ */
