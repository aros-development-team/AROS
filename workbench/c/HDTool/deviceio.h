#ifndef DEVICEIO_H
#define DEVICEIO_H

#include <devices/trackdisk.h>
#include <exec/ports.h>

struct DeviceIO {
	struct IOExtTD *iotd;
	struct MsgPort *mp;
};

BOOL openIO(struct DeviceIO *, STRPTR, ULONG);
void closeIO(struct DeviceIO *);
BOOL iscorrectType(struct IOExtTD *);
BOOL identify(struct IOExtTD *, STRPTR);

#endif /* DEVICEIO_H */

