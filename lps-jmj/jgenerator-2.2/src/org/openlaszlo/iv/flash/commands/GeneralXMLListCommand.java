/*
 * $Id: GeneralXMLListCommand.java,v 1.5 2002/07/15 22:39:32 skavish Exp $
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

/*
 * 12/11/2001 fixed alignment problems, bounds now taken into account
 *
 */

package org.openlaszlo.iv.flash.commands;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.context.*;

import java.io.*;
import java.util.*;
import java.awt.geom.*;

public class GeneralXMLListCommand extends GenericXMLCommand {

    public static final int STARTX = 0;     // 20
    public static final int STARTY = 0;     // 20

    protected boolean mask;
    protected String  instancename;
    protected boolean isFixed;

    protected int orient;

    protected static final int ORIENT_HORIZ = 0;
    protected static final int ORIENT_VERT = 1;
    protected static final int ORIENT_TEMPORAL = 2;

    protected int order;

    protected static final int ASCENDING_ORDER = 1;
    protected static final int DESCENDING_ORDER = -1;
    protected static final int NO_ORDER = 0;

    protected int listSize; // in twips
    protected int winWidth;
    protected int winHeight;

    protected String halign;
    protected String valign;
    protected String itemspace;
    protected String clip;
    protected String sortby;
    protected String itemname;

    protected String scrollPaneName;

    protected boolean isSetFileSize;
    protected int fileWidthAdd;
    protected int fileHeightAdd;

    protected void initParms( Context context ) throws IVException
    {
        super.initParms( context );

        mask = getBoolParameter( context, "mask", true );
        instancename = getParameter( context, "instancename" );

        // mx stuff
        scrollPaneName = getParameter( context, "scrollpanename" );
        if( scrollPaneName != null && scrollPaneName.length() == 0 ) scrollPaneName = null;

        String sorient = getParameter( context, "orient", "horizontal" );

        if ( sorient.equalsIgnoreCase( "vertical" ) )
        {
            orient = ORIENT_VERT;
        }
        else if ( sorient.equalsIgnoreCase( "temporal" ) )
        {
            orient = ORIENT_TEMPORAL;
        }
        else // default to horizontal
        {
            orient = ORIENT_HORIZ;
        }

        String spacing = getParameter( context, "spacing", "auto" );

        isFixed = spacing.equalsIgnoreCase("fixed");

        String sorder = getParameter( context, "order", "none" );

        if ( sorder.equalsIgnoreCase("descending") )
        {
            order = DESCENDING_ORDER;
        }
        else if ( sorder.equalsIgnoreCase("ascending") )
        {
            order = ASCENDING_ORDER;
        }
        else
        {
            order = NO_ORDER;
        }

        halign = getParameter( context, "halign", "'left'" );
        valign = getParameter( context, "valign", "'top'" );
        itemspace = getParameter( context, "itemspace", "0" );
        clip = getParameter( context, "clip", "name()" );
        sortby = getParameter( context, "sortby", "'none'" );
        itemname = getParameter( context, "itemname", null );

        if( (isSetFileSize=getBoolParameter(context, "setfilesize", false)) ) {
            fileWidthAdd = getIntParameter(context, "filewidthadd", 0)*20;
            fileHeightAdd = getIntParameter(context, "fileheightadd", 0)*20;
        }
    }

    protected Script makeList( FlashFile file, Context context, Script parent, int frameNum ) throws IVException
    {
        GraphContext gc = getGraphContext( file, context );

        if ( gc == null )
        {
            throw new IVException( Resource.NOGRAPHCONTEXT );
        }

        List contextList = gc.getValueList( select );

        if ( contextList == null )
        {
            throw new IVException( Resource.NOMATCHINGCONTEXTS, new Object[] {select} );
        }

        Instance mainInst = getInstance();
        Rectangle2D winBounds = GeomHelper.getTransformedSize( mainInst.matrix,
            GeomHelper.newRectangle(-1024, -1024, 2048, 2048) ); // mask of the list
        winWidth = (int) winBounds.getWidth();
        winHeight = (int) winBounds.getHeight();

        Script listScript = new Script(1);
        Frame frame = listScript.newFrame();

        if ( orient == ORIENT_TEMPORAL )
        {
            frame.addStopAction();
        }

        int x = STARTX, y = STARTY;

        // sort items

        if ( order != NO_ORDER )
        {
            contextList = GraphContext.sortValueList( contextList, sortby, order == ASCENDING_ORDER );
        }

        // process datasource

        Context myContext;
        ListIterator iter = contextList.listIterator();
        int i = 1;

        while( iter.hasNext() )
        {
            myContext = ( Context ) iter.next();

            String clipName = evalStringParameter( myContext, clip, "" );

            Script template = file.getScript( clipName );

            if( template == null )
            {
                Log.logRB( Resource.CMDSCRIPTNOTFOUND, new Object[] {clipName, getCommandName()} );
            }
            else
            {
                Script script = template.copyScript();

                file.processScript( script, myContext );

                int delta = 0;

                if( itemspace != null )
                {
                    delta = getIntParameter( myContext, "itemspace", 0 ) * 20;
                }

                // we will set matrix later
                Instance scInst = frame.addInstance( script, i, null, null );

                if( itemname != null )
                {
                    scInst.name = evalStringParameter( myContext, itemname, "item"+i );
                }

                Rectangle2D bounds = script.getBounds();
                int myX = (int) bounds.getX();
                int myY = (int) bounds.getY();
                int myWidth = (int) bounds.getWidth();
                int myHeight = (int) bounds.getHeight();

                int dx = x;
                int dy = y;

                if ( orient == ORIENT_VERT )
                {
                    if ( ! isFixed ) { delta += myHeight; }
                    y += delta;
                }
                else if ( orient == ORIENT_HORIZ )
                {
                    if ( ! isFixed ) { delta += myWidth; }
                    x += delta;
                }
                else if ( orient == ORIENT_TEMPORAL )
                {
                    frame = listScript.newFrame();
                }

                String s_halign = getParameter( myContext, halign, "left" );
                String s_valign = getParameter( myContext, valign, "top" );

                // alignment

                double shiftX, shiftY;

                if ( orient == ORIENT_VERT )
                {
                    shiftX = winWidth;
                    shiftY = myHeight;
                }
                else if ( orient == ORIENT_HORIZ )
                {
                    shiftX = myWidth;
                    shiftY = winHeight;
                }
                else
                {
                    shiftX = 0;
                    shiftY = 0;
                }

                if ( s_halign.equalsIgnoreCase( "right" ) )
                {
                    dx += shiftX;
                }
                else if ( s_halign.equalsIgnoreCase("center") )
                {
                    dx += shiftX / 2;
                }

                if ( s_valign.equalsIgnoreCase("bottom") )
                {
                    dy += shiftY;
                }
                else if ( s_valign.equalsIgnoreCase("center") )
                {
                    dy += shiftY / 2;
                }

                // set matrix
                AffineTransform matrix = AffineTransform.getTranslateInstance(dx-myX,dy-myY);
                scInst.matrix = matrix;
            }

            i++;
        }

        // calculate list size in twips

        if( orient == ORIENT_VERT )
        {
            listSize = y - STARTY;
        }
        else if ( orient == ORIENT_HORIZ )
        {
            listSize = x - STARTX;
        }

        GeomHelper.deScaleMatrix( mainInst.matrix );
        mainInst.matrix.translate( -winWidth/2, -winHeight/2 );

        if ( instancename != null )
        {
            mainInst.name = instancename;
        }

        if( isSetFileSize ) {
            int width = fileWidthAdd, height = fileHeightAdd;
            if( orient == ORIENT_VERT ) {
                height += listSize;
                width += winWidth;
            } else {
                width += listSize;
                height += winHeight;
            }
            file.setFrameSize(GeomHelper.newRectangle(0, 0, width, height));
        }

        return listScript;
    }

    protected void addMask( Script parent, int frameNum ) {
        addMask(parent, frameNum, getInstance());
    }

    protected void addMask( Script parent, int frameNum, Instance inst ) {
        if( mask ) {
            addMask(parent, frameNum, inst, winWidth, winHeight);
        }
    }
}
