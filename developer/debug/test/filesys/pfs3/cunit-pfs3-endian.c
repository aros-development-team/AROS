#include <exec/types.h>
#include <string.h>

#include <CUnit/Automated.h>
#include <CUnit/Basic.h>

#include "blocks.h"
#include "endian.h"

void TestListTypeLayout(void);

#define BLOCK_BYTES_1K 1024
#define BLOCK_BYTES_2K 2048
#define BLOCK_BYTES_4K 4096

static void AssertRoundTrip(UBYTE *data, ULONG bytes,
	enum pfs3_metadata_type type)
{
	UBYTE original[BLOCK_BYTES_4K];

	CU_ASSERT_TRUE_FATAL(bytes <= sizeof(original));
	memcpy(original, data, bytes);
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToDisk(data, bytes, type));
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToHost(data, bytes, type));
	CU_ASSERT_EQUAL(memcmp(data, original, bytes), 0);
}

static void TestBootBlock(void)
{
	UBYTE data[512];
	struct bootblock *block = (struct bootblock *)data;

	memset(data, 0xa5, sizeof(data));
	block->disktype = ID_PFS_DISK;
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToDisk(data, sizeof(data),
		PFS3_METADATA_BOOT));
	CU_ASSERT_EQUAL(data[0], 'P');
	CU_ASSERT_EQUAL(data[1], 'F');
	CU_ASSERT_EQUAL(data[2], 'S');
	CU_ASSERT_EQUAL(data[3], 1);
	CU_ASSERT_EQUAL(data[4], 0xa5);
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToHost(data, sizeof(data),
		PFS3_METADATA_BOOT));
	CU_ASSERT_EQUAL(block->disktype, ID_PFS_DISK);
}

static void TestRootBlock(void)
{
	UBYTE data[BLOCK_BYTES_1K];
	struct rootblock *root = (struct rootblock *)data;
	struct bitmapblock *bitmap;

	memset(data, 0, sizeof(data));
	root->disktype = ID_PFS2_DISK;
	root->options = 0x12345678;
	root->datestamp = 0x11223344;
	root->creationday = 0x1357;
	root->creationminute = 0x2468;
	root->creationtick = 0x369c;
	root->lastreserved = 0x01020304;
	root->reserved_blksize = BLOCK_BYTES_1K;
	root->rblkcluster = 2;
	root->idx.large.bitmapindex[0] = 0x89abcdef;

	bitmap = (struct bitmapblock *)(root + 1);
	bitmap->id = BMBLKID;
	bitmap->datestamp = 0x55667788;
	bitmap->seqnr = 0x10203040;
	bitmap->bitmap[0] = 0x12345678;

	CU_ASSERT_TRUE_FATAL(PFS3MetadataToDisk(data, sizeof(data),
		PFS3_METADATA_ROOT));
	CU_ASSERT_EQUAL(data[0], 'P');
	CU_ASSERT_EQUAL(data[1], 'F');
	CU_ASSERT_EQUAL(data[2], 'S');
	CU_ASSERT_EQUAL(data[3], 2);
	CU_ASSERT_EQUAL(data[4], 0x12);
	CU_ASSERT_EQUAL(data[5], 0x34);
	CU_ASSERT_EQUAL(data[sizeof(*root)], 'B');
	CU_ASSERT_EQUAL(data[sizeof(*root) + 1], 'M');
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToHost(data, sizeof(data),
		PFS3_METADATA_ROOT));
	CU_ASSERT_EQUAL(root->options, 0x12345678);
	CU_ASSERT_EQUAL(root->idx.large.bitmapindex[0], 0x89abcdef);
	CU_ASSERT_EQUAL(bitmap->bitmap[0], 0x12345678);
}

static void TestBitmapSizes(void)
{
	ULONG sizes[] = { BLOCK_BYTES_1K, BLOCK_BYTES_2K, BLOCK_BYTES_4K };
	UBYTE data[BLOCK_BYTES_4K];
	ULONG i;

	for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++)
	{
		struct bitmapblock *block = (struct bitmapblock *)data;
		ULONG count = (sizes[i] - sizeof(*block)) / sizeof(ULONG);

		memset(data, 0, sizes[i]);
		block->id = BMBLKID;
		block->datestamp = 0x10203040;
		block->seqnr = 0x50607080;
		block->bitmap[0] = 0x12345678;
		block->bitmap[count - 1] = 0x89abcdef;
		AssertRoundTrip(data, sizes[i], PFS3_METADATA_RESERVED);
	}
}

static void TestIndexAndAnodeBlocks(void)
{
	UBYTE index_data[BLOCK_BYTES_2K];
	UBYTE anode_data[BLOCK_BYTES_4K];
	struct indexblock *index = (struct indexblock *)index_data;
	struct anodeblock *anodes = (struct anodeblock *)anode_data;
	ULONG count;

	memset(index_data, 0, sizeof(index_data));
	index->id = IBLKID;
	index->datestamp = 0x10203040;
	index->seqnr = 0x11223344;
	count = (sizeof(index_data) - sizeof(*index)) / sizeof(ULONG);
	index->index[0] = 0x12345678;
	index->index[count - 1] = 0x89abcdef;
	AssertRoundTrip(index_data, sizeof(index_data), PFS3_METADATA_RESERVED);

	memset(anode_data, 0, sizeof(anode_data));
	anodes->id = ABLKID;
	anodes->datestamp = 0x50607080;
	anodes->seqnr = 0x11223344;
	anodes->nodes[0].clustersize = 0x01020304;
	anodes->nodes[0].blocknr = 0x12345678;
	anodes->nodes[0].next = 0x89abcdef;
	AssertRoundTrip(anode_data, sizeof(anode_data),
		PFS3_METADATA_RESERVED);
}

static void TestFixedReservedBlocks(void)
{
	UBYTE data[BLOCK_BYTES_1K];
	struct deldirblock *deldir = (struct deldirblock *)data;
	struct rootblockextension *extension =
		(struct rootblockextension *)data;

	memset(data, 0, sizeof(data));
	deldir->id = DELDIRID;
	deldir->datestamp = 0x10203040;
	deldir->seqnr = 0x11223344;
	deldir->uid = 0x1357;
	deldir->gid = 0x2468;
	deldir->protection = 0x55667788;
	deldir->entries[0].anodenr = 0x12345678;
	deldir->entries[0].fsize = 0x89abcdef;
	deldir->entries[0].fsizex = 0x369c;
	AssertRoundTrip(data, sizeof(data), PFS3_METADATA_RESERVED);

	memset(data, 0, sizeof(data));
	extension->id = EXTENSIONID;
	extension->ext_options = 0x10203040;
	extension->datestamp = 0x11223344;
	extension->pfs2version = 0x55667788;
	extension->root_date[0] = 0x1357;
	extension->tobedone.operation_id = 0x12345678;
	extension->tobedone.argument3 = 0x89abcdef;
	extension->reserved_roving = 0x50607080;
	extension->superindex[0] = 0x01020304;
	extension->superindex[MAXSUPER] = 0x90abcdef;
	extension->dd_protection = 0xa1b2c3d4;
	extension->deldir[31] = 0x18273645;
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToDisk(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(data[0], 'E');
	CU_ASSERT_EQUAL(data[1], 'X');
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToHost(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(extension->ext_options, 0x10203040);
	CU_ASSERT_EQUAL(extension->superindex[MAXSUPER], 0x90abcdef);
	CU_ASSERT_EQUAL(extension->deldir[31], 0x18273645);
}

static void TestDirectoryEntriesAndExtras(void)
{
	UBYTE data[BLOCK_BYTES_1K];
	UBYTE original[BLOCK_BYTES_1K];
	struct dirblock *block = (struct dirblock *)data;
	struct direntry *entry;
	struct extrafields input, output;
	UBYTE *packed;

	memset(data, 0, sizeof(data));
	block->id = DBLKID;
	block->datestamp = 0x10203040;
	block->anodenr = 0x11223344;
	block->parent = 0x55667788;
	entry = (struct direntry *)block->entries;
	entry->type = 2;
	entry->anode = 0x12345678;
	entry->fsize = 0x89abcdef;
	entry->creationday = 0x1357;
	entry->creationminute = 0x2468;
	entry->creationtick = 0x369c;
	entry->protection = 0x5a;
	entry->nlength = 1;
	*(UBYTE *)&entry->startofname = 'x';
	*((UBYTE *)&entry->startofname + 1) = 0;

	memset(&input, 0, sizeof(input));
	input.link = 0x10203040;
	input.uid = 0x1357;
	input.gid = 0x2468;
	input.prot = 0xabcdef5a;
	input.virtualsize = 0x11223344;
	input.rollpointer = 0x55667788;
	input.fsizex = 0x369c;
	PFS3AddExtraFields(entry, &input);
	memset(&output, 0, sizeof(output));
	CU_ASSERT_EQUAL(PFS3GetExtraFields(entry, &output), 0);
	CU_ASSERT_EQUAL(output.link, 0x10203040);
	CU_ASSERT_EQUAL(output.uid, 0x1357);
	CU_ASSERT_EQUAL(output.gid, 0x2468);
	CU_ASSERT_EQUAL(output.prot, 0xabcdef5a);
	CU_ASSERT_EQUAL(output.virtualsize, 0x11223344);
	CU_ASSERT_EQUAL(output.rollpointer, 0x55667788);
	CU_ASSERT_EQUAL(output.fsizex, 0x369c);

	packed = (UBYTE *)entry + sizeof(*entry);
	CU_ASSERT_EQUAL(packed[0], 0x36);
	CU_ASSERT_EQUAL(packed[1], 0x9c);
	CU_ASSERT_EQUAL(packed[18], 0x30);
	CU_ASSERT_EQUAL(packed[19], 0x40);
	CU_ASSERT_EQUAL(packed[20], 0x10);
	CU_ASSERT_EQUAL(packed[21], 0x20);
	CU_ASSERT_EQUAL(packed[22], 0x07);
	CU_ASSERT_EQUAL(packed[23], 0xff);

	memcpy(original, data, sizeof(data));
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToDisk(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(data[0], 'D');
	CU_ASSERT_EQUAL(data[1], 'B');
	CU_ASSERT_EQUAL(data[sizeof(*block) + 2], 0x12);
	CU_ASSERT_EQUAL(data[sizeof(*block) + 3], 0x34);
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToHost(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(memcmp(data, original, sizeof(data)), 0);
}

static void TestMalformedDirectoryEntry(void)
{
	UBYTE data[BLOCK_BYTES_1K];
	UBYTE original[BLOCK_BYTES_1K];
	struct dirblock *block = (struct dirblock *)data;
	struct direntry *entry;

	memset(data, 0, sizeof(data));
	block->id = DBLKID;
	entry = (struct direntry *)block->entries;
	entry->next = 3;
	memcpy(original, data, sizeof(data));
	CU_ASSERT_FALSE(PFS3MetadataToDisk(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(memcmp(data, original, sizeof(data)), 0);
}

static void TestDirectoryEntryWithoutTerminator(void)
{
	UBYTE data[BLOCK_BYTES_1K];
	UBYTE original[BLOCK_BYTES_1K];
	struct dirblock *block = (struct dirblock *)data;
	UBYTE lengths[] = { 254, 250, 250, 250 };
	ULONG offset = sizeof(*block);
	ULONG i;

	memset(data, 0, sizeof(data));
	block->id = DBLKID;
	for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++)
	{
		struct direntry *entry = (struct direntry *)(data + offset);
		entry->next = lengths[i];
		entry->nlength = 0;
		*(UBYTE *)&entry->startofname = 0;
		offset += lengths[i];
	}
	CU_ASSERT_EQUAL(offset, sizeof(data));
	memcpy(original, data, sizeof(data));

	CU_ASSERT_FALSE(PFS3MetadataToDisk(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(memcmp(data, original, sizeof(data)), 0);
	CU_ASSERT_FALSE(PFS3MetadataToDiskForRecovery(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(memcmp(data, original, sizeof(data)), 0);
}

static void TestMalformedExtraFields(void)
{
	UBYTE data[BLOCK_BYTES_1K];
	UBYTE original[BLOCK_BYTES_1K];
	struct dirblock *block = (struct dirblock *)data;
	struct direntry *entry;
	struct extrafields extra;

	memset(data, 0, sizeof(data));
	block->id = DBLKID;
	entry = (struct direntry *)block->entries;
	entry->next = sizeof(*entry) + sizeof(UWORD);
	entry->nlength = 0;
	*(UBYTE *)&entry->startofname = 0;
	data[sizeof(*block) + entry->next - 2] = 0xff;
	data[sizeof(*block) + entry->next - 1] = 0xff;
	memcpy(original, data, sizeof(data));

	CU_ASSERT_FALSE(PFS3MetadataToDisk(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(memcmp(data, original, sizeof(data)), 0);
	memset(&extra, 0xa5, sizeof(extra));
	CU_ASSERT_EQUAL(PFS3GetExtraFields(entry, &extra), 0xffff);
	CU_ASSERT_EQUAL(extra.link, 0);
	CU_ASSERT_EQUAL(extra.uid, 0);
	CU_ASSERT_EQUAL(extra.gid, 0);
	CU_ASSERT_EQUAL(extra.prot, 0);
	CU_ASSERT_EQUAL(extra.virtualsize, 0);
	CU_ASSERT_EQUAL(extra.rollpointer, 0);
	CU_ASSERT_EQUAL(extra.fsizex, 0);
}

static void TestRecoveryDirectoryEntry(void)
{
	UBYTE data[BLOCK_BYTES_1K];
	UBYTE original[BLOCK_BYTES_1K];
	struct dirblock *block = (struct dirblock *)data;
	struct direntry *entry;
	ULONG entry_offset = offsetof(struct dirblock, entries);

	memset(data, 0, sizeof(data));
	data[0] = 'D';
	data[1] = 'B';
	data[offsetof(struct dirblock, anodenr)] = 0x11;
	data[offsetof(struct dirblock, anodenr) + 1] = 0x22;
	data[offsetof(struct dirblock, anodenr) + 2] = 0x33;
	data[offsetof(struct dirblock, anodenr) + 3] = 0x44;
	data[entry_offset] = sizeof(*entry) + 1;
	data[entry_offset + offsetof(struct direntry, anode)] = 0x12;
	data[entry_offset + offsetof(struct direntry, anode) + 1] = 0x34;
	data[entry_offset + offsetof(struct direntry, anode) + 2] = 0x56;
	data[entry_offset + offsetof(struct direntry, anode) + 3] = 0x78;
	data[entry_offset + offsetof(struct direntry, nlength)] = 0xff;
	memcpy(original, data, sizeof(data));

	CU_ASSERT_FALSE(PFS3MetadataToHost(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToHostForRecovery(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(block->anodenr, 0x11223344);
	entry = (struct direntry *)block->entries;
	CU_ASSERT_EQUAL(entry->anode, 0x12345678);
	CU_ASSERT_TRUE_FATAL(PFS3MetadataToDiskForRecovery(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(memcmp(data, original, sizeof(data)), 0);
}

static void TestUnsafeRecoveryDirectoryEntry(void)
{
	UBYTE data[64];
	UBYTE original[64];
	ULONG entry_offset = offsetof(struct dirblock, entries);

	memset(data, 0, sizeof(data));
	data[0] = 'D';
	data[1] = 'B';
	data[entry_offset] = sizeof(data);
	memcpy(original, data, sizeof(data));

	CU_ASSERT_FALSE(PFS3MetadataToHostForRecovery(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_EQUAL(memcmp(data, original, sizeof(data)), 0);
}

static void TestUnknownReservedBlock(void)
{
	UBYTE data[BLOCK_BYTES_1K];

	memset(data, 0, sizeof(data));
	CU_ASSERT_FALSE(PFS3MetadataToHost(data, sizeof(data),
		PFS3_METADATA_RESERVED));
	CU_ASSERT_FALSE(PFS3MetadataToDisk(data, sizeof(data),
		PFS3_METADATA_RESERVED));
}

int main(void)
{
	CU_pSuite suite;

	if (CU_initialize_registry() != CUE_SUCCESS)
		return CU_get_error();

	suite = CU_add_suite("PFS3 endian conversion", NULL, NULL);
	if (suite == NULL ||
		CU_add_test(suite, "boot block", TestBootBlock) == NULL ||
		CU_add_test(suite, "root block", TestRootBlock) == NULL ||
		CU_add_test(suite, "bitmap block sizes", TestBitmapSizes) == NULL ||
		CU_add_test(suite, "index and anode blocks",
			TestIndexAndAnodeBlocks) == NULL ||
		CU_add_test(suite, "fixed reserved blocks",
			TestFixedReservedBlocks) == NULL ||
		CU_add_test(suite, "directory entries and extras",
			TestDirectoryEntriesAndExtras) == NULL ||
		CU_add_test(suite, "malformed directory entry",
			TestMalformedDirectoryEntry) == NULL ||
		CU_add_test(suite, "directory entry without terminator",
			TestDirectoryEntryWithoutTerminator) == NULL ||
		CU_add_test(suite, "malformed extra fields",
			TestMalformedExtraFields) == NULL ||
		CU_add_test(suite, "recovery directory entry",
			TestRecoveryDirectoryEntry) == NULL ||
		CU_add_test(suite, "unsafe recovery directory entry",
			TestUnsafeRecoveryDirectoryEntry) == NULL ||
		CU_add_test(suite, "unknown reserved block",
			TestUnknownReservedBlock) == NULL ||
		CU_add_test(suite, "list type layout",
			TestListTypeLayout) == NULL)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_basic_set_mode(CU_BRM_SILENT);
	CU_automated_package_name_set("PFS3EndianUnitTests");
	CU_set_output_filename("PFS3Endian");
	CU_automated_enable_junit_xml(CU_TRUE);
	CU_automated_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
