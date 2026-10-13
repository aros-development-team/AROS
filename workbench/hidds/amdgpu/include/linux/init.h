/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_INIT_H_
#define _LINUX_INIT_H_

#define __init
#define __exit
#define __initdata
#define __initconst
#define __exitdata
#define __init_or_module
#define module_init(fn) \
    int __aros_module_init_##fn(void); \
    int __aros_module_init_##fn(void) { return fn(); }
#define module_exit(fn) \
    void __aros_module_exit_##fn(void); \
    void __aros_module_exit_##fn(void) { fn(); }
#define subsys_initcall(x)
#define late_initcall(x)

#ifndef THIS_MODULE
#define THIS_MODULE             ((struct module *)0)
#endif

#endif /* _LINUX_INIT_H_ */
