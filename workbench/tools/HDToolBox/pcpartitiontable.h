/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PC_PARTITION_TABLE_H
#define PC_PARTITION_TABLE_H

struct PCPartitionTable {
	unsigned char status;
	unsigned char start_head;
	unsigned char start_sector;
	unsigned char start_cylinder;
	unsigned char type;
	unsigned char end_head;
	unsigned char end_sector;
	unsigned char end_cylinder;
	LONG first_sector;
	LONG count_sector;
};

struct MBR {
	char boot_data[0x1BE];
#warning "I don't want that f...ing alignment here!!!"
	struct PCPartitionTable pcpt[4];
	char pad[2];
};

#endif

