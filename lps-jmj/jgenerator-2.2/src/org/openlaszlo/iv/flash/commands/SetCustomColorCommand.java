/*
 * $Id: SetCustomColorCommand.java,v 1.5 2002/06/12 23:54:14 skavish Exp $
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

public class SetCustomColorCommand extends GenericCommand {

    public SetCustomColorCommand() {}

    private int parseNumber( String num, int def, int max ) {
        if( num == null ) return def;
        try {
            double d = Double.valueOf(num).doubleValue();
            if( d < 0 || d > max ) {
                return def;
            }
            return (int) d;
        } catch( NumberFormatException e ) {
            return def;
        }
    }

    private int parsePercent( String num ) {
        int percent = parseNumber( num, 100, 100 );
        return (percent*255)/100;
    }

    public void doCommand( FlashFile file, Context context, Script parent, int frameNum ) throws IVException {
        Instance inst = getCommandInstance(file, context, parent, frameNum);

        // process corresponding flash def first
        processFlashDef(inst, file, context);

        int percentred   = parsePercent( getParameter( context, "percentred" ) );
        int percentgreen = parsePercent( getParameter( context, "percentgreen" ) );
        int percentblue  = parsePercent( getParameter( context, "percentblue" ) );
        int percentalpha = parsePercent( getParameter( context, "percentalpha" ) );
        int offsetred    = parseNumber( getParameter( context, "offsetred" ), 0, 255 );
        int offsetgreen  = parseNumber( getParameter( context, "offsetgreen" ), 0, 255 );
        int offsetblue   = parseNumber( getParameter( context, "offsetblue" ), 0, 255 );
        int offsetalpha  = parseNumber( getParameter( context, "offsetalpha" ), 0, 255 );

        CXForm cx = inst.cxform = new CXForm(true);

        cx.setRedMul( percentred );
        cx.setGreenMul( percentgreen );
        cx.setBlueMul( percentblue );
        cx.setAlphaMul( percentalpha );

        cx.setRedAdd( offsetred );
        cx.setGreenAdd( offsetgreen );
        cx.setBlueAdd( offsetblue );
        cx.setAlphaAdd( offsetalpha );
    }

}
