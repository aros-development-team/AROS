/*
 * $Id: Bitmap.java,v 1.4 2002/06/24 13:53:36 awason Exp $
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

package org.openlaszlo.iv.flash.api.image;

import java.io.*;
import java.awt.geom.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.shape.*;

/**
 * Abstract class which represent image
 */
public abstract class Bitmap extends FlashDef {

    protected int tagCode;

    public int getTag() {
        return tagCode;
    }

    public void setTag( int tagCode ) {
        this.tagCode = tagCode;
    }

    public boolean isConstant() {
        return true;
    }

    /**
     * Return width of the bitmap in pixels
     */
    public int getWidth() { return (int) getBounds().getWidth(); }

    /**
     * Return height of the bitmap in pixels
     */
    public int getHeight() { return (int) getBounds().getHeight(); }

    /**
     * Return size of the image in bytes, which it's occupied in memory
     */
    public abstract int getSize();

    /**
     * Create instance of this bitmap<br>
     * No scaling, no translating
     *
     * @return newly created instance (can be added to a frame)
     */
    public Instance newInstance() {
        return newInstance( 0, 0, false, false );
    }

    /**
     * Create instance of this bitmap
     *
     * @param width if scale is true it's width of target image in twixels (there is no resampling, just transformation)
     * @param height if scale is true it's height of target image in twixels (there is no resampling, just transformation)
     * @param scale if true then transform (scale) image to fit width and height (there is no resampling, just transformation)
     * @param center if true then transform (translate) image so that its origin in the center
     * @return newly created instance (can be added to a frame)
     */
    public Instance newInstance( int width, int height, boolean scale, boolean center ) {

        int bmWidth = getWidth()*20;
        int bmHeight = getHeight()*20;

        Shape shape = new Shape();
        shape.setFillStyle0( 0 );

        // Translate by -1/2 pixel (10 twips) to avoid Flash rendering
        // with top/left pixels duplicated and bottom right pixels truncated.
        AffineTransform fillMatrix = AffineTransform.getTranslateInstance(-10, -10);
        // Scale from twips to pixels
        fillMatrix.scale(20, 20);
        shape.setFillStyle1( FillStyle.newClippedBitmap(this, fillMatrix) );

        shape.drawRectangle(0, 0, bmWidth, bmHeight);
        shape.setBounds( GeomHelper.newRectangle(0, 0, bmWidth, bmHeight) );

        AffineTransform myMatrix;
        if( scale ) {
            myMatrix = AffineTransform.getScaleInstance( (double)width/bmWidth, (double)height/bmHeight );
        } else {
            myMatrix = center? new AffineTransform(): null;
        }
        if( center ) {
            myMatrix.translate( -bmWidth/2, -bmHeight/2 );
        }

        Instance inst = new Instance();
        inst.def = shape;
        inst.matrix = myMatrix;

        return inst;
    }

    public static Bitmap newBitmap( String fileName )
        throws IVException, IOException
    {
        return newBitmap( IVUrl.newUrl(fileName) );
    }

    public static Bitmap newBitmap( IVUrl url )
        throws IVException, IOException
    {
        Bitmap bitmap = newBitmap( Util.readUrl(url) );
        if( bitmap != null ) return bitmap;
        throw new IVException( Resource.UNSUPMEDIA, new Object[] {url.getName()} );
    }

    /**
     * Create bitmap from buffer
     *
     */
    public static Bitmap newBitmap( FlashBuffer fb ) throws IVException {
        // check the type of the bitmap
        fb.setPos(0);
        int b0 = fb.getUByte();
        int b1 = fb.getUByte();
        int b2 = fb.getUByte();
        int b3 = fb.getUByte();

        if( b0 == 0xff && b1 == 0xd8 ) {  // jpeg: 0xff, 0xd8
            return JPEGBitmap.newJPEGBitmap(fb);
        } else if( b0 == 'G' && b1 == 'I' && b2 == 'F' ) {  // gif: 'GIF'
            return LLBitmap.newGIFBitmap(fb);
        } else if( b0 == 0x89 && b1 == 0x50 && b2 == 0x4e && b3 == 0x47 ) { // png
            return LLBitmap.newPNGBitmap(fb);
        } else {
            return null;
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((Bitmap)item).tagCode = tagCode;
        return item;
    }

}

