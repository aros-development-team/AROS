/******************************************************************************
 * ResponderXML.java
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
import javax.servlet.http.HttpSession;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.StringUtils;
import org.apache.log4j.Logger;

public final class ResponderXML extends ResponderCompile
{
    private static Logger mLogger = Logger.getLogger(ResponderXML.class);

    /**
     * Overridden method from ReponseCompile.
     *
     * @param req unused
     */
    protected long getLastModified(String fileName, HttpServletRequest req)
    {
        // We don't care about other dependencies since all we show is the
        // top-level LZX file.
        return new File(fileName).lastModified();
    }

    /**
     * @param fileName Full pathname to file from request.
     */
    protected void respondImpl(String fileName, HttpServletRequest req, 
                               HttpServletResponse res)
        throws IOException
    {
        mLogger.info("Responding with XML for " + fileName);
        if (fileName.endsWith(".lzo")) {
            fileName = StringUtils.setCharAt(fileName, fileName.length() - 1, 'x');
        }
    
        ServletOutputStream  out = res.getOutputStream();
        try {
            res.setContentType ("text/xml");
            out.print(FileUtils.readFileString(new File(fileName)));
        } finally {
            FileUtils.close(out);
        }
    }

    public int getMimeType()
    {
        return MIME_TYPE_XML;
    }
}
