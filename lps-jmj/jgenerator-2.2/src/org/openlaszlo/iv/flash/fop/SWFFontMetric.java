/*
 * $Id: SWFFontMetric.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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

package org.openlaszlo.iv.flash.fop;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.button.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;


import java.io.*;
import java.util.*;

/**
 * Implementation of the FOP FontMetric interface. This class
 * returns information about Flash fonts to the FOP Framework.
 *
 * @author Johan "Spocke" Sörlin
 * @author James Taylor
 */

public class SWFFontMetric implements org.apache.fop.layout.FontMetric
{
    private String fontName = null;
    private Font flashFont = null;
    private int widths[] = null;

    /**
     * Create a new SWFFontMetric for the given font filename
     */

    public SWFFontMetric( String fontName )
    {
        this.fontName = fontName;
    }

    public int getAscender( int size )
    {
        Font font = getFont( );
        int realAscent;

        realAscent = font.ascent - font.descent;

        return convertUnits( size * realAscent );
    }

    public int getCapHeight( int size )
    {
        return this.getAscender( size );
    }

    public int getDescender( int size )
    {
        Font font = getFont( );

        return convertUnits( -1 * size * font.descent );
    }

    public int getFirstChar( )
    {
        return 32;
    }

    public int getLastChar( )
    {
        return 255;
    }

    public int[] getWidths( int size )
    {
        Font font = getFont( );

        if ( this.widths == null )
        {
            this.widths = new int[ 255 ];

            for ( int i = 0; i < this.widths.length; i++ )
            {
                int code = font.getIndex( ( char ) i );
                this.widths[ i ] = convertUnits( size * ( font.getAdvanceValue( code ) * 2 ) );
            }
        }

        return this.widths;
    }

    public int getXHeight( int size )
    {
        Font font = getFont( );

        return convertUnits( ( size
                 + ( font.ascent * 1000 )
                 + ( font.descent * 1000 ) ) * 1000 );
    }

    public int width( int i, int size )
    {
        Font font = getFont( );

        int code = font.getIndex( ( char ) i );
        int width = font.getAdvanceValue( code );

        return convertUnits( width * size );
    }

    /**
     * Return the Font object for this metric.
     */

    public Font getFont()
    {
        if ( flashFont == null )
        {
            flashFont = FontDef.load( fontName );
        }

        return flashFont;
    }

    /**
     * SWF uses a 1024 x 1024 EM square, while fop uses 1000x1000, thus all
     * metric values must be converted.
     */

    private int convertUnits( int n )
    {
        return Math.round( n * ( 1000f / 1024f ) );
    }
}
