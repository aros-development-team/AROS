/*
 * $Id: ShapeRecords.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

package org.openlaszlo.iv.flash.api.shape;

import java.io.PrintStream;
import java.awt.geom.*;
import java.util.*;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.parser.*;

/**
 * Shape records.
 * <P>
 * Provides basic functionality for drawing geometric primitives.
 *
 * @author Dmitry Skavish
 */
public class ShapeRecords extends FlashItem {

    private IVVector shape_records;

    // last drawing positions, used to keep track of drawing pen
    private int last_x = Integer.MAX_VALUE;
    private int last_y = Integer.MAX_VALUE;

    public ShapeRecords() {
        this( new IVVector() );
    }

    public ShapeRecords( IVVector shape_records ) {
        this.shape_records = shape_records;
    }

    /**
     * Returns vector of shape records.
     *
     * @return vector of shape records
     */
    public IVVector getShapeRecords() {
        return shape_records;
    }

    /**
     * Draws curve record.<p>
     * All coordinates are in twixels.
     *
     * @param cx     X control point
     * @param cy     Y control point
     * @param ax     X anchor point
     * @param ay     Y anchor point
     */
    public void drawCurveTo( int cx, int cy, int ax, int ay ) {
        shape_records.addElement( new CurvedEdgeRecord(cx-last_x, cy-last_y, ax-cx, ay-cy) );
        last_x = ax;
        last_y = ay;
    }

    /**
     * Draws curve record.<p>
     * All coordinates are in twixels.
     *
     * @param ax1    X anchor point 1
     * @param ay1    Y anchor point 1
     * @param cx     X control point
     * @param cy     Y control point
     * @param ax2    X anchor point 2
     * @param ay2    Y anchor point 2
     */
    public void drawCurve( int ax1, int ay1, int cx, int cy, int ax2, int ay2 ) {
        movePenTo(ax1, ay1);
        drawCurveTo(cx, cy, ax2, ay2);
    }

    /**
     * Draws curve record.<p>
     * All coordinates are in twixels.
     *
     * @param anchor0 first anchor point
     * @param control control point
     * @param anchor1 second anchor point
     */
    public void drawCurve( Point2D anchor1, Point2D control, Point2D anchor2 ) {
        drawCurve( (int) anchor1.getX(), (int) anchor1.getY(),
                   (int) control.getX(), (int) control.getY(),
                   (int) anchor2.getX(), (int) anchor2.getY() );
    }

    /**
     * Draws a straight line from current position to the specified one.<p>
     * All coordinates are in twixels.
     *
     * @param x      X of end of line
     * @param y      Y of end of line
     */
    public void drawLineTo( int x, int y ) {
        int deltaX = x-last_x;
        int deltaY = y-last_y;
        if( deltaX == 0 ) {
            if( deltaY == 0 ) return;
            shape_records.addElement( StrightEdgeRecord.newVLine(deltaY) );
        } else if( deltaY == 0 ) {
            shape_records.addElement( StrightEdgeRecord.newHLine(deltaX) );
        } else {
            shape_records.addElement( StrightEdgeRecord.newLine(deltaX,deltaY) );
        }
        last_x = x;
        last_y = y;
    }

    /**
     * Draws a straight line from current position to the specified one.<p>
     * All coordinates are in twixels.
     *
     * @param p1     end of line
     */
    public void drawLineTo( Point2D p1 ) {
        drawLineTo( (int) p1.getX(), (int) p1.getY() );
    }

    /**
     * Draws a straight line specified by two points.
     * <P>
     * All coordinates are in twixels.
     *
     * @param x1     X of the beginning of the line
     * @param y1     Y of the beginning of the line
     * @param x2     X of the end of the line
     * @param y2     Y of the end of the line
     */
    public void drawLine( int x1, int y1, int x2, int y2 ) {
        movePenTo( x1, y1 );
        drawLineTo( x2, y2 );
    }

    /**
     * Draws a straight line specified by two points.
     * <P>
     * All coordinates are in twixels.
     *
     * @param p0     first point
     * @param p1     second point
     */
    public void drawLine( Point2D p0, Point2D p1 ) {
        drawLine( (int) p0.getX(), (int) p0.getY(), (int) p1.getX(), (int) p1.getY() );
    }

    /**
     * Draws a rectangle specified by its top-left corner and width and height
     * <p>
     * All coordinates are in twixels.
     *
     * @param x      x coordinates of top-left corner of the rectangle
     * @param y      y coordinates of top-left corner of the rectangle
     * @param width  width of the rectangle
     * @param height height of the rectangle
     */
    public void drawRectangle( int x, int y, int width, int height ) {
        movePenTo( x, y );
        drawLineTo( x+width, y );
        drawLineTo( x+width, y+height );
        drawLineTo( x, y+height );
        drawLineTo( x, y );
    }

    /**
     * Draws a rectangle specified by {@link java.awt.geom.Rectangle2D}
     * <p>
     * All coordinates are in twixels.
     *
     * @param r      specified rectangle
     */
    public void drawRectangle( Rectangle2D r ) {
        drawRectangle( (int) r.getX(), (int) r.getY(), (int) r.getWidth(), (int) r.getHeight() );
    }

    /**
     * Moves pen to the specified position.<p>
     * All coordinates are ABSOLUTE and are in twixels.
     *
     * @param x      new current X
     * @param y      new current Y
     */
    public void movePenTo( int x, int y ) {
        if( last_x != x || last_y != y ) {
            StyleChangeRecord sc = getStyleChange();
            sc.addFlags( StyleChangeRecord.MOVETO );
            sc.setDeltaX(x);
            sc.setDeltaY(y);
            last_x = x;
            last_y = y;
        }
    }

    /**
     * Moves pen to the specified point.<p>
     * All coordinates are ABSOLUTE and are in twixels!
     *
     * @param p     new pen position
     */
    public void movePenTo( Point2D p ) {
        movePenTo( (int) p.getX(), (int) p.getY() );
    }

    /**
     * Returns current pen position
     *
     * @return current pen position
     */
    public Point2D getCurrentPos() {
        return new Point2D.Double(last_x, last_y);
    }

    /**
     * Returns first pen position (first moveTo)
     *
     * @return first pen position
     */
    public Point2D getFirstPos() {
        for( int i=0; i<shape_records.size(); i++ ) {
            FlashItem item = (FlashItem) shape_records.elementAt(i);
            if( item instanceof StyleChangeRecord ) {
                StyleChangeRecord sr = (StyleChangeRecord) item;
                if( (sr.getFlags()&StyleChangeRecord.MOVETO) == 0 ) continue;
                return new Point2D.Double(sr.getDeltaX(), sr.getDeltaY());
            }

        }
        return new Point2D.Double(0,0);
    }

    /**
     * Draw AWT Shape
     * <P>
     * All shape coordinates are in twixels!
     *
     * @param shape  AWT shape
     */
    public void drawAWTShape( java.awt.Shape shape ) {
        PathIterator pi = shape.getPathIterator(null);
        drawAWTPathIterator(pi);
    }

    /**
     * Draw AWT Shape
     * <P>
     * All shape coordinates are in twixels!
     *
     * @param shape  AWT shape
     */
    public void drawAWTShape( java.awt.Shape shape, AffineTransform matrix ) {
        PathIterator pi = shape.getPathIterator(matrix);
        drawAWTPathIterator(pi);
    }

    /**
     * Draw AWT PathIterator
     * <P>
     * All coordinates are in twixels!
     *
     * @param pi   AWT PathIterator
     */
    public void drawAWTPathIterator( java.awt.geom.PathIterator pi ) {
        double[] coords = new double[6];

        int last_move_x = 0;
        int last_move_y = 0;

        while( !pi.isDone() ) {
            int type = pi.currentSegment( coords );
            switch( type ) {
                case PathIterator.SEG_MOVETO: {
                    last_move_x = (int) coords[0];
                    last_move_y = (int) coords[1];
                    movePenTo( last_move_x, last_move_y );
                    break;
                }
                case PathIterator.SEG_CLOSE: {
                    drawLineTo( last_move_x, last_move_y );
                    break;
                }
                case PathIterator.SEG_LINETO: {
                    int x = (int) coords[0];
                    int y = (int) coords[1];
                    drawLineTo( x, y );
                    break;
                }
                case PathIterator.SEG_QUADTO: {
                    int cx = (int) coords[0];
                    int cy = (int) coords[1];
                    int ax = (int) coords[2];
                    int ay = (int) coords[3];
                    drawCurveTo( cx, cy, ax, ay );
                    break;
                }
                case PathIterator.SEG_CUBICTO: {
                    double c1x = coords[0];
                    double c1y = coords[1];
                    double c2x = coords[2];
                    double c2y = coords[3];
                    double ax  = coords[4];
                    double ay  = coords[5];

                    // construct coefficients of qubic curve
                    Point2D p0 = new Point2D.Double(last_x, last_y);
                    Point2D p1 = new Point2D.Double(c1x, c1y);
                    Point2D p2 = new Point2D.Double(c2x, c2y);
                    Point2D p3 = new Point2D.Double(ax, ay);

                    // break cubic curve into qudratic ones
                    Point2D[] quad_curves = GeomHelper.CubicToQudratricBezier(p0, p1, p2, p3);

                    // draw resulting quadratic curves
                    for( int i=0; i<quad_curves.length; i+=3 ) {
                        Point2D anchor1 = quad_curves[i];
                        Point2D control = quad_curves[i+1];
                        Point2D anchor2 = quad_curves[i+2];
                        drawCurve(anchor1, control, anchor2);
                    }
                    break;
                }
            }
            pi.next();
        }
    }

    /**
     * Add style change record
     *
     * @param scr    add this record
     */
    public StyleChangeRecord addStyleChangeRecord( StyleChangeRecord scr ) {
        shape_records.addElement(scr);
        return scr;
    }

    /**
     * Add style change record
     */
    public StyleChangeRecord addStyleChangeRecord() {
        StyleChangeRecord scr = new StyleChangeRecord();
        shape_records.addElement(scr);
        return scr;
    }

    protected StyleChangeRecord getStyleChange() {
        if( shape_records.size() > 0 ) {
            FlashItem item = (FlashItem) shape_records.elementAt( shape_records.size()-1 );
            if( item instanceof StyleChangeRecord ) return (StyleChangeRecord) item;
        }
        return addStyleChangeRecord();
    }

    public static ShapeRecords parse( Parser p ) {
        return new ShapeRecords(parseShapeRecords(p));
    }

    public static IVVector parseShapeRecords( Parser p ) {
        IVVector shape_records = new IVVector();

        // parse shape records
        int nBits = p.getUByte();
        int nFillBits = (nBits&0xf0)>>4;
        int nLineBits = nBits&0x0f;
        p.initBits();
        for(;;) {
            if( p.getBool() ) { // edge record
                if( p.getBool() ) { // stright edge
                    int nb = p.getBits(4)+2;
                    if( p.getBool() ) { // general line
                        int deltaX = p.getSBits(nb);
                        int deltaY = p.getSBits(nb);
                        shape_records.addElement( StrightEdgeRecord.newLine(deltaX, deltaY) );
                    } else if( p.getBool() ) {  // vertical line
                        int deltaY = p.getSBits(nb);
                        shape_records.addElement( StrightEdgeRecord.newVLine(deltaY) );
                    } else {    // horizontal line
                        int deltaX = p.getSBits(nb);
                        shape_records.addElement( StrightEdgeRecord.newHLine(deltaX) );
                    }
                } else { // curved edge
                    int nb = p.getBits(4)+2;
                    int cx = p.getSBits(nb);
                    int cy = p.getSBits(nb);
                    int ax = p.getSBits(nb);
                    int ay = p.getSBits(nb);
                    shape_records.addElement( new CurvedEdgeRecord(cx, cy, ax, ay) );
                }
            } else { // style-change record (non-edge)
                int flags = p.getBits(5);
                if( flags == 0 ) break; // end record
                StyleChangeRecord scr = new StyleChangeRecord();
                scr.setFlags( flags );

                if( (flags & StyleChangeRecord.MOVETO) != 0 ) {
                    int nMoveBits = p.getBits(5);
                    scr.setDeltaX( p.getSBits(nMoveBits) );
                    scr.setDeltaY( p.getSBits(nMoveBits) );
                }

                if( (flags & StyleChangeRecord.FILLSTYLE0) != 0 ) {
                    scr.setFillStyle0( p.getBits(nFillBits) );
                }

                if( (flags & StyleChangeRecord.FILLSTYLE1) != 0 ) {
                    scr.setFillStyle1( p.getBits(nFillBits) );
                }

                if( (flags & StyleChangeRecord.LINESTYLE) != 0 ) {
                    scr.setLineStyle( p.getBits(nLineBits) );
                }

                shape_records.addElement( scr );

                if( (flags&0x80) != 0 ) {
                    break;
                }
            }
        }

        return shape_records;
    }

    /**
     * Writes this shape records into specified buffer, writes end of records and flush the bits.
     * Number of fill and line style bits is considered to be 0 (zero).
     *
     * @param fob    buffer
     */
    public void write( FlashOutput fob ) {
        fob.initBits();
        shape_records.write(fob);
        fob.writeBits(0, 6);
        fob.flushBits();
    }

    /**
     * Writes this shape records into specified buffer but does not flush last bits and
     * does not write end of record, it has to be done separatedly.
     *
     * @param fob       specified buffer
     * @param nFillBits number of fill bits
     * @param nLineBits number of line bits
     */
    public void write( FlashOutput fob, int nFillBits, int nLineBits ) {
        fob.initBits();
        for( int i=0; i<shape_records.size(); i++ ) {
            FlashItem item = (FlashItem) shape_records.elementAt(i);
            if( item instanceof StyleChangeRecord ) {
                StyleChangeRecord scr = (StyleChangeRecord) item;
                scr.write(fob, nFillBits, nLineBits);
            } else {
                item.write(fob);
            }
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"ShapeRecords:" );
        shape_records.printContent(out, indent+"    ");
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((ShapeRecords)item).shape_records = shape_records.getCopy(copier);
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new ShapeRecords(null), copier );
    }
}

