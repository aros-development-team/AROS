/******************************************************************************
 * ResponderCONNECT.java
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
import org.openlaszlo.server.*;
import org.openlaszlo.utils.*;
import org.openlaszlo.servlets.LoadCount;
import org.apache.log4j.*;

public final class ResponderCONNECT extends ResponderConnection
{
    private static boolean mIsInitialized = false;
    private static boolean mEmitConnectHeaders = false;

    private static int[] mSTAT_activeCount = new int[1];

    private static Logger mLogger = Logger.getLogger(ResponderCONNECT.class);

    synchronized public void init(String reqName, ServletConfig config, 
                                  Properties prop)
        throws ServletException, IOException
    {
        super.init(reqName, config, prop);
        if (! mIsInitialized) {
            mEmitConnectHeaders = 
                prop.getProperty("emitConnectHeaders", "false").intern() == "true";
            mIsInitialized = true;
        }
    }

    /**
     * Get current number of active connections.
     */
    public static int getActiveCount()
    {
        synchronized (mSTAT_activeCount) {
            return mSTAT_activeCount[0];
        }
    }

    protected void respondImpl(HttpServletRequest req, HttpServletResponse res,
                               Application app, int serial, String username)
        throws IOException
    {
        mLogger.debug("respondImpl(username=" + username + ")");

        // This case is annoying
        String ua = req.getHeader(LZHttpUtils.USER_AGENT);
        if (ua == null) {
            respondWithErrorSWF(res, "request has no user-agent header");
            mLogger.warn("request has no user-agent header");
            return;
        } 

        if (! LPS.configuration.optionAllows("connection-user-agent", ua)) {
            respondWithErrorSWF(res, "forbidden user-agent: " + ua);
            mLogger.warn("forbidden user-agent: " + ua);
            return;
        }

        // ...create the connection...
        HTTPConnection connection = null;
        HTTPConnection prevConnection = app.getConnection(req.getParameter("i"));

        // for reconnects!
        String type = req.getParameter("type");
        boolean isReconnect = (type!=null && type.equals("r"));
        boolean doConnect = true;
        if (isReconnect) {
            if (prevConnection != null) {
                doConnect = false;
                connection = new HTTPConnection(res, prevConnection);
            } else {
                // Log that reconnection request has lost previous session.
                // This can happen if client takes longer than the reconnection
                // request wait interval and tries to make a reconnect request.
                mLogger.warn("tried reconnecting but there was no previous connection");
            }
        } 

        if (doConnect) {
            // ...pad connection headers with empty bytes if MSIE...
            String userAgent = req.getHeader("User-Agent");
            boolean doPad = (userAgent!=null && userAgent.indexOf("MSIE") != -1);

            connection = new HTTPConnection(res, username, mSWFVersionNum)
                .setHeartbeatInterval(app.getHeartbeat())
                .setEmitConnectHeaders(mEmitConnectHeaders)
                .setDoPad(doPad);
        }

        synchronized (mSTAT_activeCount) {
            // ...and finally register. Check to see if we had a previous
            // connection. If so, disconnect it after we register the new one.
            //
            // Registering the new connection and disconnecting the old one *must*
            // be an atomic operation.
            app.register(connection);
            if (prevConnection != null)
                prevConnection.disconnect(true);

            ++mSTAT_activeCount[0];
        }

        try {
            // this call blocks until a new connection comes in or is
            // disconnected
            connection.connect();
        } catch (IOException e) {
            mLogger.debug("ioexception: " + e.getMessage());
        } catch (Exception e) {
            mLogger.debug("exception: " + e.getMessage());
        } finally {
            app.unregister(connection);
            synchronized (mSTAT_activeCount) {
                --mSTAT_activeCount[0];
            }
        }


        // Send disconnect message when it isn't reconnect
        if ( ! connection.toBeReconnected()  && app.doSendUserDisconnect() ) { 
            String disconnectInfo =
                HTTPConnection.getConnectionInfoXML("__LPSUSERDISCONNECT", 
                                                    connection.getUsername());
            ConnectionGroup group = app.getConnectionGroup();
            group.sendMessage("*", disconnectInfo, "all", null);
        }

        try {

            // Send a byte to flush out socket. The flash player keeps its
            // socket open for a few seconds after disconnect unless we do
            // this. See bug 1447 for more info.
            byte[] buf = new byte[1];
            buf[0] = (byte) 0; // sending a buch of end tags (code 0).

            ServletOutputStream out = res.getOutputStream();
            out.write(buf);
            out.close();

        } catch (IOException e) {
            mLogger.debug(e.getMessage());
        }
    }
}
