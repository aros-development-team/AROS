/******************************************************************************
 * ConfigDir.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.server;

/**
 * ConfigDir is a simple class that depends on nothing else
 * and locates the configuration directory.
 * 
 * @author Eric Bloch
 * @version 1.0 
 */

import java.io.File;

public class ConfigDir {

    /** 
     * @return the absolute path for the configuration directory
     * @param home LPS_HOME to be sued if the path is determined
     * to be relative
     */

    private static String mDir = null;

    public synchronized static String get(String home) {

        if (mDir != null)
            return mDir;

        try {
            mDir = System.getProperty("lps.config.dir.abs");
            if (mDir != null && !mDir.equals("")) {
                return mDir;
            } 
        } catch (SecurityException t) {
        }

        try {
            mDir = System.getProperty("lps.config.dir");
        } catch (SecurityException t) {
        }

        if (mDir == null || mDir.equals("")) {
            mDir = "config";
        }

        mDir = home + File.separator + "WEB-INF" 
                    + File.separator + "lps" 
                    + File.separator + mDir;

        return mDir;
    }
}

