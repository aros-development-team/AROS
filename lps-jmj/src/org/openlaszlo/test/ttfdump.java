/* *****************************************************************************
 * ttfdump.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test;

import java.io.*;
import org.openlaszlo.utils.FileUtils;
import org.apache.log4j.*;

// Apache Batik TrueType Font Parser
import org.apache.batik.svggen.font.*;
import org.apache.batik.svggen.font.table.*;

public class ttfdump {
   static public void main (String args[]) {
       try {
           File src = new File (args[0]);
           System.out.println("Trying " + src.getPath());
           org.apache.batik.svggen.font.Font ttf;
           ttf = org.apache.batik.svggen.font.Font.create(src.getPath());
           NameTable nameTable = ttf.getNameTable();
           System.out.println("Font name: " + nameTable.getRecord((short)1)); 
       } catch (Exception e) {
           e.printStackTrace();
       }
   } 
}
