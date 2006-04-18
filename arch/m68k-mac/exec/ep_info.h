#ifndef _EP_INFO_H
#define _EP_INFO_H

struct ep_entry {
    unsigned short tag;			/* tag ID */
    unsigned short size;		/* size of entry (in bytes) */
    unsigned long data[0];			/* data */
};


#define EP_LAST			0x0000	/* last record */
#define EP_MACHTYPE		0x0001	/* machine type (ULONG) */
#define EP_CPUTYPE		0x0002	/* cpu type (ULONG) */
#define EP_FPUTYPE		0x0003	/* fpu type (ULONG) */
#define EP_MMUTYPE		0x0004	/* mmu type (ULONG) */
#define EP_MEMCHUNK		0x0005	/* memory chunk address and size */
					/* (struct mem_info) */
#define EP_RAMDISK		0x0006	/* ramdisk address and size */
					/* (struct mem_info) */
#define EP_COMMAND_LINE		0x0007	/* kernel command line parameters */
					/* (string) */

#define EP_MODEL		0x8000	/* Gestalt ID */
#define EP_VADDR		0x8001	/* video base address */
#define EP_VDEPTH		0x8002	/* video depth */
#define EP_VROW			0x8003	/* video rowbytes */
#define EP_VDIM			0x8004	/* video dimensions */
#define EP_VLOGICAL		0x8005	/* video logical base */
#define EP_SCCBASE		0x8006	/* SCC base address */
#define EP_BTIME		0x8007	/* boot time */
#define EP_GMTBIAS		0x8008	/* GMT timezone offset */
#define EP_MEMSIZE		0x8009	/* RAM size (sanity check) */
#define EP_CPUID		0x800a	/* CPU type (sanity check) */
#define EP_ROMBASE		0x800b	/* system ROM base address */

#define EP_VIA1BASE		0x8010	/* VIA1 base address (always present) */
#define EP_VIA2BASE		0x8011	/* VIA2 base address (type varies) */
#define EP_VIA2TYPE		0x8012	/* VIA2 type (VIA, RBV, OSS) */
#define EP_ADBTYPE		0x8013	/* ADB interface type */
#define EP_ASCBASE		0x8014	/* Apple Sound Chip base address */
#define EP_SCSI5380		0x8015	/* NCR 5380 SCSI (base address, multi) */
#define EP_SCSIDMA		0x8016	/* SCSI DMA (base address) */
#define EP_SCSI5396		0x8017	/* NCR 53C96 SCSI (base address, multi) */
#define EP_IDETYPE		0x8018	/* IDE interface type */
#define EP_IDEBASE		0x8019	/* IDE interface base address */
#define EP_NUBUS		0x801a	/* Nubus type (none, regular, pseudo) */
#define EP_SLOTMASK		0x801b	/* Nubus slots present */
#define EP_SCCTYPE		0x801c	/* SCC serial type (normal, IOP) */
#define EP_ETHTYPE		0x801d	/* builtin ethernet type (Sonic, MACE */
#define EP_ETHBASE		0x801e	/* builtin ethernet base address */
#define EP_PMU			0x801f	/* power management / poweroff hardware */
#define EP_IOP_SWIM		0x8020	/* SWIM floppy IOP */
#define EP_IOP_ADB		0x8021	/* ADB IOP */

#define MAX_MEM_HEADERS 10

struct mac68k_init_stuff {
	unsigned long vidaddr;
	unsigned long viddepth;
	unsigned long vidrow;
	unsigned long vidwidth;
	unsigned long vidheight;
	unsigned long model;
	unsigned long cpu;
	unsigned long mem;
	unsigned long memchunk[MAX_MEM_HEADERS];
	unsigned long memchunksize[MAX_MEM_HEADERS];
	unsigned char chrrows;
	unsigned char chrcols;
	unsigned char curcol;
	unsigned char currow;
	char fontbyte[128][10][8];
};

#endif /* _EP_INFO_H */
