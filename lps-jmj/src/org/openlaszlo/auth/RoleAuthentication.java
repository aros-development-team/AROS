/* *****************************************************************************
 * RoleAuthentication.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.auth;

import org.openlaszlo.data.*;
import org.openlaszlo.server.*;
import org.openlaszlo.servlets.*;
import org.openlaszlo.utils.*;
import java.io.*;
import java.net.*;
import java.security.*;
import java.util.*;
import javax.servlet.http.*;
import org.apache.commons.httpclient.*;
import org.apache.commons.httpclient.methods.*;
import org.apache.log4j.*;
import org.jdom.*;
import org.jdom.input.*;


/** 
 * Role implementation of Authentication. 
 *
 * This class implements the Authentication interface 
 * methods.  Every public member is an implementation of
 * the Authentication interface.  
 **/
public class RoleAuthentication implements Authentication
{
    /** RoleAuthentication logger */
    protected static Logger mLogger = Logger.getLogger(RoleAuthentication.class);

    public void init(Properties prop) 
    {
    }


    public int login(HttpServletRequest req, HttpServletResponse res,
                     HashMap param, StringBuffer xmlResponse)
    {

        mLogger.debug("login(req,res,param,xmlResponse)");
        String role = req.getParameter("role");
        return req.isUserInRole(role) ? 0 : 1;
    }

    public int logout(HttpServletRequest req, HttpServletResponse res,
                      HashMap param, StringBuffer xmlResponse)
    {

        mLogger.debug("logout(req,res,param,xmlResponse)");
        // get the current session, if it doesn't exist, we can't logout
        HttpSession sess = req.getSession(false);
        if (sess == null) {
            return 1;
        } else {
            req.getSession().invalidate();
            return 0;
        }
    }


    public String getUsername(HttpServletRequest req, HttpServletResponse res,
                              HashMap param)
    {
        mLogger.debug("getUsername(req,res,param)");
            return req.getRemoteUser();
    }

}
