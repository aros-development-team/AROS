/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>

#include <pthread.h>

int __attribute__((weak))
pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id)
{
    (void)attr;
    (void)clock_id;
    return 0;
}

int __attribute__((weak))
pthread_getcpuclockid(pthread_t thread, clockid_t *clock_id)
{
    (void)thread;
    if (clock_id != NULL) {
        *clock_id = CLOCK_REALTIME;
    }
    return 0;
}

int __attribute__((weak))
pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset)
{
#if defined(SIG_BLOCK)
    return sigprocmask(how, set, oldset);
#else
    (void)how;
    (void)set;
    (void)oldset;
    return 0;
#endif
}

int __attribute__((weak))
mkstemps(char *template, int suffix_len)
{
    (void)template;
    (void)suffix_len;
    errno = ENOSYS;
    return -1;
}

void __attribute__((weak))
openlog(const char *ident, int option, int facility)
{
    (void)ident;
    (void)option;
    (void)facility;
}

void __attribute__((weak))
syslog(int priority, const char *format, ...)
{
    (void)priority;
    (void)format;
}

/*
 * aros_mesa_kprintf - kernel debug output for Mesa internals.
 * log.h redefines mesa_loge/logw/logi/logd to call this.
 * Must be in libmesautil so all consumers (mesa3dgl, softpipe,
 * radeonsi, etc.) can link it.
 */
int aros_mesa_kprintf(const char *fmt, ...)
{
    /* Minimal stub — Mesa logs discarded on AROS.
     * Replace with KPrintF(fmt, args) for kernel debug output. */
    (void)fmt;
    return 0;
}
