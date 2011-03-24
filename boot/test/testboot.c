#include <utility/tagitem.h>

#include <bootconsole.h>
#include <stdarg.h>

int __vcformat(void *data, int(*outc)(int, void *), const char * format, va_list args);
int kprintf(const char *format, ...);

void __startup start(struct TagItem *tags)
{
    fb_Mirror = (void *)0x100000;
    con_InitTagList(tags);
    
    kprintf("Test module succesfully started\n");
    kprintf("Taglist at 0x%p\n", tags);

    for (;;);
}

static int kputc(int c, void *data)
{
    con_Putc(c);
    return 1;
}

int kprintf(const char *format, ...)
{
    va_list ap;
    int res;

    va_start(ap, format);
    res = __vcformat(NULL, kputc, format, ap);
    va_end(ap);

    return res;
}
