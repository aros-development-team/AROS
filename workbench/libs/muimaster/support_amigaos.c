#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "support_amigaos.h"
 
/***************************************************************************/

#ifndef __amigaos4__   

/************************************************************
 Like AllocVec() but for pools
*************************************************************/
APTR AllocVecPooled(APTR pool, ULONG size)
{
    IPTR *memory;
    
    if (pool == NULL) return NULL;
    
    size   += sizeof(IPTR);
    memory  = AllocPooled(pool, size);
    
    if (memory != NULL)
    {
        *memory++ = size;
    }

    return memory;
}

/************************************************************
 Like FreeVec() but for pools
*************************************************************/
VOID FreeVecPooled(APTR pool, APTR memory)
{   
    if (memory != NULL)
    {
        IPTR *real = (IPTR *) memory;
        IPTR size  = *--real;

        FreePooled(pool, real, size);
    }
}

struct snprintf_msg
{
	int size;
	char *buf;
};

/************************************************************
 Snprintf function for RawDoFmt()
*************************************************************/
__asm void snprintf_func(register __d0 UBYTE chr, register __a3 struct snprintf_msg *msg)
{
    if (msg->size)
    {
		  *msg->buf++ = chr;
    	msg->size--;
    }
}

/************************************************************
 Snprintf via RawDoFmt()
*************************************************************/
int snprintf(char *buf, int size, const char *fmt, ...)
{
    struct snprintf_msg msg;
		if (!size) return 0;

    msg.size = size;
    msg.buf = buf;

    RawDoFmt(fmt, (((ULONG *)&fmt)+1), snprintf_func, &msg);

    buf[size-1] = 0;

		return (int)strlen(buf);
}

/************************************************************
 sprintf via RawDoFmt()
*************************************************************/
int sprintf(char *buf, const char *fmt, ...)
{
		static const ULONG cpy_func = 0x16c04e75; /* move.b d0,(a3)+ ; rts */
		RawDoFmt(fmt, (((ULONG *)&fmt)+1), (void(*)())&cpy_func, buf);
		return (int)strlen(buf);
}
#else
ASM ULONG HookEntry(REG(a0, struct Hook *hook),REG(a2, APTR obj), REG(a1, APTR msg))
{
	return hook->h_SubEntry(hook,obj,msg);
}
#endif

/***************************************************************************/

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

size_t strlcat(char *buf, const char *src, size_t len)
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
