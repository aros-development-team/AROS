/*
 * $Id: LazyMorphShape.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;

public class LazyMorphShape extends LazyShape {

    public Rectangle2D endBounds;

    public LazyMorphShape() {}

    private static void extractBitmaps( Parser p, LazyMorphShape shape ) {
        int pos = p.getPos();

        p.skip(4);    // skip offset

        // Get the number of fills.
        int nFills = p.getUByte();
        if( nFills == 255 ) {
            nFills = p.getUWord();
        }

        // Get each of the fill style.
        for( int i=0; i<nFills; i++ ) {
            int fillStyle = p.getUByte();
            if( (fillStyle&0x10) != 0 ) { // gradient
                p.skipMatrix();
                p.skipMatrix();
                // Get the number of colors.
                int nColors = p.getUByte();
                // Get each of the colors.
                for( int j=0; j<nColors; j++ ) {
                    p.skip(1); // ratio
                    AlphaColor.skip(p);
                    p.skip(1); // ratio
                    AlphaColor.skip(p);
                }
            } else if( (fillStyle&0x40) != 0 ) { // bitmap
                int id_pos = p.getPos()-pos;
                int id = p.getUWord();
                FlashDef def = p.getDef(id);
                //System.out.println( "MORPHSHAPE: bitmap found: ID="+id+", def="+def );
                shape.addBitmap(def, id_pos);
                p.skipMatrix();
                p.skipMatrix();
            } else { // A solid color
                AlphaColor.skip(p);
                AlphaColor.skip(p);
            }
        }
    }

    public static FlashDef parse( Parser p ) {
        FlashFile file = p.getFile();
        if( file.isFullParsing() ) {
            MorphShape shape = MorphShape.parse(p);
            return shape;
        } else {
            LazyMorphShape shape = new LazyMorphShape();
            shape.tagCode = p.getTagCode();
            shape.setID( p.getUWord() );
            shape.bounds = p.getRect();
            shape.endBounds = p.getRect();
            // skip the data
            shape.data = new DataMarker( p.getBuf(), p.getPos(), p.getTagEndPos() );
            extractBitmaps( p, shape );
            return shape;
        }
    }

    public void write( FlashOutput fob ) {
        fob.writeTag( tagCode,
            2 +
            GeomHelper.getSize(bounds) +
            GeomHelper.getSize(endBounds) +
            data.length()
        );
        fob.writeDefID( this );
        fob.write(bounds);
        fob.write(endBounds);
        int pos = fob.getPos();
        data.write( fob );
        if( bitmaps != null ) {
            for( int i=0; i<bitmaps.size(); i++ ) {
                BitmapRef ref = (BitmapRef) bitmaps.elementAt(i);
                fob.writeWordAt( fob.getDefID(ref.bitmap), pos+ref.offset );
            }
        }
    }

    public boolean isConstant() {
        return true;
    }

    public Rectangle2D getBounds() {
        Rectangle2D dst = GeomHelper.newRectangle();
        Rectangle2D.union( bounds, endBounds, dst );
        return dst;
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"LazyMorphShape: id="+getID()+", name='"+getName()+"'" );
        out.println( indent+"   "+bounds );
        out.println( indent+"   "+endBounds );
        if( bitmaps != null ) {
            out.print( indent+"    bitmaps used: " );
            for( int i=0; i<bitmaps.size(); i++ ) {
                BitmapRef ref = (BitmapRef) bitmaps.elementAt(i);
                out.print( "id["+i+"]="+ref.bitmap.getID()+" " );
            }
            out.println();
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((LazyMorphShape)item).endBounds = (Rectangle2D) endBounds.clone();
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new LazyMorphShape(), copier );
    }
}
