/*
 * $Id: AlphaColor.java,v 1.2 2002/02/15 23:44:27 skavish Exp $
 *
 * ==========================================================================
 *
 * The JGenerator Software License, Version 1.0
 *
 * Copyright (c) 2000 Dmitry Skavish (skavish@usa.net). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution, if
 *    any, must include the following acknowlegement:
 *    "This product includes software developed by Dmitry Skavish
 *     (skavish@usa.net, http://www.flashgap.com/)."
 *    Alternately, this acknowlegement may appear in the software itself,
 *    if and wherever such third-party acknowlegements normally appear.
 *
 * 4. The name "The JGenerator" must not be used to endorse or promote
 *    products derived from this software without prior written permission.
 *    For written permission, please contact skavish@usa.net.
 *
 * 5. Products derived from this software may not be called "The JGenerator"
 *    nor may "The JGenerator" appear in their names without prior written
 *    permission of Dmitry Skavish.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL DMITRY SKAVISH OR THE OTHER
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

package org.openlaszlo.iv.flash.api;

import java.io.*;
import java.util.*;
import java.lang.Math;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;

/**
 * Color with alpha value
 *
 * @author Dmitry Skavish
 * @see Color
 */
public final class AlphaColor extends Color {

    // some predefined colors
    public static final AlphaColor white = new AlphaColor( 255, 255, 255 );
    public static final AlphaColor black = new AlphaColor( 0, 0, 0 );
    public static final AlphaColor red   = new AlphaColor( 255, 0, 0 );
    public static final AlphaColor blue  = new AlphaColor( 0, 0, 255 );
    public static final AlphaColor green = new AlphaColor( 0, 255, 0 );
    public static final AlphaColor lightgray  = new AlphaColor( 180, 180, 180 );
    public static final AlphaColor gray  = new AlphaColor( 150, 150, 150 );
    public static final AlphaColor darkgray  = new AlphaColor( 90, 90, 90 );

    private int a;

    public AlphaColor() {}

    /**
     * Creates AlphaColor from int
     *
     * @param rgba   color encoded as follows: alpha<<24 | red<<16 | green<<8 | blue
     */
    public AlphaColor( int rgba ) {
        setRGBA( rgba );
    }

    /**
     * Creates opaque AlphaColor from three components
     *
     * @param r      red
     * @param g      green
     * @param b      blue
     */
    public AlphaColor( int r, int g, int b ) {
        this( r, g, b, 255 );
    }

    /**
     * Creates opaque AlphaColor from three float components
     *
     * @param r      red
     * @param g      green
     * @param b      blue
     */
    public AlphaColor( float r, float g, float b ) {
        this( r, g, b, 1.0f );
    }

    /**
     * Creates AlphaColor from four components
     *
     * @param r      red
     * @param g      green
     * @param b      blue
     * @param a      alpha
     */
    public AlphaColor( int r, int g, int b, int a ) {
        super( r, g, b );
        this.a = a;
    }

    /**
     * Creates AlphaColor from four components
     *
     * @param r      red
     * @param g      green
     * @param b      blue
     * @param a      alpha
     */
    public AlphaColor( float r, float g, float b, float a ) {
        super( r, g, b );
        this.a = Math.round( a * 255 );
    }

    public int getAlpha() {
        return a;
    }

    /**
     * Returns color encoded into int
     *
     * @return color encoded as follows: alpha<<24 | red<<16 | green<<8 | blue
     */
    public int getRGBA() {
        return a<<24 | getRGB();
    }

    public void setAlpha( int a ) {
        this.a = a;
    }

    /**
     * Sets color encoded into int
     *
     * @param rgba   color encoded as folows:  alpha<<24 | red<<16 | green<<8 | blue
     */
    public void setRGBA( int rgba ) {
        setRGB( rgba );
        this.a = (rgba&0xff000000)>>>24;
    }

    /**
     * Writes object into flash buffer
     *
     * @param fob    buffer to write
     */
    public void write( FlashOutput fob ) {
        writeRGBA( fob );
    }

    public void writeRGBA( FlashOutput fob ) {
        super.writeRGB( fob );
        fob.writeByte( a );
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"AlphaColor: (0x"+Util.b2h(r)+",0x"+Util.b2h(g)+",0x"+Util.b2h(b)+",0x"+Util.b2h(a)+")" );
    }

    public static AlphaColor parse( Parser p ) {
        return parseRGBA(p);
    }

    /**
     * Skips color without parsing
     *
     * @param p      parser to skip in
     */
    public static void skip( Parser p ) {
        p.skip(4);
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((AlphaColor)item).a = a;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new AlphaColor(), copier );
    }

    public boolean equals( Object o ) {
        if( o instanceof AlphaColor ) {
            AlphaColor c = (AlphaColor) o;
            return r == c.r && g == c.g && b == c.b && a == c.a;
        } else
            return a == 255 && super.equals( o );
    }

    /**
     * Returns true if this color has alpha<BR>
     * Always returns true
     *
     * @return true
     */
    public boolean hasAlpha() { return true; }

    /**
     * Creates new AlphaColor from this one with reduced alpha component by half
     *
     * @return reduced color
     */
    public AlphaColor getReducedColor() {
        double k = 0.5;
        //    return new Color( (int)(r*k), (int)(g*k), (int)(b*k), a );
        return new AlphaColor( r, g, b, (int)(a*k) );
    }

    /**
     * Returns color by name
     *
     * @param name   name of the color<BR>
     *               see static initalizer
     * @return color or null
     */
    public static AlphaColor getColor( String name ) {
        return (AlphaColor) colors.get(name.toLowerCase());
    }

    public static Enumeration getAllColors() {
        return colors.elements();
    }

    // static stuff
    private static final Hashtable colors = new Hashtable();

    private static void add( String name, int rgb ) {
        AlphaColor c = new AlphaColor();
        c.setRGB(rgb);
        c.setAlpha(255);
        colors.put(name.toLowerCase(), c);
    }

    static {
        add("black",0x000000);
        add("navy",0x000080);
        add("blue",0x0000FF);
        add("green",0x008000);
        add("dark cyan",0x008B8B);
        add("dark turquoise",0x00CED1);
        add("lime",0x00FF00);
        add("aqua",0x00FFFF);
        add("midnight blue",0x191970);
        add("light seagreen",0x20B2AA);
        add("forest green",0x228B22);
        add("lime green",0x32CD32);
        add("turquoise",0x40E0D0);
        add("dark slateblue",0x4682B4);
        add("medium turquoise",0x48D1CC);
        add("cadet blue",0x5F9EA0);
        add("medium aquamarine",0x66CDAA);
        add("slate blue",0x6A5ACD);
        add("slate gray",0x708090);
        add("medium slateblue",0x7B68EE);
        add("chartreuse",0x7FFF00);
        add("maroon",0x800000);
        add("olive",0x80800);
        add("sky blue",0x87CEEB);
        add("dark red",0x8B0000);
        add("saddle brown",0x8B4513);
        add("light green",0x90EE90);
        add("dark violet",0x9400D3);
        add("dark orchid",0x9932CC);
        add("sienna",0xA0522D);
        add("dark gray",0xA9A9A9);
        add("green yellow",0xADFF2F);
        add("pale turquoise",0xAFEEEE);
        add("firebrick",0xB22222);
        add("dark goldenrod",0xB8860B);
        add("medium orchid",0xBA55D);
        add("medium violetred",0xC71585);
        add("indian red",0xCD5C5C);
        add("peru",0xCD853F);
        add("thistle",0xD8BFD8);
        add("orchid",0xDA70D6);
        add("goldenrod",0xDAA520);
        add("pale violetred",0xDB7093);
        add("crimson",0xDC143C);
        add("dark salmon",0xE9967A);
        add("violet",0xEE82EE);
        add("pale goldenrod",0xEEE8AA);
        add("light coral",0xF08080);
        add("sandy brown",0xF4A460);
        add("wheat",0xF5DEB3);
        add("ghost white",0xF8F8FF);
        add("salmon",0xFA8072);
        add("antique white",0xFAEBD7);
        add("red",0xFF0000);
        add("fuchsia",0xFF00FF);
        add("magenta",0xFF00FF);
        add("hot pink",0xFF69B4);
        add("coral",0xFF7F50);
        add("dark orange",0xFF8C00);
        add("light salmon",0xFFA07A);
        add("orange",0xFFA500);
        add("light pink",0xFFB6C1);
        add("pink",0xFFC0CB);
        add("papaya whip",0xFFEFD5);
        add("lavender blush",0xFFF0F5);
        add("seashel",0xFFF5EE);
        add("snow",0xFFFAFA);
        add("yellow",0xFFFF00);
        add("white",0xFFFFFF);
        add("dark blue",0x00008B);
        add("medium blue",0x0000CD);
        add("dark green",0x006400);
        add("teal",0x008080);
        add("deep skyblue",0x00BFFF);
        add("medium springgreen",0x00FA9A);
        add("spring green",0x00FF7F);
        add("cyan",0x00FFFF);
        add("dodger blue",0x1E90FF);
        add("sea green",0x2E8B57);
        add("dark slategray",0x2F4F4F);
        add("medium seagreen",0x3CB371);
        add("royal blue",0x4169E1);
        add("steel blue",0x4682B4);
        add("indigo",0x4B0082);
        add("cornflower blue",0x6495ED);
        add("dim gray",0x696969);
        add("olive drab",0x6B8E23);
        add("light slategray",0x778899);
        add("lawn green",0x7CFC00);
        add("aquamarine",0x7FFFD4);
        add("purple",0x800080);
        add("gray",0x808080);
        add("blue violet",0x87CEFA);
        add("dark magenta",0x8B008B);
        add("dark seagreen",0x8FBC8F);
        add("medium purple",0x9370DB);
        add("pale green",0x98FB98);
        add("yellow green",0x9ACD32);
        add("brown",0xA52A2A);
        add("light blue",0xADD8E6);
        add("light steelblue",0xB0C4DE);
        add("powder blue",0xB0E0E6);
        add("rosy brown",0xBC8F8F);
        add("dark khaki",0xBC8F8F);
        add("silver",0xC0C0C0);
        add("chocolate",0xD2691E);
        add("tan",0xD2B48C);
        add("light grey",0xD3D3D3);
        add("gainsboro",0xDCDCDC);
        add("plum",0xDDA0DD);
        add("burlywood",0xDEB887);
        add("light cyan",0xE0FFFF);
        add("lavender",0xE6E6FA);
        add("khaki",0xF0E68C);
        add("alice blue",0xF0F8FF);
        add("honeydew",0xF0FFF0);
        add("azure",0xF0FFFF);
        add("white smoke",0xF5F5F5);
        add("mint cream",0xF5FFFA);
        add("linen",0xFAF0E6);
        add("light goldenrodyellow",0xFAFAD2);
        add("old lace",0xFDF5E6);
        add("deep pink",0xFF1493);
        add("orange red",0xFF4500);
        add("tomato",0xFF6347);
        add("gold",0xFFD700);
        add("peach puff",0xFFDAB9);
        add("navajo white",0xFFDEAD);
        add("moccasin",0xFFE4B5);
        add("bisque",0xFFE4C4);
        add("misty rose",0xFFE4E1);
        add("blanched almond",0xFFEBCD);
        add("cornsilk",0xFFF8DC);
        add("lemon chiffon",0xFFFACD);
        add("floral white",0xFFFAF0);
        add("light yellow",0xFFFFE0);
        add("ivory",0xFFFFF0);
    }

}
