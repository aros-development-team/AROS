/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_UACCESS_H_
#define _LINUX_UACCESS_H_

#include <string.h>
#include <linux/types.h>

/*
 * There is no user/kernel split: "userspace" is the hidd, running in the
 * same address space, so every copy is a memcpy.
 */
static inline unsigned long copy_from_user(void *to, const void __user *from, unsigned long n) { memcpy(to, from, n); return 0; }
static inline unsigned long copy_to_user(void __user *to, const void *from, unsigned long n)   { memcpy(to, from, n); return 0; }
static inline unsigned long __copy_from_user(void *to, const void __user *from, unsigned long n) { memcpy(to, from, n); return 0; }
static inline unsigned long __copy_to_user(void __user *to, const void *from, unsigned long n)   { memcpy(to, from, n); return 0; }
static inline unsigned long clear_user(void __user *to, unsigned long n) { memset(to, 0, n); return 0; }
#define __copy_from_user_inatomic(t, f, n)      copy_from_user(t, f, n)
#define __copy_to_user_inatomic(t, f, n)        copy_to_user(t, f, n)
#define __copy_from_user_inatomic_nocache(t, f, n) copy_from_user(t, f, n)
#define copy_struct_from_user(d, ds, s, ss)     ({ memset(d, 0, ds); memcpy(d, s, (ss) < (ds) ? (ss) : (ds)); 0; })
#define get_user(x, ptr)        ({ (x) = *(ptr); 0; })
#define put_user(x, ptr)        ({ *(ptr) = (x); 0; })
#define __get_user(x, ptr)      get_user(x, ptr)
#define __put_user(x, ptr)      put_user(x, ptr)
#define access_ok(addr, size)   (1)
#define user_access_begin(a, s) (1)
#define user_access_end()       do { } while (0)
#define unsafe_put_user(x, p, l) put_user(x, p)
#define unsafe_get_user(x, p, l) get_user(x, p)
#define user_write_access_begin(a, s) (1)
#define user_write_access_end() do { } while (0)
#define user_read_access_begin(a, s) (1)
#define user_read_access_end()  do { } while (0)
#define pagefault_disable()     do { } while (0)
#define pagefault_enable()      do { } while (0)
#define pagefault_disabled()    (0)
#define u64_to_user_ptr(x)      ((void __user *)(IPTR)(x))
#define VERIFY_READ             0
#define VERIFY_WRITE            1
#define memdup_user(src, len)   kmemdup(src, len, GFP_KERNEL)
#define vmemdup_user(src, len)  kmemdup(src, len, GFP_KERNEL)
#define memdup_array_user(src, n, sz) kmemdup(src, (n) * (sz), GFP_KERNEL)
#define strndup_user(s, n)      kstrndup(s, n, GFP_KERNEL)

#endif /* _LINUX_UACCESS_H_ */
