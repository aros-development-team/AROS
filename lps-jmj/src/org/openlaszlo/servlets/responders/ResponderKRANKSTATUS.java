/******************************************************************************
 * ResponderKRANKSTATUS.java
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
import org.openlaszlo.cm.CompilationManager;
import org.openlaszlo.utils.*;
import org.openlaszlo.xml.internal.XMLUtils;
import org.apache.log4j.Logger;
import org.openlaszlo.servlets.KrankListener;

public final class ResponderKRANKSTATUS extends ResponderAdmin
{
    private static Logger mLogger = Logger.getLogger(ResponderKRANKSTATUS.class);

    /**
       set abort=true in the request params to abort the krank thread
     */
    protected void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        String abort = req.getParameter("abort");
        String lps  = req.getContextPath();
        String msg = req.getParameter("msg");
        String lzourl  = StringUtils.replace(req.getRequestURI(), ".lzx", ".lzo");
        KrankListener kl = CompilationManager.sKrankListener;
        res.setContentType("text/html");
        ServletOutputStream out = res.getOutputStream();
        
        if ("true".equals(abort)) {
            CompilationManager.abortKrankProcess();
            String xmlString = "<abort " +
                "lps=\"" + XMLUtils.escapeXml(lps) + "\" ";
            if (msg != null)
                xmlString += "msg=\"" + XMLUtils.escapeXml(msg) + "\" ";
            xmlString += ">" +
                ResponderAPP_CONSOLE.getRequestXML(req, null) +
                "</abort>";
            String pathname =
                org.openlaszlo.server.LPS.getTemplateDirectory() +
                File.separator + "krank-abort.xslt";
            TransformUtils.applyTransform(pathname, xmlString, out);
        } else {
            if (kl.isBusy()) {
                kl.setDuration(System.currentTimeMillis() - kl.starttime());
            }
            String xmlString = "<krank " +
                "appName=\"" + XMLUtils.escapeXml(""+kl.getAppname()) + "\" " +
                "seconds=\"" + kl.getDuration()/1000 + "\" " +
                "lzourl=\"" + lzourl + "\" ";
            if (kl.isAborted())
                xmlString += "isAborted=\"" + XMLUtils.escapeXml(""+kl.isAborted()) + "\" ";
            if (kl.isBusy()) {
                xmlString += "isBusy=\"" + XMLUtils.escapeXml(""+kl.isBusy()) + "\" ";
            }
            if (kl.isFinished())
                xmlString += "isFinished=\"" + XMLUtils.escapeXml(""+kl.isFinished()) + "\" ";
            if (kl.isIdle())
                xmlString += "isIdle=\"" + XMLUtils.escapeXml(""+kl.isIdle()) + "\" ";
            xmlString += ">" +
                ResponderAPP_CONSOLE.getRequestXML(req, null) +
                "</krank>";
            String pathname =
                org.openlaszlo.server.LPS.getTemplateDirectory() +
                File.separator + "kranking.xslt";
            TransformUtils.applyTransform(pathname, xmlString, out);
        }
        FileUtils.close(out);
    }

    public int getMimeType()
    {
        return MIME_TYPE_HTML;
    }

}
