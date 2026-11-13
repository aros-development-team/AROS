/*
 * Copyright (C) 2022, The AROS Development Team.  All rights reserved.
  */
 
#define ABS_BOOT_MAGIC 0x4d363802
struct BootStruct
{
    ULONG magic;
    struct ExecBase *RealBase;
    struct ExecBase *RealBase2;
    struct List *mlist;
    struct TagItem *kerneltags;
    struct Resident **reslist;
    struct ExecBase *FakeBase;
    APTR bootcode;
    APTR ss_address;
    LONG ss_size;
    APTR magicfastmem;
    LONG magicfastmemsize;
};