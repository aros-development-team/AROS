/* Definitions for what m68kemu_memory.c reaches outside itself; the host
   tests have no tasks, shadows, screens, sound or log. Included after
   m68kemu_intern.h by each test. */
#include <proto/exec.h>
#include "m68kemu_shadow.h"
static struct ExecBase SysBase_storage = {};
struct ExecBase *SysBase_stub = &SysBase_storage;
static char host_task_storage;
struct Task *m68kemu_host_task = (struct Task *)&host_task_storage;
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
void  m68k_paula_free(struct M68KEmuContext *) {}
/* the optional engine: m68kemu_engine.c is not built here. A test that
   wants one sets eng_ops and eng_domain itself; these two do for it what
   that file does. */
void  m68k_engine_invalidate(struct M68KEmuContext *ctx)
{
    if (ctx->eng_domain)
        ctx->eng_ops->invalidate(ctx->eng_domain, 0, M68KENG_INVALIDATE_ALL);
}
void  m68k_engine_close(struct M68KEmuContext *ctx)
{
    /* a CPU of the program is still there: the engine keeps its domain and
       the context its memory, as in the library */
    if (ctx->eng_domain && ctx->eng_ops->domain_destroy(ctx->eng_domain) != M68KENG_OK)
        ctx->eng_kept = TRUE;
    ctx->eng_domain = nullptr;
    ctx->eng_ops = nullptr;
    ctx->eng_run = FALSE;
}
}
ULONG shadow_register(struct M68KEmuContext *, ULONG, void *, UWORD) { return 0; }
void  shadow_sync(struct M68KEmuContext *, const struct M68KStructLayout *, ULONG, void *) {}
const struct M68KStructLayout *shadow_find_layout(const char *) { return nullptr; }
#ifdef M68KEMU_HOST_WITH_MOIRA
/* the CPU driver's task, time, window and chip hooks: one task, no clock,
   no windows, no CIA timers, no Paula */
extern "C" {
struct M68KTaskState *m68k_ts(struct M68KEmuContext *ctx) { return &ctx->orphan; }
void  m68k_ts_main(struct M68KEmuContext *) {}
void  m68k_ts_drop(struct M68KEmuContext *) {}
void  m68k_program_stop(struct M68KEmuContext *ctx, BOOL) { ctx->running = FALSE; }
/* the program lock has an owner, so that a slice without it is refused */
void  m68k_gil_obtain(struct M68KEmuContext *ctx) { ctx->gil.ss_Owner = FindTask(NULL); }
void  m68k_gil_release(struct M68KEmuContext *ctx) { ctx->gil.ss_Owner = nullptr; }
void  m68k_gil_yield(struct M68KEmuContext *) {}
ULONG m68k_bptr_out(struct M68KEmuContext *, BPTR b) { return (ULONG)(IPTR)b; }
void  m68k_planar_touch(struct M68KEmuContext *, ULONG, ULONG) {}
BOOL  m68k_vbl_due(struct M68KEmuContext *) { return FALSE; }
ULONG m68k_eclock_now(struct M68KEmuContext *, ULONG *freq) { if (freq) *freq = 709379; return 0; }
void  m68k_watchdog_start(struct M68KEmuContext *) {}
void  m68k_watchdog_stop(struct M68KEmuContext *) {}
void  m68k_sync_windowptr(struct M68KEmuContext *) {}
void  m68k_dump_callring(struct M68KEmuContext *) {}
void  m68k_cia_timer_control(struct M68KEmuContext *, int, int, UBYTE) {}
BOOL  m68k_cia_due(struct M68KEmuContext *, ULONG *, ULONG *) { return FALSE; }
void  m68k_paula_write(struct M68KEmuContext *, UWORD, UWORD) {}
UWORD m68k_paula_dmacon(struct M68KEmuContext *) { return 0; }
BOOL  m68k_engine_map(struct M68KEmuContext *) { return TRUE; }
void  m68k_engine_report(struct M68KEmuContext *) {}
void  m68k_engine_say(struct M68KEmuContext *, const char *, ...) {}
const char *m68k_engine_name(struct M68KEmuContext *) { return "host test"; }
}
#endif
