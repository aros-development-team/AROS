/* ****************************************************************************
 * PerfTest.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test.xmlrpc;

import java.util.*;
import java.io.*;
import javax.xml.rpc.*;
import javax.xml.parsers.*;
import javax.xml.namespace.*;
import org.w3c.dom.*;
import org.xml.sax.*;
import org.apache.axis.Constants;
import org.apache.axis.utils.*;
import org.apache.log4j.Logger;

class PerfDataParser
{
    DocumentBuilder mBuilder;

    public PerfDataParser() throws ParserConfigurationException
    {
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        mBuilder = factory.newDocumentBuilder();
    }
    
    
    public Map parse(String file) 
        throws IOException, SAXException {

        Map map = new HashMap();

        Element root = mBuilder.parse(new InputSource(new FileReader(file))).getDocumentElement();
        NodeList children = root.getChildNodes();
        for (int i=0; i < children.getLength(); i++) {
            Node node = children.item(i);
            if (node.getNodeType() != Node.ELEMENT_NODE) continue;
            Map item = new HashMap();
            NamedNodeMap attributes = ((Element)node).getAttributes();
            for (int j=0; j < attributes.getLength(); j++) { 
                Attr attr = (Attr)attributes.item(j);
                item.put(attr.getName(), attr.getValue());
            }
            map.put(item.get("id"), item);
        }

        return map;
    }
}


public class PerfTest
{
    static String PERF_DATA_DIR = "/home/pkang/perforce/qa/testharness/docroot/perf-data";

    static Map addrMap = new HashMap();

    static {
        try {
            PerfDataParser p = new PerfDataParser();
            addrMap.put(new Integer(0), p.parse(PERF_DATA_DIR + "/0k.xml"));
            addrMap.put(new Integer(1), p.parse(PERF_DATA_DIR + "/1k.xml"));
            addrMap.put(new Integer(10), p.parse(PERF_DATA_DIR + "/10k.xml"));
            addrMap.put(new Integer(25), p.parse(PERF_DATA_DIR + "/25k.xml"));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    public static Map getAddresses(int k) {
        return (Map)addrMap.get(new Integer(k));
    }
}
