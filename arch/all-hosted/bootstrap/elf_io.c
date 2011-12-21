#include <elfloader.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * On MinGW32CE we:
 * 1. Don't have 'errno'
 * 2. Have a problem with errno.h (it tries to #include_next <errno.h>, which does not exist.
 */
#ifdef __COREDLL__
#include <windows.h>
#define errno GetLastError()
#else
#include <errno.h>
#endif

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

    /* Reset list pointers. The list can ocassionally be reused (on Android, for example) */
    LastELF = (struct ELFNode *)&FirstELF;
}

void *open_file(struct ELFNode *n, unsigned int *err)
{
    FILE *f;

    f = fopen(((struct ExtELFNode *)n)->FullName, "rb");
    *err = f ? 0 : errno;
    
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
    	return errno;

    err = fread(dest, length, 1, file);
    if (err == 0)
    	return errno;

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
    	*err = errno;

    return dest;
}

void free_block(void *addr)
{
    free(addr);
}
