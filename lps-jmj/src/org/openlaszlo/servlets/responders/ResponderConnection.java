/******************************************************************************
 * ResponderConnection.java
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
import org.openlaszlo.auth.*;
import org.openlaszlo.compiler.*;
import org.openlaszlo.connection.*;
import org.openlaszlo.utils.*;
import org.apache.log4j.*;


public abstract class ResponderConnection extends ResponderCompile
{
    private static final String DEFAULT_AUTHENTICATOR =
        "org.openlaszlo.auth.HTTPAuthentication";
    private static final String ANONYMOUS_AUTHENTICATOR =
        "org.openlaszlo.auth.NullAuthentication";

    private static String mDefaultAuthClass = null;

    private static String mDefaultUserName = null;
    private static Properties mLPSProperties = null;
    private static String mCMOption = "check";
    private static boolean mIsInitialized = false;
    private static Logger mLogger = Logger.getLogger(ResponderConnection.class);

    private static Hashtable mAuthenticators = new Hashtable();

    protected abstract void respondImpl(HttpServletRequest req, HttpServletResponse res,
                                        Application app, int serial, String username)
        throws IOException;


    synchronized public void init(String reqName, ServletConfig config, Properties prop)
        throws ServletException, IOException
    {
        super.init(reqName, config, prop);

        if (! mIsInitialized) {

            HTTPConnection.init(prop);
            mLPSProperties = prop;
            mDefaultUserName = 
                prop.getProperty("connection.none-authenticator.username", 
                                 NullAuthentication.DEFAULT_USERNAME);
            mDefaultAuthClass = 
                prop.getProperty("connection.default.authenticator", 
                                 DEFAULT_AUTHENTICATOR);
            mCMOption = prop.getProperty("compMgrDependencyOption", "check");

            mIsInitialized = true;
        }
    }


    protected final void respondImpl(String fileName, HttpServletRequest req,
                                     HttpServletResponse res)
        throws IOException
    {
        try {
            String path = req.getServletPath();
            String info = null;

            Application app = getConnectedApplication(fileName, req);
            if (app == null) {
                info = "application does not allow persistent connection calls";
                respondWithErrorSWF(res, info);
                return;
            }

            // Session is valid if a username is returned
            String username = mDefaultUserName;
            Authentication authenticator = app.getAuthenticator();

            // Kludge: skip authentication if lzt=connectionlogin. 
            String lzt = req.getParameter("lzt");
            if (! lzt.equals("connectionlogin") && authenticator != null ) {
                username = authenticator.getUsername(req, res, getAuthParam(req));
                if (username == null) {
                    respondWithErrorSWF(res, "invalid user or session");
                    return;
                }
            }

            // s: serial number of request; this number needs to be echoed
            // back for successful response as an attribute of the resultset
            // tag. See xml response in respondWithStatusSWF() for more
            // information.
            int serial = 0;
            try {
                String serialStr = req.getParameter("s");
                if (serialStr != null) {
                    serial = Integer.parseInt(serialStr);
                }
            } catch (NumberFormatException e) {
                respondWithErrorSWF(res, "'s' is not a number");
                return;
            }

            respondImpl(req, res, app, serial, username);

        } catch (AuthenticationException e) {

            respondWithExceptionSWF(res, e);

        }
    }


    synchronized private 
        Application getConnectedApplication(String filename, HttpServletRequest req)
        throws IOException, AuthenticationException
    {
        mLogger.debug("checkConnection(filename,req)");

        if (filename.endsWith(".lzo")) {
            filename = filename.substring(0, filename.length() - 1) + "x";
        }

        Application app = null;
        String path = req.getServletPath();

        app = Application.getApplication(path, false);
        if (app != null && mCMOption.equals("never"))
            return app;

        // FIXME: [2003-11-03 pkang] should have another object that connection and
        // connectionlogin inherit from. This is the safer option for now.
        String lzt = req.getParameter("lzt");
        if (! lzt.equals("connect") && ! lzt.equals("connectionlogin") )
            return app;

        Canvas canvas = getCanvas(filename, req);
        if (canvas.isConnected()) {

            app = Application.getApplication(path);

            // Fetching lmt should be low impact, since the file should already
            // be cached.
            long lmt = getLastModified(filename, req);
            if (lmt != app.getLastModifiedTime()) {

                String authClass = canvas.getAuthenticator();
                Authentication authenticator;
                try {
                    authenticator = ( authClass == null
                                      ? getAuthenticator(mDefaultAuthClass)
                                      : getAuthenticator(authClass) );
                } catch (ClassNotFoundException e) {
                    if (app != null) Application.removeApplication(app);
                    throw new AuthenticationException("ClassNotFoundException: " + e.getMessage());
                } catch (InstantiationException e) {
                    if (app != null) Application.removeApplication(app);
                    throw new AuthenticationException("InstantiationException: " + e.getMessage());
                } catch (IllegalAccessException e) {
                    if (app != null) Application.removeApplication(app);
                    throw new AuthenticationException("IllegalAccessException: " + e.getMessage());
                }

                long heartbeat = canvas.getHeartbeat();
                boolean sendUserDisconnect = canvas.doSendUserDisconnect();

                String grName = canvas.getGroup();
                if (grName == null)
                    grName = path;
                ConnectionGroup group = ConnectionGroup.getGroup(grName);

                app.setAgents(canvas.getAgents());

                app.setConnectionGroup(group);
                app.setAuthenticator(authenticator);
                app.setHeartbeat(heartbeat);
                app.setLastModifiedTime(lmt);
                app.setSendUserDisconnect(sendUserDisconnect);

                mLogger.debug("connected app settings " +
                              "- authenticator: " + authenticator +
                              ", senduserdisconnect: " + sendUserDisconnect +
                              ", heartbeat: " + heartbeat +
                              ", lmt: " + lmt);
            }

        } else if (app != null) {

            Application.removeApplication(app);

            mLogger.debug("Removed " + path + " as a connected application"); 
        }

        return app;
    }


    protected HashMap getAuthParam(HttpServletRequest req)
    {
        HashMap map = new HashMap();
        String authParam = req.getParameter("authparam");
        if (authParam != null) {
            authParam = URLDecoder.decode(authParam);
            String[] params = StringUtils.split(authParam, "&");
            for (int i=0; i < params.length; i++) {
                String[] kv = StringUtils.split(params[i], "=");
                if (kv.length == 2) {
                    map.put(kv[0], kv[1]);
                }
            }
        }

        if (mLogger.isDebugEnabled()) {
            Iterator iter=map.keySet().iterator();
            while (iter.hasNext()) {
                String k = (String) iter.next();
                mLogger.debug(k + ": "  + map.get(k));
            }
        }

        return map;
    }

    synchronized private Authentication getAuthenticator(String className)
        throws AuthenticationException, InstantiationException, 
               IllegalAccessException, ClassNotFoundException
    {
        if (className.equals("anonymous"))
            className = ANONYMOUS_AUTHENTICATOR;

        Authentication auth = (Authentication)mAuthenticators.get(className);
        if (auth == null) {
            auth = (Authentication)Class.forName(className).newInstance(); 
            auth.init(mLPSProperties);
            mAuthenticators.put(className, auth);
        }
        return auth;
    }

    public final int getMimeType()
    {
        return MIME_TYPE_SWF;
    }
}
