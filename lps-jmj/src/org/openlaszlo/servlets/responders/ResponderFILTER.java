/******************************************************************************
 * ResponderFILTER.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.net.URL;
import java.util.Properties;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.openlaszlo.utils.LZHttpUtils;
import org.openlaszlo.utils.StringUtils;
import org.apache.log4j.Logger;

public final class ResponderFILTER extends ResponderCompile
{
    private static Logger mLogger = Logger.getLogger(ResponderFILTER.class);

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
        mLogger.info("Responding with FILTER for " + fileName);
        if (fileName.endsWith(".lzo")) {
            fileName = StringUtils.setCharAt(fileName, fileName.length() - 1, 'x');
        }

        try {
            // Give the filter our compilation mgr and tell it our uri
            // and filename requested. (would like to send req, but Tomcat 3.3
            // bug prevents it.  neato.
            URL url = LZHttpUtils.getRequestURL(req);
            req.setAttribute ("LZF_URL",      url.toString());
            req.setAttribute ("LZF_FILENAME", fileName);
            req.setAttribute ("LZF_COMPMGR", mCompMgr);
            String FILTER = req.getParameter("filter");
            mLogger.debug("FILTER is " + FILTER);
            RequestDispatcher dispatcher = mContext.getRequestDispatcher(FILTER);
            if (dispatcher==null) {
                res.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
                              "Bad or undefined filter: " + FILTER);
                mLogger.debug("Bad or undefined filter: " + FILTER);
                return;
            }
            dispatcher.forward(req, res);

        } catch (IOException e) {
            // FIXME: [2002-12-31 bloch]
            // Chances are, this is the client closing the
            // connection.  We should probably just bag here
            // rather than attempting to respond again
            // For sure, printing the stack trace is wrong.
            e.printStackTrace();
        } catch (Exception e) {
            respondWithException(res, e);
        }
    }

    public int getMimeType()
    {
        return MIME_TYPE_HTML;
    }

}
