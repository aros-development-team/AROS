/******************************************************************************
 * ResponderConnectionAgent.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.util.*;
import java.net.*;
import javax.servlet.*;
import javax.servlet.http.*;
import org.openlaszlo.compiler.*;
import org.openlaszlo.connection.*;
import org.openlaszlo.server.*;
import org.openlaszlo.utils.*;
import org.apache.log4j.*;


// Connection agents are configured in ResponderConnection
public abstract class ResponderConnectionAgent extends Responder
{
    private static Logger mLogger = Logger.getLogger(ResponderConnectionAgent.class);

    public final static int SC_ERROR             = 800;
    public final static int SC_MISSING_PARAMETER = 801;

    protected abstract void respondAgent(HttpServletRequest req, HttpServletResponse res,
                                         ConnectionGroup group)
        throws IOException;


    protected void respondImpl(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        String ip      = req.getRemoteAddr();
        String _url    = req.getParameter("url");
        String _group  = req.getParameter("group");

        if ( ! LPS.configuration.optionAllows("connection-agent-ip", ip)) {
            String err = "Forbidden connection agent host: " +  ip;
            replyWithXMLStatus(res, err, HttpServletResponse.SC_FORBIDDEN);
            return;
        }

        ConnectionAgent agent = ConnectionAgent.getAgent(_url, false);
        if (agent == null) {
            replyWithXMLStatus(res, "Bad connection agent", SC_ERROR);
            return;
        }

        ConnectionGroup group = ConnectionGroup.getGroup(_group);
        if (group == null) {
            replyWithXMLStatus(res, "Bad connection group", SC_ERROR);
            return;
        }

        respondAgent(req, res, group);
    }

    protected void replyWithXMLStatus(HttpServletResponse res, String message) 
        throws IOException {
        replyWithXML(res, message, null, HttpServletResponse.SC_OK);
    } 

    protected void replyWithXMLStatus(HttpServletResponse res, String message, int sc) 
        throws IOException {
        replyWithXML(res, message, null, sc);
    } 

    protected void replyWithXML(HttpServletResponse res, String message, String xml) 
        throws IOException {
        replyWithXML(res, message, xml, HttpServletResponse.SC_OK);
    } 

    protected void replyWithXML(HttpServletResponse res, String message, String xml, int sc) 
        throws IOException {
        ServletOutputStream out = res.getOutputStream();;
        try {
            res.setContentType ("text/xml");
            out.println("<lps>");
            out.println("<status>" + sc + "</status>");
            out.println("<message>" + message + "</message>");
            if (xml != null)
                out.println("<body>" + xml + "</body>");
            out.println("</lps>");
        } finally {
            FileUtils.close(out);
        }
    } 

    public final int getMimeType()
    {
        return MIME_TYPE_XML;
    }
}
