/******************************************************************************
 * FontType.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.media;

/**
 * Class for holding font type strings.
 *
 * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
 */
public class FontType {
    public static final String FFT = "fft";
    public static final String TTF = "ttf";

    /**
     * Return the font type from a filename
     */
    public static String fromName(String name) {
        int index = name.lastIndexOf('.');
        if (index == -1 || index + 2 >= name.length()) {
            return "";
        }

        return name.substring(index+1).toLowerCase();
    }
}

