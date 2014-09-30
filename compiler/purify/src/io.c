/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include "funcs.h"
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

char * Purify_fgets (char * ptr, int size, FILE * stream)
{
    if (!Purify_CheckMemoryAccess (ptr, size, PURIFY_MemAccess_Write))
    {
	if (Purify_Error == NotOwnMemory)
	{
	    Purify_Error = IllPointer;
	    Purify_PrintError ("fgets (ptr=%p, size=%d, stream=%p)"
		, ptr, size, stream);
	}
	else
	{
	    Purify_PrintAccessError ("fgets()", ptr, size);
	}
    }

    return fgets (ptr, size, stream);
}

char * Purify_getcwd (char * buf, size_t size)
{
    char * ret;

    if (buf)
    {
	if (!Purify_CheckMemoryAccess (buf, size, PURIFY_MemAccess_Write))
	{
	    if (Purify_Error == NotOwnMemory)
	    {
		Purify_Error = IllPointer;
		Purify_PrintError ("getcwd (buf=%p, size=%ld)"
		    , buf, size);
	    }
	    else
	    {
		Purify_PrintAccessError ("getcwd()", buf, size);
	    }
	}

	ret = getcwd (buf, size);
    }
    else
    {
	char * ptr;

	ptr = getcwd (NULL, size);

	if (!ptr)
	    return NULL;

	if (!size)
	{
	    ret = Purify_strdup (ptr);

	    if (!ret)
		return NULL;
	}
	else
	{
	    MemHash * hash;

	    ret = Purify_malloc (size);

	    if (!ret)
		return NULL;

	    strcpy (ret, ptr);

	    hash = Purify_FindMemory (ret);
	    Purify_SetMemoryFlags (hash, 0, strlen (ptr) + 1,
		PURIFY_MemFlag_Readable|PURIFY_MemFlag_Writable
	    );
	}

	free (ptr);
    }

    return ret;
}

int Purify_stat (char * path, struct stat * st)
{
    int len;

    if (!Purify_CheckMemoryAccess (path, 1, PURIFY_MemAccess_Read))
	Purify_PrintAccessError ("stat (path, ...)", path, 1);

    len = strlen (path)+1;

    if (!Purify_CheckMemoryAccess (path, len, PURIFY_MemAccess_Read))
	Purify_PrintAccessError ("stat (path, ...)", path, len);

    if (!Purify_CheckMemoryAccess (st, sizeof (struct stat), PURIFY_MemAccess_Write))
	Purify_PrintAccessError ("stat (..., stat)", st, sizeof (struct stat));

    return stat (path, st);
}

int Purify_lstat (char * path, struct stat * st)
{
    int len;

    if (!Purify_CheckMemoryAccess (path, 1, PURIFY_MemAccess_Read))
	Purify_PrintAccessError ("lstat (path, ...)", path, 1);

    len = strlen (path)+1;

    if (!Purify_CheckMemoryAccess (path, len, PURIFY_MemAccess_Read))
	Purify_PrintAccessError ("lstat (path, ...)", path, len);

    if (!Purify_CheckMemoryAccess (st, sizeof (struct stat), PURIFY_MemAccess_Write))
	Purify_PrintAccessError ("lstat (..., stat)", st, sizeof (struct stat));

    return lstat (path, st);
}

int Purify_fstat (int fd, struct stat * st)
{
    if (!Purify_CheckMemoryAccess (st, sizeof (struct stat), PURIFY_MemAccess_Write))
	Purify_PrintAccessError ("fstat (..., stat)", st, sizeof (struct stat));

    return fstat (fd, st);
}

struct dirent * Purify_readdir (DIR * dir)
{
    static struct dirent * last_de = NULL;
    struct dirent * de;

    de = readdir (dir);

    if (de != last_de)
    {
	MemHash * hash;

	if (last_de)
	    Purify_RemMemory (last_de);

	hash = Purify_AddMemory (de, sizeof (struct dirent),
	    PURIFY_MemFlag_Writable
	    | PURIFY_MemFlag_Readable
	    , PURIFY_MemType_Data
	);

	hash->data = "dirent";

	last_de = de;
    }

    return de;
}

