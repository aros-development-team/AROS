/*
 * $Id: TableCommand.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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
 * 03/23/2001 fixed some problems with help from Patrick Talbot
 * 12/11/2001 fixed alignment of symbols in table cells (now bounds are taken into account)
 */

package org.openlaszlo.iv.flash.commands;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.context.Context;
import java.io.*;
import java.text.*;
import java.util.*;
import java.awt.geom.*;

/**
 * Insert Table command
 *
 */
public class TableCommand extends GenericCommand {

    protected String  datasource;       // datasource
    protected String  halign;           // left, center, right
    protected String  valign;           // top, center, bottom
    protected int     rows, cols;       // number of rows and columns
    protected double  contentscale;     // auto, fixed, half, double
    protected String  defsym;           // default symbol name
    protected String  labelformat;      // Arial, Times, Courier, name of the font
    protected double  labelscale;       // auto, fixed, half, double
    protected int     labelsize;        // size of label text
    protected boolean borders;          // true, false
    protected AlphaColor bordercolor;   // border color
    protected int     borderthickness;  // thickness of border in twips
    protected String  mediafile;        // external symbol file
    protected String  instancename;     // instancename
    protected String[] rlabels;         // labels for rows
    protected String[] clabels;         // labels for columns

    protected int tableSize; // in twips
    protected int cellWidth, cellHeight;
    protected int winWidth;
    protected int winHeight;

    public TableCommand() {}

    protected String[] parseLabels( String s ) {
        if( s == null || s.length() == 0 ) return null;
        StringTokenizer st = new StringTokenizer(s, ",\t\n \r");
        IVVector v = new IVVector();
        while( st.hasMoreTokens() ) {
            v.addElement( st.nextToken() );
        }
        String[] rs = new String[v.size()];
        v.copyInto(rs);
        return rs;
    }

    protected double parseLabelSizing( String s ) {
        try {
            labelsize = Integer.parseInt(s) * 20;
            return 1.0;
        } catch( NumberFormatException e ) {
            labelsize = 18 * 20;
            return parseSizing(s);
        }
    }

    protected double parseSizing( String s ) {
        if( s.equalsIgnoreCase("fixed")  ) return 1.0;
        if( s.equalsIgnoreCase("auto")   ) return -1.0;
        if( s.equalsIgnoreCase("half")   ) return 0.5;
        if( s.equalsIgnoreCase("double") ) return 2.0;
        return Util.toDouble(s, -1.0);
    }

    protected void initParms( Context context ) throws IVException {
        datasource      = getParameter( context, "datasource", "" );
        halign          = getParameter( context, "halign" );
        valign          = getParameter( context, "valign" );
        rows            = getIntParameter( context, "rows", 0 );
        cols            = getIntParameter( context, "cols", 0 );
        contentscale    = parseSizing( getParameter( context, "sizing" ) );
        defsym          = getParameter( context, "defsym" );
        labelformat     = getParameter( context, "labelformat" );
        labelscale      = parseLabelSizing( getParameter( context, "labelsizing" ) );
        rlabels         = parseLabels( getParameter( context, "rlabels" ) );
        clabels         = parseLabels( getParameter( context, "clabels" ) );
        mediafile       = getParameter( context, "mediafile" );
        borders         = getBoolParameter( context, "borders", true );
        bordercolor     = getColorParameter( context, "bordercolor", AlphaColor.black );
        borderthickness = borders? getIntParameter( context, "borderthickness", 20 ): 0;
        instancename = getParameter( context, "instancename" );
    }

    protected Instance putSymbolInCell( Frame frame, FlashDef sym, Rectangle2D rect, int layer, double scale, boolean prop ) {
        double scaleX, scaleY, translateX = rect.getMinX(), translateY = rect.getMinY();

        Rectangle2D bounds = sym.getBounds();
        double x = bounds.getX();
        double y = bounds.getY();
        if( scale < 0.0 ) {
            double xk = cellWidth/bounds.getWidth();
            double yk = cellHeight/bounds.getHeight();
            if( prop ) {
                double k = Math.min(xk,yk);
                scaleX = scaleY = k;
            } else {
                scaleX = xk;
                scaleY = yk;
            }
        } else {
            scaleX = scaleY = scale;
        }

        //AffineTransform m = AffineTransform.getTranslateInstance(translateX,translateY);
        //m.scale(scaleX, scaleY);
        //m.translate(-x,-y);
        AffineTransform m = new AffineTransform(scaleX, 0, 0, scaleY, translateX-scaleX*x, translateY-scaleY*y);

        return frame.addInstance(sym, layer, m, null);
    }

    protected Rectangle2D getCellRect( int col, int row ) {
        int xmin = borderthickness+(cellWidth+borderthickness)*col;
        int ymin = borderthickness+(cellHeight+borderthickness)*row;

        return GeomHelper.newRectangle(xmin,ymin,cellWidth,cellHeight);
    }

    protected Script makeTable( FlashFile file, Context context, Script parent, int frameNum ) throws IVException {

        String[][] data;
        try {
            UrlDataSource ds = new UrlDataSource(datasource,file);
            data = ds.getData();
        } catch( IOException e ) {
            throw new IVException(Resource.ERRDATAREAD, new Object[] {datasource, getCommandName()}, e);
        }

        if( data.length < 1 ) {
            throw new IVException( Resource.INVALDATASOURCE, new Object[] {datasource, getCommandName()} );
        }

        Instance mainInst = getInstance();
        Rectangle2D winBounds = GeomHelper.getTransformedSize( mainInst.matrix,
            GeomHelper.newRectangle(-1024, -1024, 2048, 2048) ); // mask of the list
        winWidth = (int) winBounds.getWidth();
        winHeight = (int) winBounds.getHeight();

        int clipIdx = findColumn( "clip", data );

        if( rows <= 0 || cols <= 0 ) {
            throw new IVException( Resource.ROWSORCOLS, new Object[] {getCommandName()} );
        }

        boolean isLabels = rlabels != null || clabels != null;
        if( defsym != null || isLabels ) {
            // loading symbol file
            if( mediafile != null ) {
                try {
                    file.addExternalFile( mediafile, true );
                } catch( IVException e ) {
                    Log.log( e );
                }
            }
            // load default symbol file
            file.getDefaultSymbolFile();
        }


        Script tableScript = new Script(1);
        Frame frame = tableScript.newFrame();

        int totalRows = rows;
        int totalCols = cols;
        if( rlabels != null ) totalCols++;
        if( clabels != null ) totalRows++;

        // calculate cell width and height
        cellWidth  = (winWidth-borderthickness*(totalCols+1)) / totalCols;
        cellHeight = (winHeight-borderthickness*(totalRows+1)) / totalRows;

        if( cellWidth <= 0 || cellHeight <= 0 ) {
            Log.logRB( Resource.BORDERTOOTHICK );
        }

        // buidl grid cells
        Rectangle2D[][] cellRects = new Rectangle2D[totalCols][totalRows];
        for( int c=0; c<totalCols; c++ ) {
            for( int r=0; r<totalRows; r++ ) {
                cellRects[c][r] = getCellRect( c, r );
            }
        }
        // draw grid (borders)
        if( borders ) {
            Shape shape = new Shape();
            int fullColor = shape.addLineStyle( new LineStyle(25, bordercolor) );
            int reducedColor = shape.addLineStyle( new LineStyle(25, bordercolor.getReducedColor()) );
            // draw cells with full color
            shape.setLineStyle(fullColor);
            for( int c=0; c<totalCols; c++ ) {
                for( int r=0; r<totalRows; r++ ) {
                    Rectangle2D rect = cellRects[c][r];
                    shape.movePenTo((int)rect.getMinX(), (int)rect.getMaxY());
                    shape.drawLineTo((int)rect.getMinX(), (int)rect.getMinY());
                    shape.drawLineTo((int)rect.getMaxX(), (int)rect.getMinY());
                }
            }
            // draw main table with full color
            shape.movePenTo(winWidth, 0);
            shape.drawLineTo(winWidth, winHeight);
            shape.drawLineTo(0, winHeight);

            // draw cells with reduced color
            shape.setLineStyle(reducedColor);
            for( int c=0; c<totalCols; c++ ) {
                for( int r=0; r<totalRows; r++ ) {
                    Rectangle2D rect = cellRects[c][r];
                    shape.movePenTo((int)rect.getMaxX(), (int)rect.getMinY());
                    shape.drawLineTo((int)rect.getMaxX(), (int)rect.getMaxY());
                    shape.drawLineTo((int)rect.getMinX(), (int)rect.getMaxY());
                }
            }
            // draw main table with reduced color
            shape.movePenTo(0, winHeight);
            shape.drawLineTo(0, 0);
            shape.drawLineTo(winWidth, 0);

            // add shape to frame
            frame.addInstance(shape, 2, null, null);
            shape.setBounds( 0, 0, winWidth, winHeight );
        }

        Script defSymbol = null;
        if( defsym != null ) {
            defSymbol = file.getScript(defsym);
        }

        Font labelFont = null;
        if( isLabels ) {
            if( labelformat == null ) labelformat = "Arial";
            // map generator font names to font names from DefaultSymbolFile
            if( labelformat.equals( "Times" ) ) labelformat = "Times New Roman";
            else if( labelformat.equals( "Courier" ) ) labelformat = "Courier New";

            labelFont = getFont( file, labelformat );
        }

        if( clipIdx == -1 && defSymbol == null ) {
            throw new IVException( Resource.CLIPORDEFSYM );
        }

        int col = 0, row = 0;
        int layer = 3;
        int rbase = clabels != null? 1: 0;
        int cbase = rlabels != null? 1: 0;

        // draw labels
        if( labelFont != null ) {
            if( rlabels != null ) {
                int nLabels = Math.min(totalRows, rlabels.length);
                for( int i=0; i<nLabels; i++ ) {
                    String label = rlabels[i];
                    Rectangle2D r = cellRects[0][i+rbase];
                    Text text = newText( file, label, labelFont, labelsize, AlphaColor.black, (int) r.getWidth(), (int) r.getHeight() );
                    ((TextItem)text.getTextItems().elementAt(0)).align = 2; // center
                    putSymbolInCell( frame, text, r, layer++, labelscale, true );
                }
                col++;
            }
            if( clabels != null ) {
                int nLabels = Math.min(totalCols, clabels.length);
                for( int i=0; i<nLabels; i++ ) {
                    String label = clabels[i];
                    Rectangle2D r = cellRects[i+cbase][0];
                    Text text = newText( file, label, labelFont, labelsize, AlphaColor.black, (int) r.getWidth(), (int) r.getHeight() );
                    ((TextItem)text.getTextItems().elementAt(0)).align = 2; // center
                    putSymbolInCell( frame, text, r, layer++, labelscale, true );
                }
                row++;
            }
        }

        // process datasource
        for( int r=1; r<data.length; r++ ) {
            Context myContext = makeContext( context, data, r );
            Script template;
            if( defSymbol != null ) {
                template = defSymbol;
            } else {
                String clipName = data[r][clipIdx];
                template = file.getScript(clipName);
                if( template == null ) {
                    throw new IVException( Resource.CMDSCRIPTNOTFOUND, new Object[] {clipName, getCommandName()} );
                }
            }
            Script myScript = template.copyScript();
            file.processScript( myScript, myContext );

            Instance myInst = putSymbolInCell(frame, myScript, cellRects[col][row], layer++, contentscale, true);

            if( !halign.equalsIgnoreCase("left") && !valign.equalsIgnoreCase("top") ) {
                double translateX = 0.0, translateY = 0.0;
                Rectangle2D bounds = myInst.getBounds();
                double width = bounds.getWidth();
                double height = bounds.getHeight();

                if( halign.equalsIgnoreCase("right") ) {
                    translateX = cellWidth-width;
                } else if( halign.equalsIgnoreCase("center") ) {
                    translateX = (cellWidth-width)/2;
                }

                if( valign.equalsIgnoreCase("bottom") ) {
                    translateY = cellHeight-height;
                } else if( valign.equalsIgnoreCase("center") ) {
                    translateY = (cellHeight-height)/2;
                }

                myInst.matrix.preConcatenate(AffineTransform.getTranslateInstance(translateX,translateY));
            }

            col++;
            if( col >= totalCols ) {
                row++;
                col = cbase;
                if( row >= totalRows ) break;
            }

        }

        return tableScript;
    }

    public void doCommand( FlashFile file, Context context, Script parent, int frameNum ) throws IVException {
        initParms( context );

        Script tableScript = makeTable( file, context, parent, frameNum );

        GeomHelper.deScaleMatrix( getInstance().matrix );
        getInstance().matrix.translate(-winWidth/2, -winHeight/2);

        if( instancename != null ) {
            getInstance().name = instancename;
        }

        getInstance().setScript( tableScript );
        addMask(parent, frameNum, getInstance(), winWidth, winHeight);
    }

}
