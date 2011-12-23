/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: I/O routines for bootstrap ELF loader
    Lang: english
*/

#include <elfloader.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "bootstrap.h"
#include "elf_io.h"
#include "filesystem.h"

struct ExtELFNode
{
    struct ELFNode node;
    char FullName[1];		/* We need to store the full pathname */
};

struct ELFNode *FirstELF = NULL;
static struct ELFNode *LastELF = (struct ELFNode *)&FirstELF;

int AddKernelFile(char *name)
{
    struct ExtELFNode *n;
    int l1 = 0;
    int len = sizeof(struct ExtELFNode) + strlen(name);

    if (*name != '\\')
    {
        l1 = strlen(bootstrapdir);
        len += l1;
    }

    n = malloc(len);
    if (!n)
        return 0;

    n->node.Next = NULL;
    
    if (l1)
        memcpy(n->FullName, bootstrapdir, l1);
    strcpy(n->FullName + l1, name);
    n->node.Name = namepart(n->FullName);

    LastELF->Next = &n->node;
    LastELF = &n->node;

    return 1;
}

void FreeKernelList(void)
{
    struct ELFNode *n, *n2;
    
    for (n = FirstELF; n; n = n2)
    {
	n2 = n->Next;
	free(n);
    }
}

void *open_file(struct ELFNode *n, unsigned int *err)
{
    FILE *f;

    f = fopen(((struct ExtELFNode *)n)->FullName, "rb");
    *err = f ? 0 : GetLastError();

    return f;
}

void close_file(void *file)
{
    fclose(file);
}

/*
 * read_block interface. we want to read from files here
 */
int read_block(void *file, unsigned long offset, void *dest, unsigned long length)
{
    int err;

    err = fseek(file, offset, SEEK_SET);
    if (err)
    	return GetLastError();

    err = fread(dest, length, 1, file);
    if (err == 0)
    	return GetLastError();

    return 0;
}

/*
 * load_block also allocates the memory
 */
void *load_block(void *file, unsigned long offset, unsigned long length, unsigned int *err)
{
    void *dest = malloc(length);
    
    if (dest)
    {
	*err = read_block(file, offset, dest, length);
	if (*err)
	{
	    free(dest);
	    return NULL;
	}
	*err = 0;
    }
    else
    	*err = GetLastError();

    return dest;
}

void free_block(void *addr)
{
    free(addr);
}
