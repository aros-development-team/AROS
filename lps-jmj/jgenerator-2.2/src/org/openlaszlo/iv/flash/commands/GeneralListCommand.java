/*
 * $Id: GeneralListCommand.java,v 1.7 2002/07/15 22:39:32 skavish Exp $
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

import java.io.*;
import java.awt.geom.*;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.context.Context;

public class GeneralListCommand extends GenericCommand {

    protected String  datasource;
    protected String  halign;
    protected String  valign;
    protected boolean mask;
    protected int     itemspace;
    protected String  instancename;
    protected boolean isVertical;
    protected boolean isFixed;

    protected int listSize; // in twips
    protected int winWidth;
    protected int winHeight;

    protected boolean isSetFileSize;
    protected int fileWidthAdd;
    protected int fileHeightAdd;

    protected String scrollPaneName;

    protected void initParms( Context context ) throws IVException {
        datasource   = getParameter( context, "datasource", "" );
        halign       = getParameter( context, "halign" );
        valign       = getParameter( context, "valign" );
        mask         = getBoolParameter( context, "mask", true );
        itemspace    = getIntParameter( context, "itemspace", 0 ) * 20;
        instancename = getParameter( context, "instancename" );

        // mx stuff
        scrollPaneName = getParameter( context, "scrollpanename" );
        if( scrollPaneName != null && scrollPaneName.length() == 0 ) scrollPaneName = null;

        String orient = getParameter( context, "orient", "horizontal" );
        String spacing = getParameter( context, "spacing", "auto" );
        isVertical = orient.equalsIgnoreCase("vertical");
        isFixed    = spacing.equalsIgnoreCase("fixed");

        if( (isSetFileSize=getBoolParameter(context, "setfilesize", false)) ) {
            fileWidthAdd = getIntParameter(context, "filewidthadd", 0)*20;
            fileHeightAdd = getIntParameter(context, "fileheightadd", 0)*20;
        }
    }

    protected Script makeList( FlashFile file, Context context, Script parent, int frameNum ) throws IVException {
        String[][] data;
        try {
            UrlDataSource ds = new UrlDataSource(datasource,file);
            data = ds.getData();
        } catch( IOException e ) {
            throw new IVException(Resource.ERRDATAREAD, new Object[] {datasource, getCommandName()}, e);
        }

        if( data.length < 1 ) {
            throw new IVException(Resource.INVALDATASOURCE, new Object[] {datasource, getCommandName()});
        }

        Instance mainInst = getInstance();
        Rectangle2D winBounds = GeomHelper.getTransformedSize( mainInst.matrix,
            GeomHelper.newRectangle(-1024, -1024, 2048, 2048) ); // mask of the list
        winWidth = (int) winBounds.getWidth();
        winHeight = (int) winBounds.getHeight();

        int clipIdx = findColumn( "clip", data );
        int spaceIdx = findColumn( "space", data );
        int instancenameIdx = findColumn( "instancename", data );
        if( clipIdx == -1 ) {
            throw new IVException(Resource.COLNOTFOUNDCMD, new Object[] {getCommandName(), "Clip", datasource});
        }

        Script listScript = new Script(1);
        Frame frame = listScript.newFrame();
        int x = 0, y = 0;

        // process datasource
        for( int row=1; row<data.length; row++ ) {
            Context myContext = makeContext( context, data, row );
            String clipName = data[row][clipIdx];
            Script template = file.getScript(clipName);
            if( template == null ) {
                Log.logRB( Resource.CMDSCRIPTNOTFOUND, new Object[] {clipName, "List"} );
            } else {
                Script myScript = template.copyScript();
                file.processScript( myScript, myContext );

                int delta = 0;
                if( spaceIdx != -1 ) {
                    delta = Util.toInt( data[row][spaceIdx], 0 ) * 20;
                }
                Instance scInst = frame.addInstance(myScript, row, null, null);
                if( instancenameIdx != -1 ) {
                    scInst.name = data[row][instancenameIdx];
                }

                Rectangle2D bounds = myScript.getBounds();
                // it turned out that we need only width and height (because of alignment parameters)
                int myX = 0;    // (int) bounds.getX();
                int myY = 0;    // (int) bounds.getY();
                int myWidth = (int) bounds.getWidth();
                int myHeight = (int) bounds.getHeight();

                int dx = x;
                int dy = y;

                // calculate next item's coordinates
                if( isVertical ) {
                    if( !isFixed ) {
                        delta += myHeight;
                    }
                    delta += itemspace;
                    y += delta;
                } else {
                    if( !isFixed ) {
                        delta += myWidth;
                    }
                    delta += itemspace;
                    x += delta;
                }

                // align current item
                double shiftX, shiftY;
                if( isVertical ) {
                    shiftX = winWidth;
                    shiftY = myHeight;
                } else {
                    shiftX = myWidth;
                    shiftY = winHeight;
                }
                if( halign.equalsIgnoreCase("right") ) {
                    dx += shiftX;
                } else if( halign.equalsIgnoreCase("center") ) {
                    dx += shiftX/2;
                }

                if( valign.equalsIgnoreCase("bottom") ) {
                    dy += shiftY;
                } else if( valign.equalsIgnoreCase("center") ) {
                    dy += shiftY/2;
                }

                // set matrix
                AffineTransform matrix = AffineTransform.getTranslateInstance(dx-myX,dy-myY);
                scInst.matrix = matrix;
            }
        }

        // calculate list size in twips
        if( isVertical ) {
            listSize = y;
        } else {
            listSize = x;
        }

        GeomHelper.deScaleMatrix( mainInst.matrix );
        mainInst.matrix.translate( -winWidth/2, -winHeight/2 );

        if( instancename != null ) {
            mainInst.name = instancename;
        }

        if( isSetFileSize ) {
            int width = fileWidthAdd, height = fileHeightAdd;
            if( isVertical ) {
                height += listSize;
                width += winWidth;
            } else {
                width += listSize;
                height += winHeight;
            }
            width = ((width + 19)/20)*20;
            height = ((height + 19)/20)*20;
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
