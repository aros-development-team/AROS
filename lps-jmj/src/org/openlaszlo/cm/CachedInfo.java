/* *****************************************************************************
 * CachedInfo.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.cm;
import org.openlaszlo.compiler.*;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;

import org.apache.log4j.*;

/** A <code>CachedInfo</code> contains all the information
 * we cache from a compilation.  This includes dependency information
 * and the canvas.
 *
 * @author  Oliver Steele
 */
public class CachedInfo implements Serializable {

    private final static Logger     mLogger  = Logger.getLogger(CachedInfo.class);
    private final DependencyTracker mTracker;
    private final Canvas            mCanvas;
    private final String            mEncoding;

    /**
     * Construct a CachedInfo
     * @param tracker the DependencyTracker
     * @param canvas the Canvas
     */
    public CachedInfo(DependencyTracker tracker, Canvas canvas, String encoding) {
        mTracker = tracker;
        mCanvas = canvas;
        mEncoding = encoding;
    }

    /**
     * Read a CachedInfo from an existing file.
     */
    public static CachedInfo readFrom(File file) 
        throws CompilationError
    {
        CachedInfo info = null;
        
        try {
            FileInputStream istream = new FileInputStream(file);
            try {
                ObjectInputStream p = new ObjectInputStream(istream);
                return (CachedInfo) p.readObject();
            } finally {
                istream.close();
            }
        } catch (java.io.InvalidClassException ioe) {
        } catch (FileNotFoundException ioe) {
        } catch (IOException ioe) {
            CompilationError e = new CompilationError(ioe);
            e.initPathname(file.getPath());
            mLogger.error(e.getMessage());
            throw e;
        } catch (ClassNotFoundException cnfe) {
        }
        return new CachedInfo(null, null, null);
    }

    /** Write a CachedInfo to an existing file
     * @param file a File to save to
     * @throws IOException if an error occurs
     */
    void writeTo(File file) throws IOException {
        mLogger.debug("writeTo " + file.getAbsolutePath());
        File dir = file.getParentFile();
        if (dir != null) {
            dir.mkdirs();
        }
        file.createNewFile();
        FileOutputStream ostream = new FileOutputStream(file);
        ObjectOutputStream p = new ObjectOutputStream(ostream);
        p.writeObject(this);
        p.flush();
        ostream.close();
    }

    public Canvas getCanvas() {
        mLogger.debug("Getting canvas with size " + mCanvas.getWidth() +
                      " by " + mCanvas.getHeight());
        return mCanvas;
    }

    public String getEncoding() {
        return mEncoding;
    }


    public DependencyTracker getDependencyTracker() {
        return mTracker;
    }
}
