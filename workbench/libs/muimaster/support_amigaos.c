#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>

#include "support_amigaos.h"
    
APTR AllocVecPooled(APTR pool, ULONG size)
{
    IPTR *memory;
    
    bug("exec/AllocVecPooled: %p, %d\n", pool, size);
    if (pool == NULL) return NULL;
    
    size   += sizeof(IPTR);
    memory  = AllocPooled(pool, size);
    
    if (memory != NULL)
    {
        *memory++ = size;
    }

    return memory;
}

VOID FreeVecPooled(APTR pool, APTR memory)
{   
    if (memory != NULL)
    {
        IPTR *real = (IPTR *) memory;
        IPTR size  = *--real;

        FreePooled(pool, real, size);
    }
}

char *StrDup(const char *x)
{
    char *dup;
    if (!x) return NULL;
    dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC);
    if (dup) CopyMem((char*)x, dup, strlen(x) + 1);
    return dup;
}

Object *DoSuperNewTagList(struct IClass *cl, Object *obj,void *dummy, struct TagItem *tags)
{
	  return (Object*)DoSuperMethod(cl,obj,OM_NEW,tags,NULL);
}

Object *DoSuperNewTags(struct IClass *cl, Object *obj, void *dummy, ...)
{
    va_list argptr;
    va_start(argptr,dummy);
    obj = DoSuperNewTagList(cl,obj,dummy,(struct TagItem*)argptr);
    va_end(argptr);
    return obj;
}

struct snprintf_msg
{
	int size;
	char *buf;
};

__asm void snprintf_func(register __d0 UBYTE chr, register __a3 struct snprintf_msg *msg)
{
    if (msg->size)
    {
		  *msg->buf++ = chr;
    	msg->size--;
    }
}

int snprintf(char *buf, int size, const char *fmt, ...)
{
#if 1
    struct snprintf_msg msg;
		if (!size) return 0;

    msg.size = size;
    msg.buf = buf;

    RawDoFmt(fmt, (((ULONG *)&fmt)+1), snprintf_func, &msg);

    buf[size-1] = 0;

		return (int)strlen(buf);
#else
    int ret;
    va_list argptr;
    va_start(argptr,fmt);
    ret = vsprintf(buf,fmt,argptr);
    va_end(argptr);
    return ret;
#endif
}

int sprintf(char *buf, const char *fmt, ...)
{
		static const ULONG cpy_func = 0x16c04e75; /* move.b d0,(a3)+ ; rts */
		RawDoFmt(fmt, (((ULONG *)&fmt)+1), (void(*)())&cpy_func, buf);
		return (int)strlen(buf);
}

int strlcat(char *buf, char *src, int len)
{
    int l = strlen(buf);
    buf += l;
    len -= l;

    if (len>0)
    {
	int i;
	for (i=0; i < len - 1 && *src; i++)
	    *buf++ = *src++;
	*buf = 0;
    }
    return 0; /* Actually don't know right rt here */
}
