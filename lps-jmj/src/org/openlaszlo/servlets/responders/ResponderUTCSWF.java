/******************************************************************************
 * ResponderUTCSWF.java
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
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.SWFUtils;
import org.apache.log4j.Logger;

/**
 * Send out a response that current utc in a SWF
 */
public final class ResponderUTCSWF extends ResponderAdmin
{
    private static Logger mLogger = Logger.getLogger(ResponderUTCSWF.class);

    public void init(String reqName, ServletConfig config, Properties prop)
        throws ServletException, IOException
    {
        super.init(reqName, config, prop);
    }

    protected void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        ServletOutputStream out = res.getOutputStream();
        InputStream in = null;
        try {
            res.setContentType ("application/x-shockwave-flash");
            long utc = System.currentTimeMillis();
            String s = "" + utc;
            in = SWFUtils.getErrorMessageSWF(s);
            res.setContentLength(in.available());
            FileUtils.sendToStream(in, out);
        } catch (FileUtils.StreamWritingException e) {
            mLogger.warn("StreamWritingException while sending utcswf: " + e.getMessage());
        } finally {
            FileUtils.close(in);
            FileUtils.close(out);
        }
    }

    public int getMimeType()
    {
        return MIME_TYPE_SWF;
    }
}
