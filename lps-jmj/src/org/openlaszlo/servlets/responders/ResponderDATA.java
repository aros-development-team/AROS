/******************************************************************************
 * ResponderDATA.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.util.Properties;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.ServletOutputStream;
import org.openlaszlo.cache.DataCache;
import org.openlaszlo.cache.RequestCache;
import org.openlaszlo.server.LPS;

import org.apache.log4j.Logger;

/**
 * 
 */
public final class ResponderDATA extends ResponderCache
{
    private static DataCache mCache = null;
    private static boolean mIsInitialized = false;
    private static Logger mLogger = Logger.getLogger(ResponderDATA.class);

    synchronized public void init(String reqName, ServletConfig config, Properties prop)
        throws ServletException, IOException
    {
        // Cache should only be initialized once.
        if (! mIsInitialized) {
            // Initialize data cache
            String cacheDir = config.getInitParameter("lps.dcache.directory");
            if (cacheDir == null) {
                cacheDir = prop.getProperty("dcache.directory");
            }
            if (cacheDir == null) {
                cacheDir = LPS.getWorkDirectory() + File.separator + "dcache";
            }

            File cache = checkDirectory(cacheDir);
            mLogger.info("Data Cache is at " + cacheDir);

            //------------------------------------------------------------
            // Support for new style data response
            //------------------------------------------------------------
            try {
                mCache = new DataCache(cache, prop);
            } catch (IOException e) {
                throw new ServletException(e.getMessage());
            }

            mIsInitialized = true;
        }
        super.init(reqName, config, mCache, prop);
    }

    static public RequestCache getCache() {
        return mCache;
    }
}
