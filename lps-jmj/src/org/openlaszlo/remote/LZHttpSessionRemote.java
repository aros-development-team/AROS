/******************************************************************************
 * LZHttpSessionRemote.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote;

import java.util.*;
import javax.servlet.http.*;

public class LZHttpSessionRemote
{
    public static Object getAttribute(String name, HttpServletRequest req) {
        HttpSession session = req.getSession();
        return session.getAttribute(name);
    }

    public static Vector getAttributeNames(HttpServletRequest req) {
        HttpSession session = req.getSession();
        Vector v = new Vector();
        Enumeration e = session.getAttributeNames();
        while (e.hasMoreElements()) {
            v.add(e.nextElement());
        }
        return v;
    }


    public static String getId(HttpServletRequest req) {
        HttpSession session = req.getSession();
        return session.getId();
    }

    public static int getMaxInactiveInterval(HttpServletRequest req) {
        HttpSession session = req.getSession();
        return session.getMaxInactiveInterval();
    }

    public static void invalidate(HttpServletRequest req) {
        HttpSession session = req.getSession();
        session.invalidate();
    }

    public static boolean isNew(HttpServletRequest req) {
        HttpSession session = req.getSession();
        return session.isNew();
    }

    public static void removeAttribute(String name, HttpServletRequest req) {
        HttpSession session = req.getSession();
        session.removeAttribute(name);
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

    /** javascript arrays are passed in as Vectors. */
    public static void setAttribute(String name, Vector value, HttpServletRequest req) {
        _setAttribute(name, value, req);
    }

    /** javascript objects are passed in as Hashtables. */
    public static void setAttribute(String name, Hashtable value, HttpServletRequest req) {
        _setAttribute(name, value, req);
    }

    static void _setAttribute(String name, Object value, HttpServletRequest req) {
        HttpSession session = req.getSession();
        session.setAttribute(name, value);
    }

    public static void setMaxInactiveInterval(int interval, HttpServletRequest req) {
        HttpSession session = req.getSession();
        session.setMaxInactiveInterval(interval);
    }
}
