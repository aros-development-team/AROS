/******************************************************************************
 * PersistentMap.java
 *****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/


package org.openlaszlo.cache;
import java.io.*;
import java.util.*;

/** This class implements a subset of java.util.AbstractMap semantics,
    persisted to the file named by cacheDirectory. */
public class PersistentMap extends Cache {
    protected final Map map = new HashMap();
    
    public PersistentMap(String name, File cacheDirectory, Properties props)
        throws IOException
    {
        super(name, cacheDirectory, props);
    }
    
    public Object get(Serializable key) {
        Object value = this.map.get(key);
        if (value != null)
            return value;
        Item item = this.getItem(key);
        if (item == null)
            return null;
        InputStream is = item.getStream();
        try {
            value = new ObjectInputStream(is).readObject();
        } catch (ClassNotFoundException e) {
            return null;
        } catch (IOException e) {
            return null;
        } finally {
            try {
                is.close();
            } catch (IOException e) {}
        }
        this.map.put(key, value);
        return value;
    }
    
    public void put(Serializable key, final Serializable value) {
        this.map.put(key, value);
        try {
            Item item = this.findItem(key, null, false);
            PipedInputStream is = new PipedInputStream();
            final PipedOutputStream os = new PipedOutputStream();
            is.connect(os);
            new Thread() {
                public void run() {
                    try {
                        ObjectOutputStream oos = new ObjectOutputStream(os);
                        oos.writeObject(value);
                        oos.close();
                    } catch (IOException e) {
                        throw new RuntimeException(e);
                    }
                }
            }.start();
            item.update(is, null);
            item.updateInfo();
            item.markClean();
        } catch (java.io.IOException e) {
            throw new RuntimeException(e);
        }
    }
}
