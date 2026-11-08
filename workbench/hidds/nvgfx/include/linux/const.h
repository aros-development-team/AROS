/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_CONST_H_
#define _LINUX_CONST_H_

#define __AC(X, Y)      (X##Y)
#define _AC(X, Y)       __AC(X, Y)
#define _AT(T, X)       ((T)(X))
#define _UL(x)          (_AC(x, UL))
#define _ULL(x)         (_AC(x, ULL))
#define _BITUL(x)       (_UL(1) << (x))
#define _BITULL(x)      (_ULL(1) << (x))
#define UL(x)           (_UL(x))
#define ULL(x)          (_ULL(x))

#endif /* _LINUX_CONST_H_ */
