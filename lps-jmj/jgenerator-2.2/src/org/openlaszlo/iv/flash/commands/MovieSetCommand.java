/*
 * $Id: MovieSetCommand.java,v 1.5 2002/05/29 03:54:49 skavish Exp $
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
import java.util.Locale;

public class MovieSetCommand extends GenericCommand {

    public MovieSetCommand() {}

    public void doCommand( FlashFile file, Context context, Script parent, int frame ) throws IVException {
        String rate = getParameter( context, "rate" );
        String width = getParameter( context, "width" );
        String height = getParameter( context, "height" );
        String color = getParameter( context, "color" );
        String encoding = getParameter( context, "encoding", "default" );

        // set frame rate
        if( rate != null ) {
            double f;
            if( isDefault(rate) ) {
                f = 12.0;
            } else {
                f = Util.toDouble(rate, -1);
                if( f <= 0.0 || f > 120.0 ) {
                    Log.logRB(Resource.INVALRATEVALUE, new Object[] {rate});
                    f = 12.0;
                }
            }
            file.setFrameRate( ((int)f) << 8 );
        }

        // set movie width
        double mywidth  = file.getFrameSize().getWidth();
        double myheight = file.getFrameSize().getHeight();

        if( width != null ) {
            if( isDefault(width) ) {
                mywidth = 550*20;
            } else {
                mywidth = Util.toInt(width, 550)*20;
            }
        }

        // set movie height
        if( height != null ) {
            if( isDefault(height) ) {
                myheight = 400*20;
            } else {
                myheight = Util.toInt(height, 400)*20;
            }
        }

        file.getFrameSize().setFrame( 0, 0, mywidth, myheight );

        // set background color
        if( color != null ) {
            Color c;
            if( isDefault(color) ) {
                c = AlphaColor.white;
            } else {
                c = Util.toColor( color, AlphaColor.white );
            }
            parent.setBackgroundColor( new SetBackgroundColor(c) );
        }

        if( isDefault(encoding) ) {
            String lang = Locale.getDefault().getLanguage();
            if( lang.equals(Locale.JAPANESE.getLanguage()) ) {
                file.setEncoding("SJIS");
            } else if( lang.equals(Locale.KOREAN.getLanguage()) ) {
                file.setEncoding("EUC_KR");
            }
        } else {
            file.setEncoding(encoding);
        }

    }

    public boolean isGlobal() {
        return true;
    }
}
