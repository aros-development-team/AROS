/* ****************************************************************************
 * FileUtils_Test.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;
import java.io.*;
import java.util.*;
import junit.framework.*;

/*
 * junit.awtui.TestRunner
 * junit.swingui.TestRunner
 * junit.textui.TestRunner
 *
 * java junit.textui.TestRunner org.openlaszlo.utils.FileUtils_Test
 *
 * @author Oliver Steele
 */

public class FileUtils_Test extends TestCase {
    public FileUtils_Test(String name) {
        super(name);
    }

    public void setUp() {
    }

    public void testPathnameFunctions() {
        assertEquals("getBase 1", FileUtils.getBase("base"), "base");
        assertEquals("getBase 2", FileUtils.getBase("base.ext"), "base");
        assertEquals("getBase 3", FileUtils.getBase("dir/base"), "dir/base");
        assertEquals("getBase 4", FileUtils.getBase("dir/base.ext"), "dir/base");
        assertEquals("getExtension 1", FileUtils.getExtension("base"), "");
        assertEquals("getExtension 1", FileUtils.getExtension("base.ext"), "ext");
        assertEquals("getExtension 2", FileUtils.getExtension("dir/base"), "");
        assertEquals("getExtension 3", FileUtils.getExtension("dir/base.ext"), "ext");
    }

    public void textFixAbsWindowsPaths() {
        assertEquals("fixAbs 1", FileUtils.fixAbsWindowsPaths("a/b/c"), "a/b/c");
        assertEquals("fixAbs 2", FileUtils.fixAbsWindowsPaths("/a/b/c"), "/a/b/c");
        assertEquals("fixAbs 3", FileUtils.fixAbsWindowsPaths("a:/b/c"), "a/b/c");
        assertEquals("fixAbs 4", FileUtils.fixAbsWindowsPaths("A:/b/c"), "a/b/c");
        assertEquals("fixAbs 5", FileUtils.fixAbsWindowsPaths("A:"), "");
        assertEquals("fixAbs 6", FileUtils.fixAbsWindowsPaths("A"), "");
    }

    public void testAdjustRelativePath() {
        // Test split first, since everything else depends on it.
        assertEquals("split 1",
                     Arrays.asList(StringUtils.split("", "/")),
                     Arrays.asList(new String[] {}));
        assertEquals("split 2",
                     Arrays.asList(StringUtils.split("a", "/")),
                     Arrays.asList(new String[] {"a"}));
        assertEquals("split 3",
                     Arrays.asList(StringUtils.split("a/b", "/")),
                     Arrays.asList(new String[] {"a", "b"}));
        assertEquals("split 4",
                     Arrays.asList(StringUtils.split("/a", "/")),
                     Arrays.asList(new String[] {"", "a"}));
        assertEquals("split 5",
                     Arrays.asList(StringUtils.split("a/", "/")),
                     Arrays.asList(new String[] {"a", ""}));
        assertEquals("split 6",
                     Arrays.asList(StringUtils.split("/a/", "/")),
                     Arrays.asList(new String[] {"", "a", ""}));
        assertEquals("split 7",
                     Arrays.asList(StringUtils.split("a//b", "/")),
                     Arrays.asList(new String[] {"a", "", "b"}));
        assertEquals("split 7b",
                     Arrays.asList(StringUtils.split("a///b", "/")),
                     Arrays.asList(new String[] {"a", "", "", "b"}));
        assertEquals("split 8",
                     Arrays.asList(StringUtils.split("//a", "/")),
                     Arrays.asList(new String[] {"", "", "a"}));
        assertEquals("split 9",
                     Arrays.asList(StringUtils.split("a//", "/")),
                     Arrays.asList(new String[] {"a", "", ""}));
        for (Iterator iter = Arrays.asList(new String[] {
            "", "a", "/a", "a/b", "/a/b",
            "/", "a/", "/a/", "a/b/", "/a/b/"})
                 .iterator(); iter.hasNext();) {
            String s = (String) iter.next();
            assertEquals("split/join(" + s + ")",
                         StringUtils.join(StringUtils.split(s, "/"), "/"),
                         s);
            // File("a/") turns into File("a"), so don't even bother
            if (s.equals("/") || !s.endsWith("/")) {
                assertEquals("toURLPath(" + s + ")",
                             FileUtils.toURLPath(new File(s)), s);
            }
        }
        String tests[] = {
            // sourcedir, destdir, source, dest
            "", "", "base.ext", "base.ext", // 1
            "from", "to", "base.ext", "../to/base.ext", // 2
            "from1/from2", "to1/to2", "base.ext", "../../to1/to2/base.ext", // 3
            "from1", "to1/to2", "base.ext", "../to1/to2/base.ext", // 4
            "from1/from2", "to", "base.ext", "../../to/base.ext", // 5
            "/a/b/c", "/a/b/c", "base.ext", "base.ext", // 6
            "/a/b/c", "/a/d/e", "base.ext", "../../d/e/base.ext", // 7
            "/a/b", "/a/d/e", "base.ext", "../d/e/base.ext", // 8
            "/a/b/c", "/a/d", "base.ext", "../../d/base.ext", // 9
            "c:/a/b/c", "c:/a/b/c", "base.ext", "base.ext", // 10
            "c:/a/b/c", "c:/a/d/e", "base.ext", "../../d/e/base.ext", // 11
            "c:/a/b", "c:/a/d/e", "base.ext", "../d/e/base.ext", // 12
            "c:/a/b/c", "c:/a/d", "base.ext", "../../d/base.ext", // 13

            ".", "", "base.ext", "base.ext", // 14
            "", ".", "base.ext", "base.ext", // 15
            ".", ".", "base.ext", "base.ext", // 16
        };
        int i = 0;
        while (i < tests.length) {
            String fromdir = tests[i++];
            String todir = tests[i++];
            String fromfile = tests[i++];
            String tofile = tests[i++];
            for (int j = 0; j < 3; j++) {
                String fromdir_ = fromdir;
                if ((j & 1) != 0)
                    fromdir_ += "/";
                String todir_ = todir;
                if ((j & 2) != 0)
                    todir_ += "/";
                if (i != 0 && (fromdir_.equals("/") || todir_.equals("/")))
                    continue;
                assertEquals(""+ i/4 + " adjustRelativePath(" + fromfile +
                             "," + fromdir_ + ", " + todir_ + ")",
                             FileUtils.adjustRelativePath(
                                 fromfile, fromdir_, todir_), tofile);
            }
        }

        // TODO [ows 11/18/02] run this on Windows
        if (false) {
            String errTests[] = {
                "c:/", "d:/",
                "c:/", "",
                "", "d:/"
            };
            i = 0;
            while (i < errTests.length) {
                String sourcedir = errTests[i++];
                String destdir = errTests[i++];
                try {
                    FileUtils.adjustRelativePath(
                        "base.ext", sourcedir, destdir); // for effect
                    fail("adjustRelativePath(..., " + sourcedir + ", " +
                         destdir + ") " +
                         "should have thrown an exception");
                } catch (FileUtils.RelativizationError e) {}
            }
        }
    }

    public void testRelativePath() {
        assertEquals("relpath 1", FileUtils.relativePath("C:\\foo\\bar\\foobar", "C:\\foo\\bar"),
                                  "/foobar");
        assertEquals("relpath 2", FileUtils.relativePath("C:\\foo\\bar\\foobar", "C:\\foo\\bar\\"),
                                  "/foobar");
        assertEquals("relpath 2", FileUtils.relativePath("C:\\fxoo\\bar\\foobar", "C:\\foo\\bar\\"),
                                  "C:/fxoo/bar/foobar");
    }

}
