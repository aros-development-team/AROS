/******************************************************************************
 * ResponderLIST.java
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
import org.openlaszlo.connection.*;
import org.openlaszlo.media.*;
import org.openlaszlo.utils.*;
import org.openlaszlo.xml.internal.*;
import org.apache.log4j.*;

public final class ResponderLIST extends ResponderConnection
{
    private static boolean mIsInitialized = false;
    private static Object mIsInitializedLock = new Object();

    private static Logger mLogger = Logger.getLogger(ResponderLIST.class);

    protected void respondImpl(HttpServletRequest req, HttpServletResponse res,
                               Application app, int serial, String username)
        throws IOException
    {
        String users = req.getParameter("users");
        if (users==null||users.equals("")) {
            respondWithErrorSWF(res, "missing 'users' parameter");
            return;
        }

        ConnectionGroup group = app.getConnectionGroup();
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

        String xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
            "<!DOCTYPE laszlo-data>" +
            "<resultset s=\"" + serial + "\">"
            + buf.toString()
            + "</resultset>";

        res.setContentType(MimeType.SWF);

        ServletOutputStream sos = res.getOutputStream();
        try {
            InputStream swfbytes = DataCompiler.compile(xml, mSWFVersionNum);
            FileUtils.sendToStream(swfbytes, sos);
            sos.flush();

        } catch (FileUtils.StreamWritingException e) {
            mLogger.warn("StreamWritingException while sending list response: " + e.getMessage());
        } catch (DataCompilerException e) {
            respondWithExceptionSWF(res, e);
            return;
        } finally {
            FileUtils.close(sos);
        }
    }
}
