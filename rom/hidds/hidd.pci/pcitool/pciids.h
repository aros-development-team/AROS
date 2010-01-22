#include <exec/types.h>

void pciids_Open(void);
void pciids_Close(void);
STRPTR pciids_GetVendorName(UWORD vendorID, STRPTR buf, ULONG bufsize);
STRPTR pciids_GetDeviceName(UWORD vendorID, UWORD deviceID, STRPTR buf,
			    ULONG bufsize);
STRPTR pciids_GetSubDeviceName(UWORD vendorID, UWORD deviceID, UWORD subVendorID,
			       UWORD subDeviceID, STRPTR buf, ULONG bufsize);

