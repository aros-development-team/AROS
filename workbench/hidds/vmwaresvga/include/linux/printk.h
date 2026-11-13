/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_PRINTK_H_
#define _LINUX_PRINTK_H_

#include <stdarg.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kern_levels.h>
#include <linux/linkage.h>
#include <linux/ratelimit_types.h>
#include <linux/once_lite.h>

/* Linux's level markers: parsed and stripped by vprintk() to decide where a
   line goes (serial and/or the log file) */
#define KERN_SOH                "\001"
#define KERN_EMERG              KERN_SOH "0"
#define KERN_ALERT              KERN_SOH "1"
#define KERN_CRIT               KERN_SOH "2"
#define KERN_ERR                KERN_SOH "3"
#define KERN_WARNING            KERN_SOH "4"
#define KERN_NOTICE             KERN_SOH "5"
#define KERN_INFO               KERN_SOH "6"
#define KERN_DEBUG              KERN_SOH "7"
#define KERN_DEFAULT            ""
#define KERN_CONT               KERN_SOH "c"
#define LOGLEVEL_EMERG          0
#define LOGLEVEL_ALERT          1
#define LOGLEVEL_CRIT           2
#define LOGLEVEL_ERR            3
#define LOGLEVEL_WARNING        4
#define LOGLEVEL_NOTICE         5
#define LOGLEVEL_INFO           6
#define LOGLEVEL_DEBUG          7

struct va_format {
    const char *fmt;
    va_list *va;
};

/*
 * A printf that understands the kernel's %pV / %pe / %ps / %*ph extensions,
 * emitting through the debug channel.
 */
int  vprintk(const char *fmt, va_list ap);
int  __printf(1, 2) printk(const char *fmt, ...);
int  vsnprintk(char *buf, size_t size, const char *fmt, va_list ap);
int  __printf(3, 4) snprintk(char *buf, size_t size, const char *fmt, ...);
int  vscnprintk(char *buf, size_t size, const char *fmt, va_list ap);
int  __printf(3, 4) scnprintk(char *buf, size_t size, const char *fmt, ...);
/* route the kernel-style formatters through the extended implementation */
#include <stdio.h>
#undef snprintf
#undef vsnprintf
#define snprintf                snprintk
#define vsnprintf               vsnprintk
#define scnprintf               scnprintk
#define vscnprintf              vscnprintk

#define printk_once(fmt, ...) ({                                        \
    static bool __print_once;                                           \
    if (!__print_once) { __print_once = true; printk(fmt, ##__VA_ARGS__); } })
#define printk_ratelimited(fmt, ...)    printk(fmt, ##__VA_ARGS__)
#define printk_deferred(fmt, ...)       printk(fmt, ##__VA_ARGS__)
#define no_printk(fmt, ...)             ({ if (0) printk(fmt, ##__VA_ARGS__); 0; })

#ifndef pr_fmt
#define pr_fmt(fmt)             fmt
#endif
#define pr_emerg(fmt, ...)      printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#define pr_alert(fmt, ...)      printk(KERN_ALERT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_crit(fmt, ...)       printk(KERN_CRIT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_err(fmt, ...)        printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warn(fmt, ...)       printk(KERN_WARNING pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warning               pr_warn
#define pr_notice(fmt, ...)     printk(KERN_NOTICE pr_fmt(fmt), ##__VA_ARGS__)
#define pr_info(fmt, ...)       printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#define pr_cont(fmt, ...)       printk(fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...)      no_printk(fmt, ##__VA_ARGS__)
#define pr_devel(fmt, ...)      no_printk(fmt, ##__VA_ARGS__)
#define pr_err_once(fmt, ...)   printk_once(fmt, ##__VA_ARGS__)
#define pr_warn_once(fmt, ...)  printk_once(fmt, ##__VA_ARGS__)
#define pr_info_once(fmt, ...)  printk_once(fmt, ##__VA_ARGS__)
#define pr_notice_once(fmt, ...) printk_once(fmt, ##__VA_ARGS__)
#define pr_err_ratelimited(fmt, ...)    printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warn_ratelimited(fmt, ...)   printk(KERN_WARNING pr_fmt(fmt), ##__VA_ARGS__)
#define pr_info_ratelimited(fmt, ...)   printk(fmt, ##__VA_ARGS__)
#define pr_debug_ratelimited(fmt, ...)  no_printk(fmt, ##__VA_ARGS__)
#define pr_notice_ratelimited(fmt, ...) printk(fmt, ##__VA_ARGS__)

#define DEFINE_RATELIMIT_STATE(name, i, b) int name __maybe_unused
#define __ratelimit(x)          (1)
#define printk_timed_ratelimit(a, b) (1)
#define console_lock()          do { } while (0)
#define console_unlock()        do { } while (0)
#define console_trylock()       (1)
#define console_verbose()       do { } while (0)
#define printk_get_level(s)     0
#define printk_skip_level(s)    (s)

#endif /* _LINUX_PRINTK_H_ */
