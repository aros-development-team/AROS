/* *****************************************************************************
 * addinputtext.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.sc;

import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.StringUtils;
import org.openlaszlo.compiler.FontInfo;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.api.sound.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;
import org.apache.log4j.*;

import java.io.*;
import java.util.*;

// jgen 1.4
import java.awt.geom.Rectangle2D;


/** Add in fixed-size input text resource with specified dimensions.
 * 
 * We get a list of resource names like this:
 * 
 * 'lzinputtext/lztahoe8/8/plain/398/157'
 *
 *  We need to parse out font name, size ,style, and width/height, and
 *  create a new textfield for them with the WORDWRAP flag asserted.
 */

public class AddInputText {
    private static Logger mLogger = Logger.getLogger(AddInputText.class);

    /** Constant */
    private static final int TWIP = 20;

    /** Leading for text and input text */
    private static int mTextLeading = 2; 

    static public void main (String args[]) {
        addtext(args[0], args[1], new String[] {args[2], args[3]} );
    }

    static public void addtext (String infile, String outfile, String resourceNames[]) {
        try {
            FlashFile f = FlashFile.parse(infile);
            OutputStream out = new FileOutputStream(outfile);

            java.util.Enumeration defs = f.definitions();
            FontsCollector fc = new FontsCollector();

            HashMap fontsTable = new HashMap();

            while(defs.hasMoreElements()) {
                FlashDef def = (FlashDef)defs.nextElement();
                if (def instanceof FontDef) {
                    FontDef fontDef = (FontDef)def; 
                    Font font = fontDef.getFont();
                    String bold = ((font.BOLD & font.flags) > 0 ? "bold" : "");
                    String italic = ((font.ITALIC & font.flags) > 0 ? 
                                     "italic" : "");
                    //System.out.println("Copying font " + font.getFontName() 
                    //+ bold + italic);
                    FontDef fdef = fc.addFont(font, null);
                    fdef.setWriteAllChars(true);
                    fdef.setWriteLayout(true);
                    fontsTable.put(font.getFontName() + "::"+bold+italic, font);
                }
            }


            // Look up fonts for resources, add custom input text fields
            for (int i = 0; i < resourceNames.length; i++) {
                String rsrc = resourceNames[i];
                System.out.println("addinputtext: processing "+rsrc);
                // parse string of form  'lzinputtext/lztahoe8/8/plain/398/157[/html][/passwd]/(m|s)' into FontInfo, w/h
                String path[] = StringUtils.split(rsrc, "/");
                if (path.length < 7) {
                    Exception e = new RuntimeException("inputtext resource name '"+rsrc+"' couldn't be parsed into a form like lzinputtext/lztahoe8/8/plain/398/157[/html][/passwd]/(m|s)");
                    e.printStackTrace();
                    throw e;
                }
                String fname = path[1];
                String fsize = path[2];
                String fstyle = path[3];
                int width =  (int) Double.parseDouble(path[4]);
                int height =  (int) Double.parseDouble(path[5]);
                Font font = (Font) fontsTable.get(fname + "::" + (fstyle.equals("plain") ? "" : fstyle));
                if (font == null) {
                    Exception e = new RuntimeException("could not find font "+fname+" " +fstyle +" for inputtext resource named '"+rsrc);
                    e.printStackTrace();
                    throw e;
                }
                boolean multiline = false;
                boolean password = false;

                // look for "passwd" or "m" tokens
                for (int j = 6; j < path.length; j++) {
                    if (path[j].equals("passwd")) {
                        password = true;
                    } else if (path[j].equals("m")) {
                        multiline = true;
                    }
                }

                FontInfo finfo = new FontInfo(fname, fsize, fstyle);
                addCustomInputText(f, font, finfo, width, height, multiline, password);
            }

            writeFlashFile(f, fc, out);

        } catch (Exception e) {
            mLogger.error("exception in AddInputText.addtext", e);
        }
    }

    static void writeFlashFile(FlashFile f, FontsCollector fc, OutputStream out) {
        try {
            InputStream input;
            input = f.generate(fc, null, false).getInputStream();
            FileUtils.send(input, out);
            input.close();
            out.close();
        } catch (Exception e) {
            mLogger.error("exception in AddInputText.writeFlashFile", e);
        }
    }


    /** Create a custom text input field that will correspond to a specific lzx input text view.
     */
    static public void addCustomInputText(FlashFile f, Font font, FontInfo fontInfo, int width, int height,
                                          boolean multiline, boolean password) 
    {
        String     fontName = fontInfo.getName();
        String mstring;
        mstring = multiline ? "/m" : "/s";

        String name =
            "lzinputtext/" + 
            fontName + "/" +
            fontInfo.getSize() + "/" +
            fontInfo.getStyle() + "/" + width + "/" + height +
            (password ? "/passwd" : "") + mstring;

        // Create a movieclip with a single frame with
        // a text field of the correct size.
        Script     movieClip = new Script(1);
        Frame      frame     = movieClip.newFrame();

        TextField  input = new TextField("", "text",
                                         font, fontInfo.getSize()*TWIP, AlphaColor.black);

        int flags = input.getFlags();

        if (password) {
            flags |= TextField.PASSWORD;
        }

        input.setFlags(flags 
                       | TextField.USEOUTLINES
                       | TextField.HASFONT
                       | (multiline ? TextField.MULTILINE : 0)
                       | (multiline ? TextField.WORDWRAP : 0)
                       );

        // left, right, indent, and align don't make sense 
        // when we do all input text wrapping ourselves.
        // Leading still matters though!
        input.setLayout(0, 0, 0, 0, mTextLeading*TWIP);

        input.setBounds(new Rectangle2D.Double(0, 0, width*TWIP, height*TWIP));

        frame.addInstance(input, 1, null, null);
        frame.addStopAction();

        // Add movieClip to library
        f.addDefToLibrary(name, movieClip);
        // Export it.
        ExportAssets ea = new ExportAssets();
        ea.addAsset(name, movieClip);
        System.out.println("addCustomInputText: added "+name);
        Timeline timeline = f.getMainScript().getTimeline();
        frame = timeline.getFrameAt(timeline.getFrameCount() - 1);
        frame.addFlashObject(ea);
    }
 
}
