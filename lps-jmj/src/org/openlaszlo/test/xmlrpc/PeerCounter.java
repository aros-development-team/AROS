/* ****************************************************************************
 * PeerCounter.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test.xmlrpc;

public class PeerCounter extends Counter
{
    int myNum;

    public PeerCounter() {
        myNum = mCount;
    }

    public int getPeerNum()
    {
        return myNum;
    }

    public int decrement()
    {
        return --mCount;
    }

    public void reset()
    {
        mCount = 0;
    }

}
