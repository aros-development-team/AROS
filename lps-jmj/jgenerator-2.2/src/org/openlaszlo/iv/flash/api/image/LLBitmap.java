/*
 * $Id: LLBitmap.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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
import java.awt.image.*;
import java.awt.geom.*;
import java.util.zip.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import com.sun.image.codec.jpeg.*;

public class LLBitmap extends Bitmap {

    protected DataMarker zlibData;
    protected Rectangle2D bounds;
    protected int format;           // 3 - 8 bit, 4 - 16 bit, 5 - 32 bit
    protected int colorTableSize;   // This value is one less than the actual size of the color table

    public LLBitmap() {}

    public static Bitmap parse( Parser p ) {
        LLBitmap o = new LLBitmap();
        o.setID( p.getUWord() );
        o.tagCode = p.getTagCode();
        o.format = p.getUByte();
        int width = p.getUWord();
        int height = p.getUWord();
        o.bounds = GeomHelper.newRectangle(0, 0, width, height);
        // in the other cases there is no color table, thus no color table size !
        if( o.format == 3 ) {
            o.colorTableSize = p.getUByte();
        }
        o.zlibData = new DataMarker( p.getBuf(), p.getPos(), p.getTagEndPos() );
        return o;
    }

    public void write( FlashOutput fob ) {

        int tagSize = 2+1+4+((colorTableSize>=0&&format==3)?1:0)+zlibData.length();
        // always write long tag?
        fob.writeLongTag( tagCode, tagSize );
        fob.writeDefID(this);
        fob.writeByte(format);
        fob.writeWord(getWidth());
        fob.writeWord(getHeight());
        // the colorTableSize should not be written if the color table is empty !
        if( colorTableSize>=0 && format == 3 ) {
            fob.writeByte( colorTableSize );
        }
        zlibData.write(fob);
    }

    public int getSize() {
        return zlibData.buffer.length;
    }

    public Rectangle2D getBounds() { return bounds; }
    public void setBounds( Rectangle2D rect ) { this.bounds = rect; }

    public void setWidth( int width ) {
        if( bounds == null ) bounds = GeomHelper.newRectangle();
        bounds.setFrame( 0, 0, width, getHeight() );
    }

    public void setHeight( int height ) {
        if( bounds == null ) bounds = GeomHelper.newRectangle();
        bounds.setFrame( 0, 0, getWidth(), height );
    }

    public int getFormat() { return format; }
    public void setFormat( int format ) { this.format = format; }

    public int getColorTableSize() { return colorTableSize; }
    public void setColorTableSize( int colorTableSize ) { this.colorTableSize = colorTableSize; }

    public DataMarker getZLibData() { return zlibData; }
    public void setZLibData( DataMarker data ) { this.zlibData = data; }

    public void setAlpha( boolean withAlpha ) {
        if( withAlpha ) {
            tagCode = Tag.DEFINEBITSLOSSLESS2;
        } else {
            tagCode = Tag.DEFINEBITSLOSSLESS;
        }
    }

    public boolean isAlpha() {
        return tagCode == Tag.DEFINEBITSLOSSLESS2;
    }

    public static LLBitmap newGIFBitmap( String fileName )
        throws IVException, IOException
    {
        return newGIFBitmap( IVUrl.newUrl(fileName) );
    }

    public static LLBitmap newGIFBitmap( IVUrl url )
        throws IVException, IOException
    {
        return newGIFBitmap( Util.readUrl(url) );
    }

    public static LLBitmap newGIFBitmap( FlashBuffer fob ) throws IVException {
        LLBitmap bitmap = new LLBitmap();
        try {
            GIFHelper gh = new GIFHelper();
            gh.doRead(fob);
            bitmap.setWidth(gh.getWidth());
            bitmap.setHeight(gh.getHeight());
            bitmap.setFormat(3); // GIF image are always 8 bits
            bitmap.setAlpha(gh.isAlpha()); // but can have transparency
            bitmap.setColorTableSize(gh.getColorTableSize());
            bitmap.setZLibData(gh.getZlibData());
        } catch( IOException e ) {
            throw new IVException(Resource.ERRREADINGGIF, e);
        }
        return bitmap;
    }

    public static LLBitmap newPNGBitmap( String fileName )
        throws IVException, IOException
    {
        return newPNGBitmap( IVUrl.newUrl(fileName) );
    }

    public static LLBitmap newPNGBitmap( IVUrl url )
        throws IVException, IOException
    {
        return newPNGBitmap( Util.readUrl(url) );
    }

    public static LLBitmap newPNGBitmap( FlashBuffer fob ) throws IVException {
        LLBitmap bitmap = new LLBitmap();
        try {
            PNGHelper pnh = new PNGHelper();
            pnh.setInputBuffer(fob);
            bitmap.setZLibData(pnh.getZlibData());
            bitmap.setWidth(pnh.getWidth());
            bitmap.setHeight(pnh.getHeight());
            bitmap.setFormat(pnh.getFormat());
            bitmap.setAlpha(pnh.hasTransparency());
            bitmap.setColorTableSize(pnh.getColorTableSize());
        } catch( IOException e ) {
            throw new IVException(Resource.ERRREADINGPNG, e);
        }
        return bitmap;
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"LossLessBitmap("+Tag.tagNames[tagCode]+"): id="+getID()+" name='"+getName()+"'");
        out.print( indent+"  format="+(format==3?"8 bit":(format==4?"16 bit":"32 bit")) );
        out.println( ", colorTableSize="+colorTableSize+", width="+getWidth()+", height="+getHeight() );

        // print zlib data info
/*        try {
            InflaterInputStream zin = new InflaterInputStream( zlibData.getInputStream() );
            FlashBuffer fob = new FlashBuffer(zin);
            System.out.println( "size="+fob.getSize() );
            out.println( indent+" colortable: RGB"+(isAlpha()?"A":"")+"["+(colorTableSize+1)+"]" );
            int size = (isAlpha()?4:3)*(colorTableSize+1);
            Util.dump(fob.getBuf(),0,size,out);

            int datasize = fob.getSize()-size;
            int width = bounds.getWidth();
            int height = bounds.getHeight();
            int datasize1 = ((width+3)&~0x03)*height;
            out.println( indent+" colordata: UI8["+datasize+"] (rounded(width)*height="+datasize1+")" );
            Util.dump(fob.getBuf(),size,datasize,out);
        } catch( IOException e ) {
        }*/
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((LLBitmap)item).zlibData = (DataMarker) zlibData.getCopy();
        ((LLBitmap)item).bounds = (Rectangle2D) bounds.clone();
        ((LLBitmap)item).format = format;
        ((LLBitmap)item).colorTableSize = colorTableSize;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new LLBitmap(), copier );
    }

/*
    public JPEGBitmap convertToJPEG( float quality ) {

        if( isAlpha() ) {
            Log.logRB( Resource.STR, "Alpha channel has been stripped from image during conversion to JPEG" );
        }

        InflaterInputStream zin = new InflaterInputStream( zlibData.getInputStream() );
        FlashBuffer fob = new FlashBuffer(zin);

        int width = bounds.getWidth();
        int height = bounds.getHeight();
        int scansize = (width+3)&~0x03;
        int datasize = scansize*height;

        int pixelsize = isAlpha()?4:3;
        int bitsperpixel;

        // if 8bit image, then get rid of color table
        if( format == 3 ) {
            bitsperpixel = 24;
            //int newscansize = (width*3+3)&~0x03;
            int newscansize = scansize*3;
            FrashBuffer fb = new FlashBuffer(newscansize*height);
            byte[] newtable = fb.getBuf();
            byte[] table = fob.getBuf();
            int dataend = fob.getSize();
            int pos = (colorTableSize+1)*pixelsize;
            int newpos = 0;
            for( ; pos<dataend; pos++ ) {
                int idx = table[pos];
                newtable[newpos++] = table[idx];    // R
                newtable[newpos++] = table[idx+1];  // G
                newtable[newpos++] = table[idx+2];  // B
            }
            fob = fb;
        } else if( isAlpha() ) {
            // get rid of alpha
        }

        DataBuffer db = new DataBufferByte(fob.getBuf(), fob.getSize());
        WritableRaster raster = Raster.createPackedRaster(db,width,height,,null);
        Raster raster = Raster.createRaster(sm,db,null);

        fob = new FlashBuffer(fob.getSize()/4);
        OutputStream os = fob.getOutputStream();
        JPEGImageEncoder encoder = JPEGCodec.createJPEGEncoder(os);
        JPEGEncodeParam params = encoder.getDefaultJPEGEncodeParam(raster,colorID);
        params.setQuality(quality,true);

        encoder.encode(raster,params);

    }*/
}

