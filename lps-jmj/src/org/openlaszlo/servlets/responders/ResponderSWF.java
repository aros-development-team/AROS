/******************************************************************************
 * ResponderSWF.java
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

public final class ResponderSWF extends ResponderCompile
{
    private static Logger mLogger = Logger.getLogger(ResponderSWF.class);

    private Object mKrankEncodingLock = new Object();

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
    {
        ServletOutputStream output = null;
        InputStream input = null;

        // Is this a request for an optimized file?
        boolean opt = fileName.endsWith(".lzo");

        // Compile the file and send it out
        try {
            mLogger.info("Requesting object for " + fileName);

            output = res.getOutputStream();
            Properties props = initCMgrProperties(req);
            String encoding = props.getProperty(LZHttpUtils.CONTENT_ENCODING);

            if (opt) {
                String objName = fileName;
                File obj = new File(objName);
                objName += ".gz";
                File gz = new File(objName); 
                // TODO: [2004-03-12 bloch] When we move to 1.4, we could use
                // per-file locking (java.io.FileLock) to avoid the global lock here.
                synchronized (mKrankEncodingLock) {
                    if (encoding != null && encoding.equals("gzip")) {
                        // Make sure gz is uptodate with obj
                        if (!gz.exists() || gz.lastModified() < obj.lastModified()) {
                            mLogger.info("Encoding into " + objName);
                            FileUtils.encode(obj, gz, "gzip");
                        }
                        input = new FileInputStream(objName);
                    } else {
                        // Simply make sure obj exists
                        if (!obj.exists()) {
                            mLogger.info("Decoding into " + objName);
                            FileUtils.decode(gz, obj, "gzip");
                        }
                        input = new FileInputStream(fileName);
                    }
                }
            } else {
                input = mCompMgr.getObjectStream(fileName, props);
            }

            long total = input.available();
            // Set length header before writing content.  WebSphere
            // requires this.
            // Ok to cast to int because SWF file must be a 32bit file
            res.setContentLength((int)total);
            res.setContentType(MimeType.SWF);
            if (encoding != null) {
                res.setHeader(LZHttpUtils.CONTENT_ENCODING, encoding);
            }

            try {
                total = 0;
                total = FileUtils.sendToStream(input, output);
            } catch (FileUtils.StreamWritingException e) {
                // This should be the client hanging up on us.
                mLogger.warn("StreamWritingException while sending SWF: " + e.getMessage());
            } catch (IOException e) {
                mLogger.error("IO exception while sending SWF: ", e);
            } 
            mLogger.info("Sent SWF, " + total + " bytes");

        } catch (Exception e) {
            mLogger.error("Exception: ", e);
            StringWriter s = new StringWriter();
            PrintWriter p = new PrintWriter(s);
            e.printStackTrace(p);
            respondWithMessageSWF (res, s.toString());
        } finally {
            FileUtils.close(input);
            FileUtils.close(output);
        }
    }

    public int getMimeType()
    {
        return MIME_TYPE_SWF;
    }

    protected void handleCompilationError(CompilationError e,
                                          HttpServletRequest req,
                                          HttpServletResponse res)
        throws IOException
    {
        respondWithMessageSWF(res, e.getMessage());
    }
}
