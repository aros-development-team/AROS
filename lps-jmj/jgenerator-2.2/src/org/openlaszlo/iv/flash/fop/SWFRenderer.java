/*
 * $Id: SWFRenderer.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

import org.xml.sax.InputSource;

import org.apache.fop.configuration.*;
import org.apache.fop.fo.properties.*;
import org.apache.fop.layout.*;
import org.apache.fop.layout.inline.*;
import org.apache.fop.messaging.MessageHandler;
import org.apache.fop.datatypes.*;
import org.apache.fop.image.*;
import org.apache.fop.svg.*;
import org.apache.fop.render.pdf.*;
import org.apache.fop.viewer.*;
import org.apache.fop.apps.*;

import java.io.*;
import java.util.*;

/**
 * This is a implementation of the FOP Renderer interface that places Flash objects
 * based on FOP data.
 *
 * @author Johan "Spocke" Sörlin
 * @author James Taylor
 */

public class SWFRenderer implements org.apache.fop.render.Renderer
{
    private int currentPosX = 0;
    private int currentPosY = 0;
    private int currentACPosX = 0;
    private int currentPage = 0;
    private int movieWidth = 0;
    private int movieHeight = 0;
    private AreaTree tree;
    private Hashtable fontNames;
    private Hashtable fontStyles;

    private FOPScriptBuilder flashMovie = new FOPScriptBuilder();

    public void startRenderer( OutputStream outputStream ) throws IOException { }


    public void stopRenderer( OutputStream outputStream ) throws IOException { }

    /**
     * Constructs a new FOPSWFRenderer
     */

    public SWFRenderer( ) { }

    /**
     * Get the script this renderer is drawing into
     */

    public FOPScriptBuilder getScriptBuilder()
    {
        return flashMovie;
    }

    public void setLinkHandler( String s )
    {
        flashMovie.setLinkHandler( s );
    }

    /**
     * set up the given FontInfo
     *
     * @param fontInfo font info passed from FOP framework
     */

    public void setupFontInfo( org.apache.fop.layout.FontInfo fontInfo )
    {
        org.apache.fop.render.pdf.FontSetup.setup( fontInfo );

        ConfigurationReader reader =
            new ConfigurationReader (
                new InputSource( org.openlaszlo.iv.flash.util.Util.getInstallDir()
                                 + java.io.File.separator
                                 + org.openlaszlo.iv.flash.util.PropertyManager
                                 .getProperty( "org.openlaszlo.iv.flash.fopConfig" ) ) );

        try
        {
            reader.start();
        }
        catch ( org.apache.fop.apps.FOPException error )
        {
            MessageHandler.errorln( "Unable to read user configuration file." );
        }

        String internalName = null;
        int fontNumber = 0;

        Vector fontInfos = Configuration.getFonts();

        if ( fontInfos == null )
        {
            return ;
        }

        for ( Enumeration e = fontInfos.elements(); e.hasMoreElements(); )
        {
            org.apache.fop.configuration.FontInfo configFontInfo =
                ( org.apache.fop.configuration.FontInfo ) e.nextElement();

            String fontFile = configFontInfo.getEmbedFile();

            internalName = "F" + ( fontNumber ++ );

            fontInfo.addMetrics( internalName, new SWFFontMetric( fontFile ) );

            Vector triplets = configFontInfo.getFontTriplets();

            for ( Enumeration t = triplets.elements(); t.hasMoreElements(); )
            {
                FontTriplet triplet = ( FontTriplet ) t.nextElement();

                fontInfo.addFontProperties( internalName,
                                            triplet.getName(),
                                            triplet.getStyle(),
                                            triplet.getWeight() );
            }
        }
    }

    /**
     * Set up renderer options -- Empty
     *
     * @param options options passed from FOP framework
     */

    public void setOptions( java.util.Hashtable options ) { }

    /**
     * Set the producer of the rendering -- Empty
     *
     * @param producer producer passed from FOP framework
     */

    public void setProducer( String producer ) { }

    /**
     * render the given page.
     * The stream parameter is ignored.
     *
     * @param area_tree area tree passed from FOP framework
     * @param stream stream to write to, ignored
     */

    public void render( Page page, OutputStream stream )
        throws IOException
    {
        renderPage( page );
    }

    /**
     * render the given area container
     *
     * @param area area passed from FOP framework
     */

    public void renderAreaContainer( AreaContainer area )
    {
        int oldX = this.currentACPosX;
        int oldY = this.currentPosY;

        if ( area.getPosition() == Position.ABSOLUTE )
        {
            this.currentPosY = area.getYPosition()
                               - ( 2 * area.getPaddingTop() )
                               - ( 2 * area.getBorderTopWidth() );

            this.currentACPosX = area.getXPosition();
        }
        else if ( area.getPosition() == Position.RELATIVE )
        {
            this.currentPosY -= area.getYPosition();
            this.currentACPosX += area.getXPosition();
        }
        else if ( area.getPosition() == Position.STATIC )
        {
            this.currentPosY -= ( area.getPaddingTop()
                                  + area.getBorderTopWidth() );

            this.currentACPosX += ( area.getPaddingLeft()
                                    + area.getBorderLeftWidth() );
        }

        drawFrame( area );

        renderChildren( area );

        this.currentACPosX = oldX;
        this.currentPosY = oldY;

        if ( area.getPosition() == Position.STATIC )
        {
            this.currentPosY -= area.getHeight();
        }
    }

    /**
     * render the given body area container
     *
     * @param area body area container passed from FOP framework
     */

    public void renderBodyAreaContainer( BodyAreaContainer area )
    {
        renderAreaContainer( area.getBeforeFloatReferenceArea() );
        renderAreaContainer( area.getFootnoteReferenceArea() );

        // Children are span areas? ( see AWTRenderer )

        renderChildren( area );
    }

    /**
     * render the given span area
     *
     * @param area span area passed from FOP framework
     */

    public void renderSpanArea( SpanArea area )
    {
        // Children are column areas? ( see AWTRenderer )

        renderChildren( area );
    }

    /**
     * render the given block area
     *
     * @param area span block passed from FOP framework
     */

    public void renderBlockArea( BlockArea area )
    {
        this.currentPosY -= ( area.getPaddingTop()
                              + area.getBorderTopWidth() );

        drawFrame( area );

        renderChildren( area );

        this.currentPosY -= ( area.getPaddingBottom()
                              + area.getBorderBottomWidth() );
    }

    /**
     * render the display space area
     *
     * @param space display space passed from FOP framework
     */

    public void renderDisplaySpace( DisplaySpace space )
    {
        currentPosY -= space.getSize();
    }

    /**
     * render the given SVG area -- Empty
     *
     * @param area area passed from FOP framework
     */

    public void renderSVGArea( SVGArea area )
    {
        currentPosX += area.getContentWidth();
    }

    /**
     * render a foreign object area -- Empty
     *
     * @param area area passed from FOP framework
     */

    public void renderForeignObjectArea( ForeignObjectArea area ) { }

    /**
     * render the given image area -- Empty
     *
     * @param area area passed from FOP framework
     */

    public void renderImageArea( ImageArea area )
    {
        currentPosY -= area.getHeight();
    }

    /**
     * render the given word area
     *
     * @param area word area passed from FOP framework
     */

    public void renderWordArea( WordArea area )
    {
        FontState fontState = area.getFontState();

        SWFFontMetric fontMetric;

        try
        {
            fontMetric = ( SWFFontMetric )
                         fontState.getFontInfo().getMetricsFor( fontState.getFontName() );
        }
        catch ( FOPException e )
        {
            MessageHandler.errorln( "Failed to get metrics for font \""
                                    + fontState.getFontName()
                                    + "\", using default font." );

            fontMetric = new SWFFontMetric( "Arial.fft" );
        }

        flashMovie.addText( area.getText(),
                            this.currentPosX,
                            this.currentPosY,
                            area.getRed(),
                            area.getGreen(),
                            area.getBlue(),
                            fontMetric,
                            fontState.getFontSize(),
                            area.getContentWidth() );

        addWordAreaLines( area, fontState.getFontSize() );

        this.currentPosX += area.getContentWidth();
    }

    protected void addWordAreaLines( WordArea area,
                                     int size )
    {
        int y;

        if ( area.getUnderlined() )
        {
            y = currentPosY - size / 14;

            flashMovie.addRect( currentPosX,
                                y,
                                area.getContentWidth(),
                                - size / 14,
                                area.getRed(),
                                area.getBlue(),
                                area.getGreen() );
        }

        if ( area.getOverlined() )
        {
            y = currentPosY + area.getFontState().getAscender() + 2 * ( size / 14 );

            flashMovie.addRect( currentPosX,
                                y,
                                area.getContentWidth(),
                                size / 14,
                                area.getRed(),
                                area.getBlue(),
                                area.getGreen() );
        }

        if ( area.getLineThrough() )
        {
            y = currentPosY + area.getFontState().getAscender() / 2;

            flashMovie.addRect( currentPosX,
                                y,
                                area.getContentWidth(),
                                - size / 14,
                                area.getRed(),
                                area.getBlue(),
                                area.getGreen() );
        }
    }

    /**
     * render the given inline space
     *
     * @param space inline space passed from FOP framework
     */

    public void renderInlineSpace( InlineSpace space )
    {
        this.currentPosX += space.getSize();
    }

    /**
     * render the given line area
     *
     * @param area line area passed from FOP framework
     */

    public void renderLineArea( LineArea area )
    {
        int x = currentACPosX + area.getStartIndent();
        int y = currentPosY;
        int w = area.getContentWidth();
        int h = area.getHeight();

        currentPosY -= area.getPlacementOffset();
        currentPosX = x;

        Enumeration e = area.getChildren().elements();

        while ( e.hasMoreElements() )
        {
            org.apache.fop.layout.Box box = ( org.apache.fop.layout.Box ) e.nextElement();

            if ( box instanceof InlineArea )
            {
                currentPosY = y - ( ( InlineArea ) box ).getYOffset();
            }
            else
            {
                currentPosY = y - area.getPlacementOffset();
            }

            box.render( this );
        }

        currentPosY = y - h;
    }

    /**
     * render the given page
     *
     * @param page page passed from FOP framework
     */

    public void renderPage( Page page )
    {
        BodyAreaContainer body;
        AreaContainer before, after;

        flashMovie.startPage( page.getWidth(), page.getHeight() );

        body = page.getBody();
        before = page.getBefore();
        after = page.getAfter();

        renderBodyAreaContainer( body );

        if ( before != null )
        {
            renderAreaContainer( before );
        }

        if ( after != null )
        {
            renderAreaContainer( after );
        }

        if ( page.hasLinks() )
        {
            Enumeration e = page.getLinkSets().elements();

            while ( e.hasMoreElements() )
            {
                LinkSet linkSet = ( LinkSet ) e.nextElement();

                linkSet.align();

                Enumeration f = linkSet.getRects().elements();

                while ( f.hasMoreElements() )
                {
                    LinkedRectangle rect = ( LinkedRectangle ) f.nextElement();

                    flashMovie.addLink( rect.getX(),
                                        rect.getY(),
                                        rect.getWidth(),
                                        rect.getHeight(),
                                        linkSet.getDest() );
                }
            }
        }
    }

    /**
     * render the leader area
     *
     * FIXME: Rule style is currently ignored.
     *
     * @param area leader area passed from FOP framework
     */

    public void renderLeaderArea( LeaderArea area )
    {
        flashMovie.addRect( currentPosX,
                            currentPosY,
                            area.getLeaderLength(),
                            area.getRuleThickness(),
                            area.getRed(),
                            area.getGreen(),
                            area.getBlue() );

        this.currentPosX += area.getContentWidth();
    }

    /**
     * Renders all child objects of an area.
     *
     * @param area area to render children on
     */

    protected void renderChildren( Area area )
    {
        Enumeration e = area.getChildren().elements();

        while ( e.hasMoreElements() )
        {
            org.apache.fop.layout.Box box = ( org.apache.fop.layout.Box ) e.nextElement();
            box.render( this );
        }
    }

    /**
     * Draws a frame of a area, filled or outlined.
     *
     * @param area area to draw frame around
     */

    private void drawFrame( Area area )
    {
        int x = this.currentACPosX;
        int y = this.currentPosY;
        int h = area.getContentHeight();
        int w = area.getContentWidth();

        if ( area instanceof BlockArea )
        {
            x += ( ( BlockArea ) area ).getStartIndent();
        }

        ColorType bg = area.getBackgroundColor();

        x = x - area.getPaddingLeft();
        y = y + area.getPaddingTop();
        w = w + area.getPaddingLeft() + area.getPaddingRight();
        h = h + area.getPaddingTop() + area.getPaddingBottom();

        if ( ( bg != null ) && ( bg.alpha() == 0 ) )
        {
            flashMovie.addRect( x, y - h, w, h, bg.red(), bg.green(), bg.blue() );
        }

        BorderAndPadding bp = area.getBorderAndPadding();

        if ( bp != null )
        {
            x = x - area.getBorderLeftWidth();
            y = y + area.getBorderTopWidth();
            w = w + area.getBorderLeftWidth() + area.getBorderRightWidth();
            h = h + area.getBorderTopWidth() + area.getBorderBottomWidth();

            ColorType borderColor;

            if ( area.getBorderTopWidth() != 0 )
            {
                borderColor = bp.getBorderColor( BorderAndPadding.TOP );

                flashMovie.addRect( x, y, w, - area.getBorderTopWidth(),
                                    borderColor.red(), borderColor.green(), borderColor.blue() );
            }

            if ( area.getBorderLeftWidth() != 0 )
            {
                borderColor = bp.getBorderColor( BorderAndPadding.LEFT );

                flashMovie.addRect( x, y, area.getBorderLeftWidth(), - h,
                                    borderColor.red(), borderColor.green(), borderColor.blue() );
            }

            if ( area.getBorderRightWidth() != 0 )
            {
                borderColor = bp.getBorderColor( BorderAndPadding.RIGHT );

                flashMovie.addRect( x + w, y, - area.getBorderRightWidth(), - h,
                                    borderColor.red(), borderColor.green(), borderColor.blue() );
            }

            if ( area.getBorderBottomWidth( ) != 0 )
            {
                borderColor = bp.getBorderColor( BorderAndPadding.BOTTOM );

                flashMovie.addRect( x, y - h, w, area.getBorderBottomWidth(),
                                borderColor.red(), borderColor.green(), borderColor.blue() );
            }
        }
    }

    // LASZLO
    public void render(org.apache.fop.layout.AreaTree t, java.io.OutputStream s) {
    }


}
