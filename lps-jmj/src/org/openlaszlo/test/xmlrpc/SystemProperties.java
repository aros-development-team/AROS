/* ****************************************************************************
 * SystemProperties.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test.xmlrpc;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.MalformedURLException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.util.Date;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Properties;
import org.xml.sax.helpers.AttributesImpl;

import org.openlaszlo.xml.DataEncoder;


public class SystemProperties
{
    public static DataEncoder getProperties() {
        DataEncoder encoder = new DataEncoder();
        encoder.startDocument();

        encoder.startElement("system", new AttributesImpl());

        Properties prop = System.getProperties();
        Enumeration enum = prop.propertyNames();
        while (enum.hasMoreElements()) {
            String key = (String)enum.nextElement();
            String val = xmlEscape(prop.getProperty(key));
            AttributesImpl attrs = new AttributesImpl();
            attrs.addAttribute("", "key", "", "CDATA", key);
            attrs.addAttribute("", "val", "", "CDATA", val);

            encoder.startElement("property", attrs);
            encoder.endElement();
        }

        encoder.endElement();
        encoder.endDocument();
        return encoder;
    }

    public static DataEncoder getProperty(String key) {
        DataEncoder encoder = new DataEncoder();
        encoder.startDocument();

        encoder.startElement("system", new AttributesImpl());

        String val = xmlEscape(System.getProperty(key));
        AttributesImpl attrs = new AttributesImpl();
        attrs.addAttribute("", "key", "", "CDATA", key);
        attrs.addAttribute("", "val", "", "CDATA", val);
        encoder.startElement("property", attrs);
        encoder.endElement();

        encoder.endElement();

        encoder.endDocument();
        return encoder;
    }

    /**
     * Escape the 5 entities defined by XML.
     * These are: '<', '>', '\', '&amp;', '"'.
     *
     * @param s an xml string
     * @return an escaped xml string
     */
    static String xmlEscape(String s) {
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
}

