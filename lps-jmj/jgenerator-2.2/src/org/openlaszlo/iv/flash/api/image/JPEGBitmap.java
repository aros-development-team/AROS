/*
 * $Id: JPEGBitmap.java,v 1.5 2002/02/25 19:07:04 ptalbot Exp $
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
import java.awt.color.ColorSpace;
import java.awt.image.ColorConvertOp;
import java.awt.image.BufferedImage;
import java.awt.image.Raster;
import java.awt.geom.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;

import com.sun.image.codec.jpeg.*;

public class JPEGBitmap extends Bitmap {

    private static final String jpegTablesName = "%JPEGTABLES$JPEGTABLES%";
    private DataMarker data;
    private FlashItem jpegTables;
    private Rectangle2D bounds;
    private JPEGInfo info;

    /** true - ready to be generated, false - not ready, has to be converted */
    private boolean readyToGen;

    public JPEGBitmap() {}

    public void setData( DataMarker data ) {
        this.data = data;
        this.bounds = null;
        this.readyToGen = false;
    }

    public static Bitmap parse( Parser p ) {
        JPEGBitmap o = new JPEGBitmap();
        o.setID( p.getUWord() );
        o.data = new DataMarker( p.getBuf(), p.getPos(), p.getTagEndPos() );
        o.readyToGen = true;
        o.tagCode = p.getTagCode();
        if( o.tagCode == Tag.DEFINEBITS ) {
            o.jpegTables = p.getDefFromLibrary(jpegTablesName);
        }
        return o;
    }

    public static void parseJPegTables( Parser p ) {
        JPEGTables tables = new JPEGTables();
        tables.setData( new DataMarker( p.getBuf(), p.getPos(), p.getTagEndPos() ) );   // 02/09/01 by sd
        tables.setName( jpegTablesName );
        p.addDefToLibrary( jpegTablesName, tables );
    }

    public int getSize() {
        return data.buffer.length;
    }

    public Rectangle2D getBounds() {
        if( bounds != null ) return bounds;
        parseData();
        return bounds;
    }

    public void collectDeps( DepsCollector dc ) {

        /* LASZLO we never output JPEGTABLES tags
        if( jpegTables != null ) {
            dc.addDep(jpegTables);
        }
        */
    }

    public static JPEGBitmap newJPEGBitmap( String fileName )
        throws IVException, IOException
    {
        return newJPEGBitmap( IVUrl.newUrl(fileName));
    }

    public static JPEGBitmap newJPEGBitmap( IVUrl url )
        throws IVException, IOException
    {
        return newJPEGBitmap( Util.readUrl(url) );
    }

    public static JPEGBitmap newJPEGBitmap( FlashBuffer fob ) throws IVException {
        JPEGBitmap bitmap = new JPEGBitmap();
        bitmap.setTag( Tag.DEFINEBITSJPEG2 );
        DataMarker data = new DataMarker(fob.getBuf(), 0, fob.getSize());
        bitmap.setData(data);
        //bitmap.parseData();
        return bitmap;
    }

    private void parseData() {
        info = JPEGHelper.getInfo(data.buffer, data.start, data.end);
        bounds = GeomHelper.newRectangle(0,0,info.width,info.height);

        // check if it's not progressive jpeg or gray
        if( info.type == 0 && info.num_comps == 3 ) {
            readyToGen = true;
        } else {
            readyToGen = false;
        }
    }

    /**
     * Reencode the image with specified quality
     * <P>
     * As a side effect this method converts from progressive jpeg
     * (if any) to regular
     *
     * @param quality quality of new jpeg: 0..1, with 1 being highest quality
     */
    public void processImage( float quality ) {
        try {
            Log.logRB( Resource.REENCODINGJPEG, new Object[] {new Float(quality)} );
            int size = data.end-data.start;
            InputStream is = data.getInputStream();
            JPEGImageDecoder decoder = JPEGCodec.createJPEGDecoder( is );
            BufferedImage image = decoder.decodeAsBufferedImage();

            info = JPEGHelper.getInfo(data.buffer, data.start, data.end);
            // Need to color convert from greyscale to RGB
            if (info.num_comps != 3) {
                ColorConvertOp op = new ColorConvertOp(image.getColorModel().getColorSpace(),
                    ColorSpace.getInstance(ColorSpace.CS_LINEAR_RGB), null);
                image = op.filter(image, null);
            }

            bounds = GeomHelper.newRectangle(0,0,image.getWidth(),image.getHeight());

            FlashBuffer fob = new FlashBuffer(size);
            OutputStream os = fob.getOutputStream();

            JPEGImageEncoder encoder = JPEGCodec.createJPEGEncoder(os);
            JPEGEncodeParam params = encoder.getDefaultJPEGEncodeParam(image);
            params.setQuality(quality,true);
            encoder.encode(image,params);

            data.buffer = fob.getBuf();
            data.end = fob.getSize();
            data.start = 0;
            readyToGen = true;
        } catch( IOException e ) {
            Log.log( e );
        }
    }

    /**
     * Rescales this image with specified width, height and quality
     *
     * @param width   new width
     * @param height  new height
     * @param quality new quality
     */
    public void rescale( int width, int height, float quality ) {
        try {
            Log.logRB(Resource.RESCALINGJPEG,
                new Object[] {new Integer(width), new Integer(height), new Float(quality)});
            int size = data.end-data.start;
            InputStream is = data.getInputStream();
            JPEGImageDecoder decoder = JPEGCodec.createJPEGDecoder(is);
            BufferedImage image = decoder.decodeAsBufferedImage();
            image = (BufferedImage) image.getScaledInstance(width, height, java.awt.Image.SCALE_SMOOTH);

            bounds = GeomHelper.newRectangle(0,0,image.getWidth(),image.getHeight());

            FlashBuffer fob = new FlashBuffer(size);
            OutputStream os = fob.getOutputStream();

            JPEGImageEncoder encoder = JPEGCodec.createJPEGEncoder(os);
            JPEGEncodeParam params = encoder.getDefaultJPEGEncodeParam(image);
            params.setQuality(quality,true);
            encoder.encode(image,params);

            data.buffer = fob.getBuf();
            data.end = fob.getSize();
            data.start = 0;
            readyToGen = true;
        } catch( IOException e ) {
            Log.log( e );
        }
    }

/*    private GraphicsConfiguration gr_config = null;
    private static GraphicsConfiguration getGraphicsConfiguration() {
        if( gr_config == null ) {
            GraphicsEnvironment gr_env = GraphicsEnvironment.getLocalGraphicsEnvironment();
            GraphicsDevice gr_device = gr_env.getDefaultScreenDevice();
            gr_config = gr_device.getDefaultConfiguration();
        }
        return gr_config;
    }
*/
    public void write( FlashOutput fob ) {
        if( !readyToGen ) {
            processImage(.75f);
        }
        // LASZLO, turn DEFINEBITS into DEFINTEBITSJPEG2
        if (tagCode == Tag.DEFINEBITS) {
            JPEGTables tables = (JPEGTables) jpegTables; 
            fob.writeTag(Tag.DEFINEBITSJPEG2, 2 + data.length() + tables.getDataLength());
            fob.writeDefID( this );
            tables.writeData(fob);
        } else {
            fob.writeTag(tagCode, 2+data.length());
            fob.writeDefID( this );
        }
        data.write(fob);
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"JPEGBitmap("+Tag.tagNames[tagCode]+"): id="+getID()+" size="+data.length()+" name='"+getName()+"'" );
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((JPEGBitmap)item).data = (DataMarker) data.getCopy();
        ((JPEGBitmap)item).jpegTables = jpegTables;
        ((JPEGBitmap)item).bounds = (Rectangle2D) bounds.clone();
        ((JPEGBitmap)item).readyToGen = readyToGen;
        ((JPEGBitmap)item).info = info!=null?info.getCopy():null;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new JPEGBitmap(), copier );
    }

    public static class JPEGInfo {
        public int type;            // 0 - regular, 1 - progressive
        public int width;
        public int height;
        public int num_comps;
        public int precision;
        public JPEGInfo getCopy() {
            JPEGInfo info = new JPEGInfo();
            info.type = type;
            info.width = width;
            info.height = height;
            info.num_comps = num_comps;
            info.precision = precision;
            return info;
        }
    }

    private static class JPEGTables extends FlashDef {
        DataMarker data;
        public JPEGTables() {}
        public int getTag() { return Tag.JPEGTABLES; }
        public void setData( DataMarker data ) { this.data = data; }
        public void write( FlashOutput fob ) {
            fob.writeTag(Tag.JPEGTABLES, data.length());
            data.write(fob);
        }

        public void writeData( FlashOutput fob ) {
            data.write(fob);
        }
        public int getDataLength( ) {
            return data.length();
        }
    }

}

