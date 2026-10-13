/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_ERRNO_H_
#define _LINUX_ERRNO_H_

#include <errno.h>

#ifndef ERESTARTSYS
#define ERESTARTSYS     512
#endif
#ifndef ENOTSUPP
#define ENOTSUPP        524
#endif
#ifndef EREMOTEIO
#define EREMOTEIO       121
#endif
#ifndef ENOPKG
#define ENOPKG          65
#endif
#ifndef EPROBE_DEFER
#define EPROBE_DEFER    517
#endif
#ifndef ERFKILL
#define ERFKILL         132
#endif
#ifndef ECOMM
#define ECOMM           70
#endif
#ifndef ENOSR
#define ENOSR           63
#endif
#ifndef ELIBBAD
#define ELIBBAD         80
#endif
#ifndef ENOKEY
#define ENOKEY          126
#endif
#ifndef ETIME
#define ETIME           62
#endif

#endif /* _LINUX_ERRNO_H_ */
