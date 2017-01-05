/*
    Copyright  1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android bitmap class definitions
    Lang: English.
*/

#define CLID_Hidd_Gfx_Android "hidd.gfx.android"

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    ULONG	width;	/* Display view size		*/
    ULONG	height;
    OOP_Object *bitmap;	/* Currently displayed bitmap   */
};
