/*****************************************************************************
 * FontManager.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import org.openlaszlo.sc.ScriptCompiler;
import org.openlaszlo.server.LPS;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.api.sound.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;

import org.openlaszlo.compiler.CompilationEnvironment;

import org.openlaszlo.media.*;

import java.io.*;
import java.util.*;
import java.lang.Math;
import java.lang.Character;

// jgen 1.4
import java.awt.geom.Rectangle2D;

import org.apache.log4j.*;



public class FontManager {
    /** Font name map */
    private final Hashtable mFontTable = new Hashtable();
        
    public FontFamily getFontFamily(String fontName) {
        if (fontName == null) return null;
        return (FontFamily) mFontTable.get(fontName);
    }
        
    public FontFamily getFontFamily(String fontName, boolean create) {
        FontFamily family = (FontFamily) mFontTable.get(fontName);
        if (family == null && create) {
            family = new FontFamily();
            mFontTable.put(fontName, family);
        }
        return family;
    }
        
    public Enumeration getNames() {
        return mFontTable.keys();
    }

    public boolean checkFontExists (FontInfo fontInfo) {
        String fontName = fontInfo.getName();
        int    size     = fontInfo.getSize();
        int    style    = fontInfo.styleBits;
        FontFamily family = getFontFamily(fontName);
        if (family == null) {
            return false;
        }
        Font font = family.getStyle(style);
        if (font == null) {
            return false;
        }
        return true;
    }

}
