/*
 * $Id: MorphFillStyle.java,v 1.1 2002/02/15 23:46:26 skavish Exp $
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
 * FillStyle of MorphShape.
 *
 * @author Dmitry Skavish
 * @see FillStyle
 */
public final class MorphFillStyle extends FlashItem {

    private int type;                       // type of this fillstyle
    private Color color_start;              // start color of this fillstyle
    private Color color_end;                // end color of this fillstyle
    private AffineTransform matrix_start;   // start matrix of this fillstyle
    private AffineTransform matrix_end;     // end matrix of this fillstyle
    private MorphGradient gradient;         // gradient of this fillstyle
    private Bitmap bitmap;                  // bitmap of this fillstyle

    public MorphFillStyle() {}

    public int getType() {
        return type;
    }

    public void setType( int type ) {
        this.type = type;
    }

    public Color getColorStart() {
        return color_start;
    }

    public void setColorStart( Color color ) {
        this.color_start = color;
    }

    public Color getColorEnd() {
        return color_end;
    }

    public void setColorEnd( Color color ) {
        this.color_end = color;
    }

    public AffineTransform getMatrixStart() {
        return matrix_start;
    }

    public void setMatrixStart( AffineTransform matrix ) {
        this.matrix_start = matrix;
    }

    public AffineTransform getMatrixEnd() {
        return matrix_end;
    }

    public void setMatrixEnd( AffineTransform matrix ) {
        this.matrix_end = matrix;
    }

    public MorphGradient getGraduent() {
        return gradient;
    }

    public void setGradient( MorphGradient gradient ) {
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
     * @param color_start start color
     * @param color_end end color
     * @return solid fillstyle
     */
    public static MorphFillStyle newSolid( Color color_start, Color color_end ) {
        MorphFillStyle fs = new MorphFillStyle();
        fs.setColorStart( color_start );
        fs.setColorEnd( color_end );
        fs.setType( FillStyle.SOLID );
        return fs;
    }

    /**
     * Creates new linear gradient fill.
     *
     * @param gradient   gradient of gradient fill
     * @param matrix_start start matrix
     * @param matrix_end end matrix
     * @return linear gradient fill
     */
    public static MorphFillStyle newLinearGradient(
        MorphGradient gradient,
        AffineTransform matrix_start,
        AffineTransform matrix_end )
    {
        MorphFillStyle fs = new MorphFillStyle();
        fs.setGradient( gradient );
        fs.setMatrixStart( matrix_start );
        fs.setMatrixEnd( matrix_end );
        fs.setType( FillStyle.LINEAR_GRADIENT );
        return fs;
    }

    /**
     * Creates new radial gradient fill.
     *
     * @param gradient gradient of gradient fill
     * @param matrix_start start matrix
     * @param matrix_end end matrix
     * @return radial gradient fill
     */
    public static MorphFillStyle newRadialGradient(
        MorphGradient gradient,
        AffineTransform matrix_start,
        AffineTransform matrix_end )
    {
        MorphFillStyle fs = new MorphFillStyle();
        fs.setGradient( gradient );
        fs.setMatrixStart( matrix_start );
        fs.setMatrixEnd( matrix_end );
        fs.setType( FillStyle.RADIAL_GRADIENT );
        return fs;
    }

    /**
     * Creates new tiled bitmap fill.
     *
     * @param bitmap bitmap of bitmap fill
     * @param matrix_start start matrix
     * @param matrix_end end matrix
     * @return tiled bitmap fill
     */
    public static MorphFillStyle newTiledBitmap(
        Bitmap bitmap,
        AffineTransform matrix_start,
        AffineTransform matrix_end )
    {
        MorphFillStyle fs = new MorphFillStyle();
        fs.setBitmap( bitmap );
        fs.setMatrixStart( matrix_start );
        fs.setMatrixEnd( matrix_end );
        fs.setType( FillStyle.TILED_BITMAP );
        return fs;
    }

    /**
     * Creates new clipped bitmap fill.
     *
     * @param bitmap bitmap of bitmap fill
     * @param matrix_start start matrix
     * @param matrix_end end matrix
     * @return clipped bitmap fill
     */
    public static MorphFillStyle newClippedBitmap(
        Bitmap bitmap,
        AffineTransform matrix_start,
        AffineTransform matrix_end )
    {
        MorphFillStyle fs = new MorphFillStyle();
        fs.setBitmap( bitmap );
        fs.setMatrixStart( matrix_start );
        fs.setMatrixEnd( matrix_end );
        fs.setType( FillStyle.CLIPPED_BITMAP );
        return fs;
    }

    /**
     * Parses morph fill style
     *
     * @param p      parser
     * @return parsed fillstyle
     */
    public static MorphFillStyle parse( Parser p ) {
        MorphFillStyle fs = new MorphFillStyle();
        int type = fs.type = p.getUByte();

        if( (type&0x10) != 0 ) { // gradient

            // Get the gradient matrix.
            fs.matrix_start = p.getMatrix();
            fs.matrix_end = p.getMatrix();
            // get gradient itself
            fs.gradient = MorphGradient.parse(p);

        } else if( (type&0x40) != 0 ) { // bitmap

            int id = p.getUWord();  // id may be equal to 0xffff, I don't know what it means
            fs.bitmap = (Bitmap) p.getDef(id);
            fs.matrix_start = p.getMatrix();
            fs.matrix_end = p.getMatrix();

        } else { // A solid color

            fs.color_start = AlphaColor.parse(p);
            fs.color_end = AlphaColor.parse(p);
        }

        return fs;
    }

    /**
     * Writes fillstyle to flash buffer
     *
     * @param fob    buffer to write
     */
    public void write( FlashOutput fob ) {
        fob.writeByte(type);
        switch( type ) {
            case FillStyle.SOLID:
                color_start.writeRGBA(fob);
                color_end.writeRGBA(fob);
                break;
            case FillStyle.LINEAR_GRADIENT:
            case FillStyle.RADIAL_GRADIENT:
                fob.write(matrix_start);
                fob.write(matrix_end);
                gradient.write(fob);
                break;
            case FillStyle.TILED_BITMAP:
            case FillStyle.CLIPPED_BITMAP:
                if( bitmap == null ) {
                    fob.writeWord(0xffff);
                } else {
                    fob.writeDefID(bitmap);
                }
                fob.write(matrix_start);
                fob.write(matrix_end);
                break;
        }
    }

    public void printContent( PrintStream out, String indent ) {
        switch( type ) {
            case FillStyle.SOLID:
                out.println( indent+"MorphFillStyle (SOLID):" );
                color_start.printContent(out, indent+"       ");
                color_end.printContent(out, indent+"       ");
                break;
            case FillStyle.LINEAR_GRADIENT:
                out.println( indent+"MorphFillStyle (LINEAR_GRADIENT):" );
                out.println( indent+"       "+matrix_start );
                out.println( indent+"       "+matrix_end );
                gradient.printContent(out, indent+"       ");
                break;
            case FillStyle.RADIAL_GRADIENT:
                out.println( indent+"FillStyle (RADIAL_GRADIENT):" );
                out.println( indent+"       "+matrix_start );
                out.println( indent+"       "+matrix_end );
                gradient.printContent(out, indent+"       ");
                break;
            case FillStyle.TILED_BITMAP:
                out.println( indent+"FillStyle (TILED_BITMAP):" );
                out.println( indent+"       "+matrix_start );
                out.println( indent+"       "+matrix_end );
                out.println( indent+"    bitmapID="+bitmap.getID() );
                break;
            case FillStyle.CLIPPED_BITMAP:
                out.println( indent+"FillStyle (CLIPPED_BITMAP):" );
                out.println( indent+"       "+matrix_start );
                out.println( indent+"       "+matrix_end );
                out.println( indent+"    bitmapID="+bitmap.getID() );
                break;
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        ((MorphFillStyle)item).type = type;
        switch( type ) {
            case FillStyle.SOLID:
                ((MorphFillStyle)item).color_start = (Color) color_start.getCopy(copier);
                ((MorphFillStyle)item).color_end = (Color) color_end.getCopy(copier);
                break;
            case FillStyle.LINEAR_GRADIENT:
            case FillStyle.RADIAL_GRADIENT:
                ((MorphFillStyle)item).gradient = (MorphGradient) gradient.getCopy(copier);
                ((MorphFillStyle)item).matrix_start = (AffineTransform) matrix_start.clone();
                ((MorphFillStyle)item).matrix_end = (AffineTransform) matrix_end.clone();
                break;
            case FillStyle.TILED_BITMAP:
            case FillStyle.CLIPPED_BITMAP:
                ((MorphFillStyle)item).bitmap = (Bitmap) copier.copy(bitmap);
                ((MorphFillStyle)item).matrix_start = (AffineTransform) matrix_start.clone();
                ((MorphFillStyle)item).matrix_end = (AffineTransform) matrix_end.clone();
                break;
        }
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new MorphFillStyle(), copier );
    }
}

