/* *****************************************************************************
 * MathUtils.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;

import java.lang.Math;
/**
 * A utility class containing math utility functions.
 */

public class MathUtils {
    /** 
     * @param d the double to format
     * @param n the number of places past the decimal place
     */
    public static String formatDouble(double d, int n) {
        int x = (int)Math.pow(10, n);
        return  Double.toString((double)(( (int)(x*d) ) / (double)x));

    }

    /**
     * @return hex representation of byte.
     */
    public static String byteToHexString(byte b) {
        int left = (int)( (b & 0xf0) >> 4 );
        int right = (int)( b & 0x0f );
        return Integer.toHexString(left) + Integer.toHexString(right);
    }
}

