/******************************************************************************
 * ResponderLOG.java
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
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.server.LPS;
import org.apache.log4j.Logger;

public final class ResponderLOG extends ResponderAdmin
{
    private static Logger mLogger = Logger.getLogger(ResponderLOG.class);

    protected void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        ServletOutputStream out = res.getOutputStream();
        FileInputStream in = null;
        try {
            res.setContentType ("text/html");
            out.println("<html><head><title>LPS Log</title></head>");
            out.println("<body><pre>");
            File logFile = ResponderLOGCONFIG.getLogFile();
            if (logFile != null) {
                in = new FileInputStream(logFile);
                FileUtils.escapeHTMLAndSend(in, out);
            } else {
                out.println("No log file.");
            }
            out.println("</pre></body></html>");
            mLogger.info("Sent log");
        } finally {
            FileUtils.close(in);
            FileUtils.close(out);
        }
    }

    public int getMimeType()
    {
        return MIME_TYPE_HTML;
    }
}
