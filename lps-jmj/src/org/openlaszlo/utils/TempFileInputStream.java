/******************************************************************************
 * TempFileInputStream.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.File;
import java.io.IOException;

/**
 * A FileInputStream that deletes the underlying file when the stream
 * is closed.
 *
 * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
 */
public class TempFileInputStream extends FileInputStream {

    private final File mFile;

    /**
     * Construct a stream that will delete this file
     * when the stream is closed
     * @param f the input file
     */
    public TempFileInputStream(File f) throws FileNotFoundException {
        super(f);
        mFile = f;
    }

    /**
     * close the stream and delete the file
     */
    public void close() throws IOException {

        try {
            super.close();
        } finally {
            mFile.delete();
        }
    }
}
