/* *****************************************************************************
 * copyswf.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/


package org.openlaszlo.test;

import java.io.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.utils.FileUtils;


public class copyswf {
   static public void main (String args[]) {
       try {
           FlashFile f = FlashFile.parse(args[0]);
           OutputStream out = new FileOutputStream(args[1]);

           java.util.Enumeration defs = f.definitions();
           FontsCollector fc = new FontsCollector();
           FontsCollector pfc = new FontsCollector();

           // Start out writing ito the preloader fc
           FontsCollector curfc = pfc;

           // A lps-2.0 generated SWF has either 
           //     1 block of fonts or
           //     2 blocks of fonts
           // If there are 2 blocks, then we know there's a preloader.
           // If there's only 1 block, then they're either all inthe
           // preloader or there's no preloader.
           //
           // If the first tag isn't a definefont tag, then we know
           // we have a preloader, too and can short circuit that
           // case.  Otherwise, we need to look for a break in
           // the defs for one that isn't a font.  
           
           int index = 0;
           int lastIndex = -1;

           boolean hasPreloader = false;

           while(defs.hasMoreElements()) {
               FlashDef def = (FlashDef)defs.nextElement();
               index++;

               // Here we know we definitely have a preloader
               // but no preloader fonts
               if (index == 1 && !(def instanceof FontDef)) {
                   hasPreloader = true;
               }

               if (def instanceof FontDef) {
                   // If we don't have a preloader 
                   // (or we don't know if we have a preloader)
                   if (!hasPreloader) {
                       // First time in
                       if (lastIndex == -1) {
                           lastIndex = index;
                       }
                       else {
                           // All other times
                           // See if we're on to a second group of fonts
                           if (index > lastIndex + 1) {
                               hasPreloader = true;
                               curfc = fc;
                           }
                       }
                   }
                   FontDef fontDef = (FontDef)def; 
                   Font font = fontDef.getFont();
                   String bold = ((font.BOLD & font.flags) > 0 ? "bold" : "");
                   String italic = ((font.ITALIC & font.flags) > 0 ? 
                           "italic" : "");
                   System.out.println("Copying font " + font.getFontName() 
                               + bold + italic);
                   FontDef fdef = curfc.addFont(font, null);
                   fdef.setWriteAllChars(true);
                   fdef.setWriteLayout(true);
               }
               index++;
           }
           InputStream input;

           // Ok we didn't have a preloader to the pfc is the regular
           // fc and the pfc should be empty.
           if (!hasPreloader) {
               fc = pfc;
               pfc = null;
           }
           input = f.generate(fc, pfc, hasPreloader).getInputStream();
           FileUtils.send(input, out);
           input.close();
           out.close();
       } catch (Exception e) {
           e.printStackTrace();
       }
   } 
}
