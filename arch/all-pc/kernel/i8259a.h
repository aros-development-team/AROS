#ifndef KERNEL_I8259A_H
#define KERNEL_I8259A_H

#include <inttypes.h>

extern struct IntrController i8259a_IntrController;

#if (0)
void XTPIC_Init(uint16_t *irqmask);
void XTPIC_DisableIRQ(uint8_t irqnum, uint16_t *irqmask);
void XTPIC_EnableIRQ(uint8_t irqnum, uint16_t *irqmask);
void XTPIC_AckIntr(uint8_t intnum, uint16_t *irqmask);
#endif

#endif /* !KERNEL_I8259A_H */
