/******************************************************************************
 * ResponderCACHEINFO.java
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
import org.openlaszlo.cache.RequestCache;
import org.openlaszlo.cache.Cache;
import org.openlaszlo.cm.CompilationManager;
import org.openlaszlo.sc.ScriptCompiler;
import org.apache.log4j.Logger;

public final class ResponderCACHEINFO extends ResponderAdmin
{
    private static Logger mLogger = Logger.getLogger(ResponderCACHEINFO.class);

    protected void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        res.setContentType ("text/xml");
        ServletOutputStream out = res.getOutputStream();
        String details = req.getParameter("details");
        String sc = req.getParameter("sc"); // get script compiler param
        try {
            String msg = cacheInfo(details != null, sc != null);
            out.println(msg);
        } finally {
            FileUtils.close(out);
        }
    }

    /**
     * send cache info out the response in XML
     */
    public static String cacheInfo(boolean doDetails, boolean doSC)
        throws IOException
    {
        Cache mediaCache    = ResponderMEDIA.getCache();
        Cache dataCache     = ResponderDATA.getCache();
        CompilationManager compilerCache = ResponderCompile.getCompilationManager();
        Cache compilerMediaCache = null;
        Cache scriptCache = ScriptCompiler.getScriptCompilerCache();

        if (compilerCache != null) {
            compilerMediaCache = compilerCache.getCompilerMediaCache();
        }

        StringBuffer buf = new StringBuffer("");
        buf.append("<lps-cacheinfo>\n");
        if (mediaCache != null) {
            mediaCache.dumpXML(buf, "media-cache", doDetails);
        }
        if (dataCache != null) {
            dataCache.dumpXML(buf, "data-cache", doDetails);
        }
        if (compilerCache != null) {
            compilerCache.dumpXML(buf, "compiler-cache", doDetails);
        }
        if (compilerMediaCache != null) {
            compilerMediaCache.dumpXML(buf, "compiler-media-cache", doDetails);
        }
        if (doSC) {
            if (scriptCache != null) {
                scriptCache.dumpXML(buf, "script-cache", doDetails);
            }
        }

        buf.append("</lps-cacheinfo>\n");
        return buf.toString();
    }

    public int getMimeType()
    {
        return MIME_TYPE_XML;
    }
}
