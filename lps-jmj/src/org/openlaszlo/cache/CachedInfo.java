/* *****************************************************************************
 * CachedInfo.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.cache;

import org.apache.log4j.*;

import java.io.Serializable;
import java.net.URL;
import java.net.MalformedURLException;

import org.openlaszlo.utils.ChainedException;

/**
 * A class for representing the serializable bits of
 * a cache item.
 *
 * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
 */
public class CachedInfo implements Serializable {

    /** Logger. */
    private static Logger mLogger = Logger.getLogger(CachedInfo.class);

    /** Size of the cached version of this item */
    private long mSize = 0;

    /** Name for this item */
    private long mName = 0;

    /** Key for this item */
    private Serializable mKey = null;

    /** Meta Data for this item */
    private Serializable mMetaData = null;

    /** True if item is in mem */
    private boolean mInMemory = false;

    /** Cached Last modified time of the remote version of this item 
     * (not the last modified time of the file in the cache) */
    private long mLastModified = -1;

    /** Encoding.
     * Arguably, this could go in the meta-data.  It's broken out here
     * for historical reasons.
     */
    private String mEncoding = null;

    /** 
     * Construct an CachedInfo based on this request
     */
    public CachedInfo(Serializable key, String encoding, boolean inMem, long name) { 

        mKey = key;
        if (encoding != null) {
            mEncoding = new String(encoding);
        }
        mName = name;
        mInMemory = inMem;
    }

    /**
     * @return true if the item is in in-mem cache
     */
    public boolean isInMemory() {
        return mInMemory;
    }

    /**
     * @return the name of this info
     */
    public long getName() {
        return mName;
    }

    /**
     * @return the size of this item in bytes 
     */
    public long getSize() {
        return mSize;
    }

    /**
     * @return the size of this item's key
     */
    public long getKeySize() {
        if (mKey instanceof String) {
            return ((String)mKey).length() * 2;
        } else {
            return -1;
        }
    }

    /**
     * @return the last modified time of this item
     */
    public long getLastModified() {
        return mLastModified;
    }

    /**
     * @return the key for this item 
     */
    public Serializable getKey() {
        return mKey;
    }

    /**
     * @return the encoding of this info
     */
    public String getEncoding() {
        return mEncoding;
    }

    /**
     * @return the meta data for this info
     */
    Serializable getMetaData() {
        return mMetaData;
    }


    /**
     * Set the in-memory state for this item
     */
    public void setInMemory(boolean inMem) {
        mInMemory = inMem;
    }

    /**
     * Set the cached size of this item in bytes 
     */
    public void setLastModified(long lm) {
        mLastModified = lm;
    }

    /**
     * Set the meta data for this info
     */
    public void setMetaData(Serializable md) {
        mMetaData = md;
    }

    /**
     * Return the cached size of this item in bytes 
     */
    public void setSize(long sz) {
        mSize = sz;
    }
}
