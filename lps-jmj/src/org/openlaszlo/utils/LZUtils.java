/* *****************************************************************************
 * LZUtils.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;

import java.util.Enumeration;
import java.util.Properties;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Calendar;

/**
 * Class for miscellaneous utility functions.
 */
public class LZUtils
{
    /**
     * Get int from object.
     *
     * @param o number to convert.
     * @return if null or non-integer, returns 0.
     */
    static public int getInt(Object o)
    {
        if (o == null)
            return 0;

        if (! o.getClass().getName().equals("java.lang.Integer"))
            return 0;

        return ((Integer)o).intValue();
    }


    /**
     * Get string from object.
     *
     * @param o object to convert.
     * @return if null or non-string, returns null.
     */
    static public String getStr(Object o)
    {
        return getStr(o, true);
    }


    /**
     * Get string from object.
     *
     * @param o object to convert.
     * @param isNullOk if true, return null for nulls or non-strings, else
     * return an empty string
     * @return if null or non-string, returns null as long as isNullOk is true,
     * else return empty string.
     */
    static public String getStr(Object o, boolean isNullOk)
    {
        if (o == null)
            return (isNullOk?null:"");

        if (! o.getClass().getName().equals("java.lang.String"))
            return (isNullOk?null:"");

        return (String)o;
    }


    /**
     * Parse a string that contains a number.
     *
     * @return number parsed from string. 0, if invalid.
     */
    static public int parseInt(String s)
    {
        int n = 0;
        try {
            n = Integer.parseInt(s);
        } catch (Exception e) {
            // ignore
        }
        return n;
    }

    /**
     * Expand system property values enclosed in ${}.
     *
     * @param p properties object to expand property values
     * @return properties object with expanded values
     * @throws Exception if a system property value doesn't exist or the
     * property value isn't enclosed properly in ${}
     */
    static public Properties expandProperties(Properties p)
        throws Exception
    {
        Properties _p = new Properties();
        Enumeration keys = p.keys();
        while (keys.hasMoreElements()) {
            String k = (String)keys.nextElement();
            String v = (String)p.getProperty(k);
            _p.setProperty(k, StringUtils.expandPropertyValues(v));
        }
        return _p;
    }
}
