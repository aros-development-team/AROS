/*
 * $Id: InsertGifFileCommand.java,v 1.4 2002/07/22 23:16:04 skavish Exp $
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
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.url.*;

import org.openlaszlo.iv.flash.context.Context;
import java.io.*;

public class InsertGifFileCommand extends GenericCommand {

    public InsertGifFileCommand() {}

    public void doCommand( FlashFile file, Context context, Script parent, int frame ) throws IVException {
        String filename = getParameter( context, "filename", "" );
        boolean scale = getBoolParameter( context, "scale", false );
        //    String export = context.apply( getParameter("export","JPEG") );
        //    String quality = context.apply( getParameter("quality","80") );
        boolean cache = getBoolParameter( context, "cache", false );
        String insertAllFrames = getParameter( context, "frames");
        String instancename = getParameter( context, "instancename" );

        Bitmap bitmap = null;
        try {
            bitmap = (Bitmap) file.addExternalMedia(filename, cache);
        } catch( IOException e ) {
            throw new IVException(Resource.ERRCMDFILEREAD, new Object[] {filename, getCommandName()}, e);
        }

        Instance myInst = bitmap.newInstance( 2048, 2048, scale, true );

        Instance inst = getInstance();
        Script script = inst.copyScript();
        Frame myFrame = script.newFrame();
        myFrame.addInstance( myInst, 1 );

        // because of some stupid bug? in flash player we have to add this
        // dummy move into second frame
        script.newFrame().addInstance(1, null);

        if( instancename != null ) {
            inst.name = instancename;
        }
    }

}
