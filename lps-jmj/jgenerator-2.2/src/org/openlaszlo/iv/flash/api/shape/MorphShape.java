/*
 * $Id: MorphShape.java,v 1.2 2002/03/14 20:27:38 awason Exp $
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

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;

/**
 * MorphShape.
 * <P>
 * The DefineMorphShape tag defines the start and end states of a morph sequence.
 * A morph object does not use previously defined shapes, it is considered a special
 * type of shape with only one character ID.  A morph object should be displayed with
 * the PlaceObject2 tag, where the ratio field specifies how far the morph has progressed.
 * <P>
 * StartBounds defines the bounding-box of the shape at the start of the morph,
 * and EndBounds defines the bounding-box at the end of the morph
 * <P>
 * MorphFillStyles contains an array interleaved fill styles for the start and end shapes.
 * The fill style for the start shape is followed the corresponding fill style for the end shape.
 * Similarly, MorphLineStyles contains an array of interleaved line styles.
 * <P>
 * The StartEdges array specifies the edges for the start shape, and the style change records for
 * both shapes.  Because the StyleChangeRecords must be the same for the start and end shapes,
 * they are defined only in the StartEdges array.  The EndEdges array specifies the edges for
 * the end shape, and contains no style change records. The number of edges specified in StartEdges
 * must equal the number of edges in EndEdges.
 * <P>
 * Note: Strictly speaking MoveTo records fall into the category of StyleChangeRecords,
 * however they should be included in both the StartEdges and EndEdges arrays.
 * <P>
 * It is possible for an edge to change type over the course of a morph sequence.
 * A straight edge can become a curved edge and vice versa.  In this case, think of
 * both edges as curved.  A straight edge can be easily represented as a curve,
 * by placing the off-curve (control) point at the mid-point of the straight edge,
 * and the on-curve (anchor) point at the end of the straight edge.
 * The calculation is as follows:
 * <PRE><CODE>
 * CurveControlDelta.x = StraightDelta.x / 2;
 * CurveControlDelta.y = StraightDelta.y / 2;
 * CurveAnchorDelta.x  = StraightDelta.x / 2;
 * CurveAnchorDelta.y  = StraightDelta.y / 2;
 * </CODE></PRE>
 *
 * @author Dmitry Skavish
 * @see Shape
 */
public final class MorphShape extends FlashDef {

    private MorphShapeStyles shape_styles;      // morph shape styles
    private ShapeRecords records_start;         // start records
    private ShapeRecords records_end;           // end records
    private Rectangle2D bounds_start;           // bounding box of start shape
    private Rectangle2D bounds_end;             // bounding box of end shape

    /**
     * Creates new empty morph shape
     */
    public MorphShape() {
    }

    /**
     * Returns shape styles
     *
     * @return object representing array of shape styles
     */
    public MorphShapeStyles getShapeStyles() {
        if( shape_styles == null ) {
            shape_styles = new MorphShapeStyles();
        }
        return shape_styles;
    }

    /**
     * Returns start shape records
     *
     * @return object representing array of shape records
     */
    public ShapeRecords getShapeRecordsStart() {
        if( records_start == null ) {
            records_start = new ShapeRecords();
        }
        return records_start;
    }

    /**
     * Returns end shape records
     *
     * @return object representing array of shape records
     */
    public ShapeRecords getShapeRecordsEnd() {
        if( records_end == null ) {
            records_end = new ShapeRecords();
        }
        return records_end;
    }

    /**
     * Adds specified fill style to the array of shape styles.
     *
     * @param fillStyle specified fill style
     * @return index of added fill style in the array
     */
    public int addFillStyle( MorphFillStyle fillStyle ) {
        return getShapeStyles().addFillStyle( fillStyle );
    }

    /**
     * Adds specified line style to the array of shape styles
     *
     * @param lineStyle specified line style
     * @return index of added line style in the array
     */
    public int addLineStyle( MorphLineStyle lineStyle ) {
        return getShapeStyles().addLineStyle( lineStyle );
    }

    /**
     * Sets specified fillstyle as current fillstyle0.
     * <P>
     * If the specified fillstyle is not in the shapestyle array
     * then it's added, if it's already there it's reused.
     *
     * @param fillStyle specified fillstyle
     * @return index of specified fillstyle in the shapestyle array
     */
    public int setFillStyle0( MorphFillStyle fillStyle ) {
        int fs = getShapeStyles().getFillStyleIndex( fillStyle );
        if( fs == -1 ) fs = addFillStyle( fillStyle );
        setFillStyle0( fs );
        return fs;
    }

    /**
     * Sets specified fillstyle as current fillstyle1.
     * <P>
     * If the specified fillstyle is not in the shapestyle array
     * then it's added, if it's already there it's reused.
     *
     * @param fillStyle specified fillstyle
     * @return index of specified fillstyle in the shapestyle array
     */
    public int setFillStyle1( MorphFillStyle fillStyle ) {
        int fs = getShapeStyles().getFillStyleIndex( fillStyle );
        if( fs == -1 ) fs = addFillStyle( fillStyle );
        setFillStyle1( fs );
        return fs;
    }

    /**
     * Sets specified linestyle as current linestyle.
     * <P>
     * If the specified linestyle is not in the shapestyle array
     * then it's added, if it's already there it's reused.
     *
     * @param lineStyle specified linestyle
     * @return index of specified linestyle in the shapestyle array
     */
    public int setLineStyle( MorphLineStyle lineStyle ) {
        int ls = getShapeStyles().getLineStyleIndex( lineStyle );
        if( ls == -1 ) ls = addLineStyle( lineStyle );
        setLineStyle( ls );
        return ls;
    }

    /**
     * Sets current fillstyle0 by its index in shapestyle array.
     *
     * @param fillStyle index of fillstyle in shapestyle array to be set as current fillstyle0
     */
    public void setFillStyle0( int fillStyle ) {
        StyleChangeRecord sc = getStyleChange();
        sc.addFlags( StyleChangeRecord.FILLSTYLE0 );
        sc.setFillStyle0( fillStyle );
    }

    /**
     * Sets current fillstyle1 by its index in shapestyle array.
     *
     * @param fillStyle index of fillstyle in shapestyle array to be set as current fillstyle1
     */
    public void setFillStyle1( int fillStyle ) {
        StyleChangeRecord sc = getStyleChange();
        sc.addFlags( StyleChangeRecord.FILLSTYLE1 );
        sc.setFillStyle1( fillStyle );
    }

    /**
     * Sets current linestyle by its index in shapestyle array.
     *
     * @param lineStyle index of linestyle in shapestyle array to be set as current linestyle
     */
    public void setLineStyle( int lineStyle ) {
        StyleChangeRecord sc = getStyleChange();
        sc.addFlags( StyleChangeRecord.LINESTYLE );
        sc.setLineStyle( lineStyle );
    }

    /**
     * Returns index of current line style
     *
     * @return index of currently used line style
     */
    public int getLineStyleIndex() {
        StyleChangeRecord sc = getStyleChange();
        return sc.getLineStyle();
    }

    /**
     * Returns current line style
     *
     * @return currently used line style
     */
    public MorphLineStyle getLineStyle() {
        int idx = getLineStyleIndex();
        return getShapeStyles().getLineStyle(idx);
    }

    /**
     * Returns index of current fill style 0
     *
     * @return index of currently used fill style 0
     */
    public int getFillStyle0Index() {
        StyleChangeRecord sc = getStyleChange();
        return sc.getFillStyle0();
    }

    /**
     * Returns current fill style 0
     *
     * @return currently used fill style 0
     */
    public MorphFillStyle getFillStyle0() {
        int idx = getFillStyle0Index();
        return getShapeStyles().getFillStyle(idx);
    }

    /**
     * Returns index of current fill style 1
     *
     * @return index of currently used fill style 1
     */
    public int getFillStyle1Index() {
        StyleChangeRecord sc = getStyleChange();
        return sc.getFillStyle1();
    }

    /**
     * Returns current fill style 1
     *
     * @return currently used fill style 1
     */
    public MorphFillStyle getFillStyle1() {
        int idx = getFillStyle1Index();
        return getShapeStyles().getFillStyle(idx);
    }

    public int getTag() {
        return Tag.DEFINEMORPHSHAPE;
    }

    public void collectDeps( DepsCollector dc ) {
        shape_styles.collectDeps(dc);
    }

    protected StyleChangeRecord getStyleChange() {
        return getShapeRecordsStart().getStyleChange();
    }

    /**
     * Parses Shape
     *
     * @param p      Parser
     * @return parsed shape
     */
    public static MorphShape parse( Parser p ) {

        int tagCode = p.getTagCode();
        MorphShape shape = new MorphShape();
        shape.setID( p.getUWord() );
        shape.bounds_start = p.getRect();
        shape.bounds_end = p.getRect();

        int offset = p.getUDWord();
        int pos = p.getPos();

        // parse first styles
        shape.shape_styles = MorphShapeStyles.parse(p);

        shape.records_start = ShapeRecords.parse(p);

        p.setPos(pos+offset);

        shape.records_end   = ShapeRecords.parse(p);

        return shape;
    }

    public void write( FlashOutput main ) {
        FlashOutput fob = new FlashOutput(main,200);

        fob.write(bounds_start);
        fob.write(bounds_end);

        fob.skip(4);
        int pos = fob.getPos();

        shape_styles.write(fob);

        int nFillBits = shape_styles.calcNFillBits();
        int nLineBits = shape_styles.calcNLineBits();
        fob.writeByte( (nFillBits<<4) | nLineBits );
        records_start.write(fob, nFillBits, nLineBits);
        fob.writeBits(0, 6);
        fob.flushBits();

        int offset = fob.getPos()-pos;
        fob.writeDWordAt(offset, pos-4);

        fob.writeByte(0);   // fill and line style bits
        records_end.write(fob);

        main.writeTag( getTag(), 2+fob.getSize() );
        main.writeDefID( this );
        main.writeFOB( fob );
    }

    public void printContent( PrintStream out, String indent ) {
        out.println(indent+"MorphShape("+Tag.tagNames[getTag()]+"): id="+getID()+", name='"+getName()+"'");
        out.println(indent+"   start bounds="+bounds_start);
        out.println(indent+"   end   bounds="+bounds_end);
        shape_styles.printContent(out, "   ");
        records_start.printContent(out, "   ");
        records_end.printContent(out, "   ");
    }

    public boolean isConstant() {
        return true;
    }

    /**
     * Returns MorphShape bounds
     *
     * @return shape bounds
     */
    public Rectangle2D getBounds() {
        Rectangle2D dst = GeomHelper.newRectangle();
        Rectangle2D.union( bounds_start, bounds_end, dst );
        return dst;
    }

    /**
     * Returns MorphShape start shape bounds
     *
     * @return shape bounds
     */
    public Rectangle2D getBoundsStart() {
        return this.bounds_start;
    }

    /**
     * Returns MorphShape end shape bounds
     *
     * @return shape bounds
     */
    public Rectangle2D getBoundsEnd() {
        return this.bounds_end;
    }

    /**
     * Sets bounding box for start shape.
     *
     * @param bounds new bounding box
     */
    public void setBoundsStart( Rectangle2D bounds ) {
        this.bounds_start = bounds;
    }

    /**
     * Sets bounding box for end shape.
     *
     * @param bounds new bounding box
     */
    public void setBoundsEnd( Rectangle2D bounds ) {
        this.bounds_end = bounds;
    }

    /**
     * Sets bounding box for start shape.
     *
     */
    public void setBoundsStart( int x, int y, int width, int height ) {
        setBoundsStart( GeomHelper.newRectangle(x,y,width,height) );
    }

    /**
     * Sets bounding box for end shape.
     *
     */
    public void setBoundsEnd( int x, int y, int width, int height ) {
        setBoundsEnd( GeomHelper.newRectangle(x,y,width,height) );
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((MorphShape)item).bounds_start = (Rectangle2D) bounds_start.clone();
        ((MorphShape)item).bounds_end = (Rectangle2D) bounds_end.clone();
        ((MorphShape)item).records_start = (ShapeRecords) records_start.getCopy(copier);
        ((MorphShape)item).records_end = (ShapeRecords) records_end.getCopy(copier);
        ((MorphShape)item).shape_styles = (MorphShapeStyles) shape_styles.getCopy(copier);
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new MorphShape(), copier );
    }

    /* ----------------------------------------------------------------------------------- */
    /*                                   D R A W I N G                                     */
    /* ----------------------------------------------------------------------------------- */

    /**
     * Draws curve record.<p>
     * All coordinates are in twixels.
     *
     * @param cx     X control point
     * @param cy     Y control point
     * @param ax     X anchor point
     * @param ay     Y anchor point
     */
    public void drawCurveToStart( int cx, int cy, int ax, int ay ) {
        getShapeRecordsStart().drawCurveTo(cx, cy, ax, ay);
    }

    public void drawCurveToEnd( int cx, int cy, int ax, int ay ) {
        getShapeRecordsEnd().drawCurveTo(cx, cy, ax, ay);
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
    public void drawCurveStart( int ax1, int ay1, int cx, int cy, int ax2, int ay2 ) {
        getShapeRecordsStart().drawCurve(ax1, ay1, cx, cy, ax2, ay2);
    }

    public void drawCurveEnd( int ax1, int ay1, int cx, int cy, int ax2, int ay2 ) {
        getShapeRecordsEnd().drawCurve(ax1, ay1, cx, cy, ax2, ay2);
    }

    /**
     * Draws curve record.<p>
     * All coordinates are in twixels.
     *
     * @param anchor0 first anchor point
     * @param control control point
     * @param anchor1 second anchor point
     */
    public void drawCurveStart( Point2D anchor1, Point2D control, Point2D anchor2 ) {
        getShapeRecordsStart().drawCurve(anchor1, control, anchor2);
    }

    public void drawCurveEnd( Point2D anchor1, Point2D control, Point2D anchor2 ) {
        getShapeRecordsEnd().drawCurve(anchor1, control, anchor2);
    }

    /**
     * Draws a straight line from current position to the specified one.<p>
     * All coordinates are in twixels.
     *
     * @param x      X of end of line
     * @param y      Y of end of line
     */
    public void drawLineToStart( int x, int y ) {
        getShapeRecordsStart().drawLineTo(x, y);
    }

    public void drawLineToEnd( int x, int y ) {
        getShapeRecordsEnd().drawLineTo(x, y);
    }

    /**
     * Draws a straight line from current position to the specified one.<p>
     * All coordinates are in twixels.
     *
     * @param p1     end of line
     */
    public void drawLineToStart( Point2D p1 ) {
        getShapeRecordsStart().drawLineTo(p1);
    }

    public void drawLineToEnd( Point2D p1 ) {
        getShapeRecordsEnd().drawLineTo(p1);
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
    public void drawLineStart( int x1, int y1, int x2, int y2 ) {
        getShapeRecordsStart().drawLine(x1, y1, x2, y2);
    }

    public void drawLineEnd( int x1, int y1, int x2, int y2 ) {
        getShapeRecordsEnd().drawLine(x1, y1, x2, y2);
    }

    /**
     * Draws a straight line specified by two points.
     * <P>
     * All coordinates are in twixels.
     *
     * @param p0     first point
     * @param p1     second point
     */
    public void drawLineStart( Point2D p0, Point2D p1 ) {
        getShapeRecordsStart().drawLine(p0, p1);
    }

    public void drawLineEnd( Point2D p0, Point2D p1 ) {
        getShapeRecordsEnd().drawLine(p0, p1);
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
    public void drawRectangleStart( int x, int y, int width, int height ) {
        getShapeRecordsStart().drawRectangle(x, y, width, height);
    }

    public void drawRectangleEnd( int x, int y, int width, int height ) {
        getShapeRecordsEnd().drawRectangle(x, y, width, height);
    }

    /**
     * Draws a rectangle specified by {@link java.awt.geom.Rectangle2D}
     * <p>
     * All coordinates are in twixels.
     *
     * @param r      specified rectangle
     */
    public void drawRectangleStart( Rectangle2D r ) {
        getShapeRecordsStart().drawRectangle(r);
    }

    public void drawRectangleEnd( Rectangle2D r ) {
        getShapeRecordsEnd().drawRectangle(r);
    }

    /**
     * Moves pen to the specified position.<p>
     * All coordinates are ABSOLUTE and are in twixels.
     *
     * @param x      new current X
     * @param y      new current Y
     */
    public void movePenToStart( int x, int y ) {
        getShapeRecordsStart().movePenTo(x, y);
    }

    public void movePenToEnd( int x, int y ) {
        getShapeRecordsEnd().movePenTo(x, y);
    }

    /**
     * Moves pen to the specified point.<p>
     * All coordinates are ABSOLUTE and are in twixels!
     *
     * @param p     new pen position
     */
    public void movePenToStart( Point2D p ) {
        getShapeRecordsStart().movePenTo(p);
    }

    public void movePenToEnd( Point2D p ) {
        getShapeRecordsEnd().movePenTo(p);
    }

    /**
     * Draw AWT Shape
     * <P>
     * All shape coordinates are in twixels!
     *
     * @param shape  AWT shape
     */
    public void drawAWTShapeStart( java.awt.Shape shape ) {
        getShapeRecordsStart().drawAWTShape(shape);
    }

    public void drawAWTShapeEnd( java.awt.Shape shape ) {
        getShapeRecordsEnd().drawAWTShape(shape);
    }

    /**
     * Draw AWT Shape
     * <P>
     * All shape coordinates are in twixels!
     *
     * @param shape  AWT shape
     */
    public void drawAWTShapeStart( java.awt.Shape shape, AffineTransform matrix ) {
        getShapeRecordsStart().drawAWTShape(shape, matrix);
    }

    public void drawAWTShapeEnd( java.awt.Shape shape, AffineTransform matrix ) {
        getShapeRecordsEnd().drawAWTShape(shape, matrix);
    }

    /**
     * Draw AWT PathIterator
     * <P>
     * All coordinates are in twixels!
     *
     * @param pi   AWT PathIterator
     */
    public void drawAWTPathIteratorStart( java.awt.geom.PathIterator pi ) {
        getShapeRecordsStart().drawAWTPathIterator(pi);
    }

    public void drawAWTPathIteratorEnd( java.awt.geom.PathIterator pi ) {
        getShapeRecordsEnd().drawAWTPathIterator(pi);
    }

}

