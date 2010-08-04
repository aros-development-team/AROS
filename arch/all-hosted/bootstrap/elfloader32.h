#ifndef ELFLOADER_H_
#define ELFLOADER_H_

/*
 Copyright (C) 2006 - 2010 The AROS Development Team. All rights reserved.
 $Id$
 */

int AddKernelFile(char *name);
void FreeKernelList(void);
int GetKernelSize(size_t *, size_t *);
int LoadKernel(void *ptr_ro, void *ptr_dbg, struct KernelBSS *tracker);

#endif /*ELFLOADER_H_*/
