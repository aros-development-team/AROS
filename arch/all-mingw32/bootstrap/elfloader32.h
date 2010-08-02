#ifndef ELFLOADER_H_
#define ELFLOADER_H_

/*
 Copyright (C) 2006 The AROS Development Team. All rights reserved.
 $Id$
 */

void *kernel_highest();
void set_base_address(void *kstart, void *tracker, void ** sysbaseaddr);

int AddKernelFile(char *name);
void FreeKernelList(void);
size_t GetKernelSize(void);
int LoadKernel(void);

#endif /*ELFLOADER_H_*/
