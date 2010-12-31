/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android bitmap class definitions
    Lang: English.
*/

#define CLID_Hidd_AGfx "hidd.graphics.android"

struct HostInterface
{
    JNIEnv  **jni;
    jclass   *cl;
    jobject  *obj;
};

/* Private instance data for Gfx hidd class */
struct gfx_data
{
    ULONG width;	/* Display view size */
    ULONG height;
};
