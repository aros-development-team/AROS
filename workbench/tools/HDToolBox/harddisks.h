#ifndef HARDDISK_H
#define HARDDISK_H

#include <libraries/partition.h>

#include "gui.h"
#include "devices.h"
#include "partitions.h"

struct HDNode {
	struct HDTBPartition root_partition;
	LONG unit;
};

void findHDs(struct HDTBDevice *);
void freeHDList(struct List *);

#endif /* HARDDISK_H */
