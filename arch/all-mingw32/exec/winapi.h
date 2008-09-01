#ifndef _WINAPI_H
#define _WINAPI_H

#define UCHAR  UBYTE
#define USHORT UWORD
#define DWORD  IPTR

/* This was taken from Mingw32's w32api/winnt.h */
#ifdef __i386__
#define SIZE_OF_80387_REGISTERS	80
#define CONTEXT_i386	0x10000
#define CONTEXT_i486	0x10000
#define CONTEXT_CONTROL	(CONTEXT_i386|0x00000001L)
#define CONTEXT_INTEGER	(CONTEXT_i386|0x00000002L)
#define CONTEXT_SEGMENTS	(CONTEXT_i386|0x00000004L)
#define CONTEXT_FLOATING_POINT	(CONTEXT_i386|0x00000008L)
#define CONTEXT_DEBUG_REGISTERS	(CONTEXT_i386|0x00000010L)
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_i386|0x00000020L)
#define CONTEXT_FULL	(CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS)
#define MAXIMUM_SUPPORTED_EXTENSION  512
typedef struct _FLOATING_SAVE_AREA {
	DWORD	ControlWord;
	DWORD	StatusWord;
	DWORD	TagWord;
	DWORD	ErrorOffset;
	DWORD	ErrorSelector;
	DWORD	DataOffset;
	DWORD	DataSelector;
	BYTE	RegisterArea[80];
	DWORD	Cr0NpxState;
} FLOATING_SAVE_AREA;
typedef struct _CONTEXT {
	DWORD	ContextFlags;
	DWORD	Dr0;
	DWORD	Dr1;
	DWORD	Dr2;
	DWORD	Dr3;
	DWORD	Dr6;
	DWORD	Dr7;
	FLOATING_SAVE_AREA FloatSave;
	DWORD	SegGs;
	DWORD	SegFs;
	DWORD	SegEs;
	DWORD	SegDs;
	DWORD	Edi;
	DWORD	Esi;
	DWORD	Ebx;
	DWORD	Edx;
	DWORD	Ecx;
	DWORD	Eax;
	DWORD	Ebp;
	DWORD	Eip;
	DWORD	SegCs;
	DWORD	EFlags;
	DWORD	Esp;
	DWORD	SegSs;
	BYTE	ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT;
#elif defined(_PPC_)
#define CONTEXT_CONTROL	1L
#define CONTEXT_FLOATING_POINT	2L
#define CONTEXT_INTEGER	4L
#define CONTEXT_DEBUG_REGISTERS	8L
#define CONTEXT_FULL (CONTEXT_CONTROL|CONTEXT_FLOATING_POINT|CONTEXT_INTEGER)
typedef struct {
	double Fpr0;
	double Fpr1;
	double Fpr2;
	double Fpr3;
	double Fpr4;
	double Fpr5;
	double Fpr6;
	double Fpr7;
	double Fpr8;
	double Fpr9;
	double Fpr10;
	double Fpr11;
	double Fpr12;
	double Fpr13;
	double Fpr14;
	double Fpr15;
	double Fpr16;
	double Fpr17;
	double Fpr18;
	double Fpr19;
	double Fpr20;
	double Fpr21;
	double Fpr22;
	double Fpr23;
	double Fpr24;
	double Fpr25;
	double Fpr26;
	double Fpr27;
	double Fpr28;
	double Fpr29;
	double Fpr30;
	double Fpr31;
	double Fpscr;
	DWORD Gpr0;
	DWORD Gpr1;
	DWORD Gpr2;
	DWORD Gpr3;
	DWORD Gpr4;
	DWORD Gpr5;
	DWORD Gpr6;
	DWORD Gpr7;
	DWORD Gpr8;
	DWORD Gpr9;
	DWORD Gpr10;
	DWORD Gpr11;
	DWORD Gpr12;
	DWORD Gpr13;
	DWORD Gpr14;
	DWORD Gpr15;
	DWORD Gpr16;
	DWORD Gpr17;
	DWORD Gpr18;
	DWORD Gpr19;
	DWORD Gpr20;
	DWORD Gpr21;
	DWORD Gpr22;
	DWORD Gpr23;
	DWORD Gpr24;
	DWORD Gpr25;
	DWORD Gpr26;
	DWORD Gpr27;
	DWORD Gpr28;
	DWORD Gpr29;
	DWORD Gpr30;
	DWORD Gpr31;
	DWORD Cr;
	DWORD Xer;
	DWORD Msr;
	DWORD Iar;
	DWORD Lr;
	DWORD Ctr;
	DWORD ContextFlags;
	DWORD Fill[3];
	DWORD Dr0;
	DWORD Dr1;
	DWORD Dr2;
	DWORD Dr3;
	DWORD Dr4;
	DWORD Dr5;
	DWORD Dr6;
	DWORD Dr7;
} CONTEXT;
#elif defined(_ALPHA_)
#define CONTEXT_ALPHA	0x20000
#define CONTEXT_CONTROL	(CONTEXT_ALPHA|1L)
#define CONTEXT_FLOATING_POINT	(CONTEXT_ALPHA|2L)
#define CONTEXT_INTEGER	(CONTEXT_ALPHA|4L)
#define CONTEXT_FULL	(CONTEXT_CONTROL|CONTEXT_FLOATING_POINT|CONTEXT_INTEGER)
typedef struct _CONTEXT {
	ULONGLONG FltF0;
	ULONGLONG FltF1;
	ULONGLONG FltF2;
	ULONGLONG FltF3;
	ULONGLONG FltF4;
	ULONGLONG FltF5;
	ULONGLONG FltF6;
	ULONGLONG FltF7;
	ULONGLONG FltF8;
	ULONGLONG FltF9;
	ULONGLONG FltF10;
	ULONGLONG FltF11;
	ULONGLONG FltF12;
	ULONGLONG FltF13;
	ULONGLONG FltF14;
	ULONGLONG FltF15;
	ULONGLONG FltF16;
	ULONGLONG FltF17;
	ULONGLONG FltF18;
	ULONGLONG FltF19;
	ULONGLONG FltF20;
	ULONGLONG FltF21;
	ULONGLONG FltF22;
	ULONGLONG FltF23;
	ULONGLONG FltF24;
	ULONGLONG FltF25;
	ULONGLONG FltF26;
	ULONGLONG FltF27;
	ULONGLONG FltF28;
	ULONGLONG FltF29;
	ULONGLONG FltF30;
	ULONGLONG FltF31;
	ULONGLONG IntV0;
	ULONGLONG IntT0;
	ULONGLONG IntT1;
	ULONGLONG IntT2;
	ULONGLONG IntT3;
	ULONGLONG IntT4;
	ULONGLONG IntT5;
	ULONGLONG IntT6;
	ULONGLONG IntT7;
	ULONGLONG IntS0;
	ULONGLONG IntS1;
	ULONGLONG IntS2;
	ULONGLONG IntS3;
	ULONGLONG IntS4;
	ULONGLONG IntS5;
	ULONGLONG IntFp;
	ULONGLONG IntA0;
	ULONGLONG IntA1;
	ULONGLONG IntA2;
	ULONGLONG IntA3;
	ULONGLONG IntA4;
	ULONGLONG IntA5;
	ULONGLONG IntT8;
	ULONGLONG IntT9;
	ULONGLONG IntT10;
	ULONGLONG IntT11;
	ULONGLONG IntRa;
	ULONGLONG IntT12;
	ULONGLONG IntAt;
	ULONGLONG IntGp;
	ULONGLONG IntSp;
	ULONGLONG IntZero;
	ULONGLONG Fpcr;
	ULONGLONG SoftFpcr;
	ULONGLONG Fir;
	DWORD Psr;
	DWORD ContextFlags;
	DWORD Fill[4];
} CONTEXT;
#elif defined(SHx)

/* These are the debug or break registers on the SH3 */
typedef struct _DEBUG_REGISTERS {
	ULONG  BarA;
	UCHAR  BasrA;
	UCHAR  BamrA;
	USHORT BbrA;
	ULONG  BarB;
	UCHAR  BasrB;
	UCHAR  BamrB;
	USHORT BbrB;
	ULONG  BdrB;
	ULONG  BdmrB;
	USHORT Brcr;
	USHORT Align;
} DEBUG_REGISTERS, *PDEBUG_REGISTERS;

/* The following flags control the contents of the CONTEXT structure. */

#define CONTEXT_SH3		0x00000040
#define CONTEXT_SH4		0x000000c0	/* CONTEXT_SH3 | 0x80 - must contain the SH3 bits */

#ifdef SH3
#define CONTEXT_CONTROL         (CONTEXT_SH3 | 0x00000001L)
#define CONTEXT_INTEGER         (CONTEXT_SH3 | 0x00000002L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_SH3 | 0x00000008L)
#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_DEBUG_REGISTERS)
#else	/* SH4 */
#define CONTEXT_CONTROL         (CONTEXT_SH4 | 0x00000001L)
#define CONTEXT_INTEGER         (CONTEXT_SH4 | 0x00000002L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_SH4 | 0x00000008L)
#define CONTEXT_FLOATING_POINT  (CONTEXT_SH4 | 0x00000004L)
#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_DEBUG_REGISTERS | CONTEXT_FLOATING_POINT)
#endif

/* Context Frame */

/*  This frame is used to store a limited processor context into the */
/* Thread structure for CPUs which have no floating point support. */

typedef struct _CONTEXT {
	/* The flags values within this flag control the contents of */
	/* a CONTEXT record. */

	/* If the context record is used as an input parameter, then */
	/* for each portion of the context record controlled by a flag */
	/* whose value is set, it is assumed that that portion of the */
	/* context record contains valid context. If the context record */
	/* is being used to modify a thread's context, then only that */
	/* portion of the threads context will be modified. */

	/* If the context record is used as an IN OUT parameter to capture */
	/* the context of a thread, then only those portions of the thread's */
	/* context corresponding to set flags will be returned. */

	/* The context record is never used as an OUT only parameter. */


	ULONG ContextFlags;

	/* This section is specified/returned if the ContextFlags word contains */
	/* the flag CONTEXT_INTEGER. */

	/* N.B. The registers RA and R15 are defined in this section, but are */
	/*  considered part of the control context rather than part of the integer */
	/*  context. */

	ULONG PR;
	ULONG MACH;
	ULONG MACL;
	ULONG GBR;
	ULONG R0;
	ULONG R1;
	ULONG R2;
	ULONG R3;
	ULONG R4;
	ULONG R5;
	ULONG R6;
	ULONG R7;
	ULONG R8;
	ULONG R9;
	ULONG R10;
	ULONG R11;
	ULONG R12;
	ULONG R13;
	ULONG R14;
	ULONG R15;

	/* This section is specified/returned if the ContextFlags word contains */
	/* the flag CONTEXT_CONTROL. */

	/* N.B. The registers r15 and ra are defined in the integer section, */
	/*   but are considered part of the control context rather than part of */
	/*   the integer context. */

	ULONG Fir;
	ULONG Psr;

#if !defined(SH3e) && !defined(SH4)
	ULONG	OldStuff[2];
	DEBUG_REGISTERS DebugRegisters;
#else
	ULONG	Fpscr;
	ULONG	Fpul;
	ULONG	FRegs[16];
#if defined(SH4)
	ULONG	xFRegs[16];
#endif
#endif
} CONTEXT;

#elif defined(MIPS)

/* The following flags control the contents of the CONTEXT structure. */

#define CONTEXT_R4000   0x00010000    /* r4000 context */

#define CONTEXT_CONTROL         (CONTEXT_R4000 | 0x00000001L)
#define CONTEXT_FLOATING_POINT  (CONTEXT_R4000 | 0x00000002L)
#define CONTEXT_INTEGER         (CONTEXT_R4000 | 0x00000004L)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER)

/* Context Frame */

/*  N.B. This frame must be exactly a multiple of 16 bytes in length. */

/*  This frame has a several purposes: 1) it is used as an argument to */
/*  NtContinue, 2) it is used to constuct a call frame for APC delivery, */
/*  3) it is used to construct a call frame for exception dispatching */
/*  in user mode, and 4) it is used in the user level thread creation */
/*  routines. */

/*  The layout of the record conforms to a standard call frame. */


typedef struct _CONTEXT {

	/* This section is always present and is used as an argument build */
	/* area. */

	DWORD Argument[4];

	/* This section is specified/returned if the ContextFlags word contains */
	/* the flag CONTEXT_FLOATING_POINT. */

	DWORD FltF0;
	DWORD FltF1;
	DWORD FltF2;
	DWORD FltF3;
	DWORD FltF4;
	DWORD FltF5;
	DWORD FltF6;
	DWORD FltF7;
	DWORD FltF8;
	DWORD FltF9;
	DWORD FltF10;
	DWORD FltF11;
	DWORD FltF12;
	DWORD FltF13;
	DWORD FltF14;
	DWORD FltF15;
	DWORD FltF16;
	DWORD FltF17;
	DWORD FltF18;
	DWORD FltF19;
	DWORD FltF20;
	DWORD FltF21;
	DWORD FltF22;
	DWORD FltF23;
	DWORD FltF24;
	DWORD FltF25;
	DWORD FltF26;
	DWORD FltF27;
	DWORD FltF28;
	DWORD FltF29;
	DWORD FltF30;
	DWORD FltF31;

	/* This section is specified/returned if the ContextFlags word contains */
	/* the flag CONTEXT_INTEGER. */

	/* N.B. The registers gp, sp, and ra are defined in this section, but are */
	/*  considered part of the control context rather than part of the integer */
	/*  context. */

	/* N.B. Register zero is not stored in the frame. */

	DWORD IntZero;
	DWORD IntAt;
	DWORD IntV0;
	DWORD IntV1;
	DWORD IntA0;
	DWORD IntA1;
	DWORD IntA2;
	DWORD IntA3;
	DWORD IntT0;
	DWORD IntT1;
	DWORD IntT2;
	DWORD IntT3;
	DWORD IntT4;
	DWORD IntT5;
	DWORD IntT6;
	DWORD IntT7;
	DWORD IntS0;
	DWORD IntS1;
	DWORD IntS2;
	DWORD IntS3;
	DWORD IntS4;
	DWORD IntS5;
	DWORD IntS6;
	DWORD IntS7;
	DWORD IntT8;
	DWORD IntT9;
	DWORD IntK0;
	DWORD IntK1;
	DWORD IntGp;
	DWORD IntSp;
	DWORD IntS8;
	DWORD IntRa;
	DWORD IntLo;
	DWORD IntHi;

	/* This section is specified/returned if the ContextFlags word contains */
	/* the flag CONTEXT_FLOATING_POINT. */

	DWORD Fsr;

	/* This section is specified/returned if the ContextFlags word contains */
	/* the flag CONTEXT_CONTROL. */

	/* N.B. The registers gp, sp, and ra are defined in the integer section, */
	/*   but are considered part of the control context rather than part of */
	/*   the integer context. */

	DWORD Fir;
	DWORD Psr;

	/* The flags values within this flag control the contents of */
	/* a CONTEXT record. */

	/* If the context record is used as an input parameter, then */
	/* for each portion of the context record controlled by a flag */
	/* whose value is set, it is assumed that that portion of the */
	/* context record contains valid context. If the context record */
	/* is being used to modify a thread's context, then only that */
	/* portion of the threads context will be modified. */

	/* If the context record is used as an IN OUT parameter to capture */
	/* the context of a thread, then only those portions of the thread's */
	/* context corresponding to set flags will be returned. */

	/* The context record is never used as an OUT only parameter. */

	DWORD ContextFlags;

	DWORD Fill[2];

} CONTEXT;
#elif defined(ARM)

/* The following flags control the contents of the CONTEXT structure. */

#define CONTEXT_ARM    0x0000040
#define CONTEXT_CONTROL         (CONTEXT_ARM | 0x00000001L)
#define CONTEXT_INTEGER         (CONTEXT_ARM | 0x00000002L)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER)

typedef struct _CONTEXT {
	/* The flags values within this flag control the contents of
	   a CONTEXT record.
	  
	   If the context record is used as an input parameter, then
	   for each portion of the context record controlled by a flag
	   whose value is set, it is assumed that that portion of the
	   context record contains valid context. If the context record
	   is being used to modify a thread's context, then only that
	   portion of the threads context will be modified.
	  
	   If the context record is used as an IN OUT parameter to capture
	   the context of a thread, then only those portions of the thread's
	   context corresponding to set flags will be returned.
	  
	   The context record is never used as an OUT only parameter. */

	ULONG ContextFlags;

	/* This section is specified/returned if the ContextFlags word contains
	   the flag CONTEXT_INTEGER. */
	ULONG R0;
	ULONG R1;
	ULONG R2;
	ULONG R3;
	ULONG R4;
	ULONG R5;
	ULONG R6;
	ULONG R7;
	ULONG R8;
	ULONG R9;
	ULONG R10;
	ULONG R11;
	ULONG R12;

	ULONG Sp;
	ULONG Lr;
	ULONG Pc;
	ULONG Psr;
} CONTEXT;

#else
#error "undefined processor type"
#endif

#endif
