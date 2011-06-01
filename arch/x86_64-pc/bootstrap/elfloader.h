#ifndef ELFLOADER_H_
#define ELFLOADER_H_

/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
*/

unsigned long long DebugInfo_ptr;

void load_elf_file(const char *Name, void *file, unsigned long long virt);
void *kernel_lowest();
void *kernel_highest();
void set_base_address(void *, void *);

#endif /*ELFLOADER_H_*/
