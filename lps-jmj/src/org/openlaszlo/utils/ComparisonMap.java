/******************************************************************************
 * ComparisonMap.java
 *****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;

import java.util.*;

/**
 * Hash table that stores keys case-sensitively, but compares them
 * case-insensitively
 *
 * This differs from
 * org.apache.commons.collections.map.CaseInsensitiveMap in that it
 * preserves the first case used for a key (rather than converting all
 * keys to lower case)
 *
 */
public class ComparisonMap extends HashMap {

    /** canonical lower-case map */
    private HashMap keyMap;

    public Set normalizedKeySet() {
        return keyMap.keySet();
    }

    public ComparisonMap() {
        super();
        keyMap = new HashMap();
    }

    public ComparisonMap(int initialCapacity) {
        super(initialCapacity);
        keyMap = new HashMap(initialCapacity);
    }

    public ComparisonMap(int initialCapacity, float loadFactor) {
        super(initialCapacity, loadFactor);
        keyMap = new HashMap(initialCapacity, loadFactor);
    }

    public ComparisonMap(Map m) {
        this(m.size());
        putAll(m);
    }

    private Object caseInsensitiveKey(Object key) {
        return key instanceof String ? ((String)key).toLowerCase() : key;
    }

    public void clear() {
        keyMap.clear();
        super.clear();
    }

    public Object clone() {
        return new ComparisonMap(this);
    }

    public boolean containsKey(Object key) {
        return keyMap.containsKey(caseInsensitiveKey(key));
    }

    /* containsValue just works */

    /* entrySet just works, keys have original case */

    public Object get(Object key) {
        key = caseInsensitiveKey(key);
        if (keyMap.containsKey(key)) {
            return super.get(keyMap.get(key));
        } else {
            return null;
        }
    }

    /* isEmpty just works */

    /* keySet just works, keys have original case */

    public Object put (Object key, Object value) {
        keyMap.put(caseInsensitiveKey(key), key);
        return super.put(key, value);
    }

    public void putAll(Map m) {
        for (Iterator i = m.entrySet().iterator(); i.hasNext(); ) {
            Map.Entry entry = (Map.Entry) i.next();
            put(entry.getKey(), entry.getValue());
        }
    }

    public Object remove(Object key) {
        key = caseInsensitiveKey(key);
        if (keyMap.containsKey(key)) {
            return super.remove(keyMap.remove(key));
        } else {
            return null;
        }
    }

    /* size just works */

    /* values just works */

}
