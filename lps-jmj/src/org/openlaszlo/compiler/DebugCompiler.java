/******************************************************************************
 * DebugCompiler.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import org.jdom.Element;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.*;

/** 
 * Compiler for <code>debug</code> element.
 *
 * @author  Henry Minsky
 */
class DebugCompiler extends ElementCompiler {
    DebugCompiler(CompilationEnvironment env) {
        super(env);
    }

    static final String DEFAULT_DEBUGGER_WIDTH = "340";
    static final String DEFAULT_DEBUGGER_HEIGHT = "125";
    static final String DEFAULT_DEBUGGER_X = "10";
    static final String DEFAULT_DEBUGGER_Y = "10";
    static final String DEFAULT_DEBUGGER_FONT = "Verdana";
    static final String DEFAULT_DEBUGGER_FONTSIZE = "10";

    /** Returns true iff this class applies to this element.
     * @param element an element
     * @return see doc
     */
    public static boolean isElement(Element element) {
        return element.getName().equals("debug");
    }

    public void compile(Element element) 
        throws CompilationError
    {
        String width = element.getAttributeValue("width", DEFAULT_DEBUGGER_WIDTH);
        String height = element.getAttributeValue("height", DEFAULT_DEBUGGER_HEIGHT);
        String x = element.getAttributeValue("x", DEFAULT_DEBUGGER_X);
        String y = element.getAttributeValue("y", DEFAULT_DEBUGGER_Y);

        // If the canvas does not have the debug flag, let's return now
         if (!mEnv.getBooleanProperty(mEnv.DEBUG_PROPERTY)) {
            return;
        }

        mEnv.setProperty("debugger_width", width);
        mEnv.setProperty("debugger_height", height);
        mEnv.setProperty("debugger_x", x);
        mEnv.setProperty("debugger_y", y);

        // Now, merge in font style from canvas, and alert the SWF generator
        // as to what input text fonts are required.

        final SWFWriter generator = mEnv.getGenerator();
        FontInfo fontInfo = new FontInfo(DEFAULT_DEBUGGER_FONT, DEFAULT_DEBUGGER_FONTSIZE, "plain");
        String font = element.getAttributeValue("font", DEFAULT_DEBUGGER_FONT);
        String fontsize = element.getAttributeValue("fontsize", DEFAULT_DEBUGGER_FONTSIZE);

        // Clone a copy of the font info
        fontInfo = new FontInfo(fontInfo);

        if (font != null) {
            mEnv.setProperty("debugger_font", font);
            fontInfo.setName(font);
        }
        if (fontsize != null) {
            mEnv.setProperty("debugger_fontsize", fontsize);
            fontInfo.setSize(fontsize);
        }

        // Add input text hint
        if (mEnv.getSWFVersion().equals("swf5")) {
            generator.addInputText(fontInfo, false);
        }
        
    }
}
