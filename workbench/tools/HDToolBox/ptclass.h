#ifndef PTCLASS_H
#define PTCLASS_H

#include <utility/tagitem.h>

#define PTCT_PartitionTable (TAG_USER+1)
#define PTCT_Selected       (TAG_USER+2)

#define PTS_EMPTY_AREA (1)
#define PTS_PARTITION  (2)

Class *makePTClass(void);

#endif
