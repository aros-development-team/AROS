/* ****************************************************************************
 * XMLUtils_Test.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml.internal;
import java.io.*;
import java.util.*;
import junit.framework.*;

/*
 * junit.awtui.TestRunner
 * junit.swingui.TestRunner
 * junit.textui.TestRunner
 *
 * java junit.textui.TestRunner org.openlaszlo.utils.XMLUtils_Test
 *
 * @author Oliver Steele
 */

public class XMLUtils_Test extends TestCase {
    public XMLUtils_Test(String name) {
        super(name);
    }

    public void setUp() {
    }

    public void testPathnameFunctions() {
        assertTrue(XMLUtils.isURL("http:foo"));
        assertTrue(XMLUtils.isURL("file:foo"));
        assertTrue(XMLUtils.isURL("https:foo"));
        assertTrue(XMLUtils.isURL("soap:foo"));
        assertTrue(XMLUtils.isURL("ftp:foo"));
        assertTrue(!XMLUtils.isURL("a/b"));
        assertTrue(!XMLUtils.isURL("/a/b"));
        assertTrue(!XMLUtils.isURL("c:/a/b"));
    }
}
