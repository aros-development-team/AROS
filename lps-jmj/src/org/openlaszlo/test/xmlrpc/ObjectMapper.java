/* ****************************************************************************
 * ObjectMapper.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test.xmlrpc;

public final class ObjectMapper
{
    public int dimensions[];
    public long bgcolor;
    public String name;

    public ObjectMapper(int h, int w, long bc, String str)
    {
        dimensions = new int[2];
        dimensions[0] = w;
        dimensions[1] = h;
        bgcolor = bc;
        name = str;
    }
};
