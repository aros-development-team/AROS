/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI BIOS stuff for standalone i386 AROS
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>

#include "pci.h"
#include "pcibios.h"

#undef	SDEBUG
#undef	DEBUG
#define	DEBUG 1
#include <aros/debug.h>

union bios32 {
	struct {
		unsigned long signature;	/* _32_ */
		unsigned long entry;		/* 32 bit physical address */
		unsigned char revision;		/* Revision level, 0 */
		unsigned char length;		/* Length in paragraphs should be 01 */
		unsigned char checksum;		/* All bytes must add up to zero */
		unsigned char reserved[5]; 	/* Must be zero */
	} fields;
	char chars[16];
};

static struct {
	unsigned long address;
	unsigned short segment;
} BIOSIndAdd = { 0, KERNEL_CS };

static struct {
	unsigned long address;
	unsigned short segment;
} PCIIndAdd = { 0, KERNEL_CS };

/*
 * Returns the entry point for the given service, NULL on error
 */

static unsigned long PCIFindService(unsigned long service)
{
	unsigned char return_code;	/* %al */
	unsigned long address;		/* %ebx */
	unsigned long length;		/* %ecx */
	unsigned long entry;		/* %edx */

	__asm__("lcall (%%edi)"
		: "=a" (return_code),
		  "=b" (address),
		  "=c" (length),
		  "=d" (entry)
		: "0" (service),
		  "1" (0),
		  "D" (&BIOSIndAdd));

	switch (return_code) {
		case 0:
			return address + entry;
		case 0x80:	/* Not present */
			D(bug("PCI: BIOS service %08x not present\n",service));
 			return 0;
		default: /* Shouldn't happen */
			D(bug("PCI: Your PCI BIOS is acting strange.\n"));
			return 0;
	}
}

static int PCICheckBios(void)
{
	unsigned long signature, eax, ebx, ecx;
	unsigned char status, major_ver, minor_ver, hw_mech, last_bus;
	unsigned long pcibios_entry;

	if ((pcibios_entry = PCIFindService(PCI_SERVICE)))
	{
		PCIIndAdd.address = pcibios_entry;

		__asm__(
			"lcall (%%edi)\n\t"
			"jc 1f\n\t"
			"xor %%ah, %%ah\n"
			"1:"
			: "=d" (signature),
			  "=a" (eax),
			  "=b" (ebx),
			  "=c" (ecx)
			: "1" (PCIBIOS_PCI_BIOS_PRESENT),
			  "D" (&PCIIndAdd)
			: "memory");

		status = (eax >> 8) & 0xff;
		hw_mech = eax & 0xff;
		major_ver = (ebx >> 8) & 0xff;
		minor_ver = ebx & 0xff;
		last_bus = ecx & 0xff;
		if (status || signature != PCI_SIGNATURE)
		{ 
			D(bug("PCI: BIOS BUG #%x[%08x] found\n",status, signature));
			return 0;
 		}
		D(bug("PCI: PCI BIOS revision %x.%02x entry at 0x%lx\n", major_ver, minor_ver, pcibios_entry));
		return 1;
	}
	return 0;
}

int PCIFindBios(void)
{
	union bios32 *check;
	unsigned char sum;
	int i, length;

	/*
	 * Follow the standard procedure for locating the BIOS32 Service
	 * directory by scanning the permissible address range from
	 * 0xe0000 through 0xfffff for a valid BIOS32 structure.
	 */

	for (check = (union bios32 *)0xe0000;
	     check <= (union bios32 *)0xffff0;
	     ++check) {
		if (check->fields.signature != BIOS32_SIGNATURE)
			continue;
		length = check->fields.length * 16;
		if (!length)
			continue;
		sum = 0;
		for (i = 0; i < length ; ++i)
			sum += check->chars[i];
		if (sum != 0)
			continue;
		if (check->fields.revision != 0) {
			D(bug("PCI: unsupported BIOS32 found\n"));
			continue;
		}
		D(bug("PCI: BIOS32 Service Directory structure at 0x%p\n", check));
		if (check->fields.entry >= 0x100000) {
			D(bug("PCI: BIOS32 entry (0x%p) in high memory, cannot use.\n", check));
			return NULL;
		} else {
			unsigned long bios32_entry = check->fields.entry;
			D(bug("PCI: BIOS32 Service Directory entry at 0x%lx\n", bios32_entry));
			BIOSIndAdd.address = bios32_entry;
			if (PCICheckBios())
				return 1;
		}
		break;
	}

	return 0;
}

static int PCIReadConfigByte(unsigned char bus,	unsigned char device_fn, unsigned char where, unsigned char *value)
{
	unsigned long ret;
	unsigned long bx = (bus << 8) | device_fn;

	__asm__("lcall (%%esi)\n\t"
		"jc 1f\n\t"
		"xor %%ah, %%ah\n"
		"1:"
		: "=c" (*value),
		  "=a" (ret)
		: "1" (PCIBIOS_READ_CONFIG_BYTE),
		  "b" (bx),
		  "D" ((long) where),
		  "S" (&PCIIndAdd));
	return (int) (ret & 0xff00) >> 8;
}

static int PCIReadConfigWord(unsigned char bus, unsigned char device_fn, unsigned char where, unsigned short *value)
{
	unsigned long ret;
	unsigned long bx = (bus << 8) | device_fn;

	__asm__("lcall (%%esi)\n\t"
		"jc 1f\n\t"
		"xor %%ah, %%ah\n"
		"1:"
		: "=c" (*value),
		  "=a" (ret)
		: "1" (PCIBIOS_READ_CONFIG_WORD),
		  "b" (bx),
		  "D" ((long) where),
		  "S" (&PCIIndAdd));
	return (int) (ret & 0xff00) >> 8;
}

static int PCIReadConfigDWord(unsigned char bus, unsigned char device_fn, unsigned char where, unsigned int *value)
{
	unsigned long ret;
	unsigned long bx = (bus << 8) | device_fn;

	__asm__("lcall (%%esi)\n\t"
		"jc 1f\n\t"
		"xor %%ah, %%ah\n"
		"1:"
		: "=c" (*value),
		  "=a" (ret)
		: "1" (PCIBIOS_READ_CONFIG_DWORD),
		  "b" (bx),
		  "D" ((long) where),
		  "S" (&PCIIndAdd));
	return (int) (ret & 0xff00) >> 8;
}

static int PCIWriteConfigByte(unsigned char bus, unsigned char device_fn, unsigned char where, unsigned char value)
{
	unsigned long ret;
	unsigned long bx = (bus << 8) | device_fn;

	__asm__("lcall (%%esi)\n\t"
		"jc 1f\n\t"
		"xor %%ah, %%ah\n"
		"1:"
		: "=a" (ret)
		: "0" (PCIBIOS_WRITE_CONFIG_BYTE),
		  "c" (value),
		  "b" (bx),
		  "D" ((long) where),
		  "S" (&PCIIndAdd));
	return (int) (ret & 0xff00) >> 8;
}

static int PCIWriteConfigWord(unsigned char bus, unsigned char device_fn, unsigned char where, unsigned short value)
{
	unsigned long ret;
	unsigned long bx = (bus << 8) | device_fn;

	__asm__("lcall (%%esi)\n\t"
		"jc 1f\n\t"
		"xor %%ah, %%ah\n"
		"1:"
		: "=a" (ret)
		: "0" (PCIBIOS_WRITE_CONFIG_WORD),
		  "c" (value),
		  "b" (bx),
		  "D" ((long) where),
		  "S" (&PCIIndAdd));
	return (int) (ret & 0xff00) >> 8;
}

static int PCIWriteConfigDWord(unsigned char bus, unsigned char device_fn, unsigned char where, unsigned int value)
{
	unsigned long ret;
	unsigned long bx = (bus << 8) | device_fn;

	__asm__("lcall (%%esi)\n\t"
		"jc 1f\n\t"
		"xor %%ah, %%ah\n"
		"1:"
		: "=a" (ret)
		: "0" (PCIBIOS_WRITE_CONFIG_DWORD),
		  "c" (value),
		  "b" (bx),
		  "D" ((long) where),
		  "S" (&PCIIndAdd));
	return (int) (ret & 0xff00) >> 8;
}

