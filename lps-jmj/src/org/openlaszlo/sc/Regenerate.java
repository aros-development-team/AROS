/* *****************************************************************************
 * Regenerate.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.sc;

public class Regenerate {
    public static void main(String args[]) {
        // props could contain the 'runtime: swf5/6" flag
        java.util.Properties props = new java.util.Properties();
        new Regenerator().compile(props, args);
    }
}
