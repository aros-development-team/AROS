/* *****************************************************************************
 * CompilationManager.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.cm;
import org.openlaszlo.compiler.CompilationError;
import org.apache.log4j.*;
import java.io.*;
import java.util.Properties;

public class Main {
    /**
     * Compile each file base.ext in args.  If compilation is
     * successful, create an object file base.swf.  Otherwise, create
     * an error report file base.html.
     *
     * Usage: <code>main [-src dir] [-cache dir] [-D...] file...</code>
     * 
     * This method is used to test the class from the command line.
     *
     * @param args a <code>String</code> value
     * @exception IOException if an error occurs
     */
    public static void main(String args[])
        throws IOException
    {
        // Configure logging
        Logger logger = Logger.getRootLogger();
        logger.setLevel(Level.ERROR);
        BasicConfigurator.configure();
        File srcDir = new File(".");
        File cacheDir = new File(".");
        Properties props = new Properties();
        
        for (int i = 0; i < args.length; i++) {
            String arg = args[i].intern();
            if (arg == "-src") {
                srcDir = new File(args[++i]);
                continue;
            }
            if (arg == "-cache") {
                cacheDir = new File(args[++i]);
                continue;
            }
            if (arg.startsWith("-D")) {
                String key = arg.substring(2);
                String value = "true";
                int offset = key.indexOf('=');
                if (offset >= 0) {
                    value = key.substring(offset + 1).intern();
                    key = key.substring(0, offset);
                }
                props.setProperty(key, value);
                continue;
            }
            if (arg.startsWith("-")) {
                System.err.println("usage error");
                return;
            }
            String fileName = arg;
            try {
                CompilationManager cm;

                cm = new CompilationManager(srcDir, cacheDir, props);
                cm.getItem(fileName, null);
            } catch (CompilationError e) {
                // Since this method is used to test the compilation
                // manager, and we're currently interested in testing
                // HTML errors, use this instead of getMessage.
                // Later, we might want to add a --html flag that
                // controls this.
                System.err.print(e.toHTML());
            }
        }
    }
}
