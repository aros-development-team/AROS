#include <stdio.h>

/****************************************************************************/
/** Dataspace                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Dataspace[];
#else
#define MUIC_Dataspace "Dataspace.mui"
#endif

/* Methods */

#define MUIM_Dataspace_Add                  0x80423366 /* V11 */
#define MUIM_Dataspace_Clear                0x8042b6c9 /* V11 */
#define MUIM_Dataspace_Find                 0x8042832c /* V11 */
#define MUIM_Dataspace_Merge                0x80423e2b /* V11 */
#define MUIM_Dataspace_ReadIFF              0x80420dfb /* V11 */
#define MUIM_Dataspace_Remove               0x8042dce1 /* V11 */
#define MUIM_Dataspace_WriteIFF             0x80425e8e /* V11 */
struct  MUIP_Dataspace_Add                  { ULONG MethodID; APTR data; LONG len; ULONG id; };
struct  MUIP_Dataspace_Clear                { ULONG MethodID; };
struct  MUIP_Dataspace_Find                 { ULONG MethodID; ULONG id; };
struct  MUIP_Dataspace_Merge                { ULONG MethodID; Object *dataspace; };
struct  MUIP_Dataspace_ReadIFF              { ULONG MethodID; struct IFFHandle *handle; };
struct  MUIP_Dataspace_Remove               { ULONG MethodID; ULONG id; };
struct  MUIP_Dataspace_WriteIFF             { ULONG MethodID; struct IFFHandle *handle; ULONG type; ULONG id; };

/* Attributes */

#define MUIA_Dataspace_Pool                 0x80424cf9 /* V11 i.. APTR              */

/* Dataspace new ASCII API */

#define MUIM_Dataspace_ReadASCII  0x80423367
#define MUIM_Dataspace_WriteASCII 0x80423368
#define MUIM_Dataspace_AddString  0x80423369
#define MUIM_Dataspace_AddInt     0x8042336b
#define MUIM_Dataspace_AddFloat   0x8042336c
#define MUIM_Dataspace_FindString 0x8042336a
#define MUIM_Dataspace_FindInt    0x8042336d
#define MUIM_Dataspace_FindFloat  0x8042336e

struct  MUIP_Dataspace_ReadASCII  { ULONG MethodID; FILE *infile; /* ... */ };
struct  MUIP_Dataspace_WriteASCII { ULONG MethodID; FILE *outfile; /* ... */ };
struct  MUIP_Dataspace_AddString  { ULONG MethodID; STRPTR objID; STRPTR key; CONST_STRPTR value; };
struct  MUIP_Dataspace_AddInt     { ULONG MethodID; STRPTR objID; STRPTR key; LONG value; };
struct  MUIP_Dataspace_AddFloat   { ULONG MethodID; STRPTR objID; STRPTR key; FLOAT value; };
struct  MUIP_Dataspace_FindString { ULONG MethodID; STRPTR objID; STRPTR key; };
struct  MUIP_Dataspace_FindInt    { ULONG MethodID; STRPTR objID; STRPTR key; };
struct  MUIP_Dataspace_FindFloat  { ULONG MethodID; STRPTR objID; STRPTR key; FLOAT *value;};

