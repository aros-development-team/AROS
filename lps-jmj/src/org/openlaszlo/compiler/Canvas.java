/* *****************************************************************************
 * Canvas.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

/**
 * A <code>Canvas</code> represents the underlying
 * area in which a compiled app will run
 */

package org.openlaszlo.compiler;
import org.openlaszlo.utils.StringUtils;
import org.openlaszlo.xml.internal.XMLUtils;
import org.openlaszlo.server.Configuration;
import org.openlaszlo.server.LPS;
import java.util.*;
import java.io.Serializable;
import java.io.File;
import org.jdom.Element;
import org.jdom.Namespace;
import org.apache.log4j.Logger;

public class Canvas implements java.io.Serializable {

    // TODO: [old ebloch] change these to properties
    // TODO: [2003-10-25 bloch] or better yet derrive them from the schema
    
    private static Logger mLogger  = Logger.getLogger(Canvas.class);

    /** Default canvas width for compilation */
    private static final int DEFAULT_WIDTH = 500;

    /** Default canvas height for compilation */
    private static final int DEFAULT_HEIGHT = 400;

    /** Default canvas backgorund color */
    private static final int DEFAULT_BGCOLOR = 0xFFFFFF;

    /** Default canvas backgorund color */
    private static final String DEFAULT_TITLE = "Laszlo Application";

    /** Default canvas font info */
    // TODO: [2003-10-25 bloch] this should come from the build system
    public static final String DEFAULT_VERSION = "1.1";

    // TODO: [2003-10-25 bloch] these should come from a properties file
    public static final String DEFAULT_SWF6_FONT          = "Verdana,Vera,sans-serif";

    public static final String DEFAULT_FONT               = "default";
    public static final String DEFAULT_FONT_FILENAME      = "verity" + File.separator + "verity11.ttf";
    public static final String DEFAULT_BOLD_FONT_FILENAME = "verity" + File.separator + "verity11bold.ttf";
    public static final String DEFAULT_ITALIC_FONT_FILENAME = "verity" + File.separator + "verity11italic.ttf";
    public static final String DEFAULT_BOLD_ITALIC_FONT_FILENAME = "verity" + File.separator + "verity11bolditalic.ttf";
    
    public String defaultFont () {
        if (mSWFVersion.equals( "swf5" )) {
            return DEFAULT_FONT;
        } else {
            return DEFAULT_SWF6_FONT;
        }
    }

    public String defaultFontsize () {
        if (mSWFVersion.equals( "swf5" )) {
            return DEFAULT_FONTSIZE;
        } else {
            return DEFAULT_SWF6_FONTSIZE;
        }
    }


    public static final String DEFAULT_FONTSIZE  = "8";
    public static final String DEFAULT_SWF6_FONTSIZE  = "11";
    public static final String DEFAULT_FONTSTYLE = "";
    
    /** Default persistent connection parameters */
    private static final long DEFAULT_HEARTBEAT = 5000; // 5 seconds
    private static final boolean DEFAULT_SENDUSERDISCONNECT = false;
    
    public String defaultFont = DEFAULT_FONT;
    public String defaultFontFilename = DEFAULT_FONT_FILENAME;
    public String defaultBoldFontFilename = DEFAULT_BOLD_FONT_FILENAME;
    public String defaultItalicFontFilename = DEFAULT_ITALIC_FONT_FILENAME;
    public String defaultBoldItalicFontFilename = DEFAULT_BOLD_ITALIC_FONT_FILENAME;
    
    /** File path relative to webapp. */
    private String mFilePath = null;
    /** Width of the canvas. */
    private int mWidth = DEFAULT_WIDTH;
    /** Height of the canvas. */
    private int mHeight = DEFAULT_HEIGHT;
    /** Background color of the canvas. */
    private int mBGColor = DEFAULT_BGCOLOR;

    // Dimension strings
    private String mWidthString = null;
    private String mHeightString = null;

    // Default to proxied deployment
    private boolean mProxied = true;

    /** Width of the root output text object (Flash 5 limits us to
     * setting this at compile time) */
    private int mMaxTextWidth = 0;

    /** Height of the root output text object (Flash 5 limits us to
     * setting this at compile time) */
    private int mMaxTextHeight = 0;
    
    /** FontInfo for the canvas. */
    private FontInfo mFontInfo = null;
    
    /** Title for the canvas. */
    private String mTitle = DEFAULT_TITLE;
    
    /** Version of the flash player file format which we compile to **/
    private String mSWFVersion = LPS.getProperty("compiler.runtime.default", "swf6");
    
    /** Persistent connection parameters. */
    private boolean mIsConnected = false;
    private boolean mSendUserDisconnect = false;
    private long mHeartbeat = 0;
    private String mAuthenticator = null;
    private String mGroup = null;
    private Set mAgents = null;

    private final Map mSecurityOptions = new Hashtable();

    private String mCompilationWarningText = null;
    private String mCompilationWarningXML = null;
    private Element mInfo = new org.jdom.Element("stats");
    
    public void setCompilationWarningText(String text) {
        mCompilationWarningText = text;
    }

    public void setCompilationWarningXML(String xml) {
        mCompilationWarningXML = xml;
    }
    
    public void setSWFVersion(String text) {
        mSWFVersion = text;
    }

    public String getSWFVersion() {
        return mSWFVersion;
    }

    public void addInfo(Element info) {
        mInfo.addContent(info);
    }

    public String getCompilationWarningText() {
        return mCompilationWarningText;
    }

    public void setFontInfo(FontInfo info) {
        mFontInfo = info;
    }
    
    public String getInfoAsString() {
        org.jdom.output.XMLOutputter outputter =
            new org.jdom.output.XMLOutputter();
        outputter.setTextNormalize(true);
        return outputter.outputString(mInfo);
    }

    /** @return file path */
    public String getFilePath() {
        return mFilePath;
    }

    /** @return file path */
    public void setFilePath(String filePath) {
        mFilePath = filePath;
    }

    /** @return width */
    public int getWidth() {
        return mWidth;
    }

    /** @return width */
    public String getWidthXML() {
        if (mWidthString == null)
            return "" + mWidth;
        else 
            return mWidthString;
    }

    /** @param width */
    public void setWidth(int w) {
        mWidth = w;
    }

    /** @param width */
    public void setWidthString(String w) {
        mWidthString = w;
    }


    /** @return height */
    public int getHeight() {
        return mHeight;
    }

    /** @return width */
    public String getHeightXML() {
        if (mHeightString == null)
            return "" + mHeight;
        else 
            return mHeightString;
    }

    /** @param height */
    public void setHeight(int h) {
        mHeight = h;
    }

    /** @param height */
    public void setHeightString(String h) {
        mHeightString = h;
    }


    /** @return maxTextWidth */
    public int getMaxTextWidth() {
        return mMaxTextWidth;
    }

    /** @param maxTextWidth */
    public void setMaxTextWidth(int h) {
        mMaxTextWidth = h;
    }

    /** @return maxTextHeight */
    public int getMaxTextHeight() {
        return mMaxTextHeight;
    }

    /** @param maxTextHeight */
    public void setMaxTextHeight(int h) {
        mMaxTextHeight = h;
    }


    /** @return Background color */
    public int getBGColor() {
        return mBGColor;
    }

    /** @return Returns bgcolor as a hexadecimal string */
    // TODO: [12-21-2002 ows] This belongs in a utility library.
    public String getBGColorString() {
        String red   = Integer.toHexString((mBGColor >> 16) & 0xff);
        String green = Integer.toHexString((mBGColor >> 8) & 0xff);
        String blue  = Integer.toHexString(mBGColor & 0xff);
        if (red.length() == 1)
            red = "0" + red;
        if (green.length() == 1)
            green = "0" + green;
        if (blue.length() == 1)
            blue = "0" + blue;
        return "#" + red + green + blue;
    }

    /** @param BGColor  Background color */
    public void setBGColor(int BGColor) {
        mBGColor = BGColor;
    }

    /** @return Title */
    public String getTitle() {
        return mTitle;

    }
    
    public String getISBN() {
        return "192975213X";
    }
    
    /** @param title */
    public void setTitle(String t) {
        mTitle = t;
    }

    /** @return Heartbeat */
    public long getHeartbeat() {
        return mHeartbeat;

    }

    /** @param heartbeat */
    public void setHeartbeat(long heartbeat) {
        mHeartbeat = heartbeat;
    }

    /** @return Group */
    public String getGroup() {
        return mGroup;

    }

    /** @param group */
    public void setGroup(String g) {
        mGroup = g;
    }

    /** @return Authenticator */
    public String getAuthenticator() {
        return mAuthenticator;

    }

    /** @param authenticator */
    public void setAuthenticator(String a) {
        mAuthenticator = a;
    }

    /** @return send user disconnect */
    public boolean doSendUserDisconnect() {
        return mSendUserDisconnect;
    }

    /** @param sud */
    public void setSendUserDisconnect(boolean sud) {
        mSendUserDisconnect = sud;
    }

    /** @param serverless */
    public void setProxied(boolean val) {
        mProxied = val;
    }

    /** @return is this app compiling for serverless deployment */
    public boolean isProxied() {
        return mProxied;
    }

    /** @return is connected */
    public boolean isConnected() {
        return mIsConnected;
    }

    /** @param isConnected */
    public void setIsConnected(boolean isConnected) {
        mIsConnected = isConnected;
    }

    /** @return agents */
    public Set getAgents() {
        return mAgents;
    }

    /** @param agent agent's URL */
    public void addAgent(String agent) {
        if (mAgents == null) 
            mAgents = new HashSet();
        mAgents.add(agent);
    }


    /** @return font info */
    public FontInfo getFontInfo() {
        return mFontInfo;
    }

    /** @return security options */
    public Map getSecurityOptions() {
        return mSecurityOptions;
    }

    /** @param element */
    public void setSecurityOptions(Element element) {
        Configuration.addOption(mSecurityOptions, element);
    }

    public String getXML(String content) {
        StringBuffer buffer = new StringBuffer();
        buffer.append(
            "<canvas " +
            "title=\"" + XMLUtils.escapeXml(getTitle()) + "\" " +
            "bgcolor=\"" + getBGColorString() + "\" " +
            "width=\"" + getWidthXML() + "\" " +
            "height=\"" + getHeightXML() + "\" " +
            "runtime=\"" + getSWFVersion() +"\" " +
            ">");
        buffer.append(content);
        buffer.append(getInfoAsString());
        if (mCompilationWarningXML != null)
            buffer.append("<warnings>" + mCompilationWarningXML + "</warnings>");
        buffer.append("</canvas>");
        return buffer.toString();
    }
    
    /** 
     * Initialize persistent connection values.
     */
    protected void initializeConnection(Element elt) {
        // TODO: [2003-10-16 pkang] Create and move this function into
        // ConnectionCompiler.java
        Element eltConnection = elt.getChild("connection", elt.getNamespace());
        if (eltConnection!=null) {

            setIsConnected(true);
            setSendUserDisconnect(DEFAULT_SENDUSERDISCONNECT);
            setHeartbeat(DEFAULT_HEARTBEAT);

            String heartbeat = eltConnection.getAttributeValue("heartbeat");
            if (heartbeat != null) {
                try {
                    setHeartbeat(Long.parseLong(heartbeat));
                } catch (NumberFormatException e) {
                    throw new CompilationError(elt,  "heartbeat", e);
                }
            }

            String sendUserDisconnect =
                eltConnection.getAttributeValue("receiveuserdisconnect");
            if (sendUserDisconnect != null) {
                setSendUserDisconnect(Boolean.valueOf(sendUserDisconnect).booleanValue());
            }

            String group = eltConnection.getAttributeValue("group");
            if (group != null) {
                setGroup(group);
            }

            // Don't set a default authenticator in canvas. We want to be able
            // to override this through lps.properties. Return null if one does
            // not exist.
            String authenticator = eltConnection.getAttributeValue("authenticator");
            if (authenticator != null) {
                setAuthenticator(authenticator);
            }

            List agents = eltConnection.getChildren("agent", elt.getNamespace());
            for (int i=0; i < agents.size(); i++) {
                Element eltAgent = (Element)agents.get(i);
                String url = eltAgent.getAttributeValue("url");
                if (url != null || ! url.equals(""))
                    addAgent(url);
            }
        }
    }
}
