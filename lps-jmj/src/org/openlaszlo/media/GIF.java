/******************************************************************************
 * GIF.java
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
 * GIF
 */
public class GIF {

    static public boolean is(InputStream str) throws IOException {

        final int magic = 0x47494638; // GIF8

        DataInputStream ds = new DataInputStream(str);
        return (ds.readInt() == magic); 
    }
}
