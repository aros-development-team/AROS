/******************************************************************************
 * DateUtils.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;

import java.text.SimpleDateFormat;
import java.text.ParseException;
import java.util.Date;

import org.openlaszlo.utils.ChainedException;

/**
 * Utility class for dates
 */
public class DateUtils {

    /**
     * Return a formatter 
     */
    private static SimpleDateFormat getFormatter() {
        return new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss z");
    }

    /**
     * Convert a long to a Date String 
     */
    public static String getDateString(long d) {
        return getFormatter().format(new Date(d));
    }

    /**
     * Convert a date String to a long 
     */
    public static long getDate(String s) {
        try {
            return getFormatter().parse(s).getTime();
        } catch (java.text.ParseException e) {
            throw new ChainedException(e);
        }
    }
}

