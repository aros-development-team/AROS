/******************************************************************************
 * ResponderLIB.java
 *****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.util.Properties;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.openlaszlo.utils.LZHttpUtils;
import javax.servlet.ServletOutputStream;
import org.openlaszlo.cache.MediaCache;
import org.openlaszlo.cache.RequestCache;
import org.openlaszlo.server.LPS;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.media.MimeType;

import org.apache.log4j.Logger;

/**
 * Respond to library requests.
 * Currently does not use compilation manager, just looks for and returns
 * the requested swf file specified by the 'libpath' query arg.
 */

public final class ResponderLIB extends Responder
{
    private static Logger mLogger = Logger.getLogger(ResponderLIB.class);

    public int getMimeType()
    {
        return MIME_TYPE_SWF;
    }

    public void init(String reqName, ServletConfig config, Properties prop)
        throws ServletException, IOException
    { 
        super.init(reqName, config, prop);
    }

    /**
     * Delivers a 'snippet' compiled library file. 
     *
     * Gets the filename of the snippet we are loading from the
     * request, where it is specified in the "libpath" query
     * arg. 'libpath' is relative to the app base filename.  Thus, an
     * app at /intl2/test/snippets/main.lzx, which has &ltimport
     * href="lib/foo.lzx"&gt; will send a request with
     * lzt=lib&libpath=lib/foo.lzx
     */
    protected void respondImpl(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {

        try {
            String patharg = req.getParameter("libpath");
            if (patharg == null) {
                throw new IOException("could not find 'libpath' query arg in lzt=lib request");
            }

            /* getContextPath() = /intl2
               getPathInfo() = null
               getPathTranslated() = null
               getRequestURI() = /intl2/test/snippets/main.lzx
               getServletPath() = /test/snippets/main.lzx
            */


            ServletOutputStream out = res.getOutputStream();
            PrintWriter p = new PrintWriter(out);

            // canonicalize the separator char
            String path  = (new File(patharg)).getPath();
            String appbasedir = (new File(req.getServletPath())).getParent();
            String libpath;
            // Check if we are merging with an absolute or relative lib path.
            // Why doesn't Java have fs:merge-pathnames? 
            if (path.charAt(0) == File.separatorChar) {
                libpath = path;
            } else {
                libpath = (new File(appbasedir, path)).getPath();
            } 
            String filename = LZHttpUtils.getRealPath(mContext, libpath);
            mLogger.info("Responding with LIB for " + filename);
            res.setContentType(MimeType.SWF);
            InputStream ins = null;
            try {
                // open the file and return it.
                ins = new BufferedInputStream(new FileInputStream(filename));
                FileUtils.send(ins, out);
            } finally {
                FileUtils.close(out);
                FileUtils.close(ins);
            }
        } catch (java.io.FileNotFoundException e) {
            throw new IOException(e.getMessage());
        }
    }
}
    
