#include <libraries/uuid.h>

struct TypeMapping
{
    UBYTE MBRType;
    ULONG DOSType;
    const uuid_t *uuid;
};

extern const struct TypeMapping PartTypes[];

ULONG MBR_FindDosType(UBYTE id);
