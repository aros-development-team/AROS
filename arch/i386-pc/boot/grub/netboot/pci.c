/*
** Support for NE2000 PCI clones added David Monro June 1997
** Generalised to other NICs by Ken Yap July 1997
**
** Most of this is taken from:
**
** /usr/src/linux/drivers/pci/pci.c
** /usr/src/linux/include/linux/pci.h
** /usr/src/linux/arch/i386/bios32.c
** /usr/src/linux/include/linux/bios32.h
** /usr/src/linux/drivers/net/ne.c
*/

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#include "etherboot.h"
#include "pci.h"

/*#define	DEBUG	1*/
static unsigned int pci_ioaddr = 0;

#ifdef	CONFIG_PCI_DIRECT
#define  PCIBIOS_SUCCESSFUL                0x00

/*
 * Functions for accessing PCI configuration space with type 1 accesses
 */

#define CONFIG_CMD(bus, device_fn, where)   (0x80000000 | (bus << 16) | (device_fn << 8) | (where & ~3))

int pcibios_read_config_byte(unsigned char bus, unsigned char device_fn,
			       unsigned char where, unsigned char *value)
{
    outl(CONFIG_CMD(bus,device_fn,where), 0xCF8);
    *value = inb(0xCFC + (where&3));
    return PCIBIOS_SUCCESSFUL;
}

int pcibios_read_config_word (unsigned char bus,
    unsigned char device_fn, unsigned char where, unsigned short *value)
{
    outl(CONFIG_CMD(bus,device_fn,where), 0xCF8);
    *value = inw(0xCFC + (where&2));
    return PCIBIOS_SUCCESSFUL;
}

static int pcibios_read_config_dword (unsigned char bus, unsigned char device_fn,
				 unsigned char where, unsigned int *value)
{
    outl(CONFIG_CMD(bus,device_fn,where), 0xCF8);
    *value = inl(0xCFC);
    return PCIBIOS_SUCCESSFUL;
}

int pcibios_write_config_byte (unsigned char bus, unsigned char device_fn,
				 unsigned char where, unsigned char value)
{
    outl(CONFIG_CMD(bus,device_fn,where), 0xCF8);
    outb(value, 0xCFC + (where&3));
    return PCIBIOS_SUCCESSFUL;
}

int pcibios_write_config_word (unsigned char bus, unsigned char device_fn,
				 unsigned char where, unsigned short value)
{
    outl(CONFIG_CMD(bus,device_fn,where), 0xCF8);
    outw(value, 0xCFC + (where&2));
    return PCIBIOS_SUCCESSFUL;
}

int pcibios_write_config_dword (unsigned char bus, unsigned char device_fn, unsigned char where, unsigned int value)
{
    outl(CONFIG_CMD(bus,device_fn,where), 0xCF8);
    outl(value, 0xCFC);
    return PCIBIOS_SUCCESSFUL;
}

#undef CONFIG_CMD

#else	 /* CONFIG_PCI_DIRECT  not defined */

static unsigned long bios32_entry = 0;
static struct {
	unsigned long address;
	unsigned short segment;
} bios32_indirect = { 0, KERN_CODE_SEG };

static long pcibios_entry = 0;
static struct {
	unsigned long address;
	unsigned short segment;
} pci_indirect = { 0, KERN_CODE_SEG };

static unsigned long bios32_service(unsigned long service)
{
	unsigned char return_code;	/* %al */
	unsigned long address;		/* %ebx */
	unsigned long length;		/* %ecx */
	unsigned long entry;		/* %edx */
	unsigned long flags;

	save_flags(flags);
	__asm__(
#ifndef ABSOLUTE_WITHOUT_ASTERISK
		"lcall *(%%edi)"
#else
		"lcall (%%edi)"
#endif
		: "=a" (return_code),
		  "=b" (address),
		  "=c" (length),
		  "=d" (entry)
		: "0" (service),
		  "1" (0),
		  "D" (&bios32_indirect));
	restore_flags(flags);

	switch (return_code) {
		case 0:
			return address + entry;
		case 0x80:	/* Not present */
			printf("bios32_service(%d) : not present\n", service);
			return 0;
		default: /* Shouldn't happen */
			printf("bios32_service(%d) : returned 0x%x, mail drew@colorado.edu\n",
				service, return_code);
			return 0;
	}
}

int pcibios_read_config_byte(unsigned char bus,
        unsigned char device_fn, unsigned char where, unsigned char *value)
{
        unsigned long ret;
        unsigned long bx = (bus << 8) | device_fn;
        unsigned long flags;

        save_flags(flags);
        __asm__(
#ifndef ABSOLUTE_WITHOUT_ASTERISK
		"lcall *(%%esi)\n\t"
#else
		"lcall (%%esi)\n\t"
#endif
                "jc 1f\n\t"
                "xor %%ah, %%ah\n"
                "1:"
                : "=c" (*value),
                  "=a" (ret)
                : "1" (PCIBIOS_READ_CONFIG_BYTE),
                  "b" (bx),
                  "D" ((long) where),
                  "S" (&pci_indirect));
        restore_flags(flags);
        return (int) (ret & 0xff00) >> 8;
}

int pcibios_read_config_word(unsigned char bus,
        unsigned char device_fn, unsigned char where, unsigned short *value)
{
        unsigned long ret;
        unsigned long bx = (bus << 8) | device_fn;
        unsigned long flags;

        save_flags(flags);
        __asm__(
#ifndef ABSOLUTE_WITHOUT_ASTERISK
		"lcall *(%%esi)\n\t"
#else
		"lcall (%%esi)\n\t"
#endif
                "jc 1f\n\t"
                "xor %%ah, %%ah\n"
                "1:"
                : "=c" (*value),
                  "=a" (ret)
                : "1" (PCIBIOS_READ_CONFIG_WORD),
                  "b" (bx),
                  "D" ((long) where),
                  "S" (&pci_indirect));
        restore_flags(flags);
        return (int) (ret & 0xff00) >> 8;
}

static int pcibios_read_config_dword(unsigned char bus,
        unsigned char device_fn, unsigned char where, unsigned int *value)
{
        unsigned long ret;
        unsigned long bx = (bus << 8) | device_fn;
        unsigned long flags;

        save_flags(flags);
        __asm__(
#ifndef ABSOLUTE_WITHOUT_ASTERISK
		"lcall *(%%esi)\n\t"
#else
		"lcall (%%esi)\n\t"
#endif
                "jc 1f\n\t"
                "xor %%ah, %%ah\n"
                "1:"
                : "=c" (*value),
                  "=a" (ret)
                : "1" (PCIBIOS_READ_CONFIG_DWORD),
                  "b" (bx),
                  "D" ((long) where),
                  "S" (&pci_indirect));
        restore_flags(flags);
        return (int) (ret & 0xff00) >> 8;
}

int pcibios_write_config_byte (unsigned char bus,
	unsigned char device_fn, unsigned char where, unsigned char value)
{
	unsigned long ret;
	unsigned long bx = (bus << 8) | device_fn;
	unsigned long flags;

	save_flags(flags); cli();
	__asm__(
#ifndef ABSOLUTE_WITHOUT_ASTERISK
		"lcall *(%%esi)\n\t"
#else
		"lcall (%%esi)\n\t"
#endif
		"jc 1f\n\t"
		"xor %%ah, %%ah\n"
		"1:"
		: "=a" (ret)
		: "0" (PCIBIOS_WRITE_CONFIG_BYTE),
		  "c" (value),
		  "b" (bx),
		  "D" ((long) where),
		  "S" (&pci_indirect));
	restore_flags(flags);
	return (int) (ret & 0xff00) >> 8;
}

int pcibios_write_config_word (unsigned char bus,
	unsigned char device_fn, unsigned char where, unsigned short value)
{
	unsigned long ret;
	unsigned long bx = (bus << 8) | device_fn;
	unsigned long flags;

	save_flags(flags); cli();
	__asm__(
#ifndef ABSOLUTE_WITHOUT_ASTERISK
		"lcall *(%%esi)\n\t"
#else
		"lcall (%%esi)\n\t"
#endif
		"jc 1f\n\t"
		"xor %%ah, %%ah\n"
		"1:"
		: "=a" (ret)
		: "0" (PCIBIOS_WRITE_CONFIG_WORD),
		  "c" (value),
		  "b" (bx),
		  "D" ((long) where),
		  "S" (&pci_indirect));
	restore_flags(flags);
	return (int) (ret & 0xff00) >> 8;
}

int pcibios_write_config_dword (unsigned char bus,
	unsigned char device_fn, unsigned char where, unsigned int value)
{
	unsigned long ret;
	unsigned long bx = (bus << 8) | device_fn;
	unsigned long flags;

	save_flags(flags); cli();
	__asm__(
#ifndef ABSOLUTE_WITHOUT_ASTERISK
		"lcall *(%%esi)\n\t"
#else
		"lcall (%%esi)\n\t"
#endif
		"jc 1f\n\t"
		"xor %%ah, %%ah\n"
		"1:"
		: "=a" (ret)
		: "0" (PCIBIOS_WRITE_CONFIG_DWORD),
		  "c" (value),
		  "b" (bx),
		  "D" ((long) where),
		  "S" (&pci_indirect));
	restore_flags(flags);
	return (int) (ret & 0xff00) >> 8;
}

static void check_pcibios(void)
{
	unsigned long signature;
	unsigned char present_status;
	unsigned char major_revision;
	unsigned char minor_revision;
	unsigned long flags;
	int pack;

	if ((pcibios_entry = bios32_service(PCI_SERVICE))) {
		pci_indirect.address = pcibios_entry;

		save_flags(flags);
		__asm__(
#ifndef ABSOLUTE_WITHOUT_ASTERISK
			"lcall *(%%edi)\n\t"
#else
			"lcall (%%edi)\n\t"
#endif
			"jc 1f\n\t"
			"xor %%ah, %%ah\n"
			"1:\tshl $8, %%eax\n\t"
			"movw %%bx, %%ax"
			: "=d" (signature),
			  "=a" (pack)
			: "1" (PCIBIOS_PCI_BIOS_PRESENT),
			  "D" (&pci_indirect)
			: "bx", "cx");
		restore_flags(flags);

		present_status = (pack >> 16) & 0xff;
		major_revision = (pack >> 8) & 0xff;
		minor_revision = pack & 0xff;
		if (present_status || (signature != PCI_SIGNATURE)) {
			printf("ERROR: BIOS32 says PCI BIOS, but no PCI "
				"BIOS????\n");
			pcibios_entry = 0;
		}
#if	DEBUG
		if (pcibios_entry) {
			printf ("pcibios_init : PCI BIOS revision %b.%b"
				" entry at 0x%X\n", major_revision,
				minor_revision, pcibios_entry);
		}
#endif
	}
}

static void pcibios_init(void)
{
	union bios32 *check;
	unsigned char sum;
	int i, length;

	/*
	 * Follow the standard procedure for locating the BIOS32 Service
	 * directory by scanning the permissible address range from
	 * 0xe0000 through 0xfffff for a valid BIOS32 structure.
	 *
	 */

	for (check = (union bios32 *) 0xe0000; check <= (union bios32 *) 0xffff0; ++check) {
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
			printf("pcibios_init : unsupported revision %d at 0x%X, mail drew@colorado.edu\n",
				check->fields.revision, check);
			continue;
		}
#if	DEBUG
		printf("pcibios_init : BIOS32 Service Directory "
			"structure at 0x%X\n", check);
#endif
		if (!bios32_entry) {
			if (check->fields.entry >= 0x100000) {
				printf("pcibios_init: entry in high "
					"memory, giving up\n");
				return;
			} else {
				bios32_entry = check->fields.entry;
#if	DEBUG
				printf("pcibios_init : BIOS32 Service Directory"
					" entry at 0x%X\n", bios32_entry);
#endif
				bios32_indirect.address = bios32_entry;
			}
		}
	}
	if (bios32_entry)
		check_pcibios();
}
#endif	/* CONFIG_PCI_DIRECT not defined*/

static void scan_bus(struct pci_device *pcidev)
{
	unsigned int devfn, l, bus, buses;
	unsigned char hdr_type = 0;
	unsigned short vendor, device;
	unsigned int membase, ioaddr, romaddr;
	unsigned char class, subclass;
	int i, reg;

	pci_ioaddr = 0;
	buses=1;
	for (bus = 0; bus < buses; ++bus) {
		for (devfn = 0; devfn < 0xff; ++devfn) {
			if (PCI_FUNC (devfn) == 0)
				pcibios_read_config_byte(bus, devfn, PCI_HEADER_TYPE, &hdr_type);
			else if (!(hdr_type & 0x80))	/* not a multi-function device */
				continue;
			pcibios_read_config_dword(bus, devfn, PCI_VENDOR_ID, &l);
			/* some broken boards return 0 if a slot is empty: */
			if (l == 0xffffffff || l == 0x00000000) {
				hdr_type = 0;
				continue;
			}
			vendor = l & 0xffff;
			device = (l >> 16) & 0xffff;

			/* check for pci-pci bridge devices!! - more buses when found */
			pcibios_read_config_byte(bus, devfn, PCI_CLASS_CODE, &class);
			pcibios_read_config_byte(bus, devfn, PCI_SUBCLASS_CODE, &subclass);
			if (class == 0x06 && subclass == 0x04)
				buses++;

#if	DEBUG
			printf("bus %x, function %x, vendor %x, device %x\n",
				bus, devfn, vendor, device);
#endif
			for (i = 0; pcidev[i].vendor != 0; i++) {
				if (vendor != pcidev[i].vendor
				    || device != pcidev[i].dev_id)
					continue;
				for (reg = PCI_BASE_ADDRESS_0; reg <= PCI_BASE_ADDRESS_5; reg += 4) {
					pcibios_read_config_dword(bus, devfn, reg, &ioaddr);

					if ((ioaddr & PCI_BASE_ADDRESS_IO_MASK) == 0 || (ioaddr & PCI_BASE_ADDRESS_SPACE_IO) == 0)
						continue;
					/* Strip the I/O address out of the returned value */
					ioaddr &= PCI_BASE_ADDRESS_IO_MASK;
					/* Get the memory base address */
					pcibios_read_config_dword(bus, devfn,
						PCI_BASE_ADDRESS_1, &membase);
					/* Get the ROM base address */
					pcibios_read_config_dword(bus, devfn, PCI_ROM_ADDRESS, &romaddr);
					romaddr >>= 10;
					printf("Found %s at 0x%x, ROM address 0x%X\n",
						pcidev[i].name, ioaddr, romaddr);
					/* Take the first one or the one that matches in boot ROM address */
					if (pci_ioaddr == 0 || romaddr == ((unsigned long) rom.rom_segment << 4)) {
						pcidev[i].membase = membase;
						pcidev[i].ioaddr = ioaddr;
						pcidev[i].devfn = devfn;

						return;
					}
				}
			}
		}
	}
}

void eth_pci_init(struct pci_device *pcidev)
{
#ifndef	CONFIG_PCI_DIRECT
	pcibios_init();
	if (!pcibios_entry) {
		printf("pci_init: no BIOS32 detected\n");
		return;
	}
#endif
	scan_bus(pcidev);
	/* return values are in pcidev structures */
}
