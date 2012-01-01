#include <aros/kernel.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>

#include <stdarg.h>

#include "../kernel/hostinterface.h"

int __vcformat(void *data, int(*outc)(int, void *), const char * format, va_list args);
int KPrintf(struct HostInterface *hif, const char *format, ...);

int __startup start(const struct TagItem *tags)
{
    struct TagItem *tag;
    struct HostInterface *hif = (struct HostInterface *)LibGetTagData(KRN_HostInterface, 0, tags);

    if (!hif)
        return -1;

    KPrintf(hif, "Test module succesfully started\n");
    KPrintf(hif, "Taglist at 0x%p:\n", tags);

    while ((tag = LibNextTagItem((struct TagItem **)&tags)))
    {
    	KPrintf(hif, "0x%08lX 0x%p\n", tag->ti_Tag, tag->ti_Data);
    }

    return 0;
}

static int kputc(int c, void *data)
{
    return ((struct HostInterface *)data)->KPutC(c);
}

int KPrintf(struct HostInterface *iface, const char *format, ...)
{
    va_list ap;
    int res;

    va_start(ap, format);
    res = __vcformat(iface, kputc, format, ap);
    va_end(ap);

    return res;
}
