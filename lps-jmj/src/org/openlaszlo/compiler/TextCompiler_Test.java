/* ****************************************************************************
 * TextCompiler_Test.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.*;
import java.util.*;
import junit.framework.*;
import org.openlaszlo.utils.ChainedException;
import org.jdom.Element;
import org.jdom.Document;
import org.jdom.input.SAXBuilder;
import org.xml.sax.InputSource;

/*
 * junit.awtui.TestRunner
 * junit.swingui.TestRunner
 * junit.textui.TestRunner
 * java junit.textui.TestRunner org.openlaszlo.compiler.TextCompiler_Test
 */

public class TextCompiler_Test extends TestCase {
    public TextCompiler_Test (String name) {
        super(name);
    }

    public void setUp () {
    }


    public void testHTMLContent () {
        String[] tests = {
            // Each case is input, expected-output (null == same as input)
            
            // whitespace
            "A foo <i></i>", "A foo<i></i>", // 0
            "B foo <i>bar</i>", "B foo <i>bar</i>", // 1
            "C foo <i> bar</i>", "C foo <i>bar</i>", // 2
            "D <i>foo</i> bar", "D <i>foo</i> bar", // 3
            "E <i>foo </i>bar", "E <i>foo </i>bar", // 4
            "F <i>foo </i> bar", "F <i>foo </i>bar", // 5
            "G <i>foo</i> <b>bar</b>", "G <i>foo</i> <b>bar</b>", // 6
            "H <i>foo</i> <b> bar</b>", "H <i>foo</i> <b>bar</b>", // 7
            "I <i>foo </i> <b> bar</b>", "I <i>foo </i><b>bar</b>",
            "J <i>foo </i><b> bar</b>",  "J <i>foo </i><b>bar</b>",
            "K <b>foo </b> <i> bar </i>", "K <b>foo </b><i>bar</i>", // final trailing spaces need to be trimmed

            "0 <i>foo </i><b> bar   </b>", "0 <i>foo </i><b>bar</b>",
            "1 foo <i>bar</i>", "1 foo <i>bar</i>",
            "2 foo <i> bar</i>  ", "2 foo <i>bar</i>",
            "3 <i>foo</i> bar  ", "3 <i>foo</i> bar",
            "4 <i>foo </i>bar", "4 <i>foo </i>bar",
            "5 <i>foo </i> bar    ", "5 <i>foo </i>bar",
            "6 <i>foo</i> <b>bar</b>", "6 <i>foo</i> <b>bar</b>",
            "7 <i>foo</i> <b>    bar  </b>", "7 <i>foo</i> <b>bar</b>",
            "8 <i>foo </i> <b> bar</b>", "8 <i>foo </i><b>bar</b>",
            "9 <i>foo </i><b> bar</b>.", "9 <i>foo </i><b>bar</b>.",
            "10 <i> foo </i> <b> bar </b> ", "10 <i>foo </i><b>bar</b>",
            "11 <i> foo </i><b> bar </b>  ", "11 <i>foo </i><b>bar</b>",
            "12 <i> foo</i><b> bar  </b>  ", "12 <i>foo</i><b> bar</b>",

            // complex test
            "<b><i>BoldItalic</i>Bold</b><i>Italic</i><u>Underline<b>Bold             Underline</b></u>", // ->
            "<b><i>BoldItalic</i>Bold</b><i>Italic</i><u>Underline<b>Bold Underline</b></u>",

            // font tags
            "<font color=\"#FF0000\">C</font><font color=\"#FFFF00\">O</font><font color=\"#00FFCC\">L</font><font color=\"#CC00CC\">O</font><font color=\"#AABB00\">R</font><font color=\"#DDA00A\">S</font> ", "<font color=\"#FF0000\">C</font><font color=\"#FFFF00\">O</font><font color=\"#00FFCC\">L</font><font color=\"#CC00CC\">O</font><font color=\"#AABB00\">R</font><font color=\"#DDA00A\">S</font>",
            "1 <font color=\"#ff0000\">foo <i>bar</i></font>", "1 <font color=\"#ff0000\">foo <i>bar</i></font>",
            "2 <font color=\"#ff0000\">foo</font><font color=\"#00ffff\"> <i> bar</i></font>", "2 <font color=\"#ff0000\">foo</font><font color=\"#00ffff\"> <i>bar</i></font>",
            "3 <font size=\"20\" color=\"#00ff00\"><i>foo</i></font><font color=\"#ff00aa\"> bar</font>", "3 <font size=\"20\" color=\"#00ff00\"><i>foo</i></font><font color=\"#ff00aa\"> bar</font>",


            // entities
            //"<b>x </b>&lt; y", "x &lt; y",
            "&lt;b&gt;this text shouldn't be bold&lt;/b&gt;", "&lt;b&gt;this text shouldn&apos;t be bold&lt;/b&gt;",

            // cdata
            "<b>x </b><![CDATA[y]]> z", "<b>x </b>y z",
            "<b>x</b> <![CDATA[y]]> z", "<b>x</b> y z",

            "<b>x </b><![CDATA[a & b < c & d > e + f]]> z", "<b>x </b>a &amp; b &lt; c &amp; d &gt; e + f z",
            "<![CDATA[<b>this text shouldn't be bold</b>]]>", "&lt;b&gt;this text shouldn&apos;t be bold&lt;/b&gt;",

              " text" , "text",
            "text " , "text",
            " <b>x</b>" , "<b>x</b>",
            "<b>x</b> " , "<b>x</b>",

            "<![CDATA[ text]]>" , "text",

            "<![CDATA[text ]]>" , "text",
            " <b>x</b>" , "<b>x</b>",
            "<b>x</b> " , "<b>x</b>",

              "<![CDATA[ ]]><b>x</b>" , "<b>x</b>",
            "<b>x</b><![CDATA[ ]]>" , "<b>x</b>",

            // whitespace next to CDATA:
            "<![CDATA[a]]>b" , "ab",
            "<![CDATA[a ]]>b" , "a b",
            "<![CDATA[a]]> b" , "a b",
            "<![CDATA[a ]]> b" , "a b",
            "a<![CDATA[b]]>" , "ab",
            "a<![CDATA[ b]]>" , "a b",
            "a <![CDATA[b]]>" , "a b",
            "a <![CDATA[ b]]>" , "a b",

            // empty cdata
            "a<![CDATA[]]>b", "ab",
            " a<![CDATA[]]>b", "ab",

            // trim methods, and attributes
            "some <attribute name=\"x\" value=\"10\"/> text", "some text",
            "some <method name=\"f\" args=\"x\">return</method> text", "some text",
            "some <state name=\"s\">state</state> text", "some text",
            
            // preserve attributes
            "<a href=\"foo\">b</a>", null,
            "<font face=\"face\" size=\"123\" color=\"red\">color</font>", null,
            
            // line breaks (TBD)

            // more complex tests

            "<b><i>BoldItalic</i>Bold&lt;&amp;&gt;</b><i>Italic</i><u>Underline<b>Bold Underline</b></u>", // ->
            "<b><i>BoldItalic</i>Bold&lt;&amp;&gt;</b><i>Italic</i><u>Underline<b>Bold Underline</b></u>",

        };
        int num = 1;
        for (Iterator iter = Arrays.asList(tests).iterator(); iter.hasNext(); num++) {
            String source = (String) iter.next();
            String result = (String) iter.next();
            if (result == null) {
                result = source;
            }
            
            try {
                org.jdom.input.SAXHandler handler = new org.jdom.input.SAXHandler();
                org.xml.sax.XMLReader reader =
                    org.xml.sax.helpers.XMLReaderFactory.createXMLReader(
                        "org.apache.xerces.parsers.SAXParser");
                reader.setContentHandler(handler);
                reader.parse(new org.xml.sax.InputSource(new StringReader("<text>" + source + "</text>")));
                Document doc = handler.getDocument();
                Element xml = doc.getRootElement();

                assertEquals("getHTMLContent(\"" + source + "\")",
                             result,
                             TextCompiler.getHTMLContent(xml));
            } catch (IOException e) {
                throw new ChainedException(e);
            } catch (org.xml.sax.SAXParseException e) {
                fail(e.getMessage());
            } catch (org.xml.sax.SAXException e) {
                fail(e.getMessage());
            }
        }
    }

    /*public void testHTMLContentWithEntities () {
        String XML2 = 

        try {
            org.jdom.input.SAXHandler handler = new org.jdom.input.SAXHandler();
            org.xml.sax.XMLReader reader =
                org.xml.sax.helpers.XMLReaderFactory.createXMLReader(
                    "org.apache.xerces.parsers.SAXParser");
            reader.setContentHandler(handler);
            // ignore DOCTYPE declarations
            reader.setEntityResolver(new org.xml.sax.helpers.DefaultHandler() {
                    public InputSource resolveEntity(
                        String publicId, String systemId)
                    {
                        return new InputSource(new StringReader(""));
                    }
                    });
            reader.parse(new org.xml.sax.InputSource(new StringReader(XML2)));
            Document doc = handler.getDocument();
            Element xml = doc.getRootElement();

            assertEquals("getHTMLContent",

                         TextCompiler.getHTMLContent(xml)
                         );

        } catch (IOException e) {
            throw new ChainedException(e);
        } catch (org.xml.sax.SAXParseException e) {
            fail(e.getMessage());
        } catch (org.xml.sax.SAXException e) {
            fail(e.getMessage());
        }
        }*/
}

