/******************************************************************************
 * ResponderAGENTMESSAGE.java
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
import org.openlaszlo.utils.*;
import org.apache.log4j.*;


public class ResponderAGENTMESSAGE extends ResponderConnectionAgent
{
    private static boolean mIsInitialized = false;
    private static Logger mLogger = Logger.getLogger(ResponderAGENTMESSAGE.class);

    protected void respondAgent(HttpServletRequest req, HttpServletResponse res,
                                ConnectionGroup group) throws IOException
    {
        String to  = req.getParameter("to");
        String msg = req.getParameter("msg");
        String range = req.getParameter("range");
        String dset = req.getParameter("dset");

        if (isEmpty(to)) {
            replyWithXMLStatus(res, "missing 'to' parameter", SC_MISSING_PARAMETER);
            return;
        }

        if (isEmpty(dset)) {
            replyWithXMLStatus(res, "missing 'dset' parameter", SC_MISSING_PARAMETER);
            return;
        }

        if (msg == null) {
            replyWithXMLStatus(res, "missing 'msg' parameter", SC_MISSING_PARAMETER);
            return;
        }

        mLogger.debug("to='" + to + "',range='" + range + "', msg='" + msg + "'");

        // Wrap it around resultset so serial number is always '0'.  This makes
        // sure it fools it into believing the local dataset got the right data.
        String xml = "<resultset s=\"0\">" +
            "<root dset=\"" + dset + "\">" + 
            msg +
            "</root>" +
            "</resultset>";

        int count = group.sendMessage(to, xml, range, null);

        replyWithXMLStatus(res, (count > 0 
                                 ? "message sent"
                                 : "no one specified connected (range: " + range + ")"));
    }

    private boolean isEmpty(String str) {
        return str==null || str.equals("");
    }
}
