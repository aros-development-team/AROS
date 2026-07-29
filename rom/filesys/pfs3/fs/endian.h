#ifndef PFS3_ENDIAN_H
#define PFS3_ENDIAN_H

#include <exec/types.h>

struct direntry;
struct extrafields;

enum pfs3_metadata_type
{
	PFS3_METADATA_BOOT,
	PFS3_METADATA_ROOT,
	PFS3_METADATA_RESERVED
};

UWORD PFS3DiskBlockId(const UBYTE *data);

/*
 * Convert metadata between its native in-memory representation and the
 * big-endian representation used on disk. Both operations are in-place.
 */
BOOL PFS3MetadataToHost(UBYTE *data, ULONG bytes, enum pfs3_metadata_type type);
/*
 * Convert metadata for a recovery tool. Directory entries are required to be
 * safely traversable, but semantic errors which PFSDoctor can repair are
 * accepted.
 */
BOOL PFS3MetadataToHostForRecovery(UBYTE *data, ULONG bytes,
	enum pfs3_metadata_type type);
BOOL PFS3MetadataToDisk(UBYTE *data, ULONG bytes, enum pfs3_metadata_type type);
BOOL PFS3MetadataToDiskForRecovery(UBYTE *data, ULONG bytes,
	enum pfs3_metadata_type type);
UWORD PFS3GetExtraFields(struct direntry *direntry,
	struct extrafields *extrafields);
void PFS3AddExtraFields(struct direntry *direntry,
	struct extrafields *extrafields);

#endif
