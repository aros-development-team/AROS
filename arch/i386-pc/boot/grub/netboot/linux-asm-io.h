#ifndef	_ASM_IO_H
#define _ASM_IO_H

/*
 * This file contains the definitions for the x86 IO instructions
 * inb/inw/inl/outb/outw/outl and the "string versions" of the same
 * (insb/insw/insl/outsb/outsw/outsl). You can also use "pausing"
 * versions of the single-IO instructions (inb_p/inw_p/..).
 *
 * This file is not meant to be obfuscating: it's just complicated
 * to (a) handle it all in a way that makes gcc able to optimize it
 * as well as possible and (b) trying to avoid writing the same thing
 * over and over again with slight variations and possibly making a
 * mistake somewhere.
 */

/*
 * Thanks to James van Artsdalen for a better timing-fix than
 * the two short jumps: using outb's to a nonexistent port seems
 * to guarantee better timings even on fast machines.
 *
 * On the other hand, I'd like to be sure of a non-existent port:
 * I feel a bit unsafe about using 0x80 (should be safe, though)
 *
 *		Linus
 */

#ifdef	SLOW_IO_BY_JUMPING
#define __SLOW_DOWN_IO __asm__ __volatile__("jmp 1f\n1:\tjmp 1f\n1:")
#else
#define __SLOW_DOWN_IO __asm__ __volatile__("outb %al,$0x80")
#endif

#ifdef	REALLY_SLOW_IO
#define SLOW_DOWN_IO { __SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO; }
#else
#define SLOW_DOWN_IO __SLOW_DOWN_IO
#endif

/*
 * readX/writeX() are used to access memory mapped devices. On some
 * architectures the memory mapped IO stuff needs to be accessed
 * differently. On the x86 architecture, we just read/write the
 * memory location directly.
 */
#define readb(addr) (*(volatile unsigned char *) (addr))
#define readw(addr) (*(volatile unsigned short *) (addr))
#define readl(addr) (*(volatile unsigned int *) (addr))

#define writeb(b,addr) ((*(volatile unsigned char *) (addr)) = (b))
#define writew(b,addr) ((*(volatile unsigned short *) (addr)) = (b))
#define writel(b,addr) ((*(volatile unsigned int *) (addr)) = (b))

#define memset_io(a,b,c)	memset((void *)(a),(b),(c))
#define memcpy_fromio(a,b,c)	memcpy((a),(void *)(b),(c))
#define memcpy_toio(a,b,c)	memcpy((void *)(a),(b),(c))

/*
 * Again, i386 does not require mem IO specific function.
 */

#define eth_io_copy_and_sum(a,b,c,d)	eth_copy_and_sum((a),(void *)(b),(c),(d))

/*
 * Talk about misusing macros..
 */

#define __OUT1(s,x) \
extern void __out##s(unsigned x value, unsigned short port); \
extern inline void __out##s(unsigned x value, unsigned short port) {

#define __OUT2(s,s1,s2) \
__asm__ __volatile__ ("out" #s " %" s1 "0,%" s2 "1"

#define __OUT(s,s1,x) \
__OUT1(s,x) __OUT2(s,s1,"w") : : "a" (value), "d" (port)); } \
__OUT1(s##c,x) __OUT2(s,s1,"") : : "a" (value), "id" (port)); } \
__OUT1(s##_p,x) __OUT2(s,s1,"w") : : "a" (value), "d" (port)); SLOW_DOWN_IO; } \
__OUT1(s##c_p,x) __OUT2(s,s1,"") : : "a" (value), "id" (port)); SLOW_DOWN_IO; }

#define __IN1(s,x) \
extern unsigned x __in##s(unsigned short port); \
extern inline unsigned x __in##s(unsigned short port) { unsigned x _v;

#define __IN2(s,s1,s2) \
__asm__ __volatile__ ("in" #s " %" s2 "1,%" s1 "0"

#define __IN(s,s1,x,i...) \
__IN1(s,x) __IN2(s,s1,"w") : "=a" (_v) : "d" (port) ,##i ); return _v; } \
__IN1(s##c,x) __IN2(s,s1,"") : "=a" (_v) : "id" (port) ,##i ); return _v; } \
__IN1(s##_p,x) __IN2(s,s1,"w") : "=a" (_v) : "d" (port) ,##i ); SLOW_DOWN_IO; return _v; } \
__IN1(s##c_p,x) __IN2(s,s1,"") : "=a" (_v) : "id" (port) ,##i ); SLOW_DOWN_IO; return _v; }

#define __INS(s) \
extern void ins##s(unsigned short port, void * addr, unsigned long count); \
extern inline void ins##s(unsigned short port, void * addr, unsigned long count) \
{ __asm__ __volatile__ ("cld ; rep ; ins" #s \
: "=D" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count)); }

#define __OUTS(s) \
extern void outs##s(unsigned short port, const void * addr, unsigned long  count); \
extern inline void outs##s(unsigned short port, const void * addr, unsigned long count) \
{ __asm__ __volatile__ ("cld ; rep ; outs" #s \
: "=S" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count)); }

__IN(b,"", char)
__IN(w,"",short)
__IN(l,"", long)

__OUT(b,"b",char)
__OUT(w,"w",short)
__OUT(l,,int)

__INS(b)
__INS(w)
__INS(l)

__OUTS(b)
__OUTS(w)
__OUTS(l)

/*
 * Note that due to the way __builtin_constant_p() works, you
 *  - can't use it inside a inline function (it will never be true)
 *  - you don't have to worry about side effects within the __builtin..
 */
#define outb(val,port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__outbc((val),(port)) : \
	__outb((val),(port)))

#define inb(port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__inbc(port) : \
	__inb(port))

#define outb_p(val,port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__outbc_p((val),(port)) : \
	__outb_p((val),(port)))

#define inb_p(port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__inbc_p(port) : \
	__inb_p(port))

#define outw(val,port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__outwc((val),(port)) : \
	__outw((val),(port)))

#define inw(port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__inwc(port) : \
	__inw(port))

#define outw_p(val,port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__outwc_p((val),(port)) : \
	__outw_p((val),(port)))

#define inw_p(port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__inwc_p(port) : \
	__inw_p(port))

#define outl(val,port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__outlc((val),(port)) : \
	__outl((val),(port)))

#define inl(port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__inlc(port) : \
	__inl(port))

#define outl_p(val,port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__outlc_p((val),(port)) : \
	__outl_p((val),(port)))

#define inl_p(port) \
((__builtin_constant_p((port)) && (port) < 256) ? \
	__inlc_p(port) : \
	__inl_p(port))

#endif
