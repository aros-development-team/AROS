/******************************************************************************
 * ResponderHTML.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.net.URLEncoder;
import java.util.Enumeration;
import java.util.Properties;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpSession;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.openlaszlo.compiler.Canvas;
import org.openlaszlo.compiler.CompilationError;
import org.openlaszlo.utils.*;
import org.apache.log4j.Logger;

public final class ResponderHTML extends ResponderCompile
{
    private static Logger mLogger = Logger.getLogger(ResponderHTML.class);

    /**
     * @param fileName Full pathname to file from request.
     */
    protected void respondImpl(String fileName, HttpServletRequest req, 
                               HttpServletResponse res)
        throws IOException
    {
        mLogger.info("Responding with HTML wrapper for " + fileName);
        res.setContentType ("text/html");
        ServletOutputStream out = res.getOutputStream();
        try {
            // Get the canvas first, so that if this fails and we
            // write the compilation error, nothing has been written
            // to out yet.
            if (fileName.endsWith(".lzo")) {
                fileName = fileName.substring(0, fileName.length() - 1) + "x";
            }
            Canvas canvas = getCanvas(fileName, req);
            /* This method doesn't call writeHeader and writeFooter, since
             * the stylesheet handles the whole HTML generation. */
            //writeHeader(out, canvas);
            writeCanvas(out, req, canvas, fileName);
            //writeFooter(out);
        } catch (CompilationError e) {
            out.print("<pre>\n"+e.toHTML()+"\n<pre>");
        } finally {
            FileUtils.close(out);
        }
    }

    /**
     * Writes the canvas html.  The canvas is the area in which the
     * Laszlo application is rendered.
     * @param out <tt>ServletOutputStream</tt> to write on
     * @param req request to retrieve scheme, server name, server port and
     * requested url
     * @param canvas the canvas for the given request
     * @param fileName file for the app
     */
    private void writeCanvas (ServletOutputStream out, HttpServletRequest req, 
                              Canvas canvas, String fileName)
        throws IOException 
    {
        String styleSheetPathname =
            org.openlaszlo.server.LPS.getTemplateDirectory() +
            File.separator + "html-response.xslt";
        String xmlString = canvas.getXML(ResponderAPP_CONSOLE.getRequestXML(req, fileName));
        Properties properties = new Properties();
        TransformUtils.applyTransform(styleSheetPathname, properties, xmlString, out);
    }
    
    /** 
     * Return all query args except for "lzt"
     */
    private String getQueryArgs(HttpServletRequest req) {
        StringBuffer query = new StringBuffer();
        Enumeration e = req.getParameterNames();
        while (e.hasMoreElements()) {
            String name = (String)e.nextElement();
            String val = req.getParameter(name);
            if (!name.equals("lzt")) {
                query.append("&"+name+"="+URLEncoder.encode(val));
            }
        }
        return query.toString();
    }

    public int getMimeType()
    {
        return MIME_TYPE_HTML;
    }
}
