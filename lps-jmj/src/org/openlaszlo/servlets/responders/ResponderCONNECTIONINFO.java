/******************************************************************************
 * ResponderCONNECTIONINFO.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;
import org.openlaszlo.connection.*;
import org.openlaszlo.utils.*;
import org.apache.log4j.Logger;

public class ResponderCONNECTIONINFO extends ResponderAdmin
{
    private static Logger mLogger = 
        Logger.getLogger(ResponderCONNECTIONINFO.class);

    protected void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        res.setContentType("text/xml");
        StringBuffer buf = new StringBuffer();

        String _details = req.getParameter("details");
        boolean details = (_details != null && _details.equals("1"));

        ConnectionGroup.dumpGroupsXML(buf, details);

        String appName = req.getParameter("application");
        if (appName != null && ! appName.equals("")) {
            if (appName.equals("*")) {
                Application.dumpApplicationsXML(buf, details);
            } else {
                Application application = 
                    Application.getApplication(appName, false);
                if (application != null)
                    application.toString();
            }
        }

        ConnectionAgent.dumpAgentsXML(buf, details);

        ServletOutputStream out = res.getOutputStream();
        try {
            out.println("<connection-info " +
                        " max-message-length=\"" + HTTPConnection.getMaxMessageLen() + "\"" + 
                        " connection-length=\"" + HTTPConnection.getConnectionLength() + "\"" +
                        " reconnection-wait-interval=\"" + HTTPConnection.getReconnectionWaitInterval() + "\"" +
                        " >");
            out.println(buf.toString());
            out.println("</connection-info>");
        } finally {
            FileUtils.close(out);
        }
    }

        


    public void printApplications(ServletOutputStream out)
        throws IOException
    {
        out.println("<application-list>");
        out.println("</application-list>");
    }

    public int getMimeType()
    {
        return MIME_TYPE_XML;
    }

}    
