#include <stdio.h>
#include <unistd.h>
#include "hash.h"
#include "error.h"

size_t Purify_fread (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (!Purify_CheckMemoryAccess (ptr, size*nmemb, PURIFY_MemAccess_Write))
    {
	if (Purify_Error == NotOwnMemory)
	{
	    Purify_Error = IllPointer;
	    Purify_PrintError ("fread (addr=%p, size=%ld, num=%ld, file=%p)"
		, ptr, size, nmemb, stream);
	}
	else
	{
	    Purify_PrintAccessError ("fread()", ptr, size);
	}
    }

    return fread (ptr, size, nmemb, stream);
}

size_t Purify_read (int fd, void *ptr, size_t size)
{
    if (!Purify_CheckMemoryAccess (ptr, size, PURIFY_MemAccess_Write))
    {
	if (Purify_Error == NotOwnMemory)
	{
	    Purify_Error = IllPointer;
	    Purify_PrintError ("read (fd=%d, addr=%p, size=%ld)"
		, fd, ptr, size);
	}
	else
	{
	    Purify_PrintAccessError ("read()", ptr, size);
	}
    }

    return read (fd, ptr, size);
}


