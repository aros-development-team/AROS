/* *****************************************************************************
 * SymbolGenerator.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

/** A unique name supply.
 *
 * @author  Oliver Steele
 * @version 1.0
 */
public class SymbolGenerator {
    /** Prefix for generated names. */
    protected final String mSymbolPrefix;
    /** Number to append to next generated name. */
    protected int mIndex = 0;

    /** Constructs an instance.
     * @param prefix a string prefix
     */
    public SymbolGenerator(String prefix) {
        this.mSymbolPrefix = prefix;
    }

    /** Returns the next unique name.
     * @return a unique name
     */
    public String next() {
        mIndex += 1;
        return mSymbolPrefix + new Integer(mIndex).toString();
    }
}
