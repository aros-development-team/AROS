/******************************************************************************
 * ResponderSETCACHESIZE.java
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
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.cache.Cache;
import org.apache.log4j.Logger;

public final class ResponderSETCACHESIZE extends ResponderAdmin
{
    private static Logger mLogger = Logger.getLogger(ResponderSETCACHESIZE.class);

    protected void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        ServletOutputStream out = res.getOutputStream();
        try {
            String l = req.getParameter("size");
            String msg = null;
            String t = req.getParameter("t"); 
            String k = req.getParameter("k"); 
            boolean inMem = true;
            Cache cache;
            if (t != null && t.equalsIgnoreCase("data")) {
                cache = ResponderDATA.getCache();
            } else if (t != null && t.equalsIgnoreCase("compiler")) {
                cache = ResponderCompile.getCompilationManager();
            } else {
                cache = ResponderMEDIA.getCache();
            }
            if (k != null && k.equalsIgnoreCase("disk")) {
                inMem = false; 
            }

            if (l != null) {
                long s = -1;
                try {
                    s = Long.parseLong(l);
                } catch (NumberFormatException e) {
                }
                if (s >= 0) {
                    if (inMem) {
                        cache.setMaxMemSize(s);
                    } else {
                        cache.setMaxDiskSize(s);
                    }
                } else {
                    mLogger.error("ignored bad size parameter");
                }
                res.setContentType ("text/xml");
                msg = ResponderCACHEINFO.cacheInfo(false, false);
                out.println(msg);
            } else {
                res.setContentType ("text/html");
                msg = "Can't set cache size: size parameter missing from setcachesize request";
                mLogger.error(msg);
                msg = msg + "<br/>" + ResponderCACHEINFO.cacheInfo(false, false);

                out.println("<html><head><title>LPS Cache Information</title></head>");
                out.println("<body>");
                out.println(msg);
                out.println("</body></html>");
            }
    
            mLogger.info(msg);
        } finally {
            FileUtils.close(out);
        }
    }

    public int getMimeType()
    {
        return MIME_TYPE_XML;
    }
}
