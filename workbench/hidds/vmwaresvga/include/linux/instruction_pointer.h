/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_INSTRUCTION_POINTER_H_
#define _LINUX_INSTRUCTION_POINTER_H_

#define _RET_IP_        (unsigned long)__builtin_return_address(0)
#define _THIS_IP_       ({ __label__ __here; __here: (unsigned long)&&__here; })

#endif /* _LINUX_INSTRUCTION_POINTER_H_ */
