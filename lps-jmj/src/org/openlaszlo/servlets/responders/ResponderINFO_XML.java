/******************************************************************************
 * ResponderINFO_XML.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.net.URL;
import java.util.Hashtable;
import java.util.Properties;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.openlaszlo.media.MimeType;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.LZHttpUtils;
import org.openlaszlo.utils.StringUtils;
import org.openlaszlo.compiler.CompilationError;
import org.apache.log4j.Logger;

public final class ResponderINFO_XML extends ResponderCompile
{
    private static Logger mLogger = Logger.getLogger(ResponderINFO_XML.class);

    public void init(String reqName, ServletConfig config, Properties prop)
        throws ServletException, IOException
    {
        super.init(reqName, config, prop);
    }


    /**
     * @param fileName Full pathname to file from request.
     */
    protected void respondImpl(String fileName, HttpServletRequest req, 
                               HttpServletResponse res)
        throws IOException
    {
        ServletOutputStream output = null;

        if (fileName.endsWith(".lzo")) {
            fileName = StringUtils.setCharAt(fileName, 
                                             fileName.length() - 1, 'x');
        }

        // Compile the file and send it out
        try {
            mLogger.info("Requesting info_xml for " + fileName);

            output = res.getOutputStream();
            Properties props = initCMgrProperties(req);
            String info = mCompMgr.getInfoXML(fileName, props);
            res.setContentType(MimeType.XML);

            output.print(info);

        } catch (Exception e) {
            respondWithException(res, e);
        } finally {
            FileUtils.close(output);
        }
    }

    public int getMimeType()
    {
        return MIME_TYPE_XML;
    }

    protected void handleCompilationError(CompilationError e,
                                          HttpServletRequest req,
                                          HttpServletResponse res)
        throws IOException
    {
        respondWithErrorXML(res, e.getMessage());
    }
}
