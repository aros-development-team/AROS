/* ****************************************************************************
 * Compiler_Test.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.*;
import java.util.*;
import org.jdom.Element;
import junit.framework.*;
import org.openlaszlo.xml.internal.XMLUtils;

/*
 * junit.awtui.TestRunner
 * junit.swingui.TestRunner
 * junit.textui.TestRunner
 * java junit.textui.TestRunner org.openlaszlo.utils.Compiler_Test
 *
 * @author Oliver Steele
 */

public class Compiler_Test extends TestCase {
    public Compiler_Test(String name) {
        super(name);
    }

    public void setUp() {
    }

    public void testAdjustRelativePath() {
        String tests[] = {
            // sourcedir, destdir, source, dest
            // dest == null is an abbreviation for dest == source
            
            // If it's not an URL, don't change it
            "from", "to", "base.ext", null, // 1
            "from", "to", "dir/base.ext", null, // 2
            "from", "to", "/dir/base.ext", null, // 3
            "from", "to", "c:/dir/base.ext", null, // 4

            // If it's an absolute URL, don't change it
            "from", "to", "http:///base.ext", null, // 5
            "from", "to", "http:///dir/base.ext", null, // 6

            // If it's on another host, don't change it
            "from", "to", "http://host.com/base.ext", null, // 7
            "from", "to", "http://host.com/dir/base.ext", null, // 8

            // Relative URL on this host
            "", "", "http:base.ext", null, // 9
            ".", "", "http:base.ext", null, // 10
            "", ".", "http:base.ext", null, // 11
            ".", ".", "http:base.ext", null, // 12
            "from", "", "http:base.ext", "http:../base.ext", // 13
            "", "to", "http:base.ext", "http:to/base.ext", // 14
            "from", "to", "http:base.ext", "http:../to/base.ext", // 15
            "/from", "/to", "http:base.ext", "http:../to/base.ext", // 16
            "c:/from", "c:/to", "http:base.ext", "http:../to/base.ext", // 17

            // Preserve protocol and query
            // https: gets "unknown protocol" under ant
            "from", "to", "ftp:base.ext", "ftp:../to/base.ext", // 18
            "from", "to", "http:base.ext?query", "http:../to/base.ext?query", // 19

            // no way to indicate port on a relative URL?
            // doesn't preserve fragment, but the agent doesn't parse this
            // anyway
            //"from", "to", "http:base.ext#frag", "http:../to/base.ext#frag", // 19

            // Absolute pathnames on same root
            "/c/from", "/c/to", "http:base.ext", "http:../to/base.ext",
            "c:/from", "c:/to", "http:base.ext", "http:../to/base.ext",
        };
        int i = 0;
        while (i < tests.length) {
            File sourcedir = new File(tests[i++]);
            File destdir = new File(tests[i++]);
            String source = tests[i++];
            String dest = tests[i++];
            if (dest == null) {
                dest = source;
            }
            String result = CompilationEnvironment.adjustRelativeURL(
                source, sourcedir, destdir);
            assertEquals("" + i/4 + ": adjustRelativeURL(\"" + source + "\", \"" + sourcedir + "\", \"" + destdir + "\")", result, dest);
        }
    }

    public void testRlementRecognition() {
        // local datasets
        for (Iterator iter = Arrays.asList(
                 new String[] {"<dataset/>",
                               "<dataset src=\"local\"/>",
                               "<dataset src=\"c:/local\"/>",
                 }).iterator(); iter.hasNext(); ) {
            String src = (String) iter.next();
            Element elt = XMLUtils.parse(src);
            assertTrue(src, DataCompiler.isElement(elt));
        }
        // non-local dataset
        for (Iterator iter = Arrays.asList(
                 new String[] {"<dataset type=\"soap\"/>",
                               "<dataset type=\"http\"/>",
                               "<dataset src=\"http:foo\"/>",
                               "<dataset src=\"https:foo\"/>",
                               "<dataset src=\"ftp:foo\"/>",
                 }).iterator(); iter.hasNext(); ) {
            String src = (String) iter.next();
            Element elt = XMLUtils.parse(src);
            assertTrue(src, !DataCompiler.isElement(elt));
        }
    }
}
