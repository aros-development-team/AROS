/* ****************************************************************************
 * ViewSchema_Test.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.*;
import java.util.*;
// import org.apache.oro.text.regex.*;
import org.jdom.*;
import org.openlaszlo.compiler.ViewSchema;
import org.openlaszlo.utils.ChainedException;
import org.jdom.input.SAXBuilder;
import org.xml.sax.InputSource;

/*
 * junit.awtui.TestRunner
 * junit.swingui.TestRunner
 * junit.textui.TestRunner
 *
 *
 * java  junit.textui.TestRunner  org.openlaszlo.ic.ViewSchema_Test
 */

import junit.framework.*;


public class ViewSchema_Test extends TestCase {
    public ViewSchema_Test (String name) {
        super(name);
    }

    public void setUp () {
    }


    public void testParseColors () {
        ViewSchema schema = new ViewSchema();
        assertEquals("parseColor #FFFFFF", 0XFFFFFF, schema.parseColor("#FFFFFF"));
        assertEquals("parseColor #FF0000", 0xFF0000, schema.parseColor("#FF0000"));
        assertEquals("parseColor #800080", 0x800080, schema.parseColor("#800080"));
        assertEquals("parseColor #FF00FF", 0xFF00FF, schema.parseColor("#FF00FF"));
        assertEquals("parseColor #123456", 0x123456, schema.parseColor("#123456"));
        assertEquals("parseColor #DEADBE", 0xDEADBE, schema.parseColor("#DEADBE"));
        assertEquals("parseColor #000000", 0x000000, schema.parseColor("#000000"));
        assertEquals("parseColor black", 0x000000, schema.parseColor("black"));
        assertEquals("parseColor green", 0x008000, schema.parseColor("green"));
        assertEquals("parseColor silver", 0xC0C0C0, schema.parseColor("silver"));
        assertEquals("parseColor lime", 0x00FF00, schema.parseColor("lime"));
        assertEquals("parseColor gray", 0x808080, schema.parseColor("gray"));
        assertEquals("parseColor olive", 0x808000, schema.parseColor("olive"));
        assertEquals("parseColor white", 0xFFFFFF, schema.parseColor("white"));
        assertEquals("parseColor yellow", 0xFFFF00, schema.parseColor("yellow"));
        assertEquals("parseColor maroon", 0x800000, schema.parseColor("maroon"));
        assertEquals("parseColor navy", 0x000080, schema.parseColor("navy"));
        assertEquals("parseColor red", 0xFF0000, schema.parseColor("red"));
        assertEquals("parseColor blue", 0x0000FF, schema.parseColor("blue"));
        assertEquals("parseColor purple", 0x800080, schema.parseColor("purple"));
        assertEquals("parseColor teal", 0x008080, schema.parseColor("teal"));
        assertEquals("parseColor fuchsia", 0xFF00FF, schema.parseColor("fuchsia"));
        assertEquals("parseColor aqua", 0x00FFFF, schema.parseColor("aqua"));

    }


    public void testHTMLContent () {
        ViewSchema schema = new ViewSchema();

        assertTrue("hasHTMLContent text",       schema.hasHTMLContent(new Element("text")));
        assertTrue("hasHTMLContent class",    !schema.hasHTMLContent(new Element("class")));
        assertTrue("hasHTMLContent method",   !schema.hasHTMLContent(new Element("method")));
        assertTrue("hasHTMLContent property", !schema.hasHTMLContent(new Element("property")));
        assertTrue("hasHTMLContent script",   !schema.hasHTMLContent(new Element("script")));
    }

    public void testSetAttributes () {

        ViewSchema schema = new ViewSchema();
        try {
            schema.loadSchema();
        } catch (JDOMException e) {
            throw new RuntimeException(e.getMessage());
        }

        String class1 = "mynewclass";
        String subclass = "mynewsubclass";

        Element elt1 = new Element("classdef1");
        Element elt2 = new Element("classdef2");

        schema.addElement(elt1, "mynewclass", "view", new ArrayList());
        schema.addElement(elt2, "mynewsubclass", "mynewclass", new ArrayList());

        assertEquals("undefined class superclass",
                     null,
                     schema.getBaseClassname("view"));

        assertEquals(" superclass",
                     "view",
                     schema.getSuperclassName("mynewclass"));

        assertEquals(" superclass",
                     "mynewclass",
                     schema.getSuperclassName("mynewsubclass"));

        assertEquals("mynewclass superclass",
                     "view",
                     schema.getBaseClassname("mynewclass"));

        assertEquals("mynewsubclass superclass",
                     "view",
                     schema.getBaseClassname("mynewsubclass"));


        schema.setAttributeType(elt1, "mynewclass", "foo-width", new AttributeSpec("foo-width", schema.SIZE_EXPRESSION_TYPE, null, null));
        schema.setAttributeType(elt1, "mynewclass", "barbaz", new AttributeSpec("barbaz", schema.STRING_TYPE, null, null));

        schema.setAttributeType(elt1, "mynewsubclass", "baz-width", new AttributeSpec("baz-width", schema.SIZE_EXPRESSION_TYPE, null, null));
        schema.setAttributeType(elt1, "mynewsubclass", "barbaz", new AttributeSpec("barbaz", schema.EVENT_TYPE, null, null));

        assertEquals("mynewclass foo-width type",
                     schema.SIZE_EXPRESSION_TYPE,
                     schema.getAttributeType(class1, "foo-width"));

        assertEquals("mynewclass barbaz type",
                     schema.STRING_TYPE,
                     schema.getAttributeType(class1, "barbaz"));

        // Check subclass attribute types
        assertEquals("mynewsubclass foo-width type",
                     schema.SIZE_EXPRESSION_TYPE,
                     schema.getAttributeType(subclass, "foo-width"));

        assertEquals("mynewsubclass baz-width type",
                     schema.SIZE_EXPRESSION_TYPE,
                     schema.getAttributeType(subclass, "baz-width"));

        // Attribute type of subclass should override superclass type
        assertEquals("mynewsubclass barbaz type",
                     schema.EVENT_TYPE,
                     schema.getAttributeType(subclass, "barbaz"));

        // test for duplicate attributes, undefined superclass, redefined class, attr inheritance


    }

}

