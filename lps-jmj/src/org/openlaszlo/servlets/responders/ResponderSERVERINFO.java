/******************************************************************************
 * ResponderSERVERINFO.java
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
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.openlaszlo.server.LPS;
import org.openlaszlo.utils.FileUtils;
import org.apache.log4j.Logger;

public final class ResponderSERVERINFO extends ResponderAdmin
{
    private static Logger mLogger = Logger.getLogger(ResponderSERVERINFO.class);

    protected void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        ServletOutputStream out = res.getOutputStream();
        try {
            res.setContentType("text/xml");
            out.println(LPS.getInfo(req, mContext, "lps-server-info"));
        } finally {
            FileUtils.close(out);
        }
    }

    public int getMimeType()
    {
        return MIME_TYPE_XML;
    }
}
