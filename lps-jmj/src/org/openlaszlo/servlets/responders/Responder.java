/******************************************************************************
 * Responder.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Date;
import java.util.Properties;
import java.util.Iterator;
import javax.servlet.ServletConfig;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpUtils;

import org.openlaszlo.compiler.Canvas;
import org.openlaszlo.compiler.CompilationError;
import org.openlaszlo.media.MimeType;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.LZHttpUtils;
import org.openlaszlo.utils.SWFUtils;
import org.openlaszlo.server.LPS;
import org.openlaszlo.xml.internal.DataCompiler;
import org.openlaszlo.xml.internal.XMLUtils;
import org.openlaszlo.servlets.LoadCount;
import org.apache.log4j.Logger;

public abstract class Responder 
{
    public static final int MIME_TYPE_SWF  = 0;
    public static final int MIME_TYPE_HTML = 1;
    public static final int MIME_TYPE_XML  = 2;

    public static final String LZCOOKIE = "lzc";

    /** Checks to see if request should be allowed. */ 
    private boolean mAllowRequest = true;

    /** Default request authorization string. */
    protected String mAllowRequestDefaultProperty = "true";

    // Class properties.
    protected static ServletContext mContext = null;

    private static int mErrorSWFCount = 0;
    private static Object mErrorSWFCountLock = new Object();
    private static boolean mIsInitialized = false;
    private static boolean mEmitErrorHeader = false;
    private static boolean mUseBogusErrorCode = false;
    private static Logger mLogger = Logger.getLogger(Responder.class);

    protected int mSWFVersionNum = -1;

    // Special logger for exceptions
    private static Logger mExceptionStackTraceLogger = 
        Logger.getLogger("org.openlaszlo.exceptions");

    //------------------------------------------------------------
    // For statistics
    //------------------------------------------------------------
    public static Date      mSTAT_startDate = new Date(); 

    public static LoadCount mSTAT_otherLoadCount   = new LoadCount(10);
    public static LoadCount mSTAT_allLoadCount     = new LoadCount(10);
    public static LoadCount mSTAT_compileLoadCount = new LoadCount(10);
    public static LoadCount mSTAT_mediaLoadCount   = new LoadCount(10);
    public static LoadCount mSTAT_dataLoadCount    = new LoadCount(10);

    // FIXME: [2003-25-07] bloch for pkang - this should be rearchitected 
    public static Class     mSTAT_adminClass;
    public static Class     mSTAT_connectClass;
    public static Class     mSTAT_compileClass;
    public static Class     mSTAT_mediaClass;
    public static Class     mSTAT_dataClass;

    public static Class     mCompilerClass;

    protected static boolean mCollectStat = true;

    /** This actually implements the responder. */
    protected abstract void respondImpl(HttpServletRequest req,
                                        HttpServletResponse res)
        throws IOException;

    /** 
     * This is the mime-type of the responder.  At the moment, this only gets
     * used by the base class to determine how to respond with errors and
     * exceptions. 
     * @return integer indicating mime-type. See MIME_TYPE properties in this
     * class.
     */
    public abstract int getMimeType();


    /** This needs to get called after the instantiation of the class object. */
    public synchronized void init(String reqName, ServletConfig config, 
                                  Properties prop)
        throws ServletException, IOException
    {
        mAllowRequest = 
            prop.getProperty("allowRequest" + reqName.toUpperCase(), 
                             mAllowRequestDefaultProperty).intern() == "true";

        if (! mIsInitialized) {

            mContext = config.getServletContext();

            try {

                // Some of these class objects are used by subclasses.
                mSTAT_adminClass = 
                    Class.forName("org.openlaszlo.servlets.responders.ResponderAdmin");
                mSTAT_connectClass = 
                    Class.forName("org.openlaszlo.servlets.responders.ResponderCONNECT");
                mSTAT_compileClass = 
                    Class.forName("org.openlaszlo.servlets.responders.ResponderCompile");
                mSTAT_mediaClass = 
                    Class.forName("org.openlaszlo.servlets.responders.ResponderMEDIA");
                mSTAT_dataClass = 
                    Class.forName("org.openlaszlo.servlets.responders.ResponderDATA");

                mCompilerClass = 
                    Class.forName("org.openlaszlo.servlets.responders.ResponderCompile");

            } catch (ClassNotFoundException e) {
                throw new ServletException(e.getMessage());
            }

            mEmitErrorHeader = 
                prop.getProperty("emitErrorHeader", "false").intern() == "true";
            mUseBogusErrorCode = 
                prop.getProperty("useBogusErrorCode", "false").intern() == "true";

            mCollectStat = 
                prop.getProperty("collectStat", "true").intern() == "true";

            mIsInitialized = true;
        }
    }

    /**
     * @return a file handle for the given directory string
     * @throws IOException if there is a problem with the directory
     */
    protected File checkDirectory(String cacheDir) throws IOException {

        File dir = new File(cacheDir);
        try {
            dir.mkdirs();
        } catch (SecurityException e) { }
        try {
            dir.mkdir();
        } catch (SecurityException e) { }
        if ( !dir.isDirectory() ) {
            throw new IOException(cacheDir + " is not a directory.");
        }
        try {
            if ( ! dir.canRead() ) {
                throw new IOException("can't read " + cacheDir);
            }
        } catch (SecurityException e) {
            throw new IOException("can't read " + cacheDir);
        }
        try {
            if ( ! dir.canWrite() ) {
                mLogger.warn(cacheDir + " is not writable.");
            }
        } catch (SecurityException e) {
            // This is info because it's likely configged to be that way.
            mLogger.info(cacheDir + " is not writable.");
        }
        return dir;
    }

    public void respond(HttpServletRequest req, HttpServletResponse res)
    {
        try {

            if ( ! mAllowRequest ) {
                String lzt = req.getParameter("lzt");
                String msg = "Forbidden request type: " + lzt;
                res.sendError(HttpServletResponse.SC_FORBIDDEN, msg);
                mLogger.info(msg);
                return;
            }

            // Don't include admin and connection reqs in stats.
            if ( mCollectStat &&
                 ! mSTAT_adminClass.isInstance(this) &&
                 ! mSTAT_connectClass.isInstance(this)) {

                LoadCount lc = mSTAT_otherLoadCount;
                if (mSTAT_compileClass.isInstance(this))
                    lc = mSTAT_compileLoadCount;
                else if (mSTAT_mediaClass.isInstance(this))
                    lc = mSTAT_mediaLoadCount;
                else if (mSTAT_dataClass.isInstance(this))
                    lc = mSTAT_dataLoadCount;

                long t0 = new Date().getTime();
                mSTAT_allLoadCount.increment();
                lc.increment();

                try {
                    mSWFVersionNum = LPS.getSWFVersionNum(req);
                    respondImpl(req, res);
                } finally {
                    long t1 = new Date().getTime();
                    int d = (int) (t1 - t0);
                    mSTAT_allLoadCount.decrement( d );
                    lc.decrement( d );
                }

            } else {

                respondImpl(req, res);

            }

        } catch (CompilationError e) {
            respondWithError(res, e.getMessage(), 0); 
        } catch (IOException e) {
            respondWithException(res, e); 
        } catch (Exception e) {
            respondWithException(res, e); 
        }
    }

    protected void respondWithError(HttpServletResponse res, String m, int status) 
    {
        switch (getMimeType()) {

            case MIME_TYPE_SWF  :
                respondWithErrorSWF(res, m); 
                break;
            case MIME_TYPE_HTML :
                respondWithErrorHTML(res, m);
                break;
            case MIME_TYPE_XML    :
                respondWithErrorXML(res, xmlErrorMsg(status, m));
                break;
            default:
                throw new ChainedException("Responder mime type unknown");
        }
    }

    public void respondWithMessage(HttpServletResponse res, String msg)
        throws IOException {
        String surl;

        switch (getMimeType()) {
            case MIME_TYPE_SWF  :
                respondWithMessageSWF(res, msg);
                break;
            case MIME_TYPE_HTML :
                respondWithErrorHTML(res, msg);
                break;
            case MIME_TYPE_XML    :
                respondWithErrorXML(res, msg);
                break;
            default:
                throw new ChainedException("Responder mime type unknown");
        }
    }


    /**
     * Send a SWF response indicating the error.
     */
    protected void respondWithErrorSWF(HttpServletResponse res, String s) 
    {
        mLogger.error("Responding with error SWF: " + s);
        ServletOutputStream out = null;
        InputStream in = null;
        try {
            res.setContentType(MimeType.SWF);
            if (mEmitErrorHeader) {
                // Make sure not to put newlines in the header
                res.setHeader("X-LPS", s.replace('\n', '_'));
            }
            if (mUseBogusErrorCode)
                res.setStatus(700);

            synchronized (mErrorSWFCountLock) {
                mErrorSWFCount++;
            }

            // Unknown.
            // res.setContentLength(output.length);

            out = res.getOutputStream();
            String buf = xmlErrorMsg(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, s);
            in = DataCompiler.compile(buf.toString(), mSWFVersionNum);
            FileUtils.sendToStream(in, out);
        } catch (FileUtils.StreamWritingException e) {
            mLogger.warn("StreamWritingException while sending error: " + e.getMessage());
        } catch (Exception e) {
            mLogger.warn("Exception while sending error: " + e.getMessage());
            mExceptionStackTraceLogger.error("exception", e);
        } finally {
            FileUtils.close(in);
            FileUtils.close(out);
        }
    }

    /**
     * Send a SWF response indicating the error.
     */
    public static void respondWithMessageSWF(HttpServletResponse res, String s) 
    {
        if (mUseBogusErrorCode)
            res.setStatus(700);

        synchronized (mErrorSWFCountLock) {
           mErrorSWFCount++;
        }

        ServletOutputStream out = null;
        InputStream in = null;
        try {
            out = res.getOutputStream();
            in = SWFUtils.getErrorMessageSWF(s);
            if (in != null) {
                res.setContentType(MimeType.SWF);
                FileUtils.sendToStream(in, out);
            }
        } catch (FileUtils.StreamWritingException e) {
            mLogger.warn("StreamWritingException while sending message: " + e.getMessage());
        } catch (Exception e) {
            mLogger.warn("Exception while sending message: " + e.getMessage());
            mExceptionStackTraceLogger.error("exception", e);
        } finally {
            FileUtils.close(in);
            FileUtils.close(out);
        }
    }


    /**
     * Send a SWF response indicating the exception
     */
    protected void respondWithExceptionSWF(HttpServletResponse res, Throwable e)
    {
        mExceptionStackTraceLogger.error(e.getMessage(), e);
        StringWriter s = new StringWriter();
        PrintWriter p = new PrintWriter(s);
        e.printStackTrace(p);
        respondWithErrorSWF(res, e.getMessage() + " : Exception stack: " + s.toString());
    }


    /**
     * Send an HTML response indicating the error.
     */
    public static void respondWithErrorHTML(HttpServletResponse res, String s) 
    {
        mLogger.info("Responding with error (text/html): " + s);
        ServletOutputStream out = null;
        try {
            res.setContentType ("text/html");
            out = res.getOutputStream();
            writeHeader(out, null);
            out.print("<pre>");
            out.println("Error: " + XMLUtils.escapeXmlForSWFHTML(s));
            out.println("</pre>");
            writeFooter(out);
        } catch (Exception e) {
            mLogger.warn("Exception while sending error HTML: " + e.getMessage());
            mExceptionStackTraceLogger.error("exception", e);
        } finally {
            if (out != null) {
                try {
                    out.close();
                } catch (Exception e) {
                }
            }
        }
    }

    /**
     * Send an XML response indicating the error.
     */
    protected void respondWithErrorXML(HttpServletResponse res, String s) 
    {
        mLogger.info("Responding with error (text/xml): " + s);
        ServletOutputStream out = null;
        try {
            res.setContentType ("text/xml");
            out = res.getOutputStream();
            out.println("<lps-error>" + XMLUtils.escapeXml(s) + "</lps-error>");
        } catch (Exception e) {
            mLogger.warn("Exception while sending error XML: " + e.getMessage());
            mExceptionStackTraceLogger.error("exception", e);
        } finally {
            FileUtils.close(out);
        }
    }


    /**
     * Send an XML response.
     */
    protected void respondWithXML(HttpServletResponse res, String xml) 
        throws IOException 
    {
        ServletOutputStream out = null;
        try {
            res.setContentType ("text/xml");
            out = res.getOutputStream();
            out.println("<lps>");
            out.println(xml);
            out.println("</lps>");
        } catch (Exception e) {
            mLogger.warn("Exception while sending XML: " + e.getMessage());
            mExceptionStackTraceLogger.error("exception", e);
        } finally {
            FileUtils.close(out);
        }
    } 


    /**
     * Send a SWF response indicating the exception.
     */
    protected void respondWithException(HttpServletResponse res, Exception e)
    {
        String m = e.getMessage();
        StringWriter s = new StringWriter();
        PrintWriter p = new PrintWriter(s);
        e.printStackTrace(p);
        if (m == null) {
            m = s.toString();
        } else {
            m += s.toString();
        }
        
        respondWithError(res, m,
                         HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
    }


    /** 
     * Sends a successful SWF status response.
     *
     * @param res client's servlet response
     * @param status status code (should be SC_OK)
     * @param mesg status message
     * @param serial serial number of request to echo back 
     */
    protected void respondWithStatusSWF(HttpServletResponse res, int status, 
                                        String mesg, int serial)
    {
        respondWithStatusSWF(res, status, mesg, null, serial);
    }


    /** 
     * Sends a successful SWF status response w/arbitrary xml.
     *
     * @param res client's servlet response
     * @param status status code (should be SC_OK)
     * @param mesg status message
     * @param xmlBody arbitrary xml
     * @param serial serial number of request to echo back 
     */
    protected void respondWithStatusSWF(HttpServletResponse res, int status, 
                                        String mesg, String xmlBody, int serial)
    {
        res.setContentType(MimeType.SWF);

        String _mesg = XMLUtils.escapeXml(mesg);

        // Successful response codes end up as an attribute of resultset. We
        // should standardize where the response code ends up.
        String xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" 
            + "<!DOCTYPE laszlo-data>" 
            + "<resultset s=\"" + serial + "\">"
            + "<success code=\"" + status + "\" msg=\"" + _mesg + "\" />"
            + ( xmlBody!=null ? xmlBody : "" )
            + "</resultset>";

        mLogger.debug("respondWithStatusSWF: " + xml);

        ServletOutputStream sos = null;
        try {
            sos = res.getOutputStream();
            InputStream swfbytes = DataCompiler.compile(xml, mSWFVersionNum);
            FileUtils.sendToStream(swfbytes, sos);
        } catch (FileUtils.StreamWritingException e) {
            mLogger.warn("StreamWritingException while sending status: " + e.getMessage());
        } catch (Exception e) {
            mLogger.warn("Exception while sending status: " + e.getMessage());
            mExceptionStackTraceLogger.error("exception", e);
        } finally {
            FileUtils.close(sos);
        }
    }

    /**
     * Respond with an "over limit" message
     */
    void respondWithOverLimitMessage(HttpServletRequest req, HttpServletResponse res) 
        throws IOException {
        StringBuffer url = HttpUtils.getRequestURL(req);
        String msg = LPS.getProperty("messages.over-limit", 
            "The Laszlo Presentation Server that is responsible for serving " + 
             url.toString() + " is over its license limit.   The site administrator has been notified.");
         respondWithMessage(res, msg);
     }

    /**
     * Creates an XML error message.
     * 
     * @param status integer status. Using HTTP status codes, for now, but
     * Laszlo status codes would be better.
     * @param msg message to display.
     */
    protected String xmlErrorMsg(int status, String msg)
    {
        return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
            "<!DOCTYPE laszlo-data>" +
            "<resultset><error status=\"" + status + "\" msg=\"" +
            XMLUtils.escapeXml(msg) +
            "\"/></resultset>";
    }

    /**
     * Writes the html header tags
     * @param out A PrintWriter
     */    
    protected static void writeHeader(ServletOutputStream out, Canvas c) 
        throws IOException
    {
        String bgc = "";
        String title = "";
        if (c != null) {
            bgc = "bgcolor=\"" + c.getBGColorString() + "\"";
            title = c.getTitle();
        }

        // Add in title and link
        out.println("<html><head><title>" + title + "</title>");

        String ico = LPS.getProperty("shortcut.icon", "http://www.laszlosystems.com/images/laszlo.ico");
        out.println("<link rel=\"SHORTCUT ICON\" href=\"" + ico + 
                    "\"></head>\n");
        out.println("<body " + bgc + 
        " marginwidth=\"0\" marginheight=\"0\" topmargin=\"0\" leftmargin=\"0\">");
    }
    
    /**
     * Writes the html footer tags
     */
    protected static void writeFooter(ServletOutputStream out) 
        throws IOException
    {
        out.println ("</body></html>");
    }

    protected static int getErrorSWFCount()
    {
        synchronized (mErrorSWFCountLock) {
            return mErrorSWFCount;
        }
    }

    protected static void clearErrorSWFCount()
    {
        synchronized (mErrorSWFCountLock) {
            mErrorSWFCount = 0;
        }
    }
}
