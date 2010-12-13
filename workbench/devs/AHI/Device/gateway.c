/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#include <config.h>
#include <exec/types.h> 
#include <clib/alib_protos.h>

#if defined( __MORPHOS__ )
# define EMUL_NOQUICKMODE
# include <emul/emulregs.h>
#endif

#include <proto/exec.h>

#include "ahi_def.h"

#include "audioctrl.h"
#include "database.h"
#include "devcommands.h"
#include "device.h"
#include "header.h"
#include "misc.h"
#include "mixer.h"
#include "modeinfo.h"
#include "requester.h"
#include "sound.h"


/*
 * All these functions are supposed to be called by the m68k "processor",
 * with the arguments in m68k registers d0-d7/a0-a6.
 * The functions relay each call to a standard C function.
 *
 */

#if defined( __MORPHOS__ )

/******************************************************************************
** MorphOS gateway functions **************************************************
******************************************************************************/

/* m68k_IndexToFrequency *****************************************************/

static LONG
gw_IndexToFrequency( void )
{
  struct Gadget* gad   = (struct Gadget*) REG_A1;
  WORD           level = (WORD)           REG_D0;

  return IndexToFrequency( gad , level );
}

struct EmulLibEntry m68k_IndexToFrequency =
{
  TRAP_LIB, 0, (void (*)(void)) gw_IndexToFrequency
};


/* m68k_DevProc **************************************************************/

struct EmulLibEntry m68k_DevProc =
{
  TRAP_LIB, 0, (void (*)(void)) DevProc
};


/* HookEntry *****************************************************************/

/* Should be in libamiga, but isn't? */

static ULONG
gw_HookEntry( void )
{
  struct Hook* h   = (struct Hook*) REG_A0;
  void*        o   = (void*)        REG_A2; 
  void*        msg = (void*)        REG_A1;

  return ( ( (ULONG(*)(struct Hook*, void*, void*)) *h->h_SubEntry)( h, o, msg ) );
}

struct EmulLibEntry _HookEntry =
{
  TRAP_LIB, 0, (void (*)(void)) &gw_HookEntry
};

__asm( ".globl HookEntry;HookEntry=_HookEntry" );

/* m68k_PreTimer  ************************************************************/

static BOOL
gw_PreTimer( void )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;

  return PreTimer( audioctrl );
}

static struct EmulLibEntry m68k_PreTimer =
{
  TRAP_LIB, 0, (void (*)(void)) &gw_PreTimer
};


/* m68k_PostTimer  ***********************************************************/

static void
gw_PostTimer( void )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;

  PostTimer( audioctrl );
}

static struct EmulLibEntry m68k_PostTimer =
{
  TRAP_LIBNR, 0, (void (*)(void)) &gw_PostTimer
};

#elif defined( __amithlon__ )

/******************************************************************************
** Amithlon gateway functions *************************************************
******************************************************************************/

// A handy macro for fetching a little endian long integer from memory
#define GET_LONG(a) ({ long r; __asm__("movl %1,%0":"=r"(r) :"m"(a)); r; })

/* m68k_IndexToFrequency *****************************************************/

static LONG
gw_IndexToFrequency( struct Gadget *gad, WORD level ) __attribute__((regparm(3)));

static LONG
gw_IndexToFrequency( struct Gadget *gad, WORD level )
{
  return IndexToFrequency( gad, level );
}

struct
{
    UWORD movel_4sp_d0[2];
    UWORD movew_10sp_a0[2];
    UWORD movel_a0_d1;
    UWORD jmp;
    ULONG addr;
} m68k_IndexToFrequency =
{
  {0x202F,0x0004},
  {0x306F,0x000A},
  0x2208,
  0x4EF9, (ULONG) gw_IndexToFrequency + 1
};


/* m68k_DevProc **************************************************************/

struct
{
    UWORD nop;  // Just to 32-bit align the long word
    UWORD jmp;
    ULONG addr;
} m68k_DevProc =
{
  0x4E71,
  0x4EF9, (ULONG) DevProc + 1
};


/* m68k_PreTimer  ************************************************************/

static BOOL
gw_PreTimer( struct AHIPrivAudioCtrl* audioctrl ) __attribute__((regparm(3)));

static BOOL
gw_PreTimer( struct AHIPrivAudioCtrl* audioctrl )
{
  return 0; //PreTimer( audioctrl );
}

static struct
{
    UWORD nop;
    UWORD movel_4sp_d0[2];
    UWORD jmp;
    ULONG addr;
} m68k_PreTimer =
{
  0x4E71,
  {0x202F, 0x0004},
  0x4EF9, (ULONG) gw_PreTimer + 1
};


/* m68k_PostTimer  ***********************************************************/

static void
gw_PostTimer( struct _Regs* regs ) __attribute__((regparm(3)));

static void
gw_PostTimer( struct _Regs* regs )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) GET_LONG( regs->a2 );

  PostTimer( audioctrl );
}

static struct
{
    UWORD nop;
    UWORD movel_4sp_d0[2];
    UWORD jmp;
    ULONG addr;
} m68k_PostTimer =
{
  0x4E71,
  {0x202F, 0x0004},
  0x4EF9, (ULONG) gw_PostTimer + 1
};

#elif defined( __AROS__ )

/******************************************************************************
** AROS gateway functions *****************************************************
******************************************************************************/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/* m68k_IndexToFrequency *****************************************************/

LONG
m68k_IndexToFrequency( struct Gadget *gad, WORD level )
{
  return IndexToFrequency( gad, level );
}


/* m68k_DevProc **************************************************************/

AROS_UFH0( void,
	   m68k_DevProc )
{
  AROS_USERFUNC_INIT
  DevProc();
  AROS_USERFUNC_EXIT
}


/* m68k_PreTimer  ************************************************************/

AROS_UFH1( BOOL,
	   m68k_PreTimer,
	   AROS_UFHA( struct AHIPrivAudioCtrl*, audioctrl, A2 ) )
{
  AROS_USERFUNC_INIT
  return PreTimer( audioctrl );
  AROS_USERFUNC_EXIT  
}


/* m68k_PostTimer  ***********************************************************/

AROS_UFH1( void,
	   m68k_PostTimer,
	   AROS_UFHA( struct AHIPrivAudioCtrl*, audioctrl, A2 ) )
{
  AROS_USERFUNC_INIT
  PostTimer( audioctrl );
  AROS_USERFUNC_EXIT  
}

#elif defined( __AMIGAOS4__ )

/******************************************************************************
** AmigaOS 4.x gateway functions **********************************************
******************************************************************************/

/* m68k_IndexToFrequency *****************************************************/

LONG STDARGS SAVEDS
m68k_IndexToFrequency( struct Gadget *gad, WORD level )
{
  return IndexToFrequency( gad, level );
}


/* m68k_DevProc **************************************************************/

void
m68k_DevProc( void )
{
  DevProc();
}


/* m68k_PreTimer  ************************************************************/

BOOL
m68k_PreTimer( REG(a2, struct AHIPrivAudioCtrl* audioctrl ) )
{
  return PreTimer( audioctrl );
}


/* m68k_PostTimer  ***********************************************************/

void
m68k_PostTimer( REG(a2, struct AHIPrivAudioCtrl* audioctrl ) )
{
  PostTimer( audioctrl );
}


UWORD PreTimerPreserveAllRegs[] = {
    0x206A, 0x0054,
    0x93C9,
    0x2F28, 0x0008,
    0x4E75
};

UWORD PostTimerPreserveAllRegs[] = {
    0x206A, 0x0058,
    0x93C9,
    0x2F28, 0x0008,
    0x4E75
};


__asm__("								\n\
/* -------------------- HookEntry ---------------------------*/		\n\
									\n\
	.globl	hookEntry						\n\
	.type	hookEntry, @function					\n\
	.globl	HookEntry						\n\
	.type	HookEntry, @function					\n\
	.globl	HookEntryPreserveAllRegs				\n\
	.type	HookEntryPreserveAllRegs, @function			\n\
	.section .data							\n\
									\n\
HookEntry:								\n\
HookEntryPreserveAllRegs:						\n\
hookEntry:								\n\
	.short	   0x4ef8		 /* JMP.w */			\n\
	.short	   0			 /* Indicate switch */		\n\
	.short	   1			 /* Trap type */		\n\
	.long	   HookEntryPPC						\n\
	/* Register mapping */						\n\
	.byte	   5							\n\
	.byte	   1,60							\n\
	.byte	   3,32							\n\
	.byte	4,40							\n\
	.byte	   5,36							\n\
	.byte	6,56							\n\
	.align	   2							\n\
									\n\
	.section .text							\n\
HookEntryPPC:								\n\
	addi	12, 1, -16		/* Calculate stackframe size */	\n\
	rlwinm	12, 12, 0, 0, 27	/* Align it */			\n\
	stw	1, 0(12)		/* Store backchain pointer */	\n\
	mr	1, 12			/* Set real stack pointer */	\n\
									\n\
	stw	11,12(1)		/* Store Enter68kQuick vector */\n\
									\n\
	lwz	12,12(3)		/* Get the sub entry */		\n\
	mtlr	12							\n\
	blrl								\n\
									\n\
	lwz	11,12(1)						\n\
	mtlr	11							\n\
									\n\
	lwz	1, 0(1)			/* Cleanup stack frame */	\n\
									\n\
	blrl				/* Return to emulation */	\n\
	.long	HookExit						\n\
	.byte	0			/* Flags */			\n\
	.byte	2			/* Two registers (a7 and d0) */	\n\
	.byte	1			/* Map r1 */			\n\
	.byte	60			/* to A7 */			\n\
	.byte	3			/* Map r3 */			\n\
	.byte	0			/* to D0 */			\n\
	.align	4							\n\
									\n\
	.section .data							\n\
HookExit:								\n\
	.short	0x4e75			 /* RTS */			\n\
");

#include <stdarg.h>

struct AHIAudioCtrl * __attribute__((linearvarargs)) _AHI_AllocAudio(

	struct AHIBase *AHIBase, ...
)
{
	va_list ap;
	struct TagItem * varargs;
	va_startlinear(ap, AHIBase);
	varargs = va_getlinearva(ap, struct TagItem *);
	return	_AHI_AllocAudioA(
		varargs, AHIBase);
}




ULONG __attribute__((linearvarargs)) _AHI_ControlAudio(

	struct AHIAudioCtrl * AudioCtrl,
	struct AHIBase *AHIBase, ...
)
{
	va_list ap;
	struct TagItem * varargs;
	va_startlinear(ap, AHIBase);
	varargs = va_getlinearva(ap, struct TagItem *);
	return	_AHI_ControlAudioA(
		(struct AHIPrivAudioCtrl*) AudioCtrl,
		varargs, AHIBase);
}


BOOL __attribute__((linearvarargs)) _AHI_GetAudioAttrs(

	ULONG ID,
	struct AHIAudioCtrl * Audioctrl,
	struct AHIBase *AHIBase,
    ...
)
{
	va_list ap;
	struct TagItem * varargs;
	va_startlinear(ap, AHIBase);
	varargs = va_getlinearva(ap, struct TagItem *);

	return	_AHI_GetAudioAttrsA(
		ID,
		(struct AHIPrivAudioCtrl*) Audioctrl,
		varargs, AHIBase);
}


ULONG __attribute__((linearvarargs)) _AHI_BestAudioID(

	struct AHIBase *AHIBase, ...
)
{
	va_list ap;
	struct TagItem * varargs;
	va_startlinear(ap, AHIBase);
	varargs = va_getlinearva(ap, struct TagItem *);
	return	_AHI_BestAudioIDA(
		varargs, AHIBase);
}


struct AHIAudioModeRequester * __attribute__((linearvarargs)) _AHI_AllocAudioRequest(

	struct AHIBase *AHIBase, ...
)
{
	va_list ap;
	struct TagItem * varargs;
	va_startlinear(ap, AHIBase);
	varargs = va_getlinearva(ap, struct TagItem *);
	return	_AHI_AllocAudioRequestA(
		varargs, AHIBase);
}


BOOL __attribute__((linearvarargs)) _AHI_AudioRequest(

	struct AHIAudioModeRequester * Requester,
	struct AHIBase *AHIBase, ...
)
{
	va_list ap;
	struct TagItem * varargs;
	va_startlinear(ap, AHIBase);
	varargs = va_getlinearva(ap, struct TagItem *);
	return	_AHI_AudioRequestA(
		Requester,
		varargs, AHIBase);
}


void __attribute__((linearvarargs)) _AHI_Play(

	struct AHIAudioCtrl * Audioctrl,
	struct AHIBase *AHIBase, ...
)
{
	va_list ap;
	struct TagItem * varargs;
	va_startlinear(ap, AHIBase);
	varargs = va_getlinearva(ap, struct TagItem *);
		_AHI_PlayA(
		(struct AHIPrivAudioCtrl*) Audioctrl,
		varargs, AHIBase);
}

#else // Not MorphOS, not Amithlon, not AROS, not AmigaOS 4.x

/******************************************************************************
** AmigaOS 2.x/3.x gateway functions ******************************************
******************************************************************************/

/* m68k_IndexToFrequency *****************************************************/

LONG __attribute__((stkparm)) __attribute__((saveds))
m68k_IndexToFrequency( struct Gadget *gad, WORD level )
{
  return IndexToFrequency( gad, level );
}


/* m68k_DevProc **************************************************************/

void
m68k_DevProc( void )
{
  DevProc();
}


/* m68k_PreTimer  ************************************************************/

BOOL
m68k_PreTimer( struct AHIPrivAudioCtrl* audioctrl __asm("a2") )
{
  return PreTimer( audioctrl );
}


/* m68k_PostTimer  ***********************************************************/

void
m68k_PostTimer( struct AHIPrivAudioCtrl* audioctrl __asm("a2") )
{
  PostTimer( audioctrl );
}

#endif


/*** Some special wrappers ***************************************************/

#if defined( __AROS__ ) && !defined( __mc68000__ )

// We're not binary compatible!

#elif defined( __AMIGAOS4__ )

// No need to do this

#else

#ifndef __mc68000__
# pragma pack(2) // Make sure the compiler does not insert pads
#endif

const struct
{
    UWORD pushm_d0_d1_a0_a1[2];
    UWORD jsr;
    ULONG addr;
    UWORD popm_d0_d1_a0_a1[2];
    UWORD rts;
} HookEntryPreserveAllRegs __attribute__ ((aligned (4))) =
{
  {0x48E7, 0xC0C0},
  0x4EB9, (ULONG) HookEntry,
  {0x4CDF, 0x0303},
  0x4E75
};


const struct
{
    UWORD pushm_d1_a0_a1[2];
    UWORD jsr;
    ULONG addr;
    UWORD popm_d1_a0_a1[2];
    UWORD extl_d0;
    UWORD rts;
} PreTimerPreserveAllRegs __attribute__ ((aligned (4))) =
{
  {0x48E7, 0x40C0},
  0x4EB9, (ULONG) &m68k_PreTimer,
  {0x4CDF, 0x0302},
  0x48C0,
  0x4E75
};


const struct
{
    UWORD pushm_d0_d1_a0_a1[2];
    UWORD jsr;
    ULONG addr;
    UWORD popm_d0_d1_a0_a1[2];
    UWORD rts;
} PostTimerPreserveAllRegs __attribute__ ((aligned (4))) =
{
  {0x48E7, 0xC0C0},
  0x4EB9, (ULONG) &m68k_PostTimer,
  {0x4CDF, 0x0303},
  0x4E75
};

#endif /* defined( __AROS__ ) / defined( __AMIGAOS4__ ) */
