/* ****************************************************************************
 * LZWebAppRemote.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote;

import java.util.*;
import javax.servlet.http.*;
import javax.servlet.*;

public class LZWebAppRemote
{
    public static Object getAttribute(String name, HttpServletRequest req) {
        ServletContext context = req.getSession().getServletContext();
        return context.getAttribute(name);
    }

    public static Vector getAttributeNames(HttpServletRequest req) {
        ServletContext context = req.getSession().getServletContext();
        Vector v = new Vector();
        Enumeration e = context.getAttributeNames();
        while (e.hasMoreElements()) {
            v.add(e.nextElement());
        }
        return v;
    }

    public static int getMajorVersion(HttpServletRequest req) {
        ServletContext context = req.getSession().getServletContext();
        return context.getMajorVersion();
    }

    public static int getMinorVersion(HttpServletRequest req) {
        ServletContext context = req.getSession().getServletContext();
        return context.getMinorVersion();
    }

    public static String getMimeType(String file, HttpServletRequest req) {
        ServletContext context = req.getSession().getServletContext();
        return context.getMimeType(file);
    }

    public static String getServerInfo(HttpServletRequest req) {
        ServletContext context = req.getSession().getServletContext();
        return context.getServerInfo();
    }

    public static String getServletContextName(HttpServletRequest req) {
        ServletContext context = req.getSession().getServletContext();
        return context.getServletContextName();
    }

    public static void log(String msg, HttpServletRequest req) {
        ServletContext context = req.getSession().getServletContext();
        context.log(msg);
    }

    public static void removeAttribute(String name, HttpServletRequest req) {
        ServletContext context = req.getSession().getServletContext();
        context.removeAttribute(name);
    }

    public static void setAttribute(String name, String value, HttpServletRequest req) {
        _setAttribute(name, value, req);
    }

    public static void setAttribute(String name, int value, HttpServletRequest req) {
        _setAttribute(name, new Integer(value), req);
    }

    public static void setAttribute(String name, double value, HttpServletRequest req) {
        _setAttribute(name, new Double(value), req);
    }

    public static void setAttribute(String name, float value, HttpServletRequest req) {
        _setAttribute(name, new Float(value), req);
    }

    public static void setAttribute(String name, Vector value, HttpServletRequest req) {
        _setAttribute(name, value, req);
    }

    public static void setAttribute(String name, Hashtable value, HttpServletRequest req) {
        _setAttribute(name, value, req);
    }

    static void _setAttribute(String name, Object value, HttpServletRequest req) {
        ServletContext context = req.getSession().getServletContext();
        context.setAttribute(name, value);
    }
}
