/******************************************************************************
 * ResponderDISCONNECT.java
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
import org.openlaszlo.compiler.*;
import org.openlaszlo.connection.*;
import org.apache.log4j.*;

public final class ResponderDISCONNECT extends ResponderConnection
{
    private static boolean mIsInitialized = false;
    private static Object mIsInitializedLock = new Object();

    private static Logger mLogger = Logger.getLogger(ResponderDISCONNECT.class);


    protected void respondImpl(HttpServletRequest req, HttpServletResponse res,
                               Application app, int serial, String username)
        throws IOException
    {
        mLogger.debug("respondImpl(username=" + username + ")");

        // Removes connection and unregisters itself from application through
        // binding event listener
        HTTPConnection connection = app.getConnection(req.getParameter("i"));
        if (connection != null) {
            connection.disconnect();
            app.unregister(connection);
        }

        respondWithStatusSWF(res, HttpServletResponse.SC_OK,
                             "disconnected", serial);
    }
}
