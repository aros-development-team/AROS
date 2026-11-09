/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MODULEPARAM_H_
#define _LINUX_MODULEPARAM_H_

#define MODULE_PARM_DESC(a, b)
#define module_param(a, b, c)
#define module_param_named(a, b, c, d)
#define module_param_unsafe(a, b, c)
#define module_param_named_unsafe(a, b, c, d)
#define module_param_call(a, b, c, d, e)
#define __MODULE_PARM_TYPE(a, b)
#define param_get_int           NULL
#define param_set_int           NULL
struct kernel_param;
struct kernel_param_ops {
    unsigned int flags;
    int (*set)(const char *val, const struct kernel_param *kp);
    int (*get)(char *buffer, const struct kernel_param *kp);
    void (*free)(void *arg);
};

#endif /* _LINUX_MODULEPARAM_H_ */
