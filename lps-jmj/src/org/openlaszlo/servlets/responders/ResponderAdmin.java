/******************************************************************************
 * ResponderAdmin.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.util.Properties;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.log4j.Logger;

public abstract class ResponderAdmin extends Responder
{
    private static boolean mIsInitialized = false;
    private static Object mIsInitializedLock = new Object();
    private static String mAdminPassword = null;

    private static Logger mLogger = Logger.getLogger(ResponderAdmin.class);

    /** Set default property for allowRequest to false for admin requests. */
    protected ResponderAdmin()
    {
        super();
        mAllowRequestDefaultProperty = "false";
    }

    abstract protected void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException;


    synchronized public void init(String reqName, ServletConfig config, Properties prop)
        throws ServletException, IOException
    {
        super.init(reqName, config, prop);

        if (! mIsInitialized) {
            mAdminPassword = prop.getProperty("adminPassword", null);
            mIsInitialized = true;
        }
    }    

    protected final void respondImpl(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        String pwd = req.getParameter("pwd");
        if ( mAdminPassword != null ) {
            if ( pwd == null || ! pwd.equals(mAdminPassword) ) {
                String lzt = req.getParameter("lzt");
                respondWithError(res, "Forbidden: " + lzt,
                                 HttpServletResponse.SC_FORBIDDEN);
                mLogger.info("Forbidden: " + lzt);
                return;
            }
        }
        respondAdmin(req, res);
    }    
}
