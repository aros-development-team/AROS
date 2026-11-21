/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/
/* Stand-ins for the host test of the shadow layout generators. The file that
   includes this one supplies the exec types.

   SampleLock is to these what spinlock_t is to exec's own: how many bytes it
   has is a matter of the build, and the m68k layout has no such member. */
#ifndef SHADOW_LAYOUTS_SAMPLE_H
#define SHADOW_LAYOUTS_SAMPLE_H

#ifndef SAMPLE_LOCK_BYTES
#define SAMPLE_LOCK_BYTES 16
#endif
#ifndef SAMPLE_VALUE_TYPE
#define SAMPLE_VALUE_TYPE ULONG
#endif

/* 4 bytes on m68k; natively 4 or 8, as an IPTR is */
typedef SAMPLE_VALUE_TYPE SampleValue;

struct SampleNode
{
    struct SampleNode *sn_Succ;
    struct SampleNode *sn_Pred;
    UBYTE sn_Type;
    BYTE sn_Pri;
    char *sn_Name;
};

struct SampleLock
{
    UBYTE sk_Bytes[SAMPLE_LOCK_BYTES];
};

struct SampleLibrary
{
    struct SampleNode sl_Node;
    UBYTE sl_Flags;
    UBYTE sl_Pad;
    UWORD sl_Version;
    ULONG sl_Sum;
    UWORD sl_OpenCnt;
    struct SampleLock sl_Lock;
};

struct SampleBase
{
    struct SampleLibrary sb_Lib;
    UWORD sb_Count;
    ULONG sb_Flags;
    APTR sb_Data;
    SampleValue sb_Value;
    UBYTE sb_Name[14];
    struct SampleNode sb_Spare;
};

#endif /* SHADOW_LAYOUTS_SAMPLE_H */
