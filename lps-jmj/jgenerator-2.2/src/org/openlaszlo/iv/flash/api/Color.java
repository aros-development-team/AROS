/*
 * $Id: Color.java,v 1.2 2002/02/15 23:44:27 skavish Exp $
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
 * Opaque RGB Color without alpha value
 *
 * @author Dmitry Skavish
 * @see AlphaColor
 */
public class Color extends FlashItem {

    protected int r;
    protected int g;
    protected int b;

    public Color() {}

    /**
     * Creates color from int
     *
     * @param rgb    color encoded as folows:  red<<16 | green<<8 | blue
     */
    public Color( int rgb ) {
        setRGB( rgb );
    }

    /**
     * Creates Color from three integer
     * components each on range 0, 255
     *
     * @param r      red
     * @param g      green
     * @param b      blue
     */
    public Color( int r, int g, int b ) {
        this.r = r;
        this.g = g;
        this.b = b;
    }

    /**
     * Creates Color from three float
     * components each on range 0.0, 1.0
     *
     * @param r      red
     * @param g      green
     * @param b      blue
     */
    public Color( float r, float g, float b ) {
        this.r = Math.round( r * 255 );
        this.g = Math.round( g * 255 );
        this.b = Math.round( b * 255 );
    }

    public int getRed() {
        return r;
    }

    public int getGreen() {
        return g;
    }

    public int getBlue() {
        return b;
    }

    public int getAlpha() {
        return 255;
    }

    /**
     * Returns color encoded as int
     *
     * @return color encoded as follows: red<<16 | green<<8 | blue
     */
    public int getRGB() {
        return r<<16 | g<<8 | b;
    }

    public void setRed( int r ) {
        this.r = r;
    }
    public void setGreen( int g ) {
        this.g = g;
    }
    public void setBlue( int b ) {
        this.b = b;
    }

    /**
     * Sets color encoded as int
     *
     * @param rgb   color encoded as folows:  red<<16 | green<<8 | blue
     */
    public void setRGB( int rgb ) {
        r = (rgb&0x00ff0000)>>16;
        g = (rgb&0x0000ff00)>>8;
        b = (rgb&0x000000ff);
    }

    /**
     * Returns true or false depending on whether this object has alpha or not
     */
    public boolean hasAlpha() { return false; }

    /**
     * Returns new color with changed brightness
     *
     * @param f      brightness change:
     *               <UL>
     *               <LI>[-1..0] - makes it darker
     *               <LI>[0..1] - makes it brighter
     *               </UL>
     * @return new color
     */
    public Color brighter( double f ) {
        return CXForm.newBrightness(f, hasAlpha()).transform(this);
    }

    /**
     * Returns java.awt.Color corresponding to this one
     */
    public java.awt.Color getAWTColor() {
        java.awt.Color color;
        if( hasAlpha() ) {
            color = new java.awt.Color(r, g, b, getAlpha());
        } else {
            color = new java.awt.Color(r, g, b);
        }
        return color;
    }

    public void write( FlashOutput fob ) {
        writeRGB( fob );
    }

    /**
     * Writes RGB color to FlashBuffer
     *
     * @param fob    flash buffer to write
     */
    public void writeRGB( FlashOutput fob ) {
        fob.writeByte( r );
        fob.writeByte( g );
        fob.writeByte( b );
    }

    /**
     * Writes RGBA color to FlashBuffer<BR>
     * Alpha is 255, i.e. color is opaque
     *
     * @param fob    flash buffer to write
     */
    public void writeRGBA( FlashOutput fob ) {
        fob.writeByte( r );
        fob.writeByte( g );
        fob.writeByte( b );
        fob.writeByte( 255 );
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"Color: (0x"+Util.b2h(r)+",0x"+Util.b2h(g)+",0x"+Util.b2h(b)+")" );
    }

    /**
     * Parses color as RGB
     *
     * @param p      Parser to parse from
     * @return always Color
     */
    public static Color parseRGB( Parser p ) {
        return new Color(p.getUByte(),p.getUByte(),p.getUByte());
    }

    /**
     * Parses color as RGBA
     *
     * @param p      Parser to parse from
     * @return always AlphaColor
     */
    public static AlphaColor parseRGBA( Parser p ) {
        return new AlphaColor(p.getUByte(),p.getUByte(),p.getUByte(),p.getUByte());
    }

    /**
     * Parses color
     *
     * @param p         parser to parse from
     * @param withAlpha if true then parse as AlphaColor<BR>
     *                  if false then parse as Color
     * @return Color or AlphaColor depending on parameter withAlpha
     */
    public static Color parse( Parser p, boolean withAlpha ) {
        if( withAlpha ) return parseRGBA(p);
        return parseRGB(p);
    }

    public static void skip( Parser p ) {
        p.skip(3);
    }

    public static void skip( Parser p, boolean withAlpha ) {
        if( withAlpha ) p.skip(4);
        else p.skip(3);
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((Color)item).r = r;
        ((Color)item).g = g;
        ((Color)item).b = b;
        return item;
    }
    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new Color(), copier );
    }

    public boolean equals( Object o ) {
        if( o instanceof Color ) {
            Color c = (Color) o;
            return r == c.r && g == c.g && b == c.b;
        }
        return false;
    }

}
