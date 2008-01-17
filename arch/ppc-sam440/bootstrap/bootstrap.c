extern unsigned long __bss_start;
extern unsigned long _end;
register void * global_data asm("r29");

#include "slb.h"

int max_entries = 10;

void set_progress(sbl_context_t *ctx, int progress)
{
    int p = progress * 68 / max_entries;
    ctx->ssc_video_repeat_char(6, 7, p, 219, 0);
    ctx->ssc_video_repeat_char(6+p, 7, 68-p, 177, 0);
}

void show_text(sbl_context_t *ctx, char *text)
{
    
}

char *entries[] = {
        "kernel.resource", "exec.library", "expansion.library", "utilities.library",
        "oop.library", "battclock.resource", "foo", "whatever", "interesting",
        "some external module"
};

int __attribute__((section(".aros.startup"))) bootstrap(sbl_context_t *ctx) 
{
    unsigned long *ptr = &__bss_start;
    int i;
    
    while(ptr < &_end)
        *ptr++ = 0;
    
    ctx->ssc_printf_like("[BOOT] AROS SAM440 Bootstrap\n");
    ctx->ssc_video_clear();
    ctx->ssc_video_draw_box(1, 0, "Booting AROS", 1, 5, 4, 70, 7);
    
    set_progress(ctx, 0);
    
    for (i=0; i < 10; i++)
    {
        ctx->ssc_udelay(200000);
        set_progress(ctx, i+1);
        ctx->ssc_video_draw_text(6, 9, 0, entries[i], 68);
    }
    
    /* Failure? */
    return SLB_ERROR_NO_CONFIG;
}
