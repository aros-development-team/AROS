#ifndef ELFLOADER_H_
#define ELFLOADER_H_

/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
*/

unsigned long long DebugInfo_ptr;

int count_elf_size(struct module *n, unsigned long *ro_size, unsigned long *rw_size);
int load_elf_file(struct module *n, unsigned long long virt);
void *kernel_lowest();
void *kernel_highest();
void set_base_address(void *, void *);

#endif /*ELFLOADER_H_*/
