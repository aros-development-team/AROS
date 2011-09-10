#include <inttypes.h>

void XTPIC_Init(uint16_t *irqmask);
void XTPIC_DisableIRQ(uint8_t irqnum, uint16_t *irqmask);
void XTPIC_EnableIRQ(uint8_t irqnum, uint16_t *irqmask);
void XTPIC_AckIntr(uint8_t intnum, uint16_t *irqmask);
