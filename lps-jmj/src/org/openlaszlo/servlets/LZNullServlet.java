/* *****************************************************************************
 * LZNullServlet.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets;

import java.io.*;
import java.net.*;
import java.util.*;
import javax.servlet.*;
import javax.servlet.http.*;

/**
 * LZNullServlet is a simple servlet for testing Tomcat performance.
 */
public class LZNullServlet extends HttpServlet {

    public void init (ServletConfig config) throws ServletException 
    {
        super.init (config);

        log("Initializing LZNullServlet!");

    }

    public void doGet(HttpServletRequest req, HttpServletResponse res) 
        throws IOException
    {
        res.setContentType ("text/html");
        ServletOutputStream  out = res.getOutputStream();

        out.println ("<html><head><title>X</title></head><body></body></html>");
        out.close();
    }

    public void doPost(HttpServletRequest req, HttpServletResponse res) 
        throws IOException
    {
        doGet(req, res);
    }
}

