/* *****************************************************************************
 * DependencyTracker.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.cm;

import org.apache.log4j.*;
import java.io.*;
import java.util.*;
import org.openlaszlo.server.LPS;
import org.openlaszlo.utils.ChainedException;

/** Tracks compilation dependencies, so that it can tell if a file
 * needs to be recompiled.  An instance stores version information
 * about all the source files that an object file depends on (the
 * files such that, if one changed, the object file would be out of
 * date).
 *
 * @author Oliver steele
 */
class DependencyTracker implements java.io.Serializable {
    private static Logger mLogger  = Logger.getLogger(DependencyTracker.class);

    /** Records information about the version of a file.
     */
    private static class FileInfo implements java.io.Serializable {
        /** pathname */
        private String mPathname;
        /** last mod time */
        private long mLastMod;
        /** can read? */
        private boolean mCanRead;
        /** File length */
        private long mLength;
        
        /** Constructs an instance.
         * @param pathname the name of a file
         */
        FileInfo(String pathname) {
            mPathname = pathname;
            File file = new File(pathname);
            // Cope with directory indexes for now
            // FIXME: [2003-05-09 bloch] is this the right place
            // for this?
            if (file.isDirectory()) {
                file = new File(pathname + File.separator + "library.lzx");
            }
            // Truncate to an seconds
            mLastMod = ((long)(file.lastModified() / 1000L)) * 1000L;
            mCanRead = file.canRead();
            //mLogger.debug("lm: " + mLastMod);
            mLength = file.length();
        }
        
        /** Returns true iff this FileInfo has up to date information
         * compared to the fileinfo argument.
         * @param info another FileInfo
         * @return see documentation
         */
        boolean isUpToDate(FileInfo info) {
            return this.mLastMod == info.mLastMod
                && this.mCanRead == info.mCanRead
                && this.mLength == info.mLength;
        }
    };

    /** A list of FileInfo records for files that are depended on. */
    private final Vector mDependencies = new Vector();
    private Properties mProperties;
    private String mWebappPath;

    DependencyTracker(Properties properties) {
        this.mProperties = properties;
        this.mWebappPath = LPS.HOME(); // get it from global
    }
    
    /** Add the specified file to the list of file dependencies.
     * @param file a file
     */
    void addFile(File file) {
        mLogger.debug("addFile Path is " + file.getPath());
        FileInfo fi = new FileInfo(file.getPath());
        try {
            fi.mPathname = file.getCanonicalPath();
        } catch (java.io.IOException e) {
            throw new ChainedException(e);
        }
        mDependencies.add(fi);
    }


    /**
     * Copy file info from the given tracker to me, omitting
     * omitting then given file.
     */
    void copyFiles(DependencyTracker t, File omitMe) {
        try {
            for (Iterator e = t.mDependencies.iterator(); e.hasNext(); ) {
                FileInfo f = (FileInfo)e.next();
                if (! f.mPathname.equals(omitMe.getCanonicalPath())) {
                    mDependencies.add(f);
                }
            }
        } catch (java.io.IOException e) {
            throw new ChainedException(e);
        }
    }


    /** This will update the FileInfo object chain to use the (possibly new)
     * webappPath once the DependencyTracker object has been reconstitutded
     * from ondisk cache.
     */
    void updateWebappPath() {
        String webappPath = LPS.HOME(); // get it from global
        if (webappPath.equals(mWebappPath))
            return;
        mLogger.debug("updating webappPath from: " + mWebappPath);
        mLogger.debug("updating webappPath to:   " + webappPath);
        for (Iterator e = mDependencies.iterator(); e.hasNext(); ) {
            FileInfo saved = (FileInfo) e.next();
            if (saved.mPathname.startsWith(mWebappPath)) {
                mLogger.debug("updating dependencies from: " + saved.mPathname);
                saved.mPathname = webappPath +
                        saved.mPathname.substring(mWebappPath.length());
                mLogger.debug("updating dependencies to  : " + saved.mPathname);
            }
        }
        mWebappPath = webappPath;
    }

    /** Returns true iff all the files listed in this tracker's
     * dependency list exist and are at the same version as when they
     * were recorded.
     * @return a boolean
     */
    boolean isUpToDate(Properties properties) {
        Iterator e;
        
        // fixes bug 962 
        {
            if (mProperties.size() != properties.size()) {
                mLogger.debug("my size " + mProperties.size());
                mLogger.debug("new size " + properties.size());
                return false;
            }

            for (e = mProperties.keySet().iterator(); e.hasNext(); ) {
                String key = (String) e.next();
                String val0 = mProperties.getProperty(key);
                String val1 = properties.getProperty(key);

                // val0 can't be null; properties don't allow that
                if (val1 == null || ! val0.equals(val1)) {
                    mLogger.debug("Missing or changed property: " + val0);
                    return false;
                }
            }
        }

        for (e = mDependencies.iterator(); e.hasNext(); ) {
            FileInfo saved = (FileInfo) e.next();
            FileInfo current = new FileInfo(saved.mPathname);
            if (!saved.isUpToDate(current)) {
                mLogger.debug(saved.mPathname + " has changed");
                mLogger.debug("was " + saved.mLastMod);
                mLogger.debug(" is " + current.mLastMod);
                return false;
            }
        }
        return true;
    }
}
