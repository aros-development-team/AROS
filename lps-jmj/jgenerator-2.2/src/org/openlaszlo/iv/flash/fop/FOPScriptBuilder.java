/*
 * $Id: FOPScriptBuilder.java,v 1.4 2002/03/19 22:19:55 skavish Exp $
 *
 * ==========================================================================

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
import java.awt.geom.*;

/**
 * This is a helper class for the SWFRenderer, this does all
 * JGen specific calls, like drawing lines etc etc.
 *
 * @author Johan "Spocke" Sörlin
 * @author James Taylor
 */

public class FOPScriptBuilder
{
    private Script script;
    private Frame currentFrame;

    // Name of actionscript function to handle links ( should take one
    // string argument )

    private String linkHandler = null;

    private Font currentFont;

    private int layerCount = 2;

    // Dimensions of the current page

    private int pageWidth = 0;
    private int pageHeight = 0;

    // Dimensions of the largest page

    private int maxPageWidth = 0;
    private int maxPageHeight = 0;

    /**
     * Constructs a new FOPScriptBuilder
     *
     */

    public FOPScriptBuilder()
    {
        script = new Script( 0 );
    }

    /**
     * Sets the name of the function used to handle links. This should be a
     * function defined on the timeline containing this script.
     *
     * FIXME: Need to evaluate other ways to do this, to allow the most
     *        flexibility
     */

    public void setLinkHandler( String methodName )
    {
        linkHandler = methodName;
    }

    public void startPage( int width, int height )
    {
        pageWidth = millipointsToTwixels( width );
        pageHeight = millipointsToTwixels( height );

        maxPageWidth = ( pageWidth > maxPageWidth ) ? pageWidth : maxPageWidth;
        maxPageHeight = ( pageHeight > maxPageHeight ) ? pageHeight : maxPageHeight;

        currentFrame = script.newFrame();

        // Start with a blank frame

        script.removeAllInstances( currentFrame );
    }

    public Script getScript()
    {
        return script;
    }

    public Rectangle2D getMaxPageBounds()
    {
        return GeomHelper.newRectangle( 0, 0, maxPageWidth, maxPageHeight );
    }

    /**
     * Adds a new line shape to the FlashMovie.
     *
     * @param x1 x1 position in FOP points
     * @param y1 y1 position in FOP points
     * @param x2 x2 position in FOP points
     * @param y1 y1 position in FOP points
     * @param th thickness in FOP points
     * @param r red color channel
     * @param g green color channel
     * @param b blue color channel
     */

    public void addLine( int x1, int y1, int x2, int y2, int th, float r, float g, float b )
    {
        x1 = millipointsToTwixels( x1 );
        y1 = millipointsToTwixels( y1 );
        x2 = millipointsToTwixels( x2 );
        y2 = millipointsToTwixels( y2 );

        th = millipointsToTwixels( th );

        // Translate y axis geometry

        y1 = pageHeight - y1;
        y2 = pageHeight - y2;

        // Build shape

        Shape shape = new Shape();

        shape.setLineStyle( new LineStyle( th, new AlphaColor( r, g, b ) ) );
        shape.drawLine( x1, y2, x2, y2 );
        shape.setBounds( x1, y1, x2-x1, y2-y1 );

        currentFrame.addInstance( shape, layerCount++, new AffineTransform(), null );
    }

    /**
     * Adds a new filled rectangle shape to the FlashMovie.
     *
     * @param x x position in FOP points
     * @param y y position in FOP points
     * @param w width in FOP points
     * @param h height in FOP points
     * @param r red color channel
     * @param g green color channel
     * @param b blue color channel
     */

    public void addRect( int x, int y, int w, int h, float r, float g, float b )
    {
        x = millipointsToTwixels( x );
        y = millipointsToTwixels( y );
        w = millipointsToTwixels( w );
        h = millipointsToTwixels( h );

        // Translate y axis geometry

        h = - h;
        y = pageHeight - y;

        // Build shape

        Shape shape = new Shape();

        shape.setFillStyle0( FillStyle.newSolid( new AlphaColor( r, g, b ) ) );

        Rectangle2D movieRect = GeomHelper.newRectangle( x, y, w, h );
        shape.drawRectangle( movieRect );
        shape.setBounds( movieRect );

        currentFrame.addInstance( shape, layerCount++, new AffineTransform(), null );
    }

    public void addLink( int x, int y, int w, int h, String destination )
    {
        x = millipointsToTwixels( x );
        y = millipointsToTwixels( y );
        w = millipointsToTwixels( w );
        h = millipointsToTwixels( h );

        y = pageHeight - y;

        Button2 button = new Button2();

        // Hit area

        Shape shape = new Shape();

        shape.setFillStyle0( FillStyle.newSolid( new AlphaColor(0x31, 0x63, 0x9c, 0x60) ));
        Rectangle2D r = GeomHelper.newRectangle( 0, 0, w, h );
        shape.drawRectangle( r );
        shape.setBounds( r );

        AffineTransform shapeMatrix = new AffineTransform();

        ButtonRecord hitState =
            new ButtonRecord( ButtonRecord.HitTest, shape, 1, shapeMatrix, CXForm.newIdentity( true ) );

        button.addButtonRecord( hitState );

        // Up state -- good for debug

        // ButtonRecord upState =
        //     new ButtonRecord( ButtonRecord.Up, shape, 1, shapeMatrix, CXForm.newDefault(true) );

        // button.addButtonRecord( upState );

        // Action

        Program p = new Program();

        if ( linkHandler != null )
        {
            p.push( destination );
            p.push( 1 );
            p.push( "_parent" );
            p.eval();
            p.push( linkHandler );
            p.callMethod();
        }
        else
        {
            p.getURL( destination, "_blank" );
        }

        ActionCondition onRelease = new ActionCondition( ActionCondition.OverDownToOverUp, p );

        button.addActionCondition( onRelease );

        AffineTransform instMatrix = AffineTransform.getTranslateInstance(x,y);

        currentFrame.addInstance( button, layerCount++, instMatrix, null );
    }

    /**
     * Adds a new text element to the FlashMovie.
     *
     * @param string String to place in text element
     * @param pos_x x position in FOP points
     * @param pos_y y position in FOP points
     * @param r red color channel
     * @param g green color channel
     * @param b blue color channel
     * @param fontMetric font metric
     * @param size font size in FOP points
     * @param underlined underlined text element
     */

    public void addText( String string,
                         int x, int y,
                         float r, float g, float b,
                         SWFFontMetric fontMetric,
                         int size,
                         int width )
    {
        x = millipointsToTwixels( x );
        y = millipointsToTwixels( y );

        size = millipointsToTwixels( size );
        width = millipointsToTwixels( width );


        // Translate y axis geometry

        y = pageHeight - y;

        /*

        // DEBUG: This draws the boxes where words should be rendered, for
        //        debugging the baseline position

        Shape shape = new Shape();

        shape.setFillStyle0( FillStyle.newSolid( new AlphaColor( 64, 64, 128 ) ) );

        Rect movieRect = new Rect( x, y, x + width, y - size );
        shape.drawRectangle( movieRect );
        shape.setBounds( movieRect );

        currentFrame.addInstance( shape, layerCount++, new Matrix(), null );

        */

        // Build text

        currentFont = fontMetric.getFont();

        // FIXME: This is a gross hack! It appears that when fop draws a text
        //        at a given y position it expects that position to correspond
        //        to the baseline. However in flash, the position correponds to
        //        the bottom of the EM square. This seems to compensate well
        //        for all fonts ( that have been tested ) but I don't know WHY!

        y += ( currentFont.descent - currentFont.leading ) / 5;

        Text text = Text.newText();

        TextItem item = new TextItem( string, currentFont, size, new AlphaColor( r, g, b ) );

        text.addTextItem( item );

        text.setBounds( 0, 0, width + 10, size );

        AffineTransform position = AffineTransform.getTranslateInstance(x, y-size);

        currentFrame.addInstance( text, layerCount++, position, null );
    }



    /**
     * Convert millipoints ( unit of formatting used by most parts of FOP ) to
     * twixels ( unit used by flash ).
     *
     * 1000 points = 1 pixel = 20 twixels, thus 1 point = .02 twixels
     */

    private int millipointsToTwixels( int points )
    {
        return Math.round( points * 0.02f );
    }

}
