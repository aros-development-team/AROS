/* *****************************************************************************
 * XMLUtils.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml.internal;
import org.openlaszlo.utils.ChainedException;
import org.jdom.Document;
import org.jdom.Element;
import org.xml.sax.SAXException;
import java.io.*;
import java.util.*;

/**
 * XMLUtils
 *
 * @author Oliver Steele
 */
abstract public class XMLUtils {
    /** Returns an element's attribute value, as a String.  Same as
     * Element.getAttributeValue, except that it takes a default value
     * to return if the attribute is missing.
     *
     * @param e an Element
     * @param aname the attribute name
     * @param defaultValue default value
     * @return a String
     */
    public static String getAttributeValue(Element e, String aname,
                                           String defaultValue)
    {
        String value = e.getAttributeValue(aname);
        if (value == null) {
            value = defaultValue;
        }
        return value;
    }

    /** Returns an element's attribute value, as a String.  Same as
     * Element.getAttributeValue, except guarantees not to return
     * void.
     *
     * @param e an Element
     * @param aname the attribute name
     * @return a String
     */
    public static String requireAttributeValue(Element e, String aname) {
        String value = e.getAttributeValue(aname);
        if (value == null) {
            throw new MissingAttributeException(e, aname);
        }
        return value;
    }
    
    /**
     * Escape the 5 entities defined by XML. 
     * These are: '<', '>', '\', '&', '"'.
     * 
     * @param s an xml string
     * @return an escaped xml string
     */
    public static String escapeXml(String s) {
        if (s == null) return null;
        StringBuffer sb = new StringBuffer();
        for(int i=0; i<s.length(); i++) {
            char c = s.charAt(i);
            if (c == '<') {
                sb.append("&lt;");
            } else if (c == '>') {
                sb.append("&gt;");
            } else if (c == '\'') {
                sb.append("&apos;");
            } else if (c == '&') {
                sb.append("&amp;");
            } else if (c == '"') {
                sb.append("&quot;");
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }

    /**
     * Escape 3 entities understood by HTML text.
     * These are: '<', '>', '&'
     * 
     * TODO: [2003-05-05 bloch] move to LZHttpUtils and rename
     * to escapeHTML
     *
     * @param s an xml string
     * @return an escaped xml string
     */
    public static String escapeXmlForSWFHTML(String s) {
        if (s == null) return null;
        StringBuffer sb = new StringBuffer();
        for(int i=0; i<s.length(); i++) {
            char c = s.charAt(i);
            if (c == '<') {
                sb.append("&lt;");
            } else if (c == '>') {
                sb.append("&gt;");
            } else if (c == '&') {
                sb.append("&amp;");
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }

    /**
     * Make sure all the following sequences
     *    &lt; &gt; &apos; &quot; and &amp;
     *
     *    have their ampersands escaped
     * 
     * @param s an xml string
     * @return an escaped xml string
     */
    public static String escapeAmpersands(String s) {
        if (s == null) return null;
        StringBuffer sb = new StringBuffer();
        for(int i=0; i<s.length(); i++) {
            char c = s.charAt(i);
            if (c == '&') {
                int j = i+1;
                if (s.regionMatches(j, "lt;", 0, 3)) {
                    sb.append("&amp;lt;"); 
                    i += 3;
                } else if (s.regionMatches(j, "gt;", 0, 3)) {
                    sb.append("&amp;gt;");
                    i += 3;
                } else if (s.regionMatches(j, "apos;", 0, 5)) {
                    sb.append("&amp;apos;");
                    i += 5;
                } else if (s.regionMatches(j, "quot;", 0, 5)) {
                    sb.append("&amp;quot;");
                    i += 5;
                } else if (s.regionMatches(j, "amp;", 0, 4)) {
                    sb.append("&amp;amp;");
                    i += 4;
                } else {
                    sb.append(c);
                }
            } else {
                sb.append(c);
            }
        }

        return sb.toString();
    }

    public static boolean isURL(String str) {
        // when running in ant, some of these protocols (https and soap, at
        // least) throw MalformedURLException
        if (str.startsWith("http:") ||
            str.startsWith("https:") ||
            str.startsWith("file:") ||
            str.startsWith("ftp:") ||
            str.startsWith("soap:")) {
            return true;
        }
        try {
            new java.net.URL(str); // for effect
            return true;
        } catch (java.net.MalformedURLException e) {
            return false;
        }
    }

    public static Element parse(String source) {
        try {
            org.jdom.input.SAXHandler handler = new org.jdom.input.SAXHandler();
            org.xml.sax.XMLReader reader =
                org.xml.sax.helpers.XMLReaderFactory.createXMLReader(
                    "org.apache.xerces.parsers.SAXParser");
            reader.setContentHandler(handler);
            reader.parse(new org.xml.sax.InputSource(new StringReader(source)));
            Document doc = handler.getDocument();
            return doc.getRootElement();
        } catch (IOException e) {
            throw new ChainedException(e);
        } catch (SAXException e) {
            throw new ChainedException(e);
        }
    }

    /** 
     * Returns xml from first non-declaration element.
     *
     * @param xml xml string buffer.
     * @return xml string with no declarations.
     */
    public static String getNoDeclarationXML(StringBuffer xml)
    {
        int len = xml.length();
        for (int i=0; i < len; i++) 
            if ( xml.charAt(i) == '<' && ( i+1 < len) ) 
                if (xml.charAt(i+1) != '?' && xml.charAt(i+1) != '!')
                    return xml.substring(i);
        return xml.toString();
    }


    public static String toString(Element element) {
        org.jdom.output.XMLOutputter outputter =
            new org.jdom.output.XMLOutputter();
        outputter.setTextNormalize(true);
        return outputter.outputString(element);
    }    
}
