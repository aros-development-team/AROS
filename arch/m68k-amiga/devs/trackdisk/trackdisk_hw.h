#ifndef  TRACKDISK_HW_H
#define  TRACKDISK_HW_H

/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hardware defs for trackdisk
    Lang: English
*/

#include <exec/types.h>

#define DISK_BUFFERSIZE 13630

/* Prototypes */
UBYTE td_getdir(struct TDU*, struct TrackDiskBase*);
void td_motoron(struct TDU*,struct TrackDiskBase *,BOOL);
void td_motoroff(struct TDU*,struct TrackDiskBase *);
UBYTE td_getprotstatus(struct TDU*,struct TrackDiskBase *);
BOOL td_recalibrate(struct TDU*, struct TrackDiskBase *);
UBYTE td_seek(struct TDU*, int, int, struct TrackDiskBase *);
UBYTE td_read(struct IOExtTD *, struct TDU*, struct TrackDiskBase *);
UBYTE td_write(struct IOExtTD *, struct TDU*, struct TrackDiskBase *);
UBYTE td_format(struct IOExtTD *, struct TDU*, struct TrackDiskBase *);
UBYTE td_getDiskChange(struct TDU*, struct TrackDiskBase*);
void td_select(struct TDU *tdu, struct TrackDiskBase *tdb);
void td_deselect(struct TDU *tdu, struct TrackDiskBase *tdb);
UBYTE td_flush(struct TDU *tdu, struct TrackDiskBase *tdb);
void td_clear(struct TrackDiskBase *tdb);
void td_detectformat(struct TDU*, struct TrackDiskBase *);

#endif /* TRACKDISK_HW_H */
















