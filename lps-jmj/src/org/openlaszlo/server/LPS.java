/******************************************************************************
 * LPS.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.server;

import java.lang.Package;
import java.lang.Class;
import java.util.Calendar;
import java.util.Properties;
import java.io.*;
import java.net.*;
import javax.servlet.*;
import javax.servlet.ServletConfig.*;
import javax.servlet.http.*;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.utils.MathUtils;
import org.openlaszlo.utils.LZUtils;

import org.apache.log4j.*;

import org.jdom.*;
import org.jdom.input.SAXBuilder;
import org.jdom.filter.ElementFilter;                         
import org.jdom.input.*;  

/**
 * LPS is a singleton for global server state.
 * 
 * @author Eric Bloch
 * @version 1.0 
 */
public class LPS {

    private static long mBootTime = Calendar.getInstance().getTime().getTime();

    private static Properties mProperties = null;

    private static String mHome = null;

    public static Configuration configuration = null;
    
    public static String VERSION_FILE = "/org/openlaszlo/server/lps.xml";

    private static String mBuildID;
    private static String mBuildDate;
    private static String mVersionID;
    private static String mRelease;

    public static String mSWFVersionDefault = null;
    public static int mSWFVersionNumDefault = -1;


    /**
     * Initialize version info
     */
    static {
        // Read in version details from XML file
        SAXBuilder builder = new SAXBuilder();
        Document doc;
        try {
            InputStream in = LPS.class.getResourceAsStream(VERSION_FILE);
            doc = builder.build(in);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
        Element root = doc.getRootElement();
        mBuildID = root.getChildTextNormalize("build-id");
        mBuildDate = root.getChildTextNormalize("build-date");
        mVersionID = root.getChildTextNormalize("version-id");
        mRelease = root.getChildTextNormalize("release");
    }


    /*
     * Set the home directory 
     */
    public static void setHome(String home) {
        mHome = home;
    }

    /*
     * Read the xml configuration file.
     */
    public static void initialize() {
        configuration = new Configuration();
    }

    /**
     * @return the LPS_HOME property
     */
    public static String HOME() { 
        if (mHome == null || mHome.equals("")) {
            mHome = getSystemProperty("LPS_HOME");
            if (mHome == null || mHome.equals("")) {
                // This is catastrophic
                throw new 
                    RuntimeException("Server configuration error: can't find LPS_HOME.");
            }
        }
        return mHome;
    }

    /** 
     * @return the "root directory" of the LPS bits.  Typically, HOME/WEB-INF/lps.
     */
    public static String ROOT() {
        return HOME() + File.separator + "WEB-INF" +
                        File.separator + "lps";
    }

    /** 
     * @return the "public root directory" of the LPS bits.  Typically, HOME/lps.
     */
    public static String PUBLIC_ROOT() {
        return HOME() + File.separator + "lps";
    }

    /**
     * @return the location of the lps.jar when 
     * running inside a servlet container
     */
    public static File getLPSJarFile() {
        return new File(HOME() + File.separator + "WEB-INF" + 
                                 File.separator + "lib" + 
                                 File.separator + "lps.jar");
    }

    /**
     * @return the location of the config directory.
     */
    public static String getConfigDirectory()  {
        return ConfigDir.get(HOME());
    }

    /**
     * @return the properties file
     */
    // TODO:[2002-12-2 bloch] add parameter/property for 
    // location of properties file.
    public static File getPropertiesFile()  {
        return new File( getConfigDirectory() + File.separator +
                         "lps.properties" );
    }

    /**
     * @return the location of the default cache directory.
     */
    public static String getWorkDirectory()  {
        return ROOT() + File.separator + "work";
    }

    /**
     * @return the location of the misc directory.
     */
    public static String getMiscDirectory()  {
        return ROOT() + File.separator + "misc";
    }

    /**
     * @return the location of the components directory.
     */
    public static String getComponentsDirectory()  {
        return PUBLIC_ROOT() + File.separator + "components";
    }

    /**
     * @return the location of the fonts directory.
     */
    public static String getFontDirectory()  {
        return PUBLIC_ROOT() + File.separator + "fonts";
    }

    /*
     * @return the location of the lfc directory
     */
    public static String getLFCDirectory()  {
        return ROOT() + File.separator + "lfc";
    }

    /*
     * @return the location of the server template directory
     */
    public static String getTemplateDirectory()  {
        return ROOT() + File.separator + "templates";
    }

    /**
     * @return a string representation of the version
     * of this build.  
     */
    public static String getBuild() {
        // The version number is actually baked into the manifest for the 
        // of the package.
        
        /* This doesn't work under Tomcat3.3 (see bug 4948, erroneous closed)
        try {
            Package p = Package.getPackage("org.openlaszlo.server");
            return p.getImplementationVersion();
        } catch (Exception e) {
            // THIS IS FATAL!
            throw new RuntimeException(e);
        }
        */

        return mBuildID;
    }

    /**
     * @return a string representation of the version
     * of this build.  
     */
    public static String getVersion() {
        // The version number is actually baked into the manifest for the 
        // of the package.
        
        /* This doesn't work under Tomcat3.3 (see bug 4948, erroneous closed)
        try {
            Package p = Package.getPackage("org.openlaszlo.server");
            return p.getSpecificationVersion();
        } catch (Exception e) {
            // THIS IS FATAL!
            throw new RuntimeException(e);
        }
        */

        return mVersionID;
    }

    /**
     * @return a string representation of the release 
     */
    public static String getRelease() {
        // The version number is actually baked into the manifest for the 
        // of the package.
        
        /* This doesn't work under Tomcat3.3 (see bug 4948, erroneous closed)
        try {
            Package p = Package.getPackage("org.openlaszlo.server");
            return p.getSpecificationVersion();
        } catch (Exception e) {
            // THIS IS FATAL!
            throw new RuntimeException(e);
        }
        */

        return mRelease;
    }

    /*
     * @return the string version of the LPS in lowercase.  E.g. lps-dr, lps-v1.
     */
    public static String getShortVersion() {
        return "lps-" + mVersionID;
    }

    /**
     * @return the boot time
     */
    public static long getBootTime() {
        return mBootTime;
    }

    /**
     *
     */
    public static String getBuildDate() {
        return mBuildDate;
    }


    /**
     * Set SWF version default.
     */
    public static void setSWFVersionDefault(String swfversion) {
        mSWFVersionDefault = swfversion;
        if (swfversion.equals("swf5")) {
            mSWFVersionNumDefault = 5; 
        } else if (swfversion.equals("swf6")) {
            mSWFVersionNumDefault = 6;
        } else if (swfversion.equals("swf7")) {
            mSWFVersionNumDefault = 7;
        } else {
            throw new RuntimeException("Unknown SWF version: " + swfversion);
        }
    }

    /**
     * @return swf version number
     */
    public static int getSWFVersionNum(String swfversion) {
        if (swfversion == null)        return mSWFVersionNumDefault;
        if (swfversion.equals("swf5")) return 5;
        if (swfversion.equals("swf6")) return 6;
        if (swfversion.equals("swf7")) return 7;
        return mSWFVersionNumDefault;
    }

    /**
     * @return swf version number
     */
    public static int getSWFVersionNum(HttpServletRequest req) {
        return getSWFVersionNum(req.getParameter("lzr"));
    }

    /**
     * @return an XML string of info
     */
    public static String getInfo(HttpServletRequest req, 
        ServletContext ctxt, String tagName) {
        StringBuffer buf = new StringBuffer();

        InetAddress    localHost;
        InetAddress [] myIPs;

        final double MEG = 1024*1024;

        // Store my ips
        try {
            localHost = InetAddress.getLocalHost();
        } catch (UnknownHostException e) {
            throw new ChainedException("LPS can't determine localhost ip address");
        }
        try {
            myIPs = InetAddress.getAllByName("localhost");
        } catch (UnknownHostException e) {
            throw new ChainedException("Can not determine server IP address!");
        }


        buf.append("<").append(tagName).append(" \n" );
        buf.append("\t server-port=\"" + req.getServerPort() + "\"\n");
        buf.append("\t servlet-container=\"" + ctxt.getServerInfo() + "\"\n");
        buf.append("\t servlet-container-version=\"" + ctxt.getMajorVersion() +
            "." + ctxt.getMinorVersion() + "\"\n");
        // buf.append("\t context=\"" + ctxt + "\"\n");
        buf.append("\t jre-version=\"" + getSystemPropertyOrUnknowable("java.version") + "\"\n");
        buf.append("\t os-name=\"" + getSystemPropertyOrUnknowable("os.name") + "\"\n");
        buf.append("\t os-version=\"" + getSystemPropertyOrUnknowable("os.version") + "\"\n");
        String level = "org.openlaszlo logger not configured!";
        Logger l = Logger.getLogger("org.openlaszlo");
        try {
            if (l != null) {
                level = Logger.getLogger("org.openlaszlo").getLevel().toString();
            }
        } catch (Throwable t) {
            level = "unknown";
        }
        buf.append("\t log4j-level=\"" + level + "\"\n");
        buf.append("\t user=\"" + getSystemPropertyOrUnknowable("user.name") + "\"\n");
        buf.append("\t version=\"" + getVersion() + "\"\n");
        buf.append("\t release=\"" + getRelease() + "\"\n");
        buf.append("\t build=\"" + getBuild() + "\"\n");
        buf.append("\t built-on=\"" + mBuildDate + "\"\n");
        buf.append("\t max-mem=\"" + 
            MathUtils.formatDouble(Runtime.getRuntime().maxMemory()/(MEG), 2) + "MB\"\n");
        buf.append("\t total-mem=\"" + 
            MathUtils.formatDouble(Runtime.getRuntime().totalMemory()/(MEG), 2) + "MB\"\n");
        buf.append("\t free-mem=\"" + 
            MathUtils.formatDouble(Runtime.getRuntime().freeMemory()/(MEG), 2) + "MB\"\n");
        buf.append("\t lps-home=\"" + mHome + "\"\n" );
        buf.append("\t localhost=\"" + localHost.getHostAddress() + "\"\n" );
        for(int i = 0; i < myIPs.length; i++) {
            buf.append("\t ipaddress-" + (i+1) + "=\"" + myIPs[i].getHostAddress() + "\"\n");
        }
        buf.append("\t client=\"" + req.getRemoteHost() + "\"\n");
        buf.append("/>");

        // TODO: [2003-02-28 bloch] add lps.properties fia properties->xml thingee

        return buf.toString();
    }

    /**
     * Safe version of System.getProperty() that won't  
     * throw a SecurityException if the property can't be read
     */
    public static String getSystemProperty(String name) {
        try {
            return System.getProperty(name);
        } catch (SecurityException e) {
            // TODO [2004-07-06 bloch]: log the failure somehow
            return "";
        }
    }


    /**
     * Safe version of System.getProperty() that won't  
     * throw a SecurityException if the property can't be read.
     * Return 'unknowable' if security won't let us know.
     */
    public static String getSystemPropertyOrUnknowable(String name) {
        return getSystemProperty(name, "unknowable");
    }

    /**
     * Safe version of System.getProperty() that won't  
     * throw a SecurityException if the property can't be read
     */
    public static String getSystemProperty(String name, String d) {
        try {
            return System.getProperty(name, d);
        } catch (SecurityException e) {
            // TODO [2004-07-06 bloch]: log the failure somehow
            return d;
        }
    }

    private static void loadProperties() {
        if (mProperties == null) {
            File propFile = getPropertiesFile();
            Properties properties = new Properties();
            Properties sysProperties = (Properties)System.getProperties().clone();
            try {
                properties.load(new FileInputStream(propFile));
                properties = LZUtils.expandProperties(properties);
                sysProperties.putAll(properties);
           
            } catch (Exception e) {
                throw new ChainedException (e);
            }
            mProperties = sysProperties;
        }
    }

    /** @return the LPS properties */
    public static Properties getProperties() {
        loadProperties();
        return mProperties;
    }

    private final static String KRANK_PORT_PROPERTY = "krankPortNum";
    private final static int DEFAULT_KRANK_PORT = 4444;

    /*** @return Krank listener port */
    public static int getKrankPort () {
        String portStr = LPS.getProperties().getProperty(KRANK_PORT_PROPERTY);
        int portnum = DEFAULT_KRANK_PORT;
        if (portStr == null) {
            return portnum;
        }
        try {
            portnum = Integer.parseInt(portStr);
        } catch (NumberFormatException e) {
            throw new 
                RuntimeException("Server configuration error: can't parse lps.properties entry '"+KRANK_PORT_PROPERTY+"'");            
        }
        return portnum;
    }

    /** @return a property from the LPS property file, defaulting
     * to value. */
    public static String getProperty(String name, String value) {
        loadProperties();
        return mProperties.getProperty(name, value);
    }

    /** @return a property from the LPS property file. */
    public static String getProperty(String name) {
        loadProperties();
        return getProperty(name, null);
    }
    
    /** Override a property in the LPS property file. */
    public static void setProperty(String name, String value) {
        loadProperties();
        mProperties.setProperty(name, value);
    }

    public static boolean isInternalBuild() {
        return LPS.getBuild().equals("INTERNAL");
    }
}
