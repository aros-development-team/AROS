/*****************************************************************************

			       XCopilot

This code is part of XCopilot, a port of copilot

     Portions of this code are Copyright (C) 1997 Ivan A. Curtis
		       icurtis@radlogic.com.au

The original MS-Windows95 copilot emulator was written by Greg Hewgill.
The following copyright notice appeared on the original copilot sources:

		  Copyright (c) 1996 Greg Hewgill

 MC68000 Emulation code is from Bernd Schmidt's Unix Amiga Emulator.
       The following copyright notice appeared in those files:

	  Original UAE code Copyright (c) 1995 Bernd Schmidt

This code must not be distributed without these copyright notices intact.

*******************************************************************************
*******************************************************************************

Filename:	memory.c

Description:	Copilot Memory management

Update History:   (most recent first)
   Ian Goldberg   26-Jun-98 12:40 -- generalized memory mappings
   Ian Goldberg   11-Sep-97 09:48 -- added bus error support
   Brian Grossman 14-Jul-97 15:00 -- $XCOPILOTRAM is now a filename
   Ian Goldberg   18-Apr-97 11:13 -- can now have multiple RAM files
   Ian Goldberg   10-Apr-97 16:43 -- support for Palm Pilot ROM
   I. Curtis       5-Mar-97 21:47 -- mmapped scratchmemory and pilotram
     rom_wput() routine now actually does something
   I. Curtis      26-Feb-97 14:59 -- modified from win95 version

******************************************************************************/
#include <config.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include "sysdeps.h"
#include "shared.h"
#include "memory.h"

int ram_size;
int ram_size_mask;
int rom_size;
extern shared_img *Shptr;

int sram_protect = 1;
int buserr = 0;
addrbank *membanks;

/* Default memory access functions */

int default_check(CPTR a, ULONG b)
{
    return 0;
}

UWORD *default_xlate(CPTR a)
{
/*    fprintf(stderr, "Your Pilot program just did something terribly stupid\n"); */
    return 0;
}

/* A dummy bank that only contains zeros */

static ULONG dummy_lget(CPTR);
static UWORD dummy_wget(CPTR);
static UBYTE dummy_bget(CPTR);
static void  dummy_lput(CPTR, ULONG);
static void  dummy_wput(CPTR, UWORD);
static void  dummy_bput(CPTR, UBYTE);
static int   dummy_check(CPTR addr, ULONG size);
static UWORD *dummy_xlate(CPTR addr);

ULONG dummy_lget(CPTR addr)
{
    fprintf(stderr,
	"Bus error: read a long from undefined memory address 0x%08lx\n",
	addr);
    buserr = 1;
    return 0;
}

UWORD dummy_wget(CPTR addr)
{
    if ((addr & 0xFF000000) != 0x10000000) {
	fprintf(stderr,
	    "Bus error: read a word from undefined memory address 0x%08lx\n",
	    addr);
	buserr = 1;
    }
    return 0;
}

UBYTE dummy_bget(CPTR addr)
{
    fprintf(stderr,
	"Bus error: read a byte from undefined memory address 0x%08lx\n",
	addr);
    buserr = 1;
    return 0;
}

void dummy_lput(CPTR addr, ULONG l)
{
    fprintf(stderr,
	"Bus error: wrote a long to undefined memory address 0x%08lx (@ %x)\n",
	addr,Shptr->regs.pc);
    buserr = 1;
}

void dummy_wput(CPTR addr, UWORD w)
{
    if ((addr & 0xFF000000) != 0x10000000) {
	fprintf(stderr,
	    "Bus error: wrote a word to undefined memory address 0x%08lx (@ %x)\n",
	    addr,Shptr->regs.pc);
	buserr = 1;
    }
}

void dummy_bput(CPTR addr, UBYTE b)
{
    fprintf(stderr,
	"Bus error: wrote a byte to undefined memory address 0x%08lx (@ %x)\n",
	addr,Shptr->regs.pc);
    buserr = 1;
}

int dummy_check(CPTR addr, ULONG size)
{
    return 0;
}

UWORD *dummy_xlate(CPTR addr)
{
    return NULL;
}

/* stegerg: AROS: debug output bank */

ULONG debug_lget(CPTR addr)
{
    fprintf(stderr,
	"debug_bank: unexpected long read from memory address 0x%08lx\n",
	addr);
   return 0;
}

UWORD debug_wget(CPTR addr)
{
    fprintf(stderr,
	    "debug_bank: unexpected word read from memory address 0x%08lx\n",
	    addr);
    return 0;
}

UBYTE debug_bget(CPTR addr)
{
    fprintf(stderr,
	"debug_bank: unexpected byte read from memory address 0x%08lx\n",
	addr);
    return 0;
}

void debug_lput(CPTR addr, ULONG l)
{
    fprintf(stderr,
	"debug_bank: unexpected long write to memory address 0x%08lx (@ %x)\n",
	addr,Shptr->regs.pc);
}

void debug_wput(CPTR addr, UWORD w)
{
	fprintf(stderr,
	    "debug_bank: unexpected word write to memory address 0x%08lx (@ %x)\n",
	    addr,Shptr->regs.pc);
}

void debug_bput(CPTR addr, UBYTE b)
{
    /* Debug output from AROS :-) */
    
    if (addr == 0xdddddebc)
    {
    	if (b) fprintf(stderr, "%c", b);
    }
    else
    {
	fprintf(stderr,
	    "debug_bank: unexpected byte write to memory address 0x%08lx (@ %x)\n",
	    addr,Shptr->regs.pc);
    }

}

int debug_check(CPTR addr, ULONG size)
{
    return 0;
}

UWORD *debug_xlate(CPTR addr)
{
    return NULL;
}

/* RAM */

UWORD *rammemory;

static ULONG ram_lget(CPTR);
static UWORD ram_wget(CPTR);
static UBYTE ram_bget(CPTR);
static void  ram_lput(CPTR, ULONG);
static void  ram_wput(CPTR, UWORD);
static void  ram_bput(CPTR, UBYTE);
static int   ram_check(CPTR addr, ULONG size);
static UWORD *ram_xlate(CPTR addr);

ULONG ram_lget(CPTR addr)
{
    addr -= ram_start & (ram_size_mask);
    addr &= ram_size_mask;
    return ((ULONG)rammemory[addr >> 1] << 16) | rammemory[(addr >> 1)+1];
}

UWORD ram_wget(CPTR addr)
{
    addr -= ram_start & (ram_size_mask);
    addr &= ram_size_mask;
    return rammemory[addr >> 1];
}

UBYTE ram_bget(CPTR addr)
{
    addr -= ram_start & (ram_size_mask);
    addr &= ram_size_mask;
    if (addr & 1) 
	return rammemory[addr >> 1];
    else
	return rammemory[addr >> 1] >> 8;
}

void ram_lput(CPTR addr, ULONG l)
{
    addr -= ram_start & (ram_size_mask);
    addr &= ram_size_mask;
    rammemory[addr >> 1] = l >> 16;
    rammemory[(addr >> 1)+1] = (UWORD)l;
}

void sram_lput(CPTR addr, ULONG l)
{
    if (sram_protect) {
	fprintf(stderr,
	    "Bus error: wrote a long to database RAM address 0x%08lx\n", addr);
	buserr = 1;
    } else {
	ram_lput(addr, l);
    }
}

void ram_wput(CPTR addr, UWORD w)
{
    addr -= ram_start & (ram_size_mask);
    addr &= ram_size_mask;
    rammemory[addr >> 1] = w;
}

void sram_wput(CPTR addr, UWORD w)
{
    if (sram_protect) {
	fprintf(stderr,
	    "Bus error: wrote a word to database RAM address 0x%08lx\n", addr);
	buserr = 1;
    } else {
	ram_wput(addr, w);
    }
}

void ram_bput(CPTR addr, UBYTE b)
{
    addr -= ram_start & (ram_size_mask);
    addr &= ram_size_mask;
    if (!(addr & 1)) {
	rammemory[addr>>1] = (rammemory[addr>>1] & 0xff) | (((UWORD)b) << 8);
    } else {
	rammemory[addr>>1] = (rammemory[addr>>1] & 0xff00) | b;
    }
}

void sram_bput(CPTR addr, UBYTE b)
{
    if (sram_protect) {
	fprintf(stderr,
	    "Bus error: wrote a byte to database RAM address 0x%08lx\n", addr);
	buserr = 1;
    } else {
	ram_bput(addr, b);
    }
}

int ram_check(CPTR addr, ULONG size)
{
    addr -= ram_start & (ram_size_mask);
    addr &= ram_size_mask;
    return (addr + size) <= (ULONG)ram_size;
}

UWORD *ram_xlate(CPTR addr)
{
    addr -= ram_start & (ram_size_mask);
    addr &= ram_size_mask;
    return rammemory + (addr >> 1);
}

/* ROM */

static UWORD *rommemory;

static ULONG rom_lget(CPTR);
static UWORD rom_wget(CPTR);
static UBYTE rom_bget(CPTR);
static void  rom_lput(CPTR, ULONG);
static void  rom_wput(CPTR, UWORD);
static void  rom_bput(CPTR, UBYTE);
static int  rom_check(CPTR addr, ULONG size);
static UWORD *rom_xlate(CPTR addr);

ULONG rom_lget(CPTR addr)
{
    addr -= rom_start & (rom_size-1);
    addr &= rom_size-1;
    return ((ULONG)rommemory[addr >> 1] << 16) | rommemory[(addr >> 1)+1];
}

UWORD rom_wget(CPTR addr)
{
    addr -= rom_start & (rom_size-1);
    addr &= rom_size-1;
    return rommemory[addr >> 1];
}

UBYTE rom_bget(CPTR addr)
{
    addr -= rom_start & (rom_size-1);
    addr &= rom_size-1;
    return rommemory[addr >> 1] >> (addr & 1 ? 0 : 8);
}

void rom_lput(CPTR addr, ULONG b)
{
    fprintf(stderr,
	"Bus error: wrote a long to ROM address 0x%08lx (@ %x)\n", addr,
	 Shptr->regs.pc);
    buserr = 1;
}

void rom_wput(CPTR addr, UWORD w)
{
    if (Shptr->allowromwrites) {
	addr -= rom_start & (rom_size - 1);
	addr &= rom_size - 1;
	rommemory[addr >> 1] = w;
    } else {
	fprintf(stderr,
	    "Bus error: wrote a word to ROM address 0x%08lx\n", addr);
	buserr = 1;
    }
}

void rom_bput(CPTR addr, UBYTE b)
{
    fprintf(stderr,
	"Bus error: wrote a byte to ROM address 0x%08lx\n", addr);
    buserr = 1;
}

int rom_check(CPTR addr, ULONG size)
{
    addr -= rom_start & (rom_size-1);
    addr &= rom_size-1;
    return (addr + size) <= rom_size;
}

UWORD *rom_xlate(CPTR addr)
{
    addr -= rom_start & (rom_size-1);
    addr &= rom_size-1;
    return rommemory + (addr >> 1);
}

static int
verify_entrypoint(const void *rom)
{
    const unsigned char _bootsign[] = { 0x4e, 0xfa, 0x00, 0x00, 'b', 'o', 'o', 't',
			       0x27, 0x10, 0xff };
    const unsigned char _bootmask[] = { 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
			       0xff, 0xff, 0x00 };

    const unsigned char *bootsign = _bootsign, *bootmask = _bootmask;
    
    while ((*bootsign & *bootmask) == *bootsign)
	if ((*((char *)rom)++ & *bootmask++) != *bootsign++)
	    return 0;

    return 1;
}

static const char *
find_entrypoint(const char *rom)
{
    const char *entry = rom;

    while (entry - rom < rom_size)
    {
	if (verify_entrypoint(entry))
	    return entry;
	else
	    entry += 2;  /* Instructions must be word aligned */
    }

    return NULL;
}

/* This routine replaces the win_load_rom routine */
/* It was grabbed from copilot-linux sources */
static int load_rom(const char *dir, const char *romfile, int nocheck)
{
  int i;
  char *rombuf;
  char *resetv;
  int f;
  struct stat st;
  
  if (romfile[0] != '/' && dir) {
    rombuf = alloca(strlen(dir)+strlen(romfile)+2);
    if (!rombuf) {
printf("%d\n",__LINE__);
      return PILOTCPU_ERROR_LOADING_ROM;
    }
    sprintf(rombuf, "%s/%s", dir, romfile);
    romfile = rombuf;
  }
  f = open(romfile, O_RDONLY);
  if (f == -1) {
printf("%d (%s)\n",__LINE__,romfile);
    return PILOTCPU_ROM_NOT_FOUND;
  }
  if (fstat(f, &st)) {
    close(f);
printf("%d\n",__LINE__);
    return PILOTCPU_ROM_NOT_FOUND;
  }
  rom_size = st.st_size;
  /* Round up to the next power of two */
  if (rom_size & (rom_size-1)) {
    /* It is not already a power of two */
    int pow_of_2;
    for(pow_of_2 = 1; pow_of_2 < rom_size; pow_of_2 <<= 1) ;
    rom_size = pow_of_2;
  }
  
  rommemory = (UWORD*)mmap(0, rom_size, PROT_READ|PROT_WRITE, 
			   MAP_FILE|MAP_PRIVATE, f, 0);
  if (rommemory == (UWORD *)-1) {
printf("%d\n",__LINE__);
    return PILOTCPU_ERROR_LOADING_ROM;
  }
  
  if (!nocheck) {
  /* Check if the reset vector looks plausible */
printf("%d\n",__LINE__);
    resetv = (char *)rommemory +
    (ntohl(*(CPTR *)(((char *)rommemory)+4)) - rom_start);
printf("resetv: %x, rom_start: %x, rommemory: %x\n",
       resetv,rom_start,rommemory);
  
    if (!verify_entrypoint(resetv)) {
      int offset;
      int pageoffset;
      void *newrom;
printf("%d\n",__LINE__);

      /* It didn't - we need to find it */
      if (1 != ntohs(*(UWORD *)(((char *)rommemory)+0x0c))) {
	offset = ntohl(*(CPTR *)(((char *)rommemory)+0x68)) +
	  sram_start - rom_start;
      }
      else {
printf("%d\n",__LINE__);
	offset = resetv - find_entrypoint((char *)rommemory);
      }

      /* Did we find it? If not, lets go with the original. */
      if ((char *)offset != resetv) {
	/* It may not always be page aligned... */
	pageoffset = ((offset-1) & ~(getpagesize() - 1)) + getpagesize();

	if ((st.st_size + offset) > rom_size)
          rom_size <<= 1;

	rommemory = (UWORD *)mmap((void*)(rommemory + pageoffset),
                                  rom_size - pageoffset,
				  PROT_READ | PROT_WRITE,
				  MAP_FILE | MAP_PRIVATE | MAP_FIXED, f, 0);
	if (rommemory == (UWORD *)-1) {
printf("%d\n",__LINE__);
	  return PILOTCPU_ERROR_LOADING_ROM;
        }

	memcpy(((char *)rommemory) - offset, rommemory, 256);
	((char *)rommemory) -= offset;
      }
    }
printf("%d\n",__LINE__);
  }

#ifndef WORDS_BIGENDIAN
  for (i = 0; i < rom_size/2; i++) {
    UWORD *p;
    UBYTE *bp;
    p = rommemory + i;
    bp = (UBYTE *)p;
    *p = (*bp << 8) | *(bp + 1);
  }
#endif

  close(f);
  return 0;
}

/* Scratch area */

UBYTE *scratchmemory = NULL;

static ULONG scratch_lget(CPTR);
static UWORD scratch_wget(CPTR);
static UBYTE scratch_bget(CPTR);
static void  scratch_lput(CPTR, ULONG);
static void  scratch_wput(CPTR, UWORD);
static void  scratch_bput(CPTR, UBYTE);
static int   scratch_check(CPTR addr, ULONG size);
static UWORD *scratch_xlate(CPTR addr);

ULONG scratch_lget(CPTR addr)
{
    if (scratchmemory == NULL) return dummy_lget(addr);
    addr -= scratch_start;
    return ((ULONG)scratchmemory[addr] << 24)
         | ((ULONG)scratchmemory[addr+1] << 16)
         | ((ULONG)scratchmemory[addr+2] << 8)
         | scratchmemory[addr+3];
}

UWORD scratch_wget(CPTR addr)
{
    if (scratchmemory == NULL) return dummy_wget(addr);
    addr -= scratch_start;
    return ((UWORD)scratchmemory[addr] << 8) | scratchmemory[addr+1];
}

UBYTE scratch_bget(CPTR addr)
{
    if (scratchmemory == NULL) return dummy_lget(addr);
    addr -= scratch_start;
    return scratchmemory[addr];
}

void scratch_lput(CPTR addr, ULONG l)
{
    if (scratchmemory == NULL) return;
    addr -= scratch_start;
    scratchmemory[addr] = l >> 24;
    scratchmemory[addr+1] = l >> 16;
    scratchmemory[addr+2] = l >> 8;
    scratchmemory[addr+3] = l;
}

void scratch_wput(CPTR addr, UWORD w)
{
    if (scratchmemory == NULL) return;
    addr -= scratch_start;
    scratchmemory[addr] = w >> 8;
    scratchmemory[addr+1] = w;
}

void scratch_bput(CPTR addr, UBYTE b)
{
    if (scratchmemory == NULL) return;
    addr -= scratch_start;
    scratchmemory[addr] = b;
}

int scratch_check(CPTR addr, ULONG size)
{
    if (scratchmemory == NULL) return 0;
    return 1;
}

UWORD *scratch_xlate(CPTR addr)
{
    return NULL; /* anything that calls this will assume swapped byte order! */
}

/* Address banks */

addrbank dummy_bank = {
    dummy_lget, dummy_wget, dummy_bget,
    dummy_lput, dummy_wput, dummy_bput,
    dummy_xlate, dummy_check
};

addrbank ram_bank = {
    ram_lget, ram_wget, ram_bget,
    ram_lput, ram_wput, ram_bput,
    ram_xlate, ram_check
};

addrbank sram_bank = {
    ram_lget, ram_wget, ram_bget,
    sram_lput, sram_wput, sram_bput,
    ram_xlate, ram_check
};

addrbank rom_bank = {
    rom_lget, rom_wget, rom_bget,
    rom_lput, rom_wput, rom_bput,
    rom_xlate, rom_check
};

addrbank scratch_bank = {
    scratch_lget, scratch_wget, scratch_bget,
    scratch_lput, scratch_wput, scratch_bput,
    scratch_xlate, scratch_check
};

addrbank debug_bank = {
    debug_lget, debug_wget, debug_bget,
    debug_lput, debug_wput, debug_bput,
    debug_xlate, debug_check
};

void map_banks(addrbank bank, int start, int size)
{
    int bnr;
    for (bnr = start; bnr < start+size; bnr++) 
       membanks[bnr] = bank;
}

#ifdef NEED_FTRUNCATE

/* -*-Mode: C;-*-
 *
 * MODULE:      ftruncate.c
 *
 * AUTHORS:     HO      Harry Ohlsen            harryo@ise.com.au
 *              RF      Rick Flower             rick.flower@trw.com
 *
 * PURPOSE:     Supplementary library function for ftruncate();
 *
 * USAGE:
 *
 *      This routine is used to augment systems which don't have access
 *      to the ftruncate() system library function (such as SunOS).
 *      This routine tries to mimic as closely as possible the regular
 *      behavior that the standard library function performs, including
 *      the same return error codes where applicable.
 *
 *      This routine will return zero on success and non-zero (-1) on
 *      failure.  If it fails, it will also set errno appropriately.
 *
 * HISTORY:
 *
 *      This routine was snitched from some code originally written by
 *      Harry Ohlsen (see Authors above) and adapted by Rick Flower
 *      for this specific purpose.
 */
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int ftruncate(int fd, off_t createSize)
{
   int bytesToGo = createSize;
   char        *crap;

    /* We will get problems when we try to write stuff
     * to the memory segment (it won't end up on disk,
     * since mmap()-ed things don't extend).
     */
   crap  = (char *)malloc(createSize);
   errno = 0;                           /* Reset errno value                 */

   (void)memset(crap, 0x00, createSize); 
   while (bytesToGo > 0)
   {
      int bytesToWrite;

      if (bytesToGo < createSize)
         bytesToWrite = bytesToGo;
      else
         bytesToWrite = createSize;

      if (write(fd, crap, bytesToWrite) != bytesToWrite)
      {
         free(crap);                    /* Free buffer space used up..       */
         errno = EIO;                   /* Set error to indicate I/O error   */
         return(-1);                    /* Return bad status to caller       */
      }
      bytesToGo -= bytesToWrite;
   }
   free(crap);                          /* Free buffer space used up..       */
   return(0);                           /* Return valid status to caller     */
}
#endif

/*
 * This routine replaces the original memory_init
 * It was taken from copilot-linux sources with minor changes
 */
int memory_init(int ramsize, const char *dir, const char *romfile,
                const char *ramfile, const char *scratchfile, int reset,
		int check_entrypoint)
{
  int i;
  int f;
  int res;
  char *rambuf, *scratchbuf;

  buserr = 0;
  if (ramsize & (ramsize-1)) {
    return PILOTCPU_ERROR_LOADING_RAM;
  }
  ram_size = ramsize * 1024;
  ram_size_mask = ram_size - 1;
  if (ramfile[0] != '/' && dir) {
    rambuf = alloca(strlen(dir)+strlen(ramfile)+2);
    if (!rambuf) {
	return PILOTCPU_ERROR_LOADING_RAM;
    }
    sprintf(rambuf, "%s/%s", dir, ramfile);
    ramfile = rambuf;
  }
  if (reset)
    f = open(ramfile, O_RDWR|O_CREAT|O_TRUNC, 0666);
  else
    f = open(ramfile, O_RDWR|O_CREAT, 0666);
  if (f == -1) {
    return PILOTCPU_ERROR_LOADING_RAM;
  }
  ftruncate(f, ram_size);
  rammemory = (UWORD*)mmap(0, ram_size, PROT_READ|PROT_WRITE,
			   MAP_FILE|MAP_SHARED, f, 0);
  if (rammemory == (UWORD*)-1) {
    return PILOTCPU_ERROR_LOADING_RAM;
  }
  close(f);

  if (scratchfile[0] != '/' && dir) {
    scratchbuf = alloca(strlen(dir)+strlen(scratchfile)+2);
    if (!scratchbuf) {
	return PILOTCPU_ERROR_LOADING_RAM;
    }
    sprintf(scratchbuf, "%s/%s", dir, scratchfile);
    scratchfile = scratchbuf;
  }
  f = open(scratchfile, O_RDWR|O_CREAT|O_TRUNC,0666);
  if (f == -1) {
    return PILOTCPU_ERROR_LOADING_RAM;
  }
  ftruncate(f, SCRATCH_SIZE);
  scratchmemory = (unsigned char*)mmap(0, SCRATCH_SIZE, PROT_READ|PROT_WRITE,
				       MAP_FILE|MAP_SHARED, f, 0);
  close(f);
	
/*  f = open("cpustate", O_RDWR|O_CREAT|O_TRUNC,0666); */
/*  if (f == -1) { */
/*    return PILOTCPU_ERROR_LOADING_RAM; */
/*  } */
/*  ftruncate(f, state_size); */
    
/*  statememory = (char*)mmap(0, state_size, PROT_READ|PROT_WRITE, */
/*			    MAP_FILE|MAP_SHARED, f, 0); */
/*  close(f); */
    
/*  sharedstate = (struct state*)statememory; */
    
/*  CpuState = (int*)(sharedstate+1); */
/*  ExceptionFlags = (int*)(CpuState+1); */

  membanks = (addrbank *)malloc(65536 * sizeof(addrbank));
  if (membanks == NULL) {
    perror("memory_init");
    exit(1);
  }

  for(i = 0; i < 65536; i++) {
    membanks[i] = dummy_bank;
  }

#define NUM_BANKS(s) ((s)>>16 + (((s)&0xFFFF) ? 1 : 0))
  map_banks(ram_bank, ram_start>>16, 16);
  map_banks(sram_bank, sram_start>>16, NUM_BANKS(ram_size));
  map_banks(scratch_bank, scratch_start>>16, NUM_BANKS(SCRATCH_SIZE));
  map_banks(custom_bank, custom_start>>16, 1);
  res = load_rom(dir, romfile, check_entrypoint);
  map_banks(rom_bank, rom_start>>16, NUM_BANKS(rom_size));

  map_banks(debug_bank, 0xDDDD, 1);
  
  return res;
}

void mem_setscratchaddr(UBYTE *addr)
{
  scratchmemory = addr;
}

unsigned int bankindex(CPTR a)
{
    return a>>16;
}

ULONG longget(CPTR addr)
{
    return membanks[bankindex(addr)].lget(addr);
}

UWORD wordget(CPTR addr)
{
    return membanks[bankindex(addr)].wget(addr);
}
UBYTE byteget(CPTR addr) 
{
    return membanks[bankindex(addr)].bget(addr);
}
void longput(CPTR addr, ULONG l)
{
    membanks[bankindex(addr)].lput(addr, l);
}
void wordput(CPTR addr, UWORD w)
{
    membanks[bankindex(addr)].wput(addr, w);
}
void byteput(CPTR addr, UBYTE b)
{
    membanks[bankindex(addr)].bput(addr, b);
}
int check_addr(CPTR a)
{
#ifdef NO_EXCEPTION_3
    return 1;
#else
    return (a & 1) == 0;
#endif
}

ULONG get_long(CPTR addr) 
{
    if (check_addr(addr))
	return longget(addr);
    fprintf(stderr,
	"Bus error: read a long from odd address 0x%08lx (@ %x)\n", addr,
	 Shptr->regs.pc);
    buserr = 1;
    return 0;
}

UWORD get_word(CPTR addr) 
{
    if (check_addr(addr))
	return wordget(addr);
    fprintf(stderr,
	"Bus error: read a word from odd address 0x%08lx (@ %x)\n", addr,Shptr->regs.pc);
    buserr = 1;
    return 0;
}

UBYTE get_byte(CPTR addr) 
{
    return byteget(addr); 
}

void put_long(CPTR addr, ULONG l) 
{
    if (!check_addr(addr)) {
	fprintf(stderr,
	    "Bus error: wrote a long to odd address 0x%08lx (@ %x)\n", addr,
	     Shptr->regs.pc);
	buserr = 1;
    }
    longput(addr, l);
}

void put_word(CPTR addr, UWORD w) 
{
    if (!check_addr(addr)) {
	fprintf(stderr,
	    "Bus error: wrote a word to odd address 0x%08lx (@ %x)\n", addr,Shptr->regs.pc);
	buserr = 1;
    }
    wordput(addr, w);
}

void put_byte(CPTR addr, UBYTE b) 
{
    byteput(addr, b);
}

UWORD *get_real_address(CPTR addr)
{
    if (!check_addr(addr)) {
	fprintf(stderr,
	    "Bus error: attempted translation of odd address 0x%08lx (pc = %x, a7 = %x)\n", addr,Shptr->regs.pc,Shptr->regs.a[7]);
	buserr = 1;
    }
    return membanks[bankindex(addr)].xlateaddr(addr);
}

int valid_address(CPTR addr, ULONG size)
{
    if (!check_addr(addr)) {
	fprintf(stderr,
	    "Bus error: attempted validation of odd address 0x%08lx (@ %x)\n", addr,Shptr->regs.pc);
	buserr = 1;
    }
    return membanks[bankindex(addr)].check(addr, size);
}
