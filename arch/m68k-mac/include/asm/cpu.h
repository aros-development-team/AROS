#ifndef CPU_H
#define CPU_H

#include <exec/types.h>

#define IRQ_LEVEL1	0x064
#define IRQ_LEVEL2	0x068
#define IRQ_LEVEL3	0x06c
#define IRQ_LEVEL4	0x070
#define IRQ_LEVEL5	0x074
#define IRQ_LEVEL6	0x078

#define TRAP_0		0x080
#define TRAP_1		0x084
#define TRAP_2		0x088
#define TRAP_3		0x08c
#define TRAP_4		0x090

#define INSTALL_IRQ_HANDLER(vectoraddress, routineaddress) \
	*(ULONG *)vectoraddress = (ULONG)routineaddress

#define INSTALL_TRAP_HANDLER(vectoraddress, routineaddress) \
	*(ULONG *)vectoraddress = (ULONG)routineaddress


#endif