/*
 * $Id: FOPCommand.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.url.*;

import org.openlaszlo.iv.flash.fop.*;
import org.openlaszlo.iv.flash.context.Context;
import java.io.*;
import java.util.*;
import java.awt.geom.*;

/**
 * Insert Text generator command<BR>
 *
 * @author James Taylor
 */

public class FOPCommand extends GenericCommand
{
    public FOPCommand() {}

    public void doCommand( FlashFile file, Context context, Script parent, int frame ) throws IVException
    {
        String filename  = getParameter( context, "filename" );
        boolean cache = getBoolParameter( context, "cache", false );
        boolean scale = getBoolParameter( context, "scale", true );
        boolean maintainAspectRatio = getBoolParameter( context, "maintainAspectRatio", true );
        String linkHandler = getParameter( context, "linkhandler", null );
        String instancename = getParameter( context, "instancename" );

        Instance inst = getInstance();

        if( filename == null )
        {
            throw new IVException( Resource.NOTEXT );
        }

        String text = null;

        IVUrl url = IVUrl.newUrl( filename, file );

        // check in cache first

        if( cache )
        {
            text = ( String ) MediaCache.getMedia( url );
        }

        if( text == null )
        {
            InputStream is = null;

            try
            {
                is = url.getInputStream();
                BufferedReader reader = new BufferedReader( new InputStreamReader(is) );
                StringBuffer sb = new StringBuffer(100);
                String s = reader.readLine();

                while( s != null )
                {
                    sb.append( s );
                    sb.append( '\n' );
                    s = reader.readLine();
                }

                text = sb.toString();

            }
            catch( IOException e )
            {
                throw new IVException(Resource.ERRCMDFILEREAD, new Object[] {url.getName(), getCommandName()}, e);
            }
            finally
            {
                try
                {
                    if ( is != null ) is.close();
                }
                catch( IOException e ) {}
            }

            MediaCache.addMedia( url, text, text.length(), cache );
        }

        try
        {
            FOPHelper fopHelper = new FOPHelper();

            fopHelper.setXSLFOInputSource( text );

            if ( linkHandler != null )
            {
                fopHelper.setLinkHandler( linkHandler );
            }

            fopHelper.render();

            Script script = fopHelper.getScript();

            inst.setScript( script );

            if ( scale )
            {
                Rectangle2D rect = fopHelper.getMaxPageBounds();
                AffineTransform m = inst.matrix;

                double width = rect.getWidth();
                double height = rect.getHeight();

                double widthFactor = ( m.getScaleX() / width );
                double heightFactor = ( m.getScaleY() / height );

                if ( maintainAspectRatio )
                {
                    if ( widthFactor < heightFactor )
                    {
                        heightFactor = widthFactor;
                    }
                    else
                    {
                        widthFactor = heightFactor;
                    }
                }

                double scaleX = 2048.0 * widthFactor;
                double scaleY = 2048.0 * heightFactor;
                inst.matrix = new AffineTransform( scaleX, 0, 0, scaleY,
                    m.getTranslateX() - ( scaleX * width ) / 2,
                    m.getTranslateY() - ( scaleY * height ) / 2
                );

            }
        }
        catch ( Exception e )
        {
            throw new IVException( e );
        }

        if ( instancename != null )
        {
            inst.name = instancename;
        }
    }
}

