/*
 * This header is preliminary. Contents of described structure is subject to change.
 * Pavel Fedin <sonic_amiga@rambler.ru>
 */

#ifndef AROS_HOSTBOOT_H
#define AROS_HOSTBOOT_H

struct HostBootInfo
{
	void *MemBase;		 /* Base address of working memory                */
	size_t MemSize;		 /* Size of working memory                        */
	void *KernelBss;	 /* BSS area			                  */
	void *KernelLowest;	 /* Lowest address of the kernel                  */
	void *KernelHighest;	 /* Highest address of the kernel                 */
	void *HostLib_Interface; /* Interface to OS-side part of hostlib.resource */
};

#endif
