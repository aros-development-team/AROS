/*
 * $Id: XMLTickerCommand.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.context.Context;

import java.io.*;
import java.awt.geom.*;

public class XMLTickerCommand extends GeneralXMLListCommand {

    public XMLTickerCommand() {}

    public void doCommand( FlashFile file, Context context, Script parent, int frameNum ) throws IVException {
        initParms( context );

        int stepsize = getIntParameter( context, "stepsize", 4 ) * 20;

        Script listScript = makeList( file, context, parent, frameNum );

        Script myScript = getInstance().copyScript();

        int scrollFrames = stepsize==0?1:(listSize+Math.abs(stepsize)-1)/Math.abs(stepsize);

        int current = 0;
        for( int i=0; i<scrollFrames; i++ ) {
            Frame scrollFrame = myScript.newFrame();
            AffineTransform m;

            if ( orient == ORIENT_VERT ) {
                m = AffineTransform.getTranslateInstance(0,current);
            } else if ( orient == ORIENT_HORIZ ) {
                m = AffineTransform.getTranslateInstance(current,0);
            } else {
                m = new AffineTransform();  // ?????
            }

            if( i==0 ) scrollFrame.addInstance(listScript, 1, m, null, "contents" );
            else scrollFrame.addInstance(1, m);

            if ( orient == ORIENT_VERT ) {
                m = AffineTransform.getTranslateInstance(0,current+listSize);
            } else if ( orient == ORIENT_HORIZ ) {
                m = AffineTransform.getTranslateInstance(current+listSize,0);
            } else {
                m = new AffineTransform();  // ?????
            }

            if( i==0 ) scrollFrame.addInstance(listScript, 2, m, null, "contents2" );
            else scrollFrame.addInstance(2, m);

            current -= stepsize;
        }

        addMask( parent, frameNum );
    }

}
