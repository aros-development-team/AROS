/******************************************************************************
 * ResponderMESSAGE.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.util.Properties;
import javax.servlet.ServletConfig;
import javax.servlet.ServletOutputStream;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.openlaszlo.compiler.Canvas;
import org.openlaszlo.connection.*;
import org.apache.log4j.Logger;

public final class ResponderMESSAGE extends ResponderConnection
{
    private static Logger mLogger = Logger.getLogger(ResponderMESSAGE.class);

    protected void respondImpl(HttpServletRequest req, HttpServletResponse res,
                               Application app, int serial, String username)
        throws IOException
    {
        String to  = req.getParameter("to");
        String msg = req.getParameter("msg"); // msg: XML message
        String range = req.getParameter("range");

        if (to==null||to.equals("")) {
            respondWithErrorSWF(res, "missing 'to' parameter");
            return;
        }
        if (msg==null) {
            respondWithErrorSWF(res, "missing 'msg' parameter" );
            return;
        }

        mLogger.debug("to='" + to + "',msg='" + msg + "',range='" + range + "',s=" + serial);

        // Wrap it around resultset so serial number is always '0'.  This makes
        // sure it fools it into believing the local dataset got the right data.
        StringBuffer xmlResult = new StringBuffer();
        String wrapper = "<resultset s=\"0\">" + msg + "</resultset>";

        ConnectionGroup group = app.getConnectionGroup();
        int count = group.sendMessage(to, wrapper, range, xmlResult);

        if (count > 0) {
            respondWithStatusSWF(res, HttpServletResponse.SC_OK, 
                                 "message sent", xmlResult.toString(), serial);
        } else {
            String m = "message not sent: no one specified connected " +
                "(range: " + range + ")";
            respondWithErrorSWF(res, m);
        }
    }
}
