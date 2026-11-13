#ifndef PTCLASS_H
#define PTCLASS_H

#include <utility/tagitem.h>

#define PTCT_PartitionTable  (TAG_USER+1)
#define PTCT_ActivePartition (TAG_USER+2)
#define PTCT_ActiveType      (TAG_USER+3)
#define PTCT_Selected        (TAG_USER+4)
#define PTCT_PartitionMove   (TAG_USER+5)
#define PTCT_Flags           (TAG_USER+6)

#define PTCTF_NoPartitionMove (1<<0)
#define PTCTF_EmptySelectOnly (1<<1)

#define PTS_NOTHING    (0)
#define PTS_EMPTY_AREA (1)
#define PTS_PARTITION  (2)

Class *makePTClass(void);

#endif
