/*
 * Copyright (C) 2006 - 2011 The AROS Development Team. All rights reserved.
 * $Id$
 */

#ifndef BOOTSTRAP_ELFLOADER_H_
#define BOOTSTRAP_ELFLOADER_H_

#include <inttypes.h>

/* The loader operates with a single-linked list of these structures */
struct ELFNode
{
    struct ELFNode   *Next;
    struct sheader   *sh;
    struct elfheader *eh;
    char	     *Name;
};

/* Some forward declarations */
struct TagItem;
struct KernelBSS;
struct ELF_ModuleInfo;

/* This is the calling convention for kickstart entry point */
typedef int (*kernel_entry_fun_t)(struct TagItem *, unsigned int);

/* Two main functions you will use */
int GetKernelSize(struct ELFNode *FirstELF, unsigned long *ro_size, unsigned long *rw_size, unsigned long *bss_size);
int LoadKernel(struct ELFNode *FirstELF, void *ptr_ro, void *ptr_rw, char *bss_tracker, uintptr_t DefSysBase,
	       void **kick_end, kernel_entry_fun_t *kernel_entry, struct ELF_ModuleInfo **kernel_debug);

/*
 * These functions are used to access files by the loader.
 * They need to be provided by your bootstrap implementation.
 */
void *open_file(struct ELFNode *n, unsigned int *err);
void close_file(void *file);
int read_block(void *file, unsigned long offset, void *dest, unsigned long length);
void *load_block(void *file, unsigned long offset, unsigned long length, unsigned int *err);
void free_block(void *addr);

#endif
