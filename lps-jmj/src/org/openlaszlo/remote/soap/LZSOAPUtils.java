/* *****************************************************************************
 * LZSOAPUtils.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote.soap;

import javax.xml.namespace.QName;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.xml.internal.DataCommon;
import org.openlaszlo.xml.internal.DataContext;
import java.io.IOException;
import java.io.StringReader;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import org.apache.log4j.Logger;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.apache.axis.utils.XMLUtils;


public class LZSOAPUtils {

    private static Logger mLogger = Logger.getLogger(LZSOAPUtils.class);

    /**
     * Get prefix if it exists.
     *
     * @param tag tag string.
     * @return prefix part of tag name, if it exists, else empty string.
     */
    static public String getPrefix(String tag) {
        int index = tag.indexOf(':');        
        if (index == -1) return "";
        return tag.substring(0, index);
    }

    /**
     * Get local part of tag.
     *
     * @param tag tag string.
     * @return local part of tag.
     */
    static public String getLocal(String tag) {
        int index = tag.indexOf(':');        
        if (index == -1) return tag;
        return tag.substring(index+1);
    }

    static public Element xmlStringToElement(String xml)
        throws IOException, SAXException {

        DocumentBuilder builder = null;
        try {
            builder = XMLUtils.getDocumentBuilder();
            return builder.parse(new InputSource( new StringReader(xml)))
                .getDocumentElement();
        } catch (ParserConfigurationException e) {
            throw new SAXException("can't create document builder");
        } finally {
            if (builder != null) {
                XMLUtils.releaseDocumentBuilder(builder);
            }
        }

    }

    static public void pushString(Program program, String str, DataContext dc) {
        if (str == null) {
            pushNull(program.body());
            return;
        }
        DataCommon.pushStringData(str, program.body(), dc);
    }

    static public void pushNull(FlashBuffer body) {
        body.writeByte(Actions.PushData);
        body.writeWord(0+1);
        body.writeByte(2);
    }

    static public void pushQName(Program program, QName qname, DataContext dc) {
        FlashBuffer body = program.body();
        if (qname == null) {
            pushNull(body);
            return;
        }
        // this can be optimized
        DataCommon.pushStringData(qname.getNamespaceURI(), body, dc);
        DataCommon.pushStringData(qname.getLocalPart(), body, dc);
        program.push(2);
        DataCommon.pushStringData("_root.QName", body, dc);
        program.newObject();
    }


    /**
     * Get first element by namespace within element. If namespace is null, get
     * first element in owner element's namespace.
     *
     * @param ns namespace.
     * @param element owner element to search tags from.
     * @param tag tag name.
     * @return first element found, or null if none matched.
     */
    static Element getFirstElementByTagNameNS(String ns, Element element, String tag) {
        if (element == null) 
            return null;

        NodeList list;
        if (ns == null) 
            list = element.getElementsByTagName(tag);
        else
            list = element.getElementsByTagNameNS(ns, tag);

        if (list.getLength() == 0) 
            return null;
        return (Element)list.item(0);
    }

    /**
     * See getFirstElementByTagNameNS().
     */
    static Element getFirstElementByTagName(Element element, String tag) {
        return getFirstElementByTagNameNS(null, element, tag);
    }

}
