/******************************************************************************
 * MP3.java
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
 * MP3
 */
public class MP3 {
    static public boolean is(InputStream str) throws IOException {

        final short magic = (short)0xfff3;

        DataInputStream ds = new DataInputStream(str);
        return (ds.readShort() == magic); 
    }
}
