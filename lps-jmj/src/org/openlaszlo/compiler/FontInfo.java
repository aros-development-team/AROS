/* ****************************************************************************
 * FontInfo.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import org.openlaszlo.utils.ChainedException;
import java.io.Serializable;
import java.util.*;


/** 
 * Font information used for measuring text.
 *
 * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
 */
public class FontInfo implements java.io.Serializable {

    public static final String NULL_FONT = null;
    public static final int NULL_SIZE = -1;
    public static final int NULL_STYLE = -1;

    /** bit fields for text style */
    public final static int PLAIN = 0x0;
    /** bit fields for text style */
    public final static int BOLD   = 0x1;
    /** bit fields for text style */
    public final static int ITALIC = 0x2;
    /** bit fields for text style */
    public final static int BOLDITALIC = 0x3;

    /** font face */
    private String mName = null;
    /** font size */
    private int mSize;

    /** used for compile-time width/height cascading */
    private int mWidth = NULL_SIZE;
    private int mHeight = NULL_SIZE;

    /** font style bits */
    public int styleBits = 0;

    /** This can have three values:
        -1 = unset
        0 = false
        1 = true
    */
    public final static int FONTINFO_NULL = -1;
    public final static int FONTINFO_FALSE = 0;
    public final static int FONTINFO_TRUE = 1;

    // resizable defaults to false
    public int resizable = FONTINFO_NULL;

    // multiline default to false
    public int multiline = FONTINFO_NULL;

    public String toString() {
        return "FontInfo: name="+mName+", size="+mSize+", style="+getStyle();
    }

    public String toStringLong() {
        return "FontInfo: name="+mName+", size="+mSize+", style="+getStyle()+", width="+mWidth+", height="+mHeight+", resizable="+resizable+", multiline="+multiline;
    }


    /** 
     * Create a new FontInfo
     * @param name name
     * @param sz size
     * @param st style
     */
    public FontInfo(String name, String sz, String st) {
        mName  = name;
        setSize(sz);
        setStyle(st);
    }

    public FontInfo(FontInfo i) {
        this.mName = i.mName;
        this.mSize = i.mSize;
        this.styleBits = i.styleBits;

        // static text optimization params
        this.resizable = i.resizable;
        this.multiline = i.multiline;
        this.mWidth = i.getWidth();
        this.mHeight = i.getHeight();
    }

    public FontInfo(String name, int sz, int st) {
        mName  = name;
        mSize = sz;
        styleBits = st;
    }

    /** Extra params for static textfield optimization.
        Tracks width and height of a view.
     */
    public int getWidth() {
        return mWidth;
    }

    public int getHeight() {
        return mHeight;
    }


    public void setWidth (int w) {
        mWidth = w;
    }

    public void setHeight (int w) {
        mHeight = w;
    }



    /**
     * Set the name
     * @param f the name
     */
    public void setName(String f) {
        mName = f;
    }

    /**
     * Set the style
     * @param st style
     */
    public void setStyle(String st) {
        styleBits = styleBitsFromString(st);
    }

    /**
     * Set the style bits directly
     * @param st stylebits
     */
    public void setStyleBits(int st) {
        styleBits = st;
    }

    /**
     * Set the size
     * @param sz the size
     */
    public void setSize(String sz) {
        mSize  = Integer.parseInt(sz);
    }

    /**
     * Set the size
     * @param sz the size
     */
    public void setSize(int sz) {
        mSize  = sz;
    }

    /**
     * @return the size
     */
    public int getSize() {
        return mSize;
    }

    /**
     * @return the stylebits
     */
    public int getStyleBits() {
        return styleBits;
    }

    /**
     * @return the name
     */
    public String getName() {
        return mName;
    }

    public static FontInfo blankFontInfo() {
        FontInfo fi = new FontInfo(FontInfo.NULL_FONT, FontInfo.NULL_SIZE, FontInfo.NULL_STYLE);
        return fi;
    }

    /** Does this font spec contain a known font name,size,style ?
     */
    public boolean isFullySpecified () {
        return ((mSize != NULL_SIZE) &&
                (styleBits != NULL_STYLE) &&
                (mName != NULL_FONT) &&
                // we don't understand constraint expressions, just string literals
                (mName.charAt(0) != '$'));
    }


    /** If OTHER has non-null fields, copy
        them from OTHER to us.

        null fields are indicated by:
        name == null,
        size == -1,
        stylebits == -1,
     */
    public void mergeFontInfoFrom(FontInfo other) {
        if (other.getSize()!= NULL_SIZE) {
            mSize = other.getSize(); 
        }

        if (other.getStyleBits() != NULL_STYLE) {
            styleBits = other.getStyleBits();
        }

        if (other.getName()!= NULL_FONT) {
            mName = other.getName();
        }

        if (other.resizable != FONTINFO_NULL) {
            resizable = other.resizable;
        }

        if (other.multiline != FONTINFO_NULL) {
            multiline = other.multiline;
        }

        if (other.getWidth() != NULL_SIZE) {
            mWidth = other.getWidth();
        }

        if (other.getHeight() != NULL_SIZE) {
            mHeight = other.getHeight();
        }
    }

    /**
     * @return the name
     */
    public final String getStyle() {
        return styleBitsToString(styleBits);
    }

    /**
     * @return the name
     */
    public final String getStyle(boolean whitespace) {
        return styleBitsToString(styleBits, whitespace);
    }


    /**
     * Return the string representation of the style.
     *
     * @param styleBits an <code>int</code> encoding the style
     * @param whitespace whether to separate style names by spaces; e.g. true for "bold italic", false for "bolditalic"
     */
    public static String styleBitsToString(int styleBits, boolean whitespace) {
        switch (styleBits) {
            case PLAIN:
                return "plain";
            case BOLD:
                return "bold";
            case ITALIC:
                return "italic";
            case BOLDITALIC:
                if (whitespace) {
                    return "bold italic";
                } else {
                    return "bolditalic";
                }
          case NULL_STYLE:
            return "UNDEFINED STYLE";
          default:
                throw new RuntimeException("Unknown style " + styleBits);
        }
    }

    /**
     * Return the string representation of the style, e.g. "bold italic".
     *
     * @param styleBits an <code>int</code> value
     */
    public static String styleBitsToString(int styleBits) {
        return styleBitsToString(styleBits, true);
    }
    
    /**
    /**
     * @return the bits for a style
     */
    public static int styleBitsFromString(String name) {
        int style = PLAIN;
        if (name != null) {
            StringTokenizer st = new StringTokenizer(name);
            while (st.hasMoreTokens()) {
                String token = st.nextToken();
                if (token.equals("bold")) {
                    style |= BOLD;
                } else if (token.equals("italic")) {
                    style |= ITALIC;
                } else if (token.equals("plain")) {
                    style |= PLAIN;
                } else if (token.equals("bolditalic")) {
                    style |= ITALIC | BOLD;
                } else {
                    throw new CompilationError("Unknown style " + name);
                }
            }
        }
        return style;
    }

    /**
     * "bold italic", "italic bold" -> "bold italic" or "bolditalic"
     * (depending on the value of <code>whitespace</code>
     *
     * @param style a <code>String</code> value
     * @return a <code>String</code> value
     */
    public static String normalizeStyleString(String style, boolean whitespace) {
        return styleBitsToString(styleBitsFromString(style), whitespace);
    }
}
