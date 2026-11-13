#ifndef RADEON_ACCEL_H_
#define RADEON_ACCEL_H_

#include "ati.h"

struct __ROP {
    int rop;
    int pattern;
};

extern struct __ROP RADEON_ROP[];

void RADEONWaitForFifoFunction(struct ati_staticdata *sd, int entries);
void RADEONWaitForIdleMMIO(struct ati_staticdata *sd);
void RADEONEngineFlush(struct ati_staticdata *sd);
void RADEONEngineReset(struct ati_staticdata *sd);
void RADEONEngineRestore(struct ati_staticdata *sd);

#endif /*RADEON_ACCEL_H_*/
