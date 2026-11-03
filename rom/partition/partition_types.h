#include <libraries/uuid.h>

struct TypeMapping
{
    UBYTE MBRType;
    ULONG DOSType;
    const uuid_t *uuid;
};

extern const struct TypeMapping PartTypes[];
extern const uuid_t GPT_Type_AROS_RDB;

ULONG MBR_FindDosType(UBYTE id);
