/******************************************************************************
 * SignMeUp.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test.xmlrpc;

//import org.openlaszlo.data.LZXMLRPC;
import java.io.*;
import java.net.*;
import java.util.*;
import javax.servlet.*;
import javax.servlet.http.*;
import org.apache.log4j.*;

public class SignMeUp
{
    private static Logger mLogger  = Logger.getLogger(SignMeUp.class);

    private String mAlias = "SignMeUp";

    public String[] getUniqueUsername(String username, String first, String last,
                                      HttpServletRequest req, HttpServletResponse res) {
        mLogger.debug("getUniqueUsername");
        String[] names = { "one", "two", "three", "four", "five", "six" };
        return names;
    }

    public int register(String username, String password, String name,
                        String occupation, String email,
                        HttpServletRequest req, HttpServletResponse res) {
        mLogger.debug("register");
        req.getSession().setAttribute("sessioned", "true");
        return 1;
    }

    public boolean isEmailRegistered(String email, HttpServletRequest req,
                                     HttpServletResponse res) {
        mLogger.debug("isEmailRegistered");
        return ("one@foo.com".equals(email));
    }

    public boolean isSessioned(HttpServletRequest req, HttpServletResponse res) {
        return "true".equals(req.getSession().getAttribute("sessioned"));
    }


    public boolean logout(HttpServletRequest req, HttpServletResponse res)
        throws IllegalStateException {
        req.getSession().invalidate();
        return true;
    }

    public boolean login(String username, String password,
                         HttpServletRequest req, HttpServletResponse res) {
        mLogger.debug("login");
        boolean ok = ("fluffy".equals(username) && "pass1".equals(password));
        if (ok) {
            req.getSession().setAttribute("sessioned", "true");
        }
        return ok;
    }

    public boolean getUsernamePassword(String email, HttpServletRequest req,
                                       HttpServletResponse res) {
        mLogger.debug("getUsernamePassword");
        return false;
    }

    public void initialize(Properties prop)
    {
        mLogger.debug("initialize");
    }
}
