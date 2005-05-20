#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <libraries/partition.h>

#include "list.h"
#include "deviceio.h"

char *listtemplate = "DEVICE/K/A,UNIT/K/N,PARTITION/K";
struct PartitionBase *PartitionBase;

/************************* list partition **********************************/
BOOL existsAttr(ULONG *list, ULONG attr) {

	while (*list)
	{
		if (*list == attr)
			return TRUE;
		list++;
	}
	return FALSE;
}

void printPartitionInfo(struct PartitionHandle *ph) {
ULONG *pattr;
struct TagItem tags[2];

	pattr = QueryPartitionAttrs(ph->root);
	tags[1].ti_Tag = TAG_DONE;
	/* get size */
	{
	struct DosEnvec de;
		tags[0].ti_Tag = PT_DOSENVEC;
		tags[0].ti_Data = (STACKIPTR)&de;
		GetPartitionAttrs(ph, tags);
		printf
		(
			"size: %lld\n",
			(
				(QUAD)(de.de_HighCyl-de.de_LowCyl+1)*de.de_Surfaces*
				(QUAD)de.de_BlocksPerTrack*(de.de_SizeBlock<<2)
			)
		);
	}
	if (existsAttr(pattr, PTA_TYPE))
	{
	struct PartitionType type;
	WORD i;

		tags[0].ti_Tag = PT_TYPE;
		tags[0].ti_Data = (STACKIPTR)&type;
		GetPartitionAttrs(ph, tags);
		printf("type: 0x");
		for (i=0;i<type.id_len;i++)
			printf("%02x", type.id[i]);
		printf("\n");
	}
	if (existsAttr(pattr, PTA_POSITION))
	{
	ULONG pos;

		tags[0].ti_Tag = PT_POSITION;
		tags[0].ti_Data = (STACKIPTR)&pos;
		GetPartitionAttrs(ph, tags);
		printf("position: %ld\n", pos);
	}
	if (existsAttr(pattr, PTA_ACTIVE))
	{
	ULONG active;

		tags[0].ti_Tag = PT_ACTIVE;
		tags[0].ti_Data = (STACKIPTR)&active;
		GetPartitionAttrs(ph, tags);
		printf("active: %ld\n", active);
	}
	if (existsAttr(pattr, PTA_NAME))
	{
	UBYTE name[32];

		tags[0].ti_Tag = PT_NAME;
		tags[0].ti_Data = (STACKIPTR)name;
		GetPartitionAttrs(ph, tags);
		printf("name: %s\n", name);
	}
	if (existsAttr(pattr, PTA_BOOTABLE))
	{
	ULONG ba;

		tags[0].ti_Tag = PT_ACTIVE;
		tags[0].ti_Data = (STACKIPTR)&ba;
		GetPartitionAttrs(ph, tags);
		printf("bootable: %ld\n", ba);
	}
	if (existsAttr(pattr, PTA_AUTOMOUNT))
	{
	ULONG am;

		tags[0].ti_Tag = PT_ACTIVE;
		tags[0].ti_Data = (STACKIPTR)&am;
		GetPartitionAttrs(ph, tags);
		printf("automount: %ld\n", am);
	}
}

LONG nextPartitionTable(struct PartitionHandle *part, STRPTR partition) {
LONG retval = RETURN_FAIL;
LONG pnum;
STRPTR newpos;
struct PartitionHandle *ph;

	pnum=strtol(partition, (char **)&newpos, 0);
	if (pnum>=0)
	{
		if (newpos != partition)
		{
			if (OpenPartitionTable(part)==0)
			{
				ph = (struct PartitionHandle *)part->table->list.lh_Head;
				while (ph->ln.ln_Succ)
				{
					if (pnum == 0)
						break;
					pnum--;
					ph = (struct PartitionHandle *)ph->ln.ln_Succ;
				}
				if (ph->ln.ln_Succ)
				{
					if (*newpos == ',')
						newpos++;
					if (*newpos == 0)
					{
						printPartitionInfo(ph);
						retval = RETURN_OK;
					}
					else
					{
						retval=nextPartitionTable(ph, newpos);
					}
				}
				else
					printf("partition not found\n");
				ClosePartitionTable(part);
			}
			else
				printf("no partition table\n");
		}
		else
			PrintFault(ERROR_BAD_NUMBER, NULL);
	}
	else
		PrintFault(ERROR_BAD_NUMBER, NULL);
	return retval;
}

LONG listPartition(STRPTR device, ULONG unit, STRPTR partition) {
LONG retval = RETURN_FAIL;
struct PartitionHandle *ph;

	PartitionBase = (struct PartitionBase *)OpenLibrary("partition.library", 1);
	if (PartitionBase)
	{
		ph = OpenRootPartition(device, unit);
		if (ph)
		{
			retval = nextPartitionTable(ph, partition);
			CloseRootPartition(ph);
		}
		else
			printf("Could not open root partition on device %s unit %ld\n", device, unit);
		CloseLibrary((struct Library *)PartitionBase);
	}
	return retval;
}

/************************* list partitions *********************************/
void printTable(struct PartitionHandle *root, WORD depth) {
struct PartitionHandle *ph;
struct TagItem tags[2];
ULONG type;
WORD i,j;

	tags[1].ti_Tag = TAG_DONE;
	if (OpenPartitionTable(root) == 0)
	{
		tags[0].ti_Tag = PTT_TYPE;
		tags[0].ti_Data = (STACKIPTR)&type;
		GetPartitionTableAttrs(root, tags);
		for (i=0;i<depth;i++) printf("\t");
		printf("Partition table type = %ld\n", type);
		ph = (struct PartitionHandle *)root->table->list.lh_Head;
		j = 0;
		while (ph->ln.ln_Succ)
		{
		struct PartitionType ptype;

			tags[0].ti_Tag = PT_TYPE;
			tags[0].ti_Data = (IPTR)&ptype;
			GetPartitionAttrs(ph, tags);
			for (i=0;i<(depth+1);i++) printf("\t");
			printf("Partition %d: type = 0x", j);
			for (i=0;i<ptype.id_len;i++)
				printf("%02x", ptype.id[i]);
			printf("\n");
			printTable(ph, depth+1);
			j++;
			ph = (struct PartitionHandle *)ph->ln.ln_Succ;
		}
		ClosePartitionTable(root);
	}
}

BOOL listPartitions(STRPTR device, ULONG unit) {
BOOL retval = RETURN_FAIL;
struct PartitionHandle *ph;

	PartitionBase = (struct PartitionBase *)OpenLibrary("partition.library", 1);
	if (PartitionBase)
	{
		ph = OpenRootPartition(device, unit);
		if (ph)
		{
			printTable(ph, 0);
			retval = RETURN_OK;
			CloseRootPartition(ph);
		}
		CloseLibrary((struct Library *)PartitionBase);
	}
	return retval;
}

/***************************** list HDs ************************************/
BOOL printHD(STRPTR device, ULONG unit) {
BOOL retval=FALSE;
char id[64];
struct DeviceIO dio;

	if (openIO(&dio, device, unit))
	{
		if (iscorrectType(dio.iotd))
		{
			printf("\tUnit=%ld: ", unit);
			if (identify(dio.iotd, id))
				printf("id\n");
			else
				printf("unknown\n");
		}
		closeIO(&dio);
		retval = TRUE;
	}
	return retval;
}

LONG listHDs(STRPTR device) {
LONG retval = RETURN_OK;
WORD i,max;

	if (strcmp(device, "ide.device") == 0)
		max=4;
	else if (strcmp(device, "scsi.devce") == 0)
		max=6;
	else
		max=1;
	printf("%s\n", device);
	for (i=0;i<max;i++)
	{
		if (!printHD(device, i))
		{
			retval = RETURN_FAIL;
			break;
		}
	}
	return retval;
}

LONG list(char *name, STRPTR args) {
BOOL retval = RETURN_FAIL;
IPTR myargs[]={0,0,0,0};
struct RDArgs *rdargs;
struct RDArgs rda = {{args, strlen(args), 0}, 0, 0, 0, NULL, 0};

	rda.RDA_Source.CS_Buffer[rda.RDA_Source.CS_Length]='\n';
	rdargs = ReadArgs(listtemplate,myargs, &rda);
	if (rdargs)
	{
		if (myargs[2])
		{
			retval = listPartition
				(
					(STRPTR)myargs[0],
					*(LONG *)myargs[1],
					(STRPTR)myargs[2]
				);
		}
		else
		{
			if (myargs[1])
				retval = listPartitions((STRPTR)myargs[0], *(LONG *)myargs[1]);
			else
				retval = listHDs((STRPTR)myargs[0]);
		}
		FreeArgs(rdargs);
	}
	else
		PrintFault(IoErr(), name);
	return retval;
}

