/*
** linux/arch/m68k/boot/amiga/bootstrap.h -- This file is part of the Amiga
**					     bootloader.
**
** Copyright 1993, 1994 by Hamish Macdonald
**
** Some minor additions by Michael Rausch 1-11-94
** Modified 11-May-94 by Geert Uytterhoeven
**			(Geert.Uytterhoeven@cs.kuleuven.ac.be)
**     - inline Supervisor() call
** Modified 10-Jan-96 by Geert Uytterhoeven
**     - The real Linux/m68k boot code moved to linuxboot.[ch]
** Modified 9-Sep-96 by Geert Uytterhoeven
**     - const library bases
**     - fixed register naming for m68k-cbm-amigados-gcc
**
** This file is subject to the terms and conditions of the GNU General Public
** License.  See the file COPYING in the main directory of this archive
** for more details.
**
*/


struct MsgPort {
    u_char fill1[15];
    u_char mp_SigBit;
    u_char fill2[18];
};

struct IOStdReq {
    u_char fill1[20];
    struct Device *io_Device;
    u_char fill2[4];
    u_short io_Command;
    u_char io_Flags;
    char io_Error;
    u_long io_Actual;
    u_long io_Length;
    void *io_Data;
    u_char fill4[4];
};

#define IOF_QUICK	(1<<0)

struct timerequest {
    u_char fill1[28];
    u_short io_Command;
    u_char io_Flags;
    u_char fill2[1];
    u_long tv_secs;
    u_long tv_micro;
};

#define UNIT_VBLANK	1
#define TR_ADDREQUEST	9


struct Library;
struct IORequest;


static __inline char OpenDevice(u_char *devName, u_long unit,
				struct IORequest *ioRequest, u_long flags)
{
    register char _res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register u_char *a0 __asm("a0") = devName;
    register u_long d0 __asm("d0") = unit;
    register struct IORequest *a1 __asm("a1") = ioRequest;
    register u_long d1 __asm("d1") = flags;

    __asm __volatile ("jsr a6@(-0x1bc)"
		      : "=r" (_res)
		      : "r" (a6), "r" (a0), "r" (a1), "r" (d0), "r" (d1)
		      : "a0","a1","d0","d1", "memory");
    return(_res);
}

static __inline void CloseDevice(struct IORequest *ioRequest)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct IORequest *a1 __asm("a1") = ioRequest;

    __asm __volatile ("jsr a6@(-0x1c2)"
		      : /* no output */
		      : "r" (a6), "r" (a1)
		      : "a0","a1","d0","d1", "memory");
}

static __inline char DoIO(struct IORequest *ioRequest)
{
    register char _res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct IORequest *a1 __asm("a1") = ioRequest;

    __asm __volatile ("jsr a6@(-0x1c8)"
		      : "=r" (_res)
		      : "r" (a6), "r" (a1)
		      : "a0","a1","d0","d1", "memory");
    return(_res);
}

static __inline void *CreateIORequest(struct MsgPort *port, u_long size)
{
    register struct Library *_res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct MsgPort *a0 __asm("a0") = port;
    register u_long d0 __asm("d0") = size;

    __asm __volatile ("jsr a6@(-0x28e)"
		      : "=r" (_res)
		      : "r" (a6), "r" (a0), "r" (d0)
		      : "a0","a1","d0","d1", "memory");
    return(_res);
}

static __inline void DeleteIORequest(void *ioRequest)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register void *a0 __asm("a0") = ioRequest;

    __asm __volatile ("jsr a6@(-0x294)"
		      : /* no output */
		      : "r" (a6), "r" (a0)
		      : "a0","a1","d0","d1", "memory");
}

static __inline struct MsgPort *CreateMsgPort(void)
{
    register struct MsgPort *_res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;

    __asm __volatile ("jsr a6@(-0x29a)"
		      : "=r" (_res)
		      : "r" (a6)
		      : "a0","a1","d0","d1", "memory");
    return(_res);
}

static __inline void DeleteMsgPort(struct MsgPort *port)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct MsgPort *a0 __asm("a0") = port;

    __asm __volatile ("jsr a6@(-0x2a0)"
		      : /* no output */
		      : "r" (a6), "r" (a0)
		      : "a0","a1","d0","d1", "memory");
}
