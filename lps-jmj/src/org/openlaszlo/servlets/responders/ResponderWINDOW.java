/******************************************************************************
 * ResponderWINDOW.java
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
import javax.servlet.http.HttpUtils;
import javax.servlet.http.HttpSession;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.openlaszlo.compiler.Canvas;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.StringUtils;
import org.openlaszlo.utils.LZHttpUtils;
import org.apache.log4j.Logger;

public final class ResponderWINDOW extends ResponderCompile
{
    private static Logger mLogger = Logger.getLogger(ResponderWINDOW.class);

    /**
     * Overridden method from ReponseCompile.
     *
     * @param req unused
     */
    protected long getLastModified(String fileName, HttpServletRequest req)
    {
        // We don't care about other dependencies since all we show is the
        // top-level LZX file.
        return new File(fileName).lastModified();
    }

    /**
     * @param fileName Full pathname to file from request.
     */
    protected void respondImpl(String fileName, HttpServletRequest req, 
                               HttpServletResponse res)
        throws IOException
    {
        mLogger.info("Responding with WINDOW for " + fileName);
        if (fileName.endsWith(".lzo")) {
            fileName = StringUtils.setCharAt(fileName, fileName.length() - 1, 'x');
        }

        ServletOutputStream  out = res.getOutputStream();
        try {
            res.setContentType ("text/html");
            writeHeader(out, null);
            String name = req.getParameter("NAME");
            if (name == null || name == "")
                name = "LaszloWindow";
            String url = LZHttpUtils.getRequestURL(req).toString();
            writeScriptWindow(out, url, name, getCanvas(fileName, req));
            writeFooter(out);
        } finally {
            FileUtils.close(out);
        }
    }

    /**
     * Writes the html header tags for script window
     * @param url The URL to show in the window
     * @param name The name (TARGET) for the window
     * @param out A PrintWriter
     */    
    protected void writeScriptWindow (ServletOutputStream out, String url, 
                                      String name, Canvas canvas) 
        throws IOException 
    {
        int wd = canvas.getWidth();
        int ht = canvas.getHeight();

        out.println ("<script>");
        out.println (name + "= window.open('" + url + "', '" 
                          +  name + "', ");
        out.print   ("     'width=" + wd + ",");
        out.print   ("height=" + ht + ",");
        out.print   ("toolbar=no,location=no,directories=no,");
        out.print   ("status=no,menubars=no,");
        out.println ("scrollbars=no,resizable=no')");
        out.println ("</script>");
    }

    public int getMimeType()
    {
        return MIME_TYPE_HTML;
    }
}
