/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef WORKMAIN_H
#define WORKMAIN_H

#include <exec/nodes.h>
#include <devices/trackdisk.h>
#include <intuition/intuition.h>

struct HDUnitNode {
	struct Node ln;
	struct MsgPort *ioport;
	struct IOExtTD *ioreq;
	struct DriveGeometry geometry;
	char mbr[512];
	char mbr_copy[512];
	BOOL partition_changed;
};

void findHDs(struct Window *, char *, int);
struct HDUnitNode *getHDUnit(struct Window *, int);
BOOL reallyExit(struct List *);
void freeHDList(struct List *);
void saveChanges(struct Window *, struct HDUnitNode *);

#endif

