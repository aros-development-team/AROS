#ifndef CHECKSUMS_H
#define CHECKSUMS_H

#include <exec/types.h>

#include "volumes.h"

ULONG calcChkSum(struct Volume *, ULONG *);

#endif
