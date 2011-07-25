#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <elfloader.h>

#include "elf_io.h"
#include "support.h"

#include <aros/config.h>

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

    n = malloc(sizeof(struct ExtELFNode) + strlen(name) + 1);
    if (!n)
        return 0;

    n->node.Next = NULL;
    strcpy(n->FullName, name);
#if AROS_MODULES_DEBUG
    n->node.Name = n->FullName;
#else
    n->node.Name = namepart(n->FullName);
#endif

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
    /* We do not reset list pointers because the list will never be reused */
}

void *open_file(struct ELFNode *n)
{
    return fopen(((struct ExtELFNode *)n)->FullName, "rb");
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
    	return 0;

    err = fread(dest, length, 1, file);
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
