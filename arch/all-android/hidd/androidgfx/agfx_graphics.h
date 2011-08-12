/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android bitmap class definitions
    Lang: English.
*/

#define CLID_Hidd_AGfx "hidd.graphics.android"

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    ULONG width;	/* Display view size */
    ULONG height;
};
