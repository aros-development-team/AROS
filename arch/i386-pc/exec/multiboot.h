#ifndef _MB_H
#define _MB_H

/*
    Copyright C 2002 AROS - The Amiga Research OS
    $Id$
 
    Desc: Multiboot information structures
    Lang: english
*/

#include <exec/types.h>


/* Structure passed from bootloader */
struct multiboot {
    ULONG   flags;
#define MB_FLAGS_MEM	    1
#define MB_FLAGS_BOOTDEV    2
#define MB_FLAGS_CMDLINE    4
#define MB_FLAGS_MODS	    8
#define MB_FLAGS_AOUT	    16
#define MB_FLAGS_ELF	    32
#define MB_FLAGS_MMAP	    64
#define MB_FLAGS_DRIVES	    128
#define MB_FLAGS_CFGTBL	    256
#define MB_FLAGS_LDRNAME    512
#define MB_FLAGS_APMTBL	    1024
#define MB_FLAGS_GFX	    2048
    ULONG   mem_lower;
    ULONG   mem_upper;
    ULONG   bootdev;
    STRPTR  cmdline;
    ULONG   mods_count;
    ULONG   mods_addr;
    ULONG   elf_num;
    ULONG   elf_size;
    ULONG   elf_addr;
    ULONG   elf_shndx;
    ULONG   mmap_length;
    ULONG   mmap_addr;
    ULONG   drives_length;
    ULONG   drives_addr;
    ULONG   config_table;
    STRPTR  loader_name;
    ULONG   apm_table;
    ULONG   vbe_control_info;
    ULONG   vbe_mode_info;
    UWORD   vbe_mode;
    UWORD   vbe_if_seg;
    UWORD   vbe_if_off;
    UWORD   vbe_if_len;
};

struct mb_mmap {
    ULONG   size;
    ULONG   addr_low;
    ULONG   addr_high;
    ULONG   len_low;
    ULONG   len_high;
    ULONG   type;
#define MMAP_TYPE_RAM	    1
#define MMAP_TYPE_RESERVED  2
#define MMAP_TYPE_ACPIDATA  3
#define MMAP_TYPE_ACPINVS   4
};

/* Structure in RAM at 0x1000 */
struct arosmb {
    ULONG   magic;		    /* Indicates if information is valid */
#define MBRAM_VALID	0x1337BABE
    ULONG   flags;		    /* Copy of the multiboot flags */
    ULONG   mem_lower;		    /* Amount of lowmem (Sub 1Mb) */
    ULONG   mem_upper;		    /* Amount of upper memory */
    ULONG   mmap_addr;		    /* Pointer to memory map */
    ULONG   mmap_len;		    /* size of memory map */
    ULONG   drives_addr;	    /* Pointer to drive information */
    ULONG   drives_len;		    /* Size of drive information */
    char    ldrname[30];	    /* String of loadername */
    char    cmdline[200];	    /* Commandline */
};

struct mb_drive {
    ULONG   size;
    UBYTE   number;
    UBYTE   mode;
#define MB_MODE_CHS 0
#define MB_MODE_LBA 1
    UWORD   cyls;
    UBYTE   heads;
    UBYTE   secs;
    UWORD   ports[10];		    /* Ugly, needs to be fixed */
};

#endif /* _MB_H */
