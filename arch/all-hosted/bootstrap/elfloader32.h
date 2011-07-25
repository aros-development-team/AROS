/*
 * Copyright (C) 2006 - 2011 The AROS Development Team. All rights reserved.
 * $Id$
 */


#ifndef ELFLOADER_H_
#define ELFLOADER_H_

struct ELFNode
{
    struct ELFNode   *Next;
    struct sheader   *sh;
    struct elfheader *eh;
    char	     *Name;
};

struct TagItem;
struct KernelBSS;
struct ELF_ModuleInfo;

typedef int (*kernel_entry_fun_t)(struct TagItem *, unsigned int);

int GetKernelSize(struct ELFNode *FirstELF, size_t *ro_size, size_t *rw_size);
int LoadKernel(struct ELFNode *FirstELF, void *ptr_ro, void *ptr_rw, struct KernelBSS *tracker, uintptr_t DefSysBase,
	       kernel_entry_fun_t *kernel_entry, struct ELF_ModuleInfo **kernel_debug);

#endif /*ELFLOADER_H_*/
