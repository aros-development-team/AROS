#include <asm/segments.h>
#include <asm/linkage.h>
#include <asm/ptrace.h>

asmlinkage void divide_error(void);

struct { long long a; } *idt_base;

#define _set_gate(gate_addr,type,dpl,addr) \
do { \
  int __d0, __d1; \
  __asm__ __volatile__ ("movw %%dx,%%ax\n\t" \
	"movw %4,%%dx\n\t" \
	"movl %%eax,%0\n\t" \
	"movl %%edx,%1" \
	:"=m" (*((long *) (gate_addr))), \
	 "=m" (*(1+(long *) (gate_addr))), "=&a" (__d0), "=&d" (__d1) \
	:"i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	 "3" ((char *) (addr)),"2" (KERNEL_CS << 16)); \
} while (0)


void set_intr_gate(unsigned int n, void *addr)
{
	_set_gate(idt_base+n,14,0,addr);
}

void set_system_gate(unsigned int n, void *addr)
{
	_set_gate(idt_base+n,14,3,addr);
}

asmlinkage void system_call(struct pt_regs);

void Init_Traps()
{
	set_system_gate(0x80, system_call);
}

