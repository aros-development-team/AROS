/******************************************************************************
 * ResponderCONNECTIONLOGOUT.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.util.*;
import javax.servlet.*;
import javax.servlet.http.*;
import org.openlaszlo.auth.*;
import org.openlaszlo.connection.*;
import org.openlaszlo.media.*;
import org.openlaszlo.utils.*;
import org.openlaszlo.xml.internal.*;
import org.apache.log4j.*;

public final class ResponderCONNECTIONLOGOUT extends ResponderConnection
{
    private static Logger mLogger = 
        Logger.getLogger(ResponderCONNECTIONLOGOUT.class);

    protected void respondImpl(HttpServletRequest req, HttpServletResponse res,
                               Application app, int serial, String username)
        throws IOException
    {
        Authentication auth = app.getAuthenticator();

        String xml;
        String status="error";
        StringBuffer buf = new StringBuffer();
        try {
            int code = auth.logout(req, res, getAuthParam(req), buf);
            status = (code == 0 ? "success" : "failure" );
            String xmlResponse = buf.toString();
            int i = xmlResponse.indexOf("<authentication>");
            if (i != -1) {
                xmlResponse = xmlResponse.substring(i);

                xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                    + "<!DOCTYPE laszlo-data>"
                    + "<resultset s=\"" + serial + "\">"
                    + "<logout status=\"" + status + "\">"
                    + xmlResponse
                    + "</logout></resultset>";

            } else {
                xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" 
                    + "<!DOCTYPE laszlo-data>" 
                    + "<resultset s=\"" + serial + "\">"
                    + "<logout status=\"error\">"
                    + "<error message=\"could not find &lt;authentication&gt; element\" />"
                    + "</logout></resultset>";
                mLogger.warn("could not find <authentication> element: " + xmlResponse);
            }
        } catch (AuthenticationException e) {
            xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" 
                + "<!DOCTYPE laszlo-data>" 
                + "<resultset s=\"" + serial + "\">"
                + "<logout status=\"error\">"
                + "<error message=\"authentication exception\" />"
                + "</logout></resultset>";
            mLogger.warn("AuthenticationException", e);
        }

        res.setContentType(MimeType.SWF);

        ServletOutputStream sos = res.getOutputStream();
        try {
            InputStream swfbytes = DataCompiler.compile(xml, mSWFVersionNum);
            FileUtils.sendToStream(swfbytes, sos);
            sos.flush();
        } catch (FileUtils.StreamWritingException e) {
            mLogger.warn("StreamWritingException while sending connection logout response: " + e.getMessage());
        } catch (DataCompilerException e) {
            respondWithExceptionSWF(res, e);
            return;
        } finally {
            FileUtils.close(sos);
        }
    }
}
