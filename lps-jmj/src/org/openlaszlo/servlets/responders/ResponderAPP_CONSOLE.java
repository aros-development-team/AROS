/******************************************************************************
 * ResponderAPP_CONSOLE.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.net.URLEncoder;
import java.util.*;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpSession;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.openlaszlo.compiler.Canvas;
import org.openlaszlo.compiler.CompilationError;
import org.openlaszlo.server.LPS;
import org.openlaszlo.utils.*;
import org.openlaszlo.xml.internal.XMLUtils;
import org.apache.log4j.Logger;

public final class ResponderAPP_CONSOLE extends ResponderCompile
{
    private static Logger mLogger = Logger.getLogger(ResponderAPP_CONSOLE.class);
    private static String sStyleSheetPathname =
        org.openlaszlo.server.LPS.getTemplateDirectory() +
        File.separator + "app-console.xslt";
    
    /*
     * @param fileName Full pathname to file from request.
     */
    protected void respondImpl(String fileName, HttpServletRequest req, 
                               HttpServletResponse res)
        throws IOException
    {
        mLogger.info("Responding with HTML wrapper for " + fileName);
        res.setContentType("text/html");
        ServletOutputStream out = res.getOutputStream();
        try {
            // Get the canvas first, so that if this fails and we
            // write the compilation error, nothing has been written
            // to out yet.
            // Replace .lzo with .lzx 
            boolean isKranked = false;
            String orig = fileName;
            if (fileName.endsWith(".lzo")) {
                isKranked = true;
                fileName = fileName.substring(0, fileName.length() - 1) + "x";
            }
            /* This method doesn't call writeHeader and writeFooter, since
             * the stylesheet handles the whole HTML generation. */
            Canvas canvas = getCanvas(fileName, req);
            writeCanvas(out, req, canvas, orig, isKranked);
        } finally {
            FileUtils.close(out);
        }
    }
    
    static public String getRequestXML(HttpServletRequest req, String fileName)
        throws IOException
    {
        String lps  = req.getContextPath();
        String agent = req.getHeader("user-agent");
        // MS-specific header
        String os   = req.getHeader("ua-os");
        String query_args = getQueryArgs(req);
        String url  = req.getRequestURI();
        int i = url.lastIndexOf("/");
        url = url.substring(i + 1, url.length() );
        String unopturl = url.substring(0, url.length() - 4) + ".lzx";
        String opturl = url.substring(0, url.length() - 4) + ".lzo";
        boolean isPocketPC = (os != null && os.indexOf("POCKET PC") > -1);
        boolean isWin = (agent != null && agent.toLowerCase().indexOf("win") > -1);

        Properties props = initCMgrProperties(req);

        StringBuffer buffer = new StringBuffer();
        buffer.append( 
            "<request " +
            "lps=\"" + XMLUtils.escapeXml(lps) + "\" " +
            "url=\"" + XMLUtils.escapeXml(url) + "\" " +
            "fullpath=\"" + XMLUtils.escapeXml(req.getRequestURI()) + "\" " +
            "opt-url=\"" + XMLUtils.escapeXml(opturl) + "\" " +
            "unopt-url=\"" + XMLUtils.escapeXml(unopturl) + "\" " +
            "query_args=\"" + XMLUtils.escapeXml(query_args) + "\" " +
            "pocketpc=\"" + isPocketPC + "\" " +
            "windows=\"" + isWin + "\" ");
        if (fileName != null) {
            String optname = fileName.substring(0, fileName.length() - 4) + ".lzo";
            buffer.append(
                "opt-exists=\"" + new File(optname).exists() + "\" ");
        }
        buffer.append(
            ">\n");

        for (Enumeration e = req.getParameterNames(); e.hasMoreElements(); ) {
            String name = (String)e.nextElement();
            String value = req.getParameter(name);
            buffer.append("<param name=\"" + XMLUtils.escapeXml(name) + "\" ");
            buffer.append("value=\"" + XMLUtils.escapeXml(value) + "\"/>\n");
        }
        buffer.append("</request>");

        String infoXML = "";
        if (fileName != null) {
            boolean isOpt = fileName.endsWith(".lzo");
            if (isOpt) {
                infoXML = getInfoXML(fileName, props);
            } else {
                infoXML = getCompilationManager().getInfoXML(fileName, props);
            }
        }

        buffer.append(infoXML);

        return buffer.toString();
    }

    /**
     * @return info XML string for .lzo files
     */
    private static String getInfoXML(String fileName, Properties props)
        throws IOException
    {
        String enc = props.getProperty(LZHttpUtils.CONTENT_ENCODING);
        String zippedSize = null;
        String unzippedSize = null;
        String size = null;
        
        try {
            unzippedSize = "" + FileUtils.fileSize(new File(fileName));
        } catch (FileNotFoundException e) {
            mLogger.error(e.getMessage());
        }
        
        {
            String gzName = fileName + ".gz";
            File f = new File(gzName);
            // Handle case where .lzo exists but not .gz yet or 
            // gz out of date
            File nongz = new File(fileName);
            if (nongz.exists()) {
                if (!f.exists() || f.lastModified() < nongz.lastModified()) {
                    FileUtils.encode(nongz, f, "gzip");
                    f = new File(gzName);
                }
            }
            try {
                zippedSize = "" + FileUtils.fileSize(f);
            } catch (FileNotFoundException e) {
                mLogger.error(e.getMessage());
            }
        }
        
        size = ("gzip".equals(enc)) ? zippedSize : unzippedSize;
        
        return "<info size=\"" + size +
            "\" zipped-size=\"" + zippedSize +
            "\" unzipped-size=\"" + unzippedSize +
            "\" encoding=\"" + enc +
            "\" />";
    }
    
    protected void handleCompilationError(CompilationError e,
                                          HttpServletRequest req,
                                          HttpServletResponse res)
        throws IOException
    {
        respondCompilationError(e, req, res);
    }

    public static void respondCompilationError(CompilationError e,
                                               HttpServletRequest req,
                                               HttpServletResponse res)
        throws IOException
    {
        res.setContentType("text/html");
        ServletOutputStream out = res.getOutputStream();
        String xmlString = 
            "<errors>" +
            getRequestXML(req, null) +
            e.toXML() +
            "</errors>";
        try {
            TransformUtils.applyTransform(sStyleSheetPathname, xmlString, out);
        } catch (Exception ex) {
            reportTransformException(req, sStyleSheetPathname, ex, out);
        }
    }
    
    /**
     * Writes the canvas html.  The canvas is the area in which the
     * Laszlo application is rendered.
     * @param out <tt>ServletOutputStream</tt> to write on
     * @param req request to retrieve scheme, server name, server port and
     * requested url
     * @param canvas the canvas for the given request
     */
    private void writeCanvas(ServletOutputStream out, HttpServletRequest req, 
                             Canvas canvas, String fileName, boolean isKranked)
        throws IOException 
    {
        String xmlString = canvas.getXML(getRequestXML(req, fileName));
        mLogger.debug(xmlString);
        Properties properties = new Properties();
        if (isKranked)
            properties.setProperty("isKranked", "true");
        try {
            TransformUtils.applyTransform(sStyleSheetPathname, properties,
                                          xmlString, out);
        } catch (Exception e) {
            reportTransformException(req, sStyleSheetPathname, e, out);
        }
    }
    
    /** Report the exception to output as an internal server error,
     * rendered in HTML, with instructions for bug reporting and with
     * a solution message if one is available.
     */
    protected static void reportTransformException(
        HttpServletRequest req,
        String styleSheetPathname,
        Exception e,
        OutputStream out)
        throws IOException
    {
        // Note that the following doesn't quote HTML in the filename,
        // message, and stacktrace.  This is to make it as likely as
        // possible that this will succeed, since it's typically
        // called when the system is having trouble calling external
        // functions or libraries.  The failure mode is that under
        // rare cases the display could be messed up, but it will
        // still have enough information and the page source will have
        // everything.
        
        // Special solution message if the error contains this string:
        final String XALAN_ERR_MSG =
            "output format must have a '{http://xml.apache.org/xalan}content-handler' property";
        
        Writer writer = new OutputStreamWriter(out);
        try {
            String title = "500 Internal Server Error";
            writer.write("<html><title>");
            writer.write(title);
            writer.write("</title>");
            writer.write("<style type='text/css'>");
            writer.write("table th {text-align: right; vertical-align: top; padding-right: 10pt}");
            writer.write("</style>");
            writer.write("</head><body><h1>");
            writer.write(title);
            writer.write("</h1>");
            writer.write("<p>An internal server error occurred. ");
            // Use toString() instead of getMessage(), since for chained
            // exceptions it's possible for the original message to
            // get buried where the latter doesn't retrieve it.
            if (e.toString().indexOf(XALAN_ERR_MSG) >= 0) {
                writer.write("If you are running Tomcat, please verify that ");
                writer.write("that the <code>xalan.jar</code> file from the ");
                writer.write("LPS distribution has been installed in the ");
                writer.write("Tomcat <code>/common/endorsed</code> directory. ");
                writer.write("Otherwise, please check the ");
                writer.write("<a href='http://www.laszlosystems.com/developers/learn/documentation/faq/'>");
                writer.write("LPS Developer FAQ</a> for additional ");
                writer.write("instructions.  If that does not work, please ");
            } else {
                writer.write("Please ");
            }
            writer.write("send a copy of this web page and a ");
            writer.write("description of your server environment (operating ");
            writer.write("system, JRE version, servlet container and ");
            writer.write("version) to ");
            writer.write("<a href='mailto:bugs@laszlosystems.com'>");
            writer.write("bugs@laszlosystems.com</a>.</p>");
            writer.write("<hr /><table border='0'>");
            // TODO [2004-04-29 ows]: Eric says the following LPS call
            // should work, but it returns an empty string
            //writer.write("<tr><th>Server Info</th>");
            //writer.write("<td>" + LPS.getInfo(req, mContext, "lps-server-info") + "</td></tr>");
            writer.write("<tr><th>Version</th>");
            writer.write("<td>" + LPS.getShortVersion() + "</td></tr>");
            writer.write("<tr><th>Build</th>");
            writer.write("<td>" + LPS.getBuildDate() + "</td></tr>");
            writer.write("<tr><th>Stylesheet</th>");
            writer.write("<td>" + new File(styleSheetPathname).getName() + "</td></tr>");
            writer.write("<tr><th>Message</th>");
            writer.write("<td>" + e.getMessage() + "</td></tr>");
            writer.write("</table><pre>");
            writer.flush();
            PrintStream ps = new PrintStream(out);
            try {
                e.printStackTrace(ps);
            } finally {
                ps.flush();
            }
            writer.write("</pre></body></html>");
        } finally {
            writer.close();
        }
    }
    
    /** 
     * Return all query args except for "lzt"
     */
    private static String getQueryArgs(HttpServletRequest req) {
        StringBuffer query = new StringBuffer();
        Enumeration e = req.getParameterNames();
        while (e.hasMoreElements()) {
            String name = (String)e.nextElement();
            String val = req.getParameter(name);
            if (!name.equals("lzt")) {
                query.append("&"+name+"="+URLEncoder.encode(val));
            }
        }
        return query.toString();
    }

    public int getMimeType()
    {
        return MIME_TYPE_HTML;
    }
}
