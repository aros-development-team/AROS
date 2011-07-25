#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "elfloader32.h"
#include "elf_io.h"
#include "support.h"

#include <aros/config.h>

struct ELFNode *FirstELF = NULL;
static struct ELFNode *LastELF = (struct ELFNode *)&FirstELF;

int AddKernelFile(char *name)
{
    struct ELFNode *n;

    n = malloc(sizeof(struct ELFNode) + strlen(name));
    if (!n)
        return 0;
    
    n->Next  = NULL;
    strcpy(n->Name, name);
#if AROS_MODULES_DEBUG
    n->NamePtr = n->Name;
#else
    n->NamePtr = namepart(n->Name);
#endif

    LastELF->Next = n;
    LastELF = n;

    return 1;
}

void FreeKernelList(void)
{
    struct ELFNode *n, *n2;
    
    for (n = FirstELF; n; n = n2) {
	n2 = n->Next;
	free(n);
    }
    /* We do not reset list pointers because the list will never be reused */
}

void *open_file(struct ELFNode *n)
{
    return fopen(n->Name, "rb");
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
    if (err) return 0;

    err = fread(dest,(size_t)length, 1, file);
    if (err == 0)
    	return 0;

    return 1;
}

/*
 * load_block also allocates the memory
 */
void *load_block(void *file, unsigned long offset, unsigned long length)
{
    void *dest = malloc(length);
    
    if (dest)
    {
	if (!read_block(file, offset, dest, length))
	{
	    free(dest);
	    return NULL;
	}
    }

    return dest;
}

void free_block(void *addr)
{
    free(addr);
}
