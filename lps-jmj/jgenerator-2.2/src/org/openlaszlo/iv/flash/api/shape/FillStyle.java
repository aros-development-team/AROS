/*
 * $Id: FillStyle.java,v 1.3 2002/03/02 01:39:44 awason Exp $
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

package org.openlaszlo.iv.flash.api.shape;

import java.awt.geom.*;
import java.io.PrintStream;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.image.*;

/**
 * FillStyle of Shape.
 * <P>
 * SWF supports three basic types of fills for a shape.
 * <UL>
 * <LI>Solid fill - A simple RGBA color that fills a portion of a shape.
 *     An alpha value of 255 means a completely opaque fill.
 *     An alpha value of zero means a completely transparent fill.
 *     Any alpha between 0 and 255 will be partially transparent.
 * <LI>Gradient Fill - A gradient fill can be either a linear or a radial gradient.
 *     See {@link org.openlaszlo.iv.flash.api.Gradient}s for an in depth description of how gradients are defined.
 * <LI>Bitmap fill - Bitmap fills refer to a bitmap characterId.
 *     There are two styles: clipped and tiled. A clipped bitmap fill repeats the color on
 *     the edge of a bitmap if the fill extends beyond the edge of the bitmap.
 *     A tiled fill repeats the bitmap if the fill extends beyond the edge of the bitmap.
 * </UL>
 *
 * @author Dmitry Skavish
 */
public final class FillStyle extends FlashItem {

    public static final int SOLID           = 0x00;
    public static final int LINEAR_GRADIENT = 0x10;
    public static final int RADIAL_GRADIENT = 0x12;
    public static final int TILED_BITMAP    = 0x40;
    public static final int CLIPPED_BITMAP  = 0x41;

    private int type;               // type of this fillstyle
    private Color color;            // color of this fillstyle
    private AffineTransform matrix; // matrix of this fillstyle
    private Gradient gradient;      // gradient of this fillstyle
    private Bitmap bitmap;          // bitmap of this fillstyle

    public FillStyle() {}

    public int getType() {
        return type;
    }

    public void setType( int type ) {
        this.type = type;
    }

    public Color getColor() {
        return color;
    }

    public void setColor( Color color ) {
        this.color = color;
    }

    public AffineTransform getMatrix() {
        return matrix;
    }

    public void setMatrix( AffineTransform matrix ) {
        this.matrix = matrix;
    }

    public Gradient getGraduent() {
        return gradient;
    }

    public void setGradient( Gradient gradient ) {
        this.gradient = gradient;
    }

    public Bitmap getBitmap() {
        return bitmap;
    }

    public void setBitmap( Bitmap bitmap ) {
        this.bitmap = bitmap;
    }

    /**
     * Creates new solid fill.
     *
     * @param color  color of solid fill
     * @return solid fillstyle
     */
    public static FillStyle newSolid( Color color ) {
        FillStyle fs = new FillStyle();
        fs.setColor( color );
        fs.setType( SOLID );
        return fs;
    }

    /**
     * Creates new linear gradient fill.
     *
     * @param gradient gradient of gradient fill
     * @param matrix   matrix of gradient fill
     * @return linear gradient fill
     */
    public static FillStyle newLinearGradient( Gradient gradient, AffineTransform matrix ) {
        FillStyle fs = new FillStyle();
        fs.setGradient( gradient );
        fs.setMatrix( matrix );
        fs.setType( LINEAR_GRADIENT );
        return fs;
    }

    /**
     * Creates new radial gradient fill.
     *
     * @param gradient gradient of gradient fill
     * @param matrix   matrix of gradient fill
     * @return radial gradient fill
     */
    public static FillStyle newRadialGradient( Gradient gradient, AffineTransform matrix ) {
        FillStyle fs = new FillStyle();
        fs.setGradient( gradient );
        fs.setMatrix( matrix );
        fs.setType( RADIAL_GRADIENT );
        return fs;
    }

    /**
     * Creates new tiled bitmap fill.
     *
     * @param bitmap bitmap of bitmap fill
     * @param matrix matrix of bitmap fill
     * @return tiled bitmap fill
     */
    public static FillStyle newTiledBitmap( Bitmap bitmap, AffineTransform matrix ) {
        FillStyle fs = new FillStyle();
        fs.setBitmap( bitmap );
        fs.setMatrix( matrix );
        fs.setType( TILED_BITMAP );
        return fs;
    }

    /**
     * Creates new clipped bitmap fill.
     *
     * @param bitmap bitmap of bitmap fill
     * @param matrix matrix of bitmap fill
     * @return clipped bitmap fill
     */
    public static FillStyle newClippedBitmap( Bitmap bitmap, AffineTransform matrix ) {
        FillStyle fs = new FillStyle();
        fs.setBitmap( bitmap );
        fs.setMatrix( matrix );
        fs.setType( CLIPPED_BITMAP );
        return fs;
    }

    /**
     * Creates new tiled bitmap fill with default tranformation matrix.
     *
     * @param bitmap bitmap of bitmap fill
     * @return tiled bitmap fill
     */
    public static FillStyle newTiledBitmap( Bitmap bitmap ) {
        AffineTransform matrix = AffineTransform.getScaleInstance(20.0, 20.0);
        return newTiledBitmap( bitmap, matrix );
    }

    /**
     * Creates new clipped bitmap fill with default tranformation matrix.
     *
     * @param bitmap bitmap of bitmap fill
     * @return clipped bitmap fill
     */
    public static FillStyle newClippedBitmap( Bitmap bitmap ) {
        AffineTransform matrix = AffineTransform.getScaleInstance(20.0, 20.0);
        return newClippedBitmap( bitmap, matrix );
    }

    public static FillStyle parse( Parser p, boolean withAlpha ) {
        FillStyle fs = new FillStyle();
        int type = fs.type = p.getUByte();

        if( (type&0x10) != 0 ) { // gradient

            // Get the gradient matrix.
            fs.matrix = p.getMatrix();
            // get gradient itself
            fs.gradient = Gradient.parse(p, withAlpha);

        } else if( (type&0x40) != 0 ) { // bitmap

            int id = p.getUWord();  // id may be equal to 0xffff, I don't know what it means
            fs.bitmap = (Bitmap) p.getDef(id);
            fs.matrix = p.getMatrix();

        } else { // A solid color

            fs.color = Color.parse(p, withAlpha);
        }

        return fs;
    }

    public void write( FlashOutput fob ) {
        fob.writeByte(type);
        switch( type ) {
            case SOLID:
                Shape shape = (Shape) fob.getUserData();
                if( shape.isWithAlpha() ) {
                    color.writeRGBA(fob);
                } else {
                    color.writeRGB(fob);
                }
                break;
            case LINEAR_GRADIENT:
            case RADIAL_GRADIENT:
                fob.write(matrix);
                gradient.write(fob);
                break;
            case TILED_BITMAP:
            case CLIPPED_BITMAP:
                if( bitmap == null ) {
                    fob.writeWord(0xffff);
                } else {
                    fob.writeDefID(bitmap);
                }
                fob.write(matrix);
                break;
        }
    }

    public void printContent( PrintStream out, String indent ) {
        switch( type ) {
            case SOLID:
                out.println( indent+"FillStyle (SOLID):" );
                color.printContent(out, indent+"    ");
                break;
            case LINEAR_GRADIENT:
                out.println( indent+"FillStyle (LINEAR_GRADIENT):" );
                out.println( indent+"    "+matrix );
                gradient.printContent(out, indent+"    ");
                break;
            case RADIAL_GRADIENT:
                out.println( indent+"FillStyle (RADIAL_GRADIENT):" );
                out.println( indent+"    "+matrix );
                gradient.printContent(out, indent+"    ");
                break;
            case TILED_BITMAP:
                out.println( indent+"FillStyle (TILED_BITMAP):" );
                out.println( indent+"    "+matrix );
                out.println( indent+"    bitmapID="+ (bitmap == null ? 0xffff : bitmap.getID()) );
                break;
            case CLIPPED_BITMAP:
                out.println( indent+"FillStyle (CLIPPED_BITMAP):" );
                out.println( indent+"    "+matrix );
                out.println( indent+"    bitmapID="+ (bitmap == null ? 0xffff : bitmap.getID()) );
                break;
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        ((FillStyle)item).type = type;
        switch( type ) {
            case SOLID:
                ((FillStyle)item).color = (Color) color.getCopy(copier);
                break;
            case LINEAR_GRADIENT:
            case RADIAL_GRADIENT:
                ((FillStyle)item).gradient = (Gradient) gradient.getCopy(copier);
                ((FillStyle)item).matrix = (AffineTransform) matrix.clone();
                break;
            case TILED_BITMAP:
            case CLIPPED_BITMAP:
                ((FillStyle)item).bitmap = (Bitmap) copier.copy(bitmap);
                ((FillStyle)item).matrix = (AffineTransform) matrix.clone();
                break;
        }
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new FillStyle(), copier );
    }
}
