#ifndef _PCI_HW_H
#define _PCI_HW_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VGA hardwired.
    Lang: English.
*/

#define inl(port) \
    ({	long __value;	\
	__asm__ __volatile__ ("inl %%dx,%%eax":"=a"(__value):"d"(port));	\
	__value;	})

#define outl(port,val) \
    ({	long __value=(val);	\
	__asm__ __volatile__ ("outl %%eax,%%dx"::"a"(__value),"d"(port)); })

#define PCI_AddressPort	0x0cf8
#define PCI_DataPort	0x0cfc

#endif /* _PCI_HW_H */
