/* *****************************************************************************
 * FileResolver.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.File;
import java.util.Vector;
import java.util.Enumeration;
import org.openlaszlo.server.*;
import org.apache.log4j.*;

/**
 * Provides an interface for resolving a pathname to a File.  The
 * Compiler class uses this to resolve includes (hrefs).
 *
 * @author Oliver Steele
 */
public interface FileResolver {
    /** An instance of the DefaultFileResolver */
    FileResolver DEFAULT_FILE_RESOLVER = new DefaultFileResolver();

    /** Given a pathname, return the File that it names.
     * @param pathname a path to resolve
     * @param base a relative URI against which to resolve it
     * @return see doc
     * @exception java.io.FileNotFoundException if an error occurs
     */
    File resolve(String pathname, String base)
        throws java.io.FileNotFoundException;
}

/** DefaultFileResolver maps each pathname onto the File that
 * it names, without doing any directory resolution or other
 * magic.  (The operating system's notion of a working directory
 * supplies the context for partial pathnames.) 
 */
class DefaultFileResolver implements FileResolver {

    public DefaultFileResolver() {
    }

    /** @see FileResolver */
    public File resolve(String pathname, String base) 
        throws java.io.FileNotFoundException
    {
        Logger mLogger = Logger.getLogger(FileResolver.class);

        final String FILE_PROTOCOL = "file";
        String protocol = FILE_PROTOCOL;

        // The >1 test allows file pathnames to start with DOS
        // drive letters.
        int pos = pathname.indexOf(':');
        if (pos > 1) {
            protocol = pathname.substring(0, pos);
            pathname = pathname.substring(pos + 1);
        }
        mLogger.debug("Resolving pathname: " + pathname + " and base: " + base);
        if (!FILE_PROTOCOL.equals(protocol)) {
            throw new CompilationError("unknown protocol: " + protocol);
        }


        File f = new File(base, pathname);
        // FIXME: [ebloch] this vector should be in a properties file
        Vector v = new Vector();
        v.add(base);
        if (!pathname.startsWith("./") && !pathname.startsWith("../")) {
            v.add(LPS.getComponentsDirectory());
            v.add(LPS.getFontDirectory());
            v.add(LPS.getLFCDirectory());
        }
        
        Enumeration e = v.elements();
        while (e.hasMoreElements()) {
            String dir = (String)e.nextElement();
            f = new File(dir, pathname);
            mLogger.debug("Trying " + f.getAbsolutePath());
            if (f.exists()) {
                // TODO: [2002-11-23 ows] check for case mismatch
                mLogger.debug("Resolving " + pathname + " to "  +
                              f.getAbsolutePath());
                /*
                 * TODO: [2004-06-01 bloch] The code below results in weird compiler errors. 
                 * I think I was going to put it here so we could add the canvas info
                 * resolver elements in one place.   
                try {
                    return f.getCanonicalFile();
                } catch (java.io.IOException ex) {
                    throw new CompilationError("Can't get canonical file name " + f.getPath());
                }
                */
                return f;
            }
        }
        throw new java.io.FileNotFoundException(pathname);
    }
}
