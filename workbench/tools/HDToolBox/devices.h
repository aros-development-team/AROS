#ifndef DEVICES_H
#define DEVICES_H

#include "gui.h"

struct HDTBDevice {
	struct ListNode listnode;
};

struct HDTBDevice *addDeviceName(STRPTR);
void freeDeviceNode(struct HDTBDevice *);
void freeDeviceList(void);

#endif /* DEVICES_H */

