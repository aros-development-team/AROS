/****************************************************************************
 * ppc_boot.c -- This program initializes the PPC on a Phase5 PowerUp board
 *               and boots the kernel.
 *
 * Copyright 1997, 1998 by Jesper Skov (jskov@cygnus.co.uk)
 ***************************************************************************/

#define SYMBOL_NAME_STR(X) #X

#define INTENA 0xdff09a
#define INTREQ 0xdff09c
#define DMACON 0xdff096

#define CHIP_PTR (0xfff00)

#define REG_SHADOW (0xf60018)
#define REGSHADOW_SETRESET (0x80)
#define REGSHADOW_SHADOW   (0x01)

#define REG_INTREG (0xf60028)
#define REGINTREG_SETRESET  (0x80)
#define REGINTREG_INTMASTER (0x01)

#define REG_IPLEMU	(0xf60030)
#define REGIPLEMU_SETRESET		(0x80)
#define REGIPLEMU_DISABLEINT	(0x40)


#define REG_INTLVL (0xf60038)
#define REGINTLVL_SETRESET (0x80)
#define REGINTLVL_LVL7     (0x40)




#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <powerup/ppclib/interface.h>
#include <powerup/gcclib/powerup_protos.h>
#include <powerup/ppclib/tasks.h>

void goSupervisor(void);
void relocate(void);
extern char relocateEnd;

void progress_init (void);
void progress (char);

unsigned int main(void)
{
	int i;
	unsigned long *s, *d;
	unsigned long ptr;

	register unsigned long r0 __asm("r0");
	register unsigned long r3 __asm("r3");

	progress_init ();

	/* Wait for signal from m68k. */
	progress ('W');
	{
		void			*PPCPort;

		if (PPCPort=(void*) PPCGetTaskAttr(PPCTASKTAG_MSGPORT))
		{
			progress ('w'); /* Wait */
			PPCWaitPort(PPCPort);
			progress ('r'); /* Received */
		} else {
			progress ('X');
			for (;;);
		}
	}
	progress ('!');

	/* Start Shutdown. */
	progress ('S');

	progress ('d');	/* Disable IRQs. */
	*(unsigned short*) INTENA = 0x7fff;
	__asm __volatile ("eieio");
	progress ('d');	/* (Stall for a bit.) */
	*(unsigned short*) INTREQ = 0x7fff;
	__asm __volatile ("eieio");
	progress ('d');	/* Disable DMA. */
	*(unsigned short*) DMACON = 0x03ff;

	
	progress ('s');	/* Get the PR bit set. */
	goSupervisor();
	
	progress ('e');	/* Disable exception handling. */
	__asm __volatile (
		"mfmsr 4            \n\t"
		"lis   3,0          \n\t"
		"ori   3,3,0x8000   \n\t" /* MSR_EE */
		"andc  4,4,3        \n\t"
		"isync              \n\t"
		"mtmsr 4            \n\t"
		"sync               \n\t"
		::: "r3", "r4");

	progress ('i');	/* Disable interrupt logic. */
	*(unsigned char*) REG_IPLEMU = REGIPLEMU_SETRESET | REGIPLEMU_DISABLEINT;
	__asm __volatile ("eieio;sync");
	
		
	progress ('i');	/* Change interrupt master. */
	*(unsigned char*) REG_INTREG = REGINTREG_INTMASTER;
	__asm __volatile ("eieio;sync;");

	progress ('w');	/* Give the m68k a wedgie. */
	*(unsigned char*) REG_INTLVL = REGINTLVL_LVL7;
	__asm __volatile ("eieio;sync");
	progress ('?');
	
	/* Stall a bit to allow the m68k to disable its MMU. */
	{
		int i;
		for (i = 0; i < 30000; i++);
	}
	progress ('!');	/* Done! PPC owns the Amiga. */

    progress ('1');
	PPCCacheFlushAll();
	__asm __volatile (
		/* Disable caches. */
		"mfspr 4,1008       \n\t" /* HID0 */
		"lis   3,0          \n\t"
		"ori   3,3,0xc000   \n\t" /* HID0_ICE|HID0_DCE */
		"andc  4,4,3        \n\t"
		"sync               \n\t"
		"isync              \n\t"
		"mtspr 1008,4       \n\t" /* HID0 */
		/* Invalidate caches. */
		"mfspr 4,1008       \n\t" /* HID0 */
		"lis   3,4          \n\t"
		"ori   3,3,0x0c00   \n\t" /* HID0_ICFI|HID0_DCFI */
		"or    3,3,4        \n\t"
		"isync              \n\t"
		"sync               \n\t"
		"mtspr 1008,3       \n\t" /* HID0 */
		"mtspr 1008,4       \n\t" /* HID0 */
		"isync              \n\t"
		/* Disable address translation and interrupts. */
		"mfmsr 4            \n\t"
		"lis   3,0          \n\t"
		"ori   3,3,0x0030   \n\t" /* MSR_IR|MSR_DR */
		"andc  4,4,3        \n\t"
		"sync               \n\t"
		"isync              \n\t"
		"mtmsr 4            \n\t"
		"isync              \n\t"
		/* Put BATs in a safe state. (Enable bits are in the Upper word) */
		"li    3,0          \n\t"
		"mtspr 528,3        \n\t" /* IBAT0U */
		"mtspr 530,3        \n\t" /* IBAT1U */
		"mtspr 532,3        \n\t" /* IBAT2U */
		"mtspr 534,3        \n\t" /* IBAT3U */
		"mtspr 536,3        \n\t" /* DBAT0U */
		"mtspr 538,3        \n\t" /* DBAT1U */
		"mtspr 540,3        \n\t" /* DBAT2U */
		"mtspr 542,3        \n\t" /* DBAT3U */
		/* Clear segment registers. */
		"mtsr  0,3          \n\t"
		"mtsr  1,3          \n\t"
		"mtsr  2,3          \n\t"
		"mtsr  3,3          \n\t"
		"mtsr  4,3          \n\t"
		"mtsr  5,3          \n\t"
		"mtsr  6,3          \n\t"
		"mtsr  7,3          \n\t"
		"mtsr  8,3          \n\t"
		"mtsr  9,3          \n\t"
		"mtsr  10,3         \n\t"
		"mtsr  11,3         \n\t"
		"mtsr  12,3         \n\t"
		"mtsr  13,3         \n\t"
		"mtsr  14,3         \n\t"
		"mtsr  15,3         \n\t"
		/* Clear SDR1 & ASR. */
		"mtspr 25,3         \n\t" /* SDR1 */
		"mtspr 280,3        \n\t" /* ASR */
		"isync              \n\t"
		::: "r3", "r4");

    progress ('2');

#if 0 
    { 
		/* I don't know if this works so it is disabled until someone with a
		   blizzard verifies the effect. mm/init.c will need to be adjusted
		   if this code is activated (init makes presumptions about
		   shadowing based on memory size). */ 
		unsigned char* reg_shadow =	(unsigned char*) REG_SHADOW; 
		*reg_shadow = (REGSHADOW_SETRESET |	REGSHADOW_SHADOW);
	} 
#endif 

    progress ('3');

	{
		const unsigned long *info;
		/* Get info-ptr from CHIP_PTR. */
		info = (const unsigned long*) *((unsigned long*) CHIP_PTR);

		/* Relocate code to memory just above info. */
		ptr = (unsigned long)info + 0x0200;
		ptr = ptr + 4 - (ptr % 4);

		s = (unsigned long*) &relocate;
		d = (unsigned long*) ptr;
		for(i = (((unsigned long)&relocateEnd) - ((unsigned long)s))/4;
		    i >= 0; i--){
				*d = *s;
				d++;
				s++;
			}

		progress ('4');
	
		/* Jump to relocated code. */
		__asm __volatile (
			     /* Change stack. */
				 "mr   1,%0 \n\t"
				 "mtlr %1   \n\t"
				 "blr       \n\t"
				 : /* no outputs */
				 : "r" (info[8]), "r" (ptr)
				 /* no return */);
	
	}

	/* fake a noreturn */
	for (;;);
}

void relocate(void)
{
	const unsigned long *info;
	unsigned char *s, *d, *e;
	
	progress ('5');

	/* Get info-ptr from CHIP_PTR. */
	info = (const unsigned long*) *((unsigned long*) CHIP_PTR);

	/* Verify checksum. */
	if (info[7])
	{
		int i = (info[2] + info[3]) / 4;
		unsigned char* k_p = (unsigned char*) info[0];
		unsigned long kcs = 0;
		
		while (i--)
		{
			unsigned long w = 0;
				
			w |= *k_p++;
			w <<= 8;
			w |= *k_p++;
			w <<= 8;
			w |= *k_p++;
			w <<= 8;
			w |= *k_p++;

				
			kcs = kcs ^ i;
			kcs = kcs ^ w;
		}
		progress ('C');
		progress ('1');
		if (info[7] == kcs)
			progress ('+');
		else
			progress ('-');
	}

	/* Copy kernel to memstart */
	s = (unsigned char *) info[0];
	d = (unsigned char *) info[1];
	e = (unsigned char *) (info[1] + info[2] + info[3]);
	while(d != e){
		*d = *s;
		s++;
		d++;
	}

	progress ('6');

	/* Verify checksum. */
	if (info[7])
	{
		int i = (info[2] + info[3]) / 4;
		unsigned char* k_p = (unsigned char*) info[1];
		unsigned long kcs = 0;
		
		while (i--)
		{
			unsigned long w = 0;
				
			w |= *k_p++;
			w <<= 8;
			w |= *k_p++;
			w <<= 8;
			w |= *k_p++;
			w <<= 8;
			w |= *k_p++;

				
			kcs = kcs ^ i;
			kcs = kcs ^ w;
		}
		progress ('C');
		progress ('2');
		if (info[7] == kcs)
			progress ('+');
		else
			progress ('-');
	}
	
	/* Copy ramdisk to memory end. */
	d = (unsigned char *) info[1] + info[5];
	e = d - info[4];
	s = s + info[4];
	while(d != e){
		s--;
		d--;
		*d = *s;
	}

	progress ('7');

#if 1 /* obsolete -- keep it for a while for backwards compatibility */
	/* Let 0xfff00000 contain sum used for VTOP/PTOV in head.S. */
	/* Also put it at kernel start since kernel copies the vectors
	 * to their proper location after modifying a few instructions.
	 */
#define KERNELBASE 0xC0000000
	*((unsigned long *)0xfff00000) = info[1] - KERNELBASE;
	*((unsigned long *) info[1]) = info[1] - KERNELBASE;
#endif

	progress ('K');
	{
		register unsigned long r3 __asm("r3");
		register unsigned long r4 __asm("r4");
		register unsigned long r5 __asm("r5");

		/* Make it easy to identify APUS in identify_machine. */
		r3 = 0x61707573;

		
		/* Memory start address. */
		r4 = info[1];
		
		/* Jump to kernel start. */
		r5 = info[1] + 0xc;
		__asm __volatile (
			"mtlr 5 \n\t"
			"blr    \n\t"
			: /* no outputs */
			: "r" (r3), "r" (r4), "r" (r5));
	}
	
	/* fake a noreturn */
	for (;;);
}

/* Play that jive... */
extern char setPR, setPRend;
void goSupervisor(void)
{
	unsigned long *s, *d;
	int len = ((int) &setPRend - (int) &setPR) / 4;

	/* Copy code to TRAP entry */
	d = (unsigned long*) 0xfff00700;
	s = (unsigned long*) &setPR;
	for(;len > 0;len--){
		*d = *s;
		d++;
		s++;
	}

	PPCCacheFlushAll();
	
	/* Execute code */
	__asm__ (
		"trap \n\t"
		);
}

void progress_init (void)
{
	const unsigned long *info;
	unsigned long *mesg_base;
	unsigned long* p;
	info = (const unsigned long*) *((unsigned long*) CHIP_PTR);
	mesg_base = (unsigned long*) info[6];
	p = mesg_base;
	
	*p++ = 'SAVE';
	*p++ = 'BOOT';
	*p++ = (unsigned long) mesg_base;
	*p++ = 0;
}
	

void progress (char c)
{
	const unsigned long *info;
	char progress_state;
	char* progress_ptr;
	unsigned long* mesg_base;
	info = (const unsigned long*) *((unsigned long*) CHIP_PTR);

	mesg_base = (unsigned long*) info[6];

	progress_ptr = mesg_base[3] + (char*)&mesg_base[4];
	*progress_ptr = c;
	mesg_base[3]++;
}

asm(
	".text\n"
	".align 4\n"
	SYMBOL_NAME_STR(setPR) ":
/* Increase the return EA so we get to the next instruction */
mtsprg       1,1
mfsrr0       1
addi         1,1,4
mtsrr0       1
mfsprg       1,1
/* Fiddle the PR bit */
mfsrr1       0
rlwinm       0,0,0,18,16
mtsrr1       0
rfi
"
	SYMBOL_NAME_STR(setPRend) ":
");

asm(
	".text\n"
	".align 4\n"
	SYMBOL_NAME_STR(relocateEnd) ":
nop
"
	);

