/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Android bitmap class definitions
*/

#define CLID_Hidd_Gfx_Android     "hidd.gfx.android"
#define CLID_Hidd_Display_Android "hidd.display.android"

struct androidDisplayData
{
};

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    ULONG	width;	/* Display view size		*/
    ULONG	height;
    OOP_Object *bitmap;	/* Currently displayed bitmap   */
};
