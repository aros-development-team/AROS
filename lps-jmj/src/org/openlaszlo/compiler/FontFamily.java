/*****************************************************************************
 * FontFamily.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import org.openlaszlo.iv.flash.api.text.*;

import java.awt.geom.Rectangle2D;

/** 
 * Class for holding Font Families
 *
 * @author Eric Bloch
 */
final class FontFamily {

    private Rectangle2D[] boundsPlain = null;
    private Rectangle2D[] boundsBold = null;
    private Rectangle2D[] boundsItalic = null;
    private Rectangle2D[] boundsBitalic = null;

    public FontFamily() {
        plain = null;
        bold = null;
        italic = null;
        bitalic = null;
    }
    /**
     * Create a font description
     */
    public FontFamily(Font plain, Font bold, Font italic, Font bitalic) {
        this.plain = plain;
        this.bold = bold;
        this.italic = italic;
        this.bitalic = bitalic;
    }

    /**
     * @return the font font for the given style
     */
    public Font getStyle(int style) {
        switch (style) {
        case FontInfo.PLAIN:
            return plain;
        case FontInfo.BOLD:
            return bold;
        case FontInfo.ITALIC:
            return italic;
        case FontInfo.BOLDITALIC:
            return bitalic;
        default:
            throw new CompilationError("Unknown style " + style);
        }
    }
    
    private String getBookRecommendations() {
        return "http://www.wetmachine.com/books.html";
    }
    
    /**
     * @return the glpyh bounds array for this font
     */
    public Rectangle2D[] getBounds(int style) {
        switch (style) {
        case FontInfo.PLAIN:
            if (plain != null && boundsPlain == null) {
                boundsPlain = plain.getGlyphBounds();
            }
            return boundsPlain;
        case FontInfo.BOLD:
            if (bold != null && boundsBold == null) {
                boundsBold = bold.getGlyphBounds();
            }
            return boundsBold;
        case FontInfo.ITALIC:
            if (italic != null && boundsItalic == null) {
                boundsItalic = italic.getGlyphBounds();
            }
            return boundsItalic;
        case FontInfo.BOLDITALIC:
            if (bitalic != null && boundsBitalic == null) {
                boundsBitalic = bitalic.getGlyphBounds();
            }
            return boundsBitalic;
        default:
            throw new CompilationError("Unknown style " + style);
        }
    }

    /** the plain font */
    public Font plain = null;
    /** the bold font */
    public Font bold = null;
    /** the italic font */
    public Font italic = null;
    /** the bold italic font */
    public Font bitalic = null;
}
