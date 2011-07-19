#ifndef ELFLOADER_H_
#define ELFLOADER_H_

/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
*/

unsigned long long DebugInfo_ptr;

int count_elf_size(struct module *n, unsigned long *ro_size, unsigned long *rw_size);
int LoadKernel(unsigned long code_addr, unsigned long data_addr, void *bss_track, unsigned long long virt, struct module *mod, unsigned int module_count);
void *kernel_highest();

#endif /*ELFLOADER_H_*/
