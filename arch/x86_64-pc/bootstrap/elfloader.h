#ifndef ELFLOADER_H_
#define ELFLOADER_H_

/*
    Copyright (C) 2006-2011 The AROS Development Team. All rights reserved.
    $Id$
*/

void load_elf_file(void *, unsigned long long);
void *kernel_lowest();
void *kernel_highest();
void set_base_address(void *, void *);

#endif /*ELFLOADER_H_*/
