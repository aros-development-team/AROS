#include <exec/types.h>
#include <stddef.h>

#ifdef __AROS__
#include <aros/macros.h>
#else
#define AROS_BE2WORD(value) (value)
#define AROS_WORD2BE(value) (value)
#define AROS_BE2LONG(value) (value)
#endif

#include "blocks.h"
#include "endian.h"

static void ConvertWords(UWORD *values, ULONG count)
{
	while (count--)
	{
		*values = AROS_BE2WORD(*values);
		values++;
	}
}

static void ConvertLongs(ULONG *values, ULONG count)
{
	while (count--)
	{
		*values = AROS_BE2LONG(*values);
		values++;
	}
}

static void ConvertUnalignedWord(UBYTE *data)
{
	union
	{
		UWORD value;
		UBYTE bytes[sizeof(UWORD)];
	} unaligned;

	unaligned.bytes[0] = data[0];
	unaligned.bytes[1] = data[1];
	unaligned.value = AROS_BE2WORD(unaligned.value);
	data[0] = unaligned.bytes[0];
	data[1] = unaligned.bytes[1];
}

static void ConvertUnalignedLong(UBYTE *data)
{
	union
	{
		ULONG value;
		UBYTE bytes[sizeof(ULONG)];
	} unaligned;

	unaligned.bytes[0] = data[0];
	unaligned.bytes[1] = data[1];
	unaligned.bytes[2] = data[2];
	unaligned.bytes[3] = data[3];
	unaligned.value = AROS_BE2LONG(unaligned.value);
	data[0] = unaligned.bytes[0];
	data[1] = unaligned.bytes[1];
	data[2] = unaligned.bytes[2];
	data[3] = unaligned.bytes[3];
}

static BOOL ConvertBootBlock(UBYTE *data, ULONG bytes)
{
	struct bootblock *block = (struct bootblock *)data;

	if (bytes < sizeof(*block))
		return FALSE;

	block->disktype = AROS_BE2LONG(block->disktype);
	return TRUE;
}

static BOOL ConvertBitmapBlock(struct bitmapblock *block, ULONG bytes)
{
	if (bytes < sizeof(*block))
		return FALSE;

	block->id = AROS_BE2WORD(block->id);
	block->not_used = AROS_BE2WORD(block->not_used);
	block->datestamp = AROS_BE2LONG(block->datestamp);
	block->seqnr = AROS_BE2LONG(block->seqnr);
	ConvertLongs(block->bitmap, (bytes - sizeof(*block)) / sizeof(ULONG));
	return TRUE;
}

static BOOL ConvertRootBlock(UBYTE *data, ULONG bytes)
{
	struct rootblock *block = (struct rootblock *)data;

	if (bytes < sizeof(*block))
		return FALSE;

	block->disktype = AROS_BE2LONG(block->disktype);
	block->options = AROS_BE2LONG(block->options);
	block->datestamp = AROS_BE2LONG(block->datestamp);
	block->creationday = AROS_BE2WORD(block->creationday);
	block->creationminute = AROS_BE2WORD(block->creationminute);
	block->creationtick = AROS_BE2WORD(block->creationtick);
	block->protection = AROS_BE2WORD(block->protection);
	block->lastreserved = AROS_BE2LONG(block->lastreserved);
	block->firstreserved = AROS_BE2LONG(block->firstreserved);
	block->reserved_free = AROS_BE2LONG(block->reserved_free);
	block->reserved_blksize = AROS_BE2WORD(block->reserved_blksize);
	block->rblkcluster = AROS_BE2WORD(block->rblkcluster);
	block->blocksfree = AROS_BE2LONG(block->blocksfree);
	block->alwaysfree = AROS_BE2LONG(block->alwaysfree);
	block->roving_ptr = AROS_BE2LONG(block->roving_ptr);
	block->deldir = AROS_BE2LONG(block->deldir);
	block->disksize = AROS_BE2LONG(block->disksize);
	block->extension = AROS_BE2LONG(block->extension);
	block->not_used = AROS_BE2LONG(block->not_used);
	ConvertLongs(block->idx.large.bitmapindex,
		sizeof(block->idx) / sizeof(ULONG));

	if (bytes > sizeof(*block))
		return ConvertBitmapBlock((struct bitmapblock *)(block + 1),
			bytes - sizeof(*block));
	return TRUE;
}

static BOOL ConvertIndexBlock(struct indexblock *block, ULONG bytes)
{
	if (bytes < sizeof(*block))
		return FALSE;

	block->id = AROS_BE2WORD(block->id);
	block->not_used = AROS_BE2WORD(block->not_used);
	block->datestamp = AROS_BE2LONG(block->datestamp);
	block->seqnr = AROS_BE2LONG(block->seqnr);
	ConvertLongs((ULONG *)block->index,
		(bytes - sizeof(*block)) / sizeof(ULONG));
	return TRUE;
}

static BOOL ConvertAnodeBlock(struct anodeblock *block, ULONG bytes)
{
	struct anode *node;
	ULONG count;

	if (bytes < sizeof(*block))
		return FALSE;

	block->id = AROS_BE2WORD(block->id);
	block->not_used = AROS_BE2WORD(block->not_used);
	block->datestamp = AROS_BE2LONG(block->datestamp);
	block->seqnr = AROS_BE2LONG(block->seqnr);
	block->not_used_2 = AROS_BE2LONG(block->not_used_2);

	node = block->nodes;
	count = (bytes - sizeof(*block)) / sizeof(*node);
	while (count--)
	{
		node->clustersize = AROS_BE2LONG(node->clustersize);
		node->blocknr = AROS_BE2LONG(node->blocknr);
		node->next = AROS_BE2LONG(node->next);
		node++;
	}
	return TRUE;
}

static BOOL ValidateDirEntries(struct dirblock *block, ULONG bytes,
	BOOL recovery)
{
	struct direntry *entry;
	ULONG offset;

	offset = sizeof(*block);
	while (offset < bytes)
	{
		entry = (struct direntry *)((UBYTE *)block + offset);
		if (entry->next == 0)
			return TRUE;
		if (entry->next < sizeof(*entry) || entry->next > bytes - offset)
			return FALSE;
		if (!recovery &&
			((entry->next & 1) ||
			 offsetof(struct direntry, startofname) + entry->nlength >=
				entry->next))
			return FALSE;

		offset += entry->next;
	}
	return TRUE;
}

static void ConvertDirEntry(struct direntry *entry)
{
	UBYTE *data = (UBYTE *)entry;

	ConvertUnalignedLong(data + offsetof(struct direntry, anode));
	ConvertUnalignedLong(data + offsetof(struct direntry, fsize));
	ConvertUnalignedWord(data + offsetof(struct direntry, creationday));
	ConvertUnalignedWord(data + offsetof(struct direntry, creationminute));
	ConvertUnalignedWord(data + offsetof(struct direntry, creationtick));
}

static BOOL ConvertDirBlock(struct dirblock *block, ULONG bytes, BOOL recovery)
{
	struct direntry *entry;
	ULONG offset;

	if (bytes < sizeof(*block) ||
		!ValidateDirEntries(block, bytes, recovery))
		return FALSE;

	block->id = AROS_BE2WORD(block->id);
	block->not_used = AROS_BE2WORD(block->not_used);
	block->datestamp = AROS_BE2LONG(block->datestamp);
	ConvertWords(block->not_used_2, 2);
	block->anodenr = AROS_BE2LONG(block->anodenr);
	block->parent = AROS_BE2LONG(block->parent);

	offset = sizeof(*block);
	while (offset < bytes)
	{
		entry = (struct direntry *)((UBYTE *)block + offset);
		if (entry->next == 0)
			return TRUE;
		ConvertDirEntry(entry);
		offset += entry->next;
	}
	return TRUE;
}

#if DELDIR
static BOOL ConvertDeldirBlock(struct deldirblock *block, ULONG bytes)
{
	struct deldirentry *entry;
	ULONG count;

	if (bytes < sizeof(*block))
		return FALSE;

	block->id = AROS_BE2WORD(block->id);
	block->not_used = AROS_BE2WORD(block->not_used);
	block->datestamp = AROS_BE2LONG(block->datestamp);
	block->seqnr = AROS_BE2LONG(block->seqnr);
	ConvertWords(block->not_used_2, 2);
	block->not_used_3 = AROS_BE2WORD(block->not_used_3);
	block->uid = AROS_BE2WORD(block->uid);
	block->gid = AROS_BE2WORD(block->gid);
	block->protection = AROS_BE2LONG(block->protection);
	block->creationday = AROS_BE2WORD(block->creationday);
	block->creationminute = AROS_BE2WORD(block->creationminute);
	block->creationtick = AROS_BE2WORD(block->creationtick);

	entry = block->entries;
	count = (bytes - sizeof(*block)) / sizeof(*entry);
	while (count--)
	{
		entry->anodenr = AROS_BE2LONG(entry->anodenr);
		entry->fsize = AROS_BE2LONG(entry->fsize);
		entry->creationday = AROS_BE2WORD(entry->creationday);
		entry->creationminute = AROS_BE2WORD(entry->creationminute);
		entry->creationtick = AROS_BE2WORD(entry->creationtick);
		entry->fsizex = AROS_BE2WORD(entry->fsizex);
		entry++;
	}
	return TRUE;
}
#endif

#if VERSION23
static BOOL ConvertRootBlockExtension(struct rootblockextension *block,
	ULONG bytes)
{
	struct postponed_op *op;

	if (bytes < sizeof(*block))
		return FALSE;

	block->id = AROS_BE2WORD(block->id);
	block->not_used_1 = AROS_BE2WORD(block->not_used_1);
	block->ext_options = AROS_BE2LONG(block->ext_options);
	block->datestamp = AROS_BE2LONG(block->datestamp);
	block->pfs2version = AROS_BE2LONG(block->pfs2version);
	ConvertWords(block->root_date, 3);
	ConvertWords(block->volume_date, 3);
	op = &block->tobedone;
	op->operation_id = AROS_BE2LONG(op->operation_id);
	op->argument1 = AROS_BE2LONG(op->argument1);
	op->argument2 = AROS_BE2LONG(op->argument2);
	op->argument3 = AROS_BE2LONG(op->argument3);
	block->reserved_roving = AROS_BE2LONG(block->reserved_roving);
	block->rovingbit = AROS_BE2WORD(block->rovingbit);
	block->curranseqnr = AROS_BE2WORD(block->curranseqnr);
	block->deldirroving = AROS_BE2WORD(block->deldirroving);
	block->deldirsize = AROS_BE2WORD(block->deldirsize);
	block->fnsize = AROS_BE2WORD(block->fnsize);
	ConvertWords(block->not_used_2, 3);
	ConvertLongs(block->superindex, MAXSUPER + 1);
	block->dd_uid = AROS_BE2WORD(block->dd_uid);
	block->dd_gid = AROS_BE2WORD(block->dd_gid);
	block->dd_protection = AROS_BE2LONG(block->dd_protection);
	block->dd_creationday = AROS_BE2WORD(block->dd_creationday);
	block->dd_creationminute = AROS_BE2WORD(block->dd_creationminute);
	block->dd_creationtick = AROS_BE2WORD(block->dd_creationtick);
	block->not_used_3 = AROS_BE2WORD(block->not_used_3);
	ConvertLongs(block->deldir, 32);
	return TRUE;
}
#endif

static UWORD DiskBlockId(const UBYTE *data)
{
	return ((UWORD)data[0] << 8) | data[1];
}

static BOOL ConvertReservedBlock(UBYTE *data, ULONG bytes, BOOL from_disk,
	BOOL recovery)
{
	UWORD id;

	if (bytes < sizeof(UWORD))
		return FALSE;

	id = from_disk ? DiskBlockId(data) : *(UWORD *)data;
	switch (id)
	{
	case BMBLKID:
		return ConvertBitmapBlock((struct bitmapblock *)data, bytes);
	case BMIBLKID:
	case IBLKID:
	case SBLKID:
		return ConvertIndexBlock((struct indexblock *)data, bytes);
	case ABLKID:
		return ConvertAnodeBlock((struct anodeblock *)data, bytes);
	case DBLKID:
		return ConvertDirBlock((struct dirblock *)data, bytes, recovery);
#if DELDIR
	case DELDIRID:
		return ConvertDeldirBlock((struct deldirblock *)data, bytes);
#endif
#if VERSION23
	case EXTENSIONID:
		return ConvertRootBlockExtension(
			(struct rootblockextension *)data, bytes);
#endif
	default:
		return FALSE;
	}
}

static BOOL ConvertMetadata(UBYTE *data, ULONG bytes,
	enum pfs3_metadata_type type, BOOL from_disk, BOOL recovery)
{
	switch (type)
	{
	case PFS3_METADATA_BOOT:
		return ConvertBootBlock(data, bytes);
	case PFS3_METADATA_ROOT:
		return ConvertRootBlock(data, bytes);
	case PFS3_METADATA_RESERVED:
		return ConvertReservedBlock(data, bytes, from_disk, recovery);
	}
	return FALSE;
}

BOOL PFS3MetadataToHost(UBYTE *data, ULONG bytes, enum pfs3_metadata_type type)
{
	return ConvertMetadata(data, bytes, type, TRUE, FALSE);
}

BOOL PFS3MetadataToHostForRecovery(UBYTE *data, ULONG bytes,
	enum pfs3_metadata_type type)
{
	return ConvertMetadata(data, bytes, type, TRUE, TRUE);
}

BOOL PFS3MetadataToDisk(UBYTE *data, ULONG bytes, enum pfs3_metadata_type type)
{
	return ConvertMetadata(data, bytes, type, FALSE, FALSE);
}

BOOL PFS3MetadataToDiskForRecovery(UBYTE *data, ULONG bytes,
	enum pfs3_metadata_type type)
{
	return ConvertMetadata(data, bytes, type, FALSE, TRUE);
}

UWORD PFS3GetExtraFields(struct direntry *direntry,
	struct extrafields *extrafields)
{
	UWORD values[11] = { 0 };
	UWORD *source = (UWORD *)((UBYTE *)direntry + direntry->next);
	UWORD flags, i;

	flags = AROS_BE2WORD(*(--source));
	for (i = 0; i < sizeof(values) / sizeof(values[0]); i++, flags >>= 1)
		values[i] = (flags & 1) ? AROS_BE2WORD(*(--source)) : 0;

	extrafields->link = ((ULONG)values[0] << 16) | values[1];
	extrafields->uid = values[2];
	extrafields->gid = values[3];
	extrafields->prot = ((ULONG)values[4] << 16) | values[5] |
		direntry->protection;
	extrafields->virtualsize = ((ULONG)values[6] << 16) | values[7];
	extrafields->rollpointer = ((ULONG)values[8] << 16) | values[9];
	extrafields->fsizex = values[10];
	return flags;
}

void PFS3AddExtraFields(struct direntry *direntry,
	struct extrafields *extrafields)
{
	UWORD offset, *destination;
	UWORD fields[11];
	UWORD values[11];
	UWORD i, count = 0;
	UWORD flags = 0, flag = 1;

	extrafields->prot &= 0xffffff00;
	fields[0] = extrafields->link >> 16;
	fields[1] = extrafields->link;
	fields[2] = extrafields->uid;
	fields[3] = extrafields->gid;
	fields[4] = extrafields->prot >> 16;
	fields[5] = extrafields->prot;
	fields[6] = extrafields->virtualsize >> 16;
	fields[7] = extrafields->virtualsize;
	fields[8] = extrafields->rollpointer >> 16;
	fields[9] = extrafields->rollpointer;
	fields[10] = extrafields->fsizex;

	offset = (sizeof(*direntry) + direntry->nlength +
		*((UBYTE *)&direntry->startofname + direntry->nlength)) & 0xfffe;
	destination = (UWORD *)((UBYTE *)direntry + offset);

	for (i = 0; i < sizeof(fields) / sizeof(fields[0]); i++)
	{
		if (fields[i])
		{
			values[count++] = fields[i];
			flags |= flag;
		}
		flag <<= 1;
	}

	i = count;
	while (i)
		*destination++ = AROS_WORD2BE(values[--i]);
	*destination = AROS_WORD2BE(flags);
	direntry->next = offset + 2 * count + 2;
}
