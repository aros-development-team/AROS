/******************************************************************************
 * ResponderLOGCONFIG.java
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
import org.openlaszlo.utils.StringUtils;
import org.openlaszlo.server.ConfigDir;
import org.openlaszlo.server.LPS;

import org.xml.sax.*;
import org.apache.log4j.*;
import org.apache.log4j.xml.*;
import org.w3c.dom.*;
import javax.xml.parsers.*;
import javax.servlet.http.HttpServlet;
import org.apache.xml.utils.*;

public final class ResponderLOGCONFIG extends ResponderAdmin
{
    private static HttpServlet mServlet = null; 
    private static boolean mIsInitialized = false;
    private static DocumentBuilder mBuilder = null;
    private static Document mDocument = null; 
    private static boolean mSaveConfig = false;
    private static File mLogFile = null;
    /** Servlet who we're logging on behalf of */
    private static HttpServlet servlet = null;


    private static class SaveConfigException extends IOException {
        public SaveConfigException() {
            super();
        }
        public SaveConfigException(String s) {
            super(s);
        }
    }

    public static File getLogFile()
    {
        return mLogFile;
    }

    synchronized protected 
        void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        res.setContentType("text/xml");

        String reread = req.getParameter("reread");
        String xml = req.getParameter("xml");
        String save = req.getParameter("save");

        mSaveConfig = (save != null && save.equals("1"));

        try {
            if (xml != null) {
                configure(xml);
            } else if (reread != null && reread.equals("1")) {
                configure(mServlet);
            }
        } catch (ParserConfigurationException e) {
            respondWithException(res, e);
            return;
        } catch (SAXException e) {
            respondWithException(res, e);
            return;
        } catch (SaveConfigException e) {
            respondWithException(res, e);
            return;
        }

        if (mDocument == null) {
            respondWithErrorXML(res, "LPS log not configured");
            return;
        }

        printNode(new PrintStream(res.getOutputStream()), mDocument, "");
    }


    /**
     * Configure log4j with xml configuration file.  Also sets the servlet we're logging for.
     * @param servlet servlet to log configured info.  Pass null if you don't want loginfo.
     */
    synchronized public static void configure(HttpServlet servlet)
        throws IOException, SecurityException, 
               ParserConfigurationException, SAXException {

        if (mBuilder == null)
            mBuilder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
        String logfile = null;
        String configFile = getConfigurationFile();
        if (configFile != null) {
            String uri = "file:///" + configFile;
            logfile = configure(mBuilder.parse(uri), null);
        } else {
            configFile = LPS.getConfigDirectory() + File.separator + "lps.xml";
            logfile = configureWithLPSConfig(configFile);
        }

        if (servlet != null) {
            mServlet = servlet;
        }

        if (mServlet != null) {
            servlet.log("Detailed LPS log is at " + logfile);
            servlet.log("LPS log configured with " + configFile);
        }
    }


    /**
     * Configure using lps.xml
     */
    synchronized public static 
        String configureWithLPSConfig(String lpsConfigFile) 
        throws SAXException, IOException {

        String logfile = null;
        String uri = "file:///" + lpsConfigFile;
        Document doc = mBuilder.parse(uri);
        NodeList nl = doc.getElementsByTagName("log4j:configuration");

        if (0 < nl.getLength()) {
            Document tmpdoc = mBuilder.newDocument();
            Node node = tmpdoc.importNode( nl.item(0) , true );
            tmpdoc.appendChild(node);
            logfile = configure(tmpdoc, null);
        }

        return logfile;
    }


    /**
     * Configure log4j with xml.
     */
    synchronized public static void configure(String xml)
        throws IOException, SecurityException, 
               ParserConfigurationException, SAXException
    {
        if (mBuilder == null)
            mBuilder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
        configure(mBuilder.parse(new InputSource(new StringReader(xml))), null);
    }


    /**
     * Configure log4j with DOM. Create lps.log, if File param has not be
     * configured.
     */
    synchronized private static String configure(Document doc, 
                                                 String replaceFileName)
        throws IOException, SecurityException
    {
        String logFileName = null;

        Element root = doc.getDocumentElement();
        NodeList appenderList = root.getElementsByTagName("appender");
        for (int i=0; i < appenderList.getLength(); i++) {

            Element appender = (Element) appenderList.item(i);
            String appenderName = appender.getAttribute("name");
            if (appenderName.equals("lps")) {
                NodeList paramList = appender.getElementsByTagName("param");
                for (int j=0; j < paramList.getLength(); j++) {

                    // Look for File param in lps appender.
                    Element param = (Element) paramList.item(j);
                    String paramName = param.getAttribute("name");
                    if (paramName.equals("File")) {
                        if (replaceFileName != null) {
                            replaceFileName = 
                                StringUtils.replace(replaceFileName, '\\', "/");
                            param.setAttribute("value", replaceFileName);
                        }
                        logFileName = param.getAttribute("value");
                        break;
                    }
                }

                // Create lps.log file iff File param does not exist.  Also,
                // replace File option if replaceFileName is passed in.
                if (logFileName == null) {

                    logFileName = LPS.getWorkDirectory() + 
                        File.separator + "logs" +
                        File.separator + "lps.log";

                    // Seems as if we have to escape back-slashes in log4j.xml
                    // strings.
                    logFileName = StringUtils.replace(logFileName, '\\', "/");
                    try {
                        Element p = doc.createElement("param");
                        p.setAttribute("name", "File");
                        p.setAttribute("value", logFileName);

                        // param must come after errorHandler, if it exists. See
                        // log4j.dtd for more info.
                        NodeList nodes = appender.getChildNodes();
                        for (int k=0; k < nodes.getLength(); k++) {
                            Node n = nodes.item(k);
                            if (n.getNodeType() == Node.ELEMENT_NODE) {
                                Element e = (Element) n;
                                if (e.getTagName().equals("errorHandler"))
                                    n = e.getNextSibling();
                                appender.insertBefore(p, n);
                                break;
                            }
                        }
                    } catch (DOMException e) {
                        Logger.getLogger(ResponderLOGCONFIG.class).debug("DOMException: " , e);
                        logFileName = null;
                    }
                }

                if (! logFileName.equals("")) {
                    // Sanity check for logfile as configured
                    mLogFile = new File(logFileName);
                    File logFileDir = mLogFile.getParentFile();
                    if (logFileDir != null)
                        logFileDir.mkdirs();
                    mLogFile.createNewFile();
                }
                break;
            }
        }
            
        DOMConfigurator.configure(root);
        mDocument = doc;

        if (servlet != null) {
            servlet.log("LPS log file is now " + logFileName);
        }

        try {
            if (mSaveConfig) {
                String configDir = LPS.getConfigDirectory();
                FileOutputStream out = new FileOutputStream
                    (new File(configDir + File.separator + "log4j.xml"));
                printNode(new PrintStream(out), mDocument, "");
                out.close();
            }
        } catch (IOException e) {
            throw new SaveConfigException("server configured but could not write out configuration.");
        }

        return logFileName;
    }


    /**
     * Prety much ripped from:
     *
     *   http://www-106.ibm.com/developerworks/java/library/x-jaxp/TestDOMParsing.java
     */
    synchronized private static void printNode(PrintStream out, Node node, String indent)
        throws IOException
    {
        switch (node.getNodeType()) {
        case Node.DOCUMENT_NODE:
            // recurse on each child
            NodeList nodes = node.getChildNodes();
            if (nodes != null) {
                for (int i=0; i<nodes.getLength(); i++) {
                    printNode(out, nodes.item(i), "");
                }
            }
            break;
                
        case Node.ELEMENT_NODE:
            String name = node.getNodeName();
            out.print(indent + "<" + name);
            NamedNodeMap attributes = node.getAttributes();
            for (int i=0; i<attributes.getLength(); i++) {
                Node current = attributes.item(i);
                out.print(" " + current.getNodeName() +
                          "=\"" + current.getNodeValue() + "\"");
            }
            out.print(">\n\n");
                
            // recurse on each child
            NodeList children = node.getChildNodes();
            if (children != null) {
                for (int i=0; i<children.getLength(); i++) {
                    printNode(out, children.item(i), indent + "    ");
                }
            }
                
            out.print(indent + "</" + name + ">\n\n");
            break;

        case Node.COMMENT_NODE:
        case Node.TEXT_NODE:
            break;
        }
    }

    /**
     * Clear the log file.
     * @param status clear log status. Log can be cleared, but state can be bad
     * if log file can't be reset.
     */
    synchronized public static boolean clearLog(String[] status)
    {
        String path = mLogFile.getAbsolutePath();
        boolean cleared = false;


        try {
            // Let go of log handle
            configure(mDocument, "");
        } catch (Exception e) {
            if (status != null)
                status[0] = "can't reconfigure log";
            Logger.getLogger(ResponderLOGCONFIG.class).info("can't reconfigure log");
            return cleared;
        }

        try {
            cleared = new File(path).delete();
        } catch (SecurityException e) { 
            Logger.getLogger(ResponderLOGCONFIG.class).info("can't clear log: ", e);
        }

        try {
            // Reset log handle
            configure(mDocument, path);
        } catch (Exception e) {
            if (status != null)
                status[0] = "can't reset log";
            Logger.getLogger(ResponderLOGCONFIG.class).info("can't reset log");
        }

        return cleared;
    }


    /**
     * Find log4j.xml configuration file. 
     */
    private static String getConfigurationFile()
        throws FileNotFoundException
    {
        String configDir = LPS.getConfigDirectory();
        String filename = configDir + File.separator + "log4j.xml";
        if (! new File(filename).isFile()) {
            filename = null;
        }
        return filename;
    }

    public int getMimeType()
    {
        return MIME_TYPE_XML;
    }
}
