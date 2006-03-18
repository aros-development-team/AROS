#ifndef RADEON_BIOS_H_
#define RADEON_BIOS_H_

#include "ati.h"
#include "radeon.h"
#include "radeon_reg.h"
#include "radeon_macros.h"

BOOL RADEONGetBIOSInfo(struct ati_staticdata *sd);
BOOL RADEONGetConnectorInfoFromBIOS(struct ati_staticdata *sd);
BOOL RADEONGetClockInfoFromBIOS(struct ati_staticdata *sd);

#endif /*RADEON_BIOS_H_*/
