/*
 * $Id: InsertTextCommand.java,v 1.5 2002/04/29 06:19:46 skavish Exp $
 *
 * ===========================================================================
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

package org.openlaszlo.iv.flash.commands;

import java.io.*;
import java.util.*;
import java.awt.geom.*;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.url.*;

import org.openlaszlo.iv.flash.context.Context;

/**
 * Insert Text generator command<BR>
 *
 * @author Dmitry Skavish
 */
public class InsertTextCommand extends GenericCommand {

    public InsertTextCommand() {}

    public void doCommand( FlashFile file, Context context, Script parent, int frameNum ) throws IVException {
        String filename  = getParameter( context, "filename" );
        String text      = getParameter( context, "text" );
        boolean mask     = getBoolParameter( context, "mask", false );          // true, false
        boolean html     = getBoolParameter( context, "html", false );          // true, false
        String embedset  = getParameter( context, "embedset", "[all]" );
        AlphaColor color = getColorParameter( context, "fontColor", AlphaColor.red );
        int fontSize     = getIntParameter( context, "fontSize", 12 ) * 20;
        String fontType  = getParameter( context, "fontType", "Arial" );
        String fftFile   = getParameter( context, "fftFilePath" );              // name of fft file
        boolean bold     = getBoolParameter( context, "bold", false );          // true, false
        boolean italic   = getBoolParameter( context, "italic", false );        // true, false
        String alignment = getParameter( context, "alignment", "left" );        // left, right, center, justify
        double spacing   = getDoubleParameter( context, "letterSpacing", 0.0 ) * 10; // in half pixels
        int lineSpacing  = getIntParameter( context, "lineSpacing", 0 ) * 20;   // in points
        int transparency = getIntParameter( context, "transparency", 100 );     // 0-100
        boolean cache   = getBoolParameter( context, "cache", false );
        String instancename = getParameter( context, "instancename" );

        Instance inst = getInstance();

        // -----------------------------------------------------------
        // read text from file if needed
        // -----------------------------------------------------------
        if( text == null ) {
            if( filename == null ) {
                throw new IVException( Resource.NOTEXT );
            }
            IVUrl url = IVUrl.newUrl(filename, file);

            // check in cache first
            if( cache ) {
                text = (String) MediaCache.getMedia( url );
            }

            if( text == null ) {
                LineReader reader = null;
                try {
                    reader = Util.getUrlReader(file, url);
                    StringBuffer sb = new StringBuffer(100);
                    String s = reader.readLine();
                    while( s != null ) {
                        sb.append(s);
                        sb.append(Util.lineSeparator);
                        s = reader.readLine();
                    }
                    text = sb.toString();
                } catch( IOException e ) {
                    throw new IVException(Resource.ERRCMDFILEREAD, new Object[] {url.getName(), getCommandName()}, e);
                } finally {
                    try {
                        if( reader != null ) reader.close();
                    } catch( IOException e ) {}
                }
                MediaCache.addMedia(url, text, text.length(), cache);
            }
        } else {
            String encoding = file.getEncoding()!=null?file.getEncoding():PropertyManager.defaultEncoding;
            try {
                text = encoding!=null? new String(text.getBytes(), encoding): text;
            } catch( UnsupportedEncodingException e ) {
                Log.log(e);
            }
        }

        // ---------------------------------------------------------------
        // get the font (it's a little bit ugly, needs to be changed!!!)
        // ---------------------------------------------------------------
        if( fftFile == null ) {
            fftFile = fontType.replace( ' ', 'G' )+".fft";
            if( italic ) fftFile = "I"+fftFile;
            if( bold ) fftFile = "B"+fftFile;
        }

        fftFile = Util.translatePath( fftFile );
        File fontFile = new File( fftFile );
        if( !fontFile.exists() ) {
            fftFile = Util.concatFileNames( PropertyManager.fontPath, fftFile );
            fontFile = new File( Util.getInstallDir(), fftFile );
        }
        fftFile = fontFile.getAbsolutePath();

        Font font = FontDef.load( fftFile, file );

        // create text block and set parameters
        Text t_block = Text.newText();
        color.setAlpha( (transparency * 255)/100 );
        TextItem t_item = new TextItem( text, font, fontSize, color );

        if( alignment.equals("left") ) t_item.align = 0;
        else if( alignment.equals("right") ) t_item.align = 1;
        else if( alignment.equals("center") ) t_item.align = 2;
        else if( alignment.equals("justify") ) t_item.align = 3;

        t_item.kerning = (int) spacing;
        t_item.linesp = lineSpacing;

        t_block.addTextItem( t_item );

        // calculate text bounds
        Rectangle2D winBounds = GeomHelper.getTransformedSize( inst.matrix,
            GeomHelper.newRectangle(-1024, -1024, 2048, 2048) );
        int winWidth = (int) winBounds.getWidth();
        int winHeight = (int) winBounds.getHeight();

        t_block.setBounds( GeomHelper.newRectangle(0,0,winWidth,winHeight) );

        GeomHelper.deScaleMatrix( inst.matrix );
        inst.matrix.translate( -winWidth/2, -winHeight/2 );

        // copy script and add the text there
        Script script = inst.copyScript();
        Frame textFrame = script.newFrame();
        Instance textInst = textFrame.addInstance( t_block, 1, null, null );

        // add mask
        if( mask ) {
            //addMask(parent, frameNum, inst, winWidth, winHeight);
            addMask(script, 0, textInst, winWidth, winHeight);
        }

        if( instancename != null ) {
            inst.name = instancename;
        }
    }

}

