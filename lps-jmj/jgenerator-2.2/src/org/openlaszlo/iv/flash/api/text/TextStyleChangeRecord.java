/*
 * $Id: TextStyleChangeRecord.java,v 1.3 2002/07/15 22:39:32 skavish Exp $
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

package org.openlaszlo.iv.flash.api.text;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;
import java.io.*;
import java.util.*;

/**
 * TextStyleChangeRecord.
 * <p>
 * Changes the text style or/and position of the following text (TextRecord)
 *
 * @author Dmitry Skavish
 * @see TextRecord
 */
public final class TextStyleChangeRecord extends FlashItem {

    private Font font = null;                   // font of the following text
    private Color color = null;                 // color of the following text
    private int xOffset = Integer.MAX_VALUE;    // x of the following text
    private int yOffset = Integer.MAX_VALUE;    // y of the following text
    private int height;                         // font height

    /**
     * Creates empty change record
     */
    public TextStyleChangeRecord() {}

    /**
     * Creates change record of font and color
     *
     * @param font   new font
     * @param height height of the new font
     * @param color  new color
     */
    public TextStyleChangeRecord( Font font, int height, Color color ) {
        this.font = font;
        this.height = height;
        this.color = color;
    }

    /**
     * Creates change record of X and Y
     *
     * @param x      new X (in twixels)
     * @param y      new Y (in twixels)
     */
    public TextStyleChangeRecord( int x, int y ) {
        this.xOffset = x;
        this.yOffset = y;
    }

    public void setFont( Font font ) { this.font = font; }
    public void setColor( Color color ) { this.color = color; }
    public void setX( int x ) { this.xOffset = x; }
    public void setY( int y ) { this.yOffset = y; }
    public void setHeight( int h ) { this.height = h; }

    public Font getFont() { return font; }
    public Color getColor() { return color; }
    public int getX() { return xOffset; }
    public int getY() { return yOffset; }
    public int getHeight() { return height; }

    /**
     * Merges this record into specified one
     * <P>
     * Attributes of the specified one have bigger priority
     *
     * @param ts     specified record
     */
    public void mergeTo( TextStyleChangeRecord ts ) {
        if( ts.getFont() == null ) {
            ts.setFont( font );
            ts.setHeight( height );
        }
        if( ts.getColor() == null ) ts.setColor( color );
        if( ts.getX() == Integer.MAX_VALUE ) ts.setX( xOffset );
        if( ts.getY() == Integer.MAX_VALUE ) ts.setY( yOffset );
    }

    public void write( FlashOutput fob ) {
        // write control record
        fob.initBits();
        fob.writeBits( 0x08, 4 );
        fob.writeBool( font != null );
        fob.writeBool( color != null );
        fob.writeBool( yOffset != Integer.MAX_VALUE );
        fob.writeBool( xOffset != Integer.MAX_VALUE );
        fob.flushBits();
        if( font != null ) fob.writeFontID( font );
        if( color != null ) color.write(fob);
        if( xOffset != Integer.MAX_VALUE ) fob.writeWord(xOffset);
        if( yOffset != Integer.MAX_VALUE ) fob.writeWord(yOffset);
        if( font != null ) fob.writeWord(height);
    }

    public boolean isX() {
        return xOffset != Integer.MAX_VALUE;
    }

    public boolean isY() {
        return yOffset != Integer.MAX_VALUE;
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"TextStyleChangeRecord: " );
        if( font != null ) out.println( indent+"    font="+font.getFontName()+", height="+height );
        if( color != null ) color.printContent(out, indent+"    ");
        if( xOffset != Integer.MAX_VALUE ) out.println( indent+"    xOffset="+xOffset );
        if( yOffset != Integer.MAX_VALUE ) out.println( indent+"    yOffset="+yOffset );
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        ((TextStyleChangeRecord)item).font = font;
        ((TextStyleChangeRecord)item).color = color == null? null: (Color) color.getCopy(copier);
        ((TextStyleChangeRecord)item).xOffset = xOffset;
        ((TextStyleChangeRecord)item).yOffset = yOffset;
        ((TextStyleChangeRecord)item).height = height;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new TextStyleChangeRecord(), copier );
    }
}

