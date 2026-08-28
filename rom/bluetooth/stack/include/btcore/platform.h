#ifndef BTCORE_PLATFORM_H
#define BTCORE_PLATFORM_H

#include <btcore/timer.h>
#include <btcore/types.h>

/*
 * Minimal seam between the portable core and whatever it runs on (project.md,
 * "Núcleo independente"). The core must never call AllocMem/CreateTask/
 * OpenDevice/pthreads/sockets/etc directly -- only through this table, so
 * the same core builds against an AROS port and a plain test-host port.
 *
 * No implementation lives here yet: nothing in the core currently drives a
 * platform through this interface (that starts once a real event loop --
 * the AROS Bluetooth Manager Task, or a test-host equivalent -- exists to
 * own it). Declared now because project.md requires it as a Fase 1
 * deliverable; ports/test-host and ports/aros each get a concrete
 * implementation when something actually calls into it.
 */
struct bt_platform_ops
{
    void *(*alloc)(size_t size);
    void (*free)(void *ptr);

    uint64_t (*time_us)(void);

    int (*timer_start)(void *platform, struct bt_timer *timer, uint64_t delay_us);
    void (*timer_cancel)(void *platform, struct bt_timer *timer);

    void (*schedule)(void *platform);

    void (*log)(void *platform, unsigned level, const char *message);
};

#endif /* BTCORE_PLATFORM_H */
