/******************************************************************************
 * ResponderAGENTLIST.java
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


public class ResponderAGENTLIST extends ResponderConnectionAgent
{
    private static boolean mIsInitialized = false;
    private static Logger mLogger = Logger.getLogger(ResponderAGENTLIST.class);

    protected void respondAgent(HttpServletRequest req, HttpServletResponse res,
                                ConnectionGroup group) throws IOException
    {
        String users = req.getParameter("users");
        if ( users == null || users.equals("") ) {
            replyWithXMLStatus(res, "missing 'users' parameter", SC_MISSING_PARAMETER);
            return;
        }

        StringBuffer buf = new StringBuffer("<list>");
        Set set = group.list(users);
        Iterator iter = set.iterator();
        while (iter.hasNext()) {
            buf.append("<user name=\"")
                .append((String)iter.next())
                .append("\" />");
        }
        buf.append("</list>");

        mLogger.debug(buf.toString());

        replyWithXML(res, "ok", buf.toString());
    }
}
