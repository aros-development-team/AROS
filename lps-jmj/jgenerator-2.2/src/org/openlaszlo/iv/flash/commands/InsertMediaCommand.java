/*
 * $Id: InsertMediaCommand.java,v 1.7 2002/07/22 23:16:04 skavish Exp $
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
import java.awt.geom.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.url.*;

import org.openlaszlo.iv.flash.context.Context;

/**
 * Insert Media generator command<BR>
 * Insert gif, jpg, png, swf or swt<BR>
 * Detect media type from its data, not from extension<BR>
 *
 * <P>if 'scalex' or 'scaley' are not null, then scale width or/and height
 * of the image accordingly, the placeholder's scaling is ignored
 * <P>if both 'scalex' and 'scaley' are null then check 'scale'. if it's true
 * then scale image to the placeholder size, if scale is false then leave the
 * image as is and ignore the placeholder's scaling
 *
 * @author Dmitry Skavish
 */
public class InsertMediaCommand extends GenericCommand {

    private static final int ALWAYS_SCALE   = 0;
    private static final int NEVER_SCALE    = 1;
    private static final int IFNEEDED_SCALE = 2;

    public InsertMediaCommand() {}

    public void doCommand( FlashFile file, Context context, Script parent, int frameNum ) throws IVException {
        String filename = getParameter( context, "filename", "" );
        boolean cache   = getBoolParameter( context, "cache", false );
        boolean expand  = getBoolParameter( context, "expand", false );
        int scaletox    = getIntParameter( context, "scalex", -1 ) * 20;
        int scaletoy    = getIntParameter( context, "scaley", -1 ) * 20;
        String halign   = getParameter( context, "halign", "left" );
        String valign   = getParameter( context, "valign", "top" );
        boolean mask    = getBoolParameter( context, "mask", false );
        boolean scaleproport = getBoolParameter(context, "scaleproport", false);
        //    String export = context.apply( getParameter("export","JPEG") );
        //    String quality = context.apply( getParameter("quality","80") );
        String instancename = getParameter( context, "instancename" );

        String mypscale  = getParameter(context, "scale", "always");

        boolean neverScale = mypscale.equalsIgnoreCase("never") || mypscale.equalsIgnoreCase("false");
        boolean ifNeededScale = mypscale.equalsIgnoreCase("if needed");
        boolean alwaysScale = !neverScale && !ifNeededScale;

        Instance inst = getInstance();

        IVUrl url = IVUrl.newUrl(filename, file);

        Object media = null;

        try {
            media = file.addExternalMedia(url, cache);
        } catch( IOException e ) {
            throw new IVException(Resource.ERRCMDFILEREAD, new Object[] {url.getName(), getCommandName()}, e);
        }

        Script script;
        double width, height;

        if( media instanceof FlashFile ) {
            // insert flash movie
            FlashFile flashFile = (FlashFile) media;
            synchronized(flashFile) {
                script = flashFile.getMainScript();
                script.resetMain();
                if( flashFile.isTemplate() ) {
                    script.removeFileDepGlobalCommands();
                    script = script.copyScript();
                }
            }
            if( flashFile.isTemplate() ) file.processScript(script, context);

            Rectangle2D r = flashFile.getFrameSize();  // script.getBounds();
            width = r.getWidth();   // in twixels
            height = r.getHeight(); // in twixels

            // expand frames
            if( expand ) {
                // create new frames if needed
                int myTotal = script.getFrameCount();
                parent.getFrameAt(frameNum+myTotal-1);
            }

        } else {
            // insert image file
            Bitmap bitmap = (Bitmap) media;
            Instance myInst = bitmap.newInstance();
            width = bitmap.getWidth()*20;   // now in twixels
            height = bitmap.getHeight()*20; // now in twixels

            script = inst.copyScript();
            Frame myFrame = script.newFrame();
            myFrame.addInstance( myInst, 1 );
        }

        inst.setScript( script );

        // add mask if needed
        if( mask ) {
            addMask(parent, frameNum, inst, GeomHelper.newRectangle(-1024, -1024, 2048, 2048));
        }

        Rectangle2D winBounds = GeomHelper.getTransformedSize(inst.matrix,
                                    GeomHelper.newRectangle(-1024, -1024, 2048, 2048));
        int winWidth = (int) winBounds.getWidth();
        int winHeight = (int) winBounds.getHeight();

        boolean deScale = true;

        double scaleX, scaleY, translateX, translateY;

        scaleX = scaleY = 1.0;
        if( scaletox >= 0 || scaletoy >= 0 ) {

            if( scaletox >= 0 ) {
                if( !ifNeededScale || width > scaletox ) {
                    scaleX = scaletox / width;
                    width = scaletox;
                }
            }

            if( scaletoy >= 0 ) {
                if( !ifNeededScale || height > scaletoy ) {
                    scaleY = scaletoy / height;
                    height = scaletoy;
                }
            }

            if( scaleproport ) {
                if( scaletox < 0 ) {
                    scaleX = scaleY;
                    width = scaleX*width;
                }
                if( scaletoy < 0 ) {
                    scaleY = scaleX;
                    height = scaleY*height;
                }
            }
        } else if( alwaysScale ) {
            if( scaleproport ) {
                scaleX = winWidth/width;
                scaleY = winHeight/height;
                if( scaleX < scaleY ) {
                    scaleY = scaleX;
                } else {
                    scaleX = scaleY;
                }
                width *= scaleX;
                height *= scaleY;
            } else {
                scaleX = 2048.0/width;
                scaleY = 2048.0/height;
                deScale = false;
            }
        } else if( ifNeededScale ) {
            if( width > winWidth || height > winHeight ) {
                scaleX = winWidth/width;
                scaleY = winHeight/height;
                if( scaleproport ) {
                    if( scaleX < scaleY ) {
                        scaleY = scaleX;
                    } else {
                        scaleX = scaleY;
                    }
                }
                width *= scaleX;
                height *= scaleY;
            }
        }

        if( deScale ) {

            // horizontal alignment
            if( halign.equalsIgnoreCase( "left" ) ) {
                translateX = -winWidth/2;
            } else if( halign.equalsIgnoreCase( "right" ) ) {
                translateX = -width+winWidth/2;
            } else {    // center
                translateX = -width/2;
            }

            // vertical alignment
            if( valign.equalsIgnoreCase( "top" ) ) {
                translateY = -winHeight/2;
            } else if( valign.equalsIgnoreCase( "bottom" ) ) {
                translateY = -height+winHeight/2;
            } else {    // center
                translateY = -height/2;
            }

            GeomHelper.deScaleMatrix( inst.matrix );
        } else {
            translateX = translateY = -1024.0;
        }

        inst.matrix.concatenate( new AffineTransform(scaleX,0,0,scaleY,translateX,translateY) );

        if( instancename != null ) {
            inst.name = instancename;
        }
    }

}
