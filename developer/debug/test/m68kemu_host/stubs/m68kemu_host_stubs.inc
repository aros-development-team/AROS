/* Definitions for what m68kemu_memory.c reaches outside itself; the host
   tests have no tasks, shadows, screens or log. Included after
   m68kemu_intern.h by each test. */
#include <proto/exec.h>
#include "m68kemu_shadow.h"
static struct ExecBase SysBase_storage = {};
struct ExecBase *SysBase_stub = &SysBase_storage;
extern "C" {
#ifndef M68KEMU_HOST_WITH_MOIRA
void  M68KEmu_FreeNestCpus(struct M68KEmuContext *, struct M68KTaskState *) {}
#endif
void  m68k_vbl_close(struct M68KEmuContext *) {}
void  m68k_publish_screens(struct M68KEmuContext *, ULONG, struct Library *) {}
void  m68k_publish_base(struct M68KEmuContext *, ULONG, const char *) {}
void  m68k_log(struct M68KEmuContext *, const char *, ...) {}
void  m68k_gil_init(struct M68KEmuContext *) {}
void  m68k_context_hold(struct M68KEmuContext *) {}
void  m68k_boopsi_cleanup(struct M68KEmuContext *) {}
}
ULONG shadow_register(struct M68KEmuContext *, ULONG, void *, UWORD) { return 0; }
void  shadow_sync(struct M68KEmuContext *, const struct M68KStructLayout *, ULONG, void *) {}
const struct M68KStructLayout *shadow_find_layout(const char *) { return nullptr; }
#ifdef M68KEMU_HOST_WITH_MOIRA
/* the CPU driver's task, time and window hooks: one task, no clock, no windows */
extern "C" {
struct M68KTaskState *m68k_ts(struct M68KEmuContext *ctx) { return &ctx->orphan; }
void  m68k_ts_main(struct M68KEmuContext *) {}
void  m68k_ts_drop(struct M68KEmuContext *) {}
void  m68k_program_stop(struct M68KEmuContext *ctx, BOOL) { ctx->running = FALSE; }
void  m68k_gil_obtain(struct M68KEmuContext *) {}
void  m68k_gil_release(struct M68KEmuContext *) {}
void  m68k_gil_yield(struct M68KEmuContext *) {}
ULONG m68k_bptr_out(struct M68KEmuContext *, BPTR b) { return (ULONG)(IPTR)b; }
void  m68k_planar_touch(struct M68KEmuContext *, ULONG, ULONG) {}
BOOL  m68k_vbl_due(struct M68KEmuContext *) { return FALSE; }
ULONG m68k_eclock_now(struct M68KEmuContext *, ULONG *freq) { if (freq) *freq = 709379; return 0; }
void  m68k_watchdog_start(struct M68KEmuContext *) {}
void  m68k_watchdog_stop(struct M68KEmuContext *) {}
void  m68k_sync_windowptr(struct M68KEmuContext *) {}
void  m68k_dump_callring(struct M68KEmuContext *) {}
}
#endif
