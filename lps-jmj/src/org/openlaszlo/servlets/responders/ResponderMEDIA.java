/******************************************************************************
 * ResponderMEDIA.java
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
import org.openlaszlo.cache.MediaCache;
import org.openlaszlo.cache.RequestCache;
import org.openlaszlo.server.LPS;

import org.apache.log4j.Logger;

public final class ResponderMEDIA extends ResponderCache
{
    private static MediaCache mCache = null;
    private static boolean mIsInitialized = false;
    private static Logger mLogger = Logger.getLogger(ResponderMEDIA.class);

    public void init(String reqName, ServletConfig config, Properties prop)
        throws ServletException, IOException
    { 
        if (! mIsInitialized) {
            // Initialize media cache
            String cacheDir = config.getInitParameter("lps.mcache.directory");
            if (cacheDir == null) {
                cacheDir = prop.getProperty("mcache.directory");
            }
            if (cacheDir == null) {
                cacheDir = LPS.getWorkDirectory() + File.separator + "mcache";
            }

            File cache = checkDirectory(cacheDir);
            mLogger.info("Media Cache is at " + cacheDir);

            try {
                mCache = new MediaCache(cache, prop);
            } catch (IOException e) {
                throw new ServletException(e.getMessage());
            }

            mIsInitialized = true;
        }

        super.init(reqName, config, mCache, prop);
    }

    static public RequestCache getCache()
    {
        return mCache;
    }
}
