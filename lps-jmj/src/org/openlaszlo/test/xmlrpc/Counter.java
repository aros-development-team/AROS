/******************************************************************************
 * Counter.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test.xmlrpc;

public class Counter
{
    protected int mCount = 0;

    public Counter() {
    }

    public Counter(int count) {
        mCount = count;
    }

    public int increment() {
        return ++mCount;
    }

    public int getCount() {
        return mCount;
    }
}
