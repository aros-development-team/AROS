#include <stdio.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <libraries/partition.h>
#include <utility/tagitem.h>

struct PartitionBase *PartitionBase;

void PrintDE(struct DosEnvec *de, ULONG i) {
ULONG a;

	for (a=i;a;a--)
		printf("  ");
	printf("SizeBlock = %ld\n", de->de_SizeBlock<<2);
	for (a=i;a;a--)
		printf("  ");
	printf("Surfaces = %ld\n", de->de_Surfaces);
	for (a=i;a;a--)
		printf("  ");
	printf("BlocksPerTrack = %ld\n", de->de_BlocksPerTrack);
	for (a=i;a;a--)
		printf("  ");
	printf("LowCyl = %ld\n", de->de_LowCyl);
	for (a=i;a;a--)
		printf("  ");
	printf("HighCyl = %ld\n", de->de_HighCyl);
}

LONG GetPartitionAttrsA(struct PartitionHandle *ph, LONG tag, ...) {

	return GetPartitionAttrs(ph, (struct TagItem *)&tag);
}

void PrintPInfo(struct PartitionHandle *ph, ULONG i) {
struct DosEnvec de;
UBYTE name[32];
LONG type;
ULONG a;

	GetPartitionAttrsA
	(
		ph,
		PT_DOSENVEC, &de,
		PT_NAME, name,
		PT_TYPE, &type,
		TAG_DONE
	);
	for (a=i;a;a--)
		printf("  ");
	printf("name: %s\n", name);
	for (a=i+1;a;a--)
		printf("  ");
	printf("type: %lx\n", type);
	PrintDE(&de, i+1);
}

void PrintPartitions(struct PartitionHandle *root, ULONG i) {
struct PartitionHandle *ph;

	ph = (struct PartitionHandle *)root->table->list.lh_Head;
	while (ph->ln.ln_Succ)
	{
		PrintPInfo(ph, i);
		ph = (struct PartitionHandle *)ph->ln.ln_Succ;
	}
}

LONG GetPartitionTableAttrsA(struct PartitionHandle *ph, LONG tag, ...) {
    //FIXME: buggy
	return GetPartitionTableAttrs(ph, (struct TagItem *) &tag);
}

void PrintPartitionTable(struct PartitionHandle *root, ULONG i) {
struct DosEnvec de;
ULONG type;
ULONG reserved;
ULONG a;

	if (OpenPartitionTable(root)==0)
	{
		GetPartitionTableAttrsA
		(
			root,
			PTT_TYPE, &type,
			PTT_RESERVED, &reserved,
			TAG_DONE
		);
		GetPartitionAttrsA(root, PT_DOSENVEC, &de, TAG_DONE);
		for (a=i;a;a--)
			printf("  ");
		printf("Partition type is ");
		switch (type)
		{
		case PHPTT_UNKNOWN:
			printf("unknown\n");
			break;
		case PHPTT_RDB:
			printf("Rigid Disk Block\n");
			break;
		case PHPTT_MBR:
			printf("MBR -> PC\n");
			break;
		}
		for (a=i;a;a--)
			printf("  ");
		printf("reserved blocks: %ld\n", reserved);
		PrintDE(&de,i);
		for (a=i;a;a--)
			printf("  ");
		printf("partitions:\n");
		PrintPartitions(root,i+1);
		ClosePartitionTable(root);
	}
	else
		printf("Couldn't read partition table\n");
}

int main(void) {
struct PartitionHandle *root;
char *device = "fdsk.device";
ULONG unit = 1;

	PartitionBase = (struct PartitionBase *)OpenLibrary("partition.library", 1);
	if (PartitionBase)
	{
		root = OpenRootPartition(device, unit);
		if (root)
		{
			printf("got root handle of %s unit %ld\n", device, unit);
			PrintPartitionTable(root, 0);
			CloseRootPartition(root);
		}
		else
			printf("No root handle\n");
		CloseLibrary((struct Library *)PartitionBase);
	}
	else
		printf("No partition.library\n");

    return 0;
}

