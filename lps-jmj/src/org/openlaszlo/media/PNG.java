/******************************************************************************
 * PNG.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.media;

import java.io.InputStream;
import java.io.IOException;
import java.io.DataInputStream;

/**
 * PNG
 */
public class PNG {
    static public boolean is(InputStream str) throws IOException {

        final int magic = 0x89504e47; // 137 PNG

        DataInputStream ds = new DataInputStream(str);
        return (ds.readInt() == magic); 
    }
}
