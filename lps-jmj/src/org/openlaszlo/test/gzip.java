/* *****************************************************************************
 * gzip.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test;

import java.io.*;
import org.openlaszlo.utils.FileUtils;

public class gzip {
   static public void main (String args[]) {
       try {
           File src = new File (args[0]);
           String out = args[0] + ".gz";
           File dest = new File (out);
           long then = System.currentTimeMillis();
           FileUtils.encode(src, dest, "gzip");
           long now = System.currentTimeMillis();
           System.out.println("wrote " + out + " in " + (now - then) + " msecs");
       } catch (Exception e) {
           e.printStackTrace();
       }
   } 
}
