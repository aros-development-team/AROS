/*
 * $Id: LazyShape.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

public class LazyShape extends FlashDef {

    public static class BitmapRef {
        public FlashDef bitmap;
        public int offset;
        public BitmapRef( FlashDef bitmap, int offset ) {
            this.bitmap = bitmap;
            this.offset = offset;
        }
    }

    protected IVVector bitmaps;
    protected DataMarker data;
    protected Rectangle2D bounds;
    protected int tagCode;

    public LazyShape() {}

    public int getTag() {
        return tagCode;
    }

    public static FlashDef parse( Parser p ) {
        FlashFile file = p.getFile();
        if( file.isFullParsing() ) {
            Shape shape = Shape.parse(p);
            return shape;
        } else {
            LazyShape shape = new LazyShape();
            shape.tagCode = p.getTagCode();
            shape.setID( p.getUWord() );
            shape.bounds = p.getRect();
            // skip the data
            shape.data = new DataMarker( p.getBuf(), p.getPos(), p.getTagEndPos() );
            extractBitmaps( p, shape );
            return shape;
        }
    }

    public void write( FlashOutput fob ) {
        fob.writeTag( tagCode, 2+GeomHelper.getSize(bounds)+data.length() );
        fob.writeDefID( this );
        fob.write(bounds);
        int pos = fob.getPos();
        data.write( fob );
        if( bitmaps != null ) {
            for( int i=0; i<bitmaps.size(); i++ ) {
                BitmapRef ref = (BitmapRef) bitmaps.elementAt(i);
                int bitmapID;
                if( ref.bitmap == null ) {  // 02/08/01 by sd
                    bitmapID = 0xffff;
                } else {
                    bitmapID = fob.getDefID(ref.bitmap);
                }
                fob.writeWordAt( bitmapID, pos+ref.offset );
            }
        }
    }

    private static void parseShapeStyle( Parser p, LazyShape shape, boolean withAlpha, int pos ) {
        // Get the number of fills.
        int nFills = p.getUByte();
        if( nFills == 255 ) {
            nFills = p.getUWord();
        }

        // Get each of the fill style.
        for( int i=0; i<nFills; i++ ) {
            int fillStyle = p.getUByte();
            if( (fillStyle&0x10) != 0 ) { // gradient
                // Get the gradient matrix.
                p.skipMatrix();
                // Get the number of colors.
                int nColors = p.getUByte();
                // Get each of the colors.
                for( int j=0; j<nColors; j++ ) {
                    p.skip(1); // ratio
                    Color.skip(p, withAlpha);
                }
            } else if( (fillStyle&0x40) != 0 ) { // bitmap
                int id_pos = p.getPos()-pos;
                int id = p.getUWord();  // id may be equal to 0xffff, I don't know what it means
                FlashDef def = p.getDef(id);
                shape.addBitmap(def, id_pos);
                p.skipMatrix();
            } else { // A solid color
                Color.skip(p, withAlpha);
            }
        }

        int nLines = p.getUByte();
        if( nLines == 255 ) {
            nLines = p.getUWord();
        }
        // Get each of the line styles.
        for( int i=0; i<nLines; i++ ) {
            p.skip(2);   // width
            Color.skip(p, withAlpha);
        }

    }

    private static void extractBitmaps( Parser p, LazyShape shape ) {
        int pos = p.getPos();

        boolean withAlpha = shape.getTag() == Tag.DEFINESHAPE3;

        parseShapeStyle( p, shape, withAlpha, pos );

        int nBits = p.getUByte();
        int nFillBits = (nBits&0xf0)>>4;
        int nLineBits = nBits&0x0f;
        // parse shape records
        p.initBits();
        for(;;) {
            if( p.getBool() ) { // edge record
                if( p.getBool() ) { // stright edge
                    int nb = p.getBits(4)+2;
                    if( p.getBool() ) {
                        p.skipBits(nb+nb);  // general line
                    } else {
                        p.skipBits(1+nb);   // horiz or vert line
                    }

                } else { // curved edge
                    int nb = p.getBits(4)+2;
                    p.skipBits(nb*4);
                }
            } else { // non-edge record
                int flags = p.getBits(5);
                if( flags == 0 ) break; // end record
                if( (flags & StyleChangeRecord.MOVETO) != 0 ) {
                    int nMoveBits = p.getBits(5);
                    p.skipBits(nMoveBits+nMoveBits);
                }
                if( (flags & StyleChangeRecord.FILLSTYLE0) != 0 ) {
                    p.skipBits(nFillBits);
                }
                if( (flags & StyleChangeRecord.FILLSTYLE1) != 0 ) {
                    p.skipBits(nFillBits);
                }
                if( (flags & StyleChangeRecord.LINESTYLE) != 0 ) {
                    p.skipBits(nLineBits);
                }
                if( (flags & StyleChangeRecord.NEW_STYLES) != 0 ) {
                    parseShapeStyle( p, shape, withAlpha, pos );
                    nBits = p.getUByte();
                    nFillBits = (nBits&0xf0)>>4;
                    nLineBits = nBits&0x0f;
                    p.initBits();
                }
                if( (flags&0x80) != 0 ) {
                    break;
                }
            }
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"LazyShape("+Tag.tagNames[tagCode]+"): id="+getID()+", name='"+getName()+"'" );
        out.println( indent+"   "+bounds );
        if( bitmaps != null ) {
            out.print( indent+"    bitmaps used: " );
            for( int i=0; i<bitmaps.size(); i++ ) {
                BitmapRef ref = (BitmapRef) bitmaps.elementAt(i);
                out.print( "id["+i+"]="+(ref.bitmap!=null?ref.bitmap.getID():0xffff)+" " );
            }
            out.println();
        }
    }

    public boolean isConstant() {
        return true;
    }

    public Rectangle2D getBounds() {
        return bounds;
    }

    public void collectDeps( DepsCollector dc ) {
        if( bitmaps != null ) {
            for( int i=0; i<bitmaps.size(); i++ ) {
                BitmapRef ref = (BitmapRef) bitmaps.elementAt(i);
                if( ref.bitmap != null ) dc.addDep( ref.bitmap );
            }
        }
    }

    public void addBitmap( FlashDef def, int offset ) {
        if( bitmaps == null ) bitmaps = new IVVector();
        bitmaps.addElement( new BitmapRef(def, offset) );
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((LazyShape)item).data = (DataMarker) data.getCopy();
        ((LazyShape)item).tagCode = tagCode;
        ((LazyShape)item).bounds = (Rectangle2D) bounds.clone();
        if( bitmaps != null ) {
            IVVector v = new IVVector(bitmaps.size());
            for( int i=0; i<bitmaps.size(); i++ ) {
                BitmapRef ref = (BitmapRef) bitmaps.elementAt(i);
                v.addElement( new BitmapRef( copier.copy(ref.bitmap), ref.offset ) );
            }
            ((LazyShape)item).bitmaps = v;
        }
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new LazyShape(), copier );
    }
}
