/*
 * $Id: Shape.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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
 * Shape.
 * <P>
 * Shapes are defined by a list of edges called a path. A path may be closed - where the start
 * and end of the path meet to close the figure, or open - where the path forms an
 * open-ended stroke. A path may contain a mixture of straight edges, curved edges, and
 * "pen up and move" commands.  The latter allows multiple disconnected figures to be
 * described by a single shape structure. (see MoveTo flag)
 * <P>
 * A fill style defines the appearance of an area enclosed by a path. Fill styles supported
 * by SWF include a color, a gradient, or a bitmapped image.<BR>
 * A line style defines the appearance of the outline of a path. The line style may be a stroke
 * of any thickness and color.
 * <P>
 * SWF format allows each edge to have its own line and fill style.
 * This can have unpredictable results when fill styles change in the middle of a path.
 * <P>
 * Flash also supports two fill styles per edge, one for each side of the edge:
 * FillStyle0 and FillStyle1. FillStyle0 should always be used first and then
 * FillStyle1 if the shape is filled on both sides of the edge.
 * <P>
 * A shape is comprised of the following elements:
 * <UL>
 * <LI>CharacterId - A 16-bit value that uniquely identifies this shape as a
 *     'character' in the dictionary.  The CharacterId can be referred to in
 *     control tags such as PlaceObject. Characters can be re-used and combined
 *     with other characters to make more complex shapes.
 * <LI>Bounding box - The rectangle that completely encloses the shape.
 * <LI>Fill style array - A list of all the fill styles used in a shape.
 * <LI>Line style array - A list of all the line styles used in a shape.
 * <LI>Shape-record array - A list of shape-records.  Shape-records can define straight
 *     or curved edges, style changes, or move the drawing position.
 * </UL>
 * Note: Line and fill styles are defined once only, and may be used (and re-used) by
 * any of the edges in the shape.
 * <p>
 * Note that objects of this class will always be generated as DefineShape3 tags which
 * means that all the colors used in the shape (in line and fillstyles) have to be AlphaColors!
 *
 * @author Dmitry Skavish
 */
public final class Shape extends FlashDef /*implements Drawable*/ {

    private StyleBlock style_block;
    private Rectangle2D bounds;                         // bounding box of this shape
    private int tagcode;                                // tag of this shape

    /**
     * Creates new empty shape (Shape3 - all colors with alpha)
     */
    public Shape() {
        this( Tag.DEFINESHAPE3 );
        newStyleBlock();
    }

    /**
     * Returns shape styles
     *
     * @return object representing array of shape styles
     */
    public ShapeStyles getShapeStyles() {
        return style_block.shapeStyles;
    }

    /**
     * Returns shape records
     *
     * @return object representing array of shape records
     */
    public ShapeRecords getShapeRecords() {
        return style_block.shapeRecords;
    }

    /**
     * Adds specified fill style to the array of shape styles.
     * <p>
     * Note: colors used in the specified fillstyle have to be AlphaColors
     *
     * @param fillStyle specified fill style
     * @return index of added fill style in the array
     */
    public int addFillStyle( FillStyle fillStyle ) {
        return getShapeStyles().addFillStyle( fillStyle );
    }

    /**
     * Adds specified line style to the array of shape styles
     * <p>
     * Note: colors used in the specified linestyle have to be AlphaColors
     *
     * @param lineStyle specified line style
     * @return index of added line style in the array
     */
    public int addLineStyle( LineStyle lineStyle ) {
        return getShapeStyles().addLineStyle( lineStyle );
    }

    /**
     * Sets specified fillstyle as current fillstyle0.
     * <P>
     * If the specified fillstyle is not in the shapestyle array
     * then it's added, if it's already there it's reused.
     * <p>
     * Note: colors used in the specified fillstyle have to be AlphaColors
     *
     * @param fillStyle specified fillstyle
     * @return index of specified fillstyle in the shapestyle array
     */
    public int setFillStyle0( FillStyle fillStyle ) {
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
     * <p>
     * Note: colors used in the specified fillstyle have to be AlphaColors
     *
     * @param fillStyle specified fillstyle
     * @return index of specified fillstyle in the shapestyle array
     */
    public int setFillStyle1( FillStyle fillStyle ) {
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
     * <p>
     * Note: color used in the specified linestyle has to be AlphaColor
     *
     * @param lineStyle specified linestyle
     * @return index of specified linestyle in the shapestyle array
     */
    public int setLineStyle( LineStyle lineStyle ) {
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
    public LineStyle getLineStyle() {
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
    public FillStyle getFillStyle0() {
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
    public FillStyle getFillStyle1() {
        int idx = getFillStyle1Index();
        return getShapeStyles().getFillStyle(idx);
    }

    /**
     * Creates new style block
     * <P>
     * Each style block contains styles and records which use
     * these styles. Records cannot use styles from different
     * style blocks. It means that after this call one cannot
     * use styles (and style indexes) from previous style blocks.
     */
    public void newStyleBlock() {
        StyleBlock sb = new StyleBlock();
        sb.prev = style_block;

        // check whether last style block has stylechange with flag NEW_STYLES
        // add this record if there is no such stylechange
        if( style_block != null ) {
            IVVector shape_records = style_block.shapeRecords.getShapeRecords();
            if( shape_records.size() > 0 ) {
                Object o = shape_records.elementAt(shape_records.size()-1);
                if( o instanceof StyleChangeRecord ) {
                    StyleChangeRecord sr = (StyleChangeRecord) o;
                    sr.addFlags(StyleChangeRecord.NEW_STYLES);
                } else {
                    StyleChangeRecord sr = new StyleChangeRecord();
                    sr.addFlags(StyleChangeRecord.NEW_STYLES);
                    shape_records.addElement(sr);
                }
            }
        }

        style_block = sb;
    }

   /**
     * Creates new Shape defined by tag DEFINESHAPE
     *
     * @return new Shape
     */
    public static Shape newShape1() {
        Shape shape = new Shape(Tag.DEFINESHAPE);
        shape.newStyleBlock();
        return shape;
    }

    /**
     * Creates new Shape defined by tag DEFINESHAPE2
     *
     * @return new Shape
     */
    public static Shape newShape2() {
        Shape shape = new Shape(Tag.DEFINESHAPE2);
        shape.newStyleBlock();
        return shape;
    }

    /**
     * Creates new Shape defined by tag DEFINESHAPE3
     *
     * @return new Shape
     */
    public static Shape newShape3() {
        Shape shape = new Shape(Tag.DEFINESHAPE3);
        shape.newStyleBlock();
        return shape;
    }

    /**
     * Creates new Shape3 from provided shape styles and records
     *
     * @return new Shape
     */
    public static Shape newShape3( ShapeStyles styles, ShapeRecords records ) {
        Shape shape = new Shape(Tag.DEFINESHAPE3);
        shape.style_block = new StyleBlock(styles, records);
        return shape;
    }

    /**
     * Creates empty Shape (DEFINESHAPE tag)
     *
     * @return empty shape
     */
    public static Shape newEmptyShape1() {
        Shape emptyShape = newShape1();
        emptyShape.setLineStyle( new LineStyle(1, new Color(0,0,0) ) );
        emptyShape.movePenTo(0,0);
        emptyShape.setBounds( GeomHelper.newRectangle(0,0,0,0) );
        return emptyShape;
    }

    public int getTag() {
        return tagcode;
    }

    public void collectDeps( DepsCollector dc ) {
        StyleBlock sb = style_block;
        while( sb != null ) {
            sb.shapeStyles.collectDeps(dc);
            sb = sb.prev;
        }
    }

    protected StyleChangeRecord getStyleChange() {
        return getShapeRecords().getStyleChange();
    }

    /**
     * Parses Shape
     *
     * @param p      Parser
     * @return parsed shape
     */
    public static Shape parse( Parser p ) {

        int tagCode = p.getTagCode();
        Shape shape = new Shape( tagCode );
        shape.setID( p.getUWord() );
        shape.bounds = p.getRect();

        boolean withAlpha = shape.isWithAlpha();

        // parse first styles
        ShapeStyles shape_styles = ShapeStyles.parse(p, withAlpha);

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

                if( (flags & StyleChangeRecord.NEW_STYLES) != 0 ) {

                    // add this style change as first record to new block
                    shape_records.addElement(scr);

                    // new styles are to be defined
                    // save all already parsed records and styles in style block
                    // and reset data to parse next style blosk

                    StyleBlock new_style_block = new StyleBlock(shape_styles, shape_records);
                    new_style_block.prev = shape.style_block;
                    shape.style_block = new_style_block;

                    // parse new styles
                    shape_styles = ShapeStyles.parse(p, withAlpha);
                    // create new records vector
                    shape_records = new IVVector();

                    // parse new fill and line number of bits
                    nBits = p.getUByte();
                    nFillBits = (nBits&0xf0)>>4;
                    nLineBits = nBits&0x0f;
                    p.initBits();

                } else {
                    shape_records.addElement(scr);
                }

                if( (flags&0x80) != 0 ) {
                    break;
                }
            }
        }

        // save parsed records in style block
        StyleBlock new_style_block = new StyleBlock(shape_styles, shape_records);
        new_style_block.prev = shape.style_block;
        shape.style_block = new_style_block;

        return shape;
    }

    public boolean isWithAlpha() {
        return getTag() == Tag.DEFINESHAPE3;
    }

    /**
     * This class represents style block: set of styles and set of corresponding
     * records which use these styles. It also holds reference to previous
     * style block. Shape may contain several different style blocks.
     * This class is not supposed to be used externally and reflects inner
     * Shape structure.
     */
    private static class StyleBlock {

        StyleBlock prev;
        ShapeStyles shapeStyles;         // collection of fill and line styles
        ShapeRecords shapeRecords;      // collection of shape records

        StyleBlock() {
            this.shapeStyles = new ShapeStyles();
            this.shapeRecords = new ShapeRecords();
        }

        StyleBlock( ShapeStyles styles, IVVector records ) {
            this(styles, new ShapeRecords(records));
        }

        StyleBlock( ShapeStyles styles, ShapeRecords records ) {
            this.shapeStyles = styles;
            this.shapeRecords = records;
        }

        private void write_block( FlashOutput fob ) {
            if( prev != null ) {
                prev.write_block(fob);
                fob.flushBits();
            }

            shapeStyles.write(fob);

            int nFillBits = shapeStyles.calcNFillBits();
            int nLineBits = shapeStyles.calcNLineBits();
            fob.writeByte( (nFillBits<<4) | nLineBits );
            shapeRecords.write(fob, nFillBits, nLineBits);
        }

        public void write( FlashOutput fob ) {
            write_block(fob);
            fob.writeBits(0, 6);
            fob.flushBits();
        }

        public void printContent( PrintStream out, String indent ) {
            if( prev != null ) {
                prev.printContent(out, indent );
                out.println( indent+"    new styleblock:" );
            }
            shapeStyles.printContent(out, indent+"        " );
            shapeRecords.printContent(out, indent+"        " );
        }

        public StyleBlock getCopy( ScriptCopier copier ) {
            StyleBlock my_prev = null;
            if( prev != null ) {
                my_prev = prev.getCopy(copier);
            }
            StyleBlock my_block = new StyleBlock(
                (ShapeStyles) shapeStyles.getCopy(copier),
                (ShapeRecords) shapeRecords.getCopy(copier)
            );
            my_block.prev = my_prev;
            return my_block;
        }
    }

    public void write( FlashOutput main ) {
        FlashOutput fob = new FlashOutput(main,100);

        // set userdata to this shape, so that styles know which color to write
        fob.setUserData(this);
        fob.write( bounds );

        style_block.write(fob);

        main.writeTag( getTag(), 2+fob.getSize() );
        main.writeDefID( this );
        main.writeFOB( fob );
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"Shape("+Tag.tagNames[getTag()]+"): id="+getID()+", name='"+getName()+"'" );
        out.println( indent+"   "+bounds );
        style_block.printContent(out, indent);
    }

    public boolean isConstant() {
        return true;
    }

    /**
     * Returns Shape bounds
     *
     * @return shape bounds
     */
    public Rectangle2D getBounds() {
        return bounds;
    }

    /**
     * Sets bounding box for this shape.
     *
     * @param bounds new bounding box
     */
    public void setBounds( Rectangle2D bounds ) {
        this.bounds = bounds;
    }

    /**
     * Sets bounding box for this shape.
     *
     */
    public void setBounds( int x, int y, int width, int height ) {
        setBounds( GeomHelper.newRectangle(x,y,width,height) );
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((Shape)item).bounds = (Rectangle2D) bounds.clone();
        ((Shape)item).style_block = (StyleBlock) style_block.getCopy(copier);
        ((Shape)item).tagcode = tagcode;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new Shape(), copier );
    }

    private Shape( int tagcode ) {
        this.tagcode = tagcode;
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
    public void drawCurveTo( int cx, int cy, int ax, int ay ) {
        getShapeRecords().drawCurveTo(cx, cy, ax, ay);
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
        getShapeRecords().drawCurve(ax1, ay1, cx, cy, ax2, ay2);
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
        getShapeRecords().drawCurve(anchor1, control, anchor2);
    }

    /**
     * Draws a straight line from current position to the specified one.<p>
     * All coordinates are in twixels.
     *
     * @param x      X of end of line
     * @param y      Y of end of line
     */
    public void drawLineTo( int x, int y ) {
        getShapeRecords().drawLineTo(x, y);
    }

    /**
     * Draws a straight line from current position to the specified one.<p>
     * All coordinates are in twixels.
     *
     * @param p1     end of line
     */
    public void drawLineTo( Point2D p1 ) {
        getShapeRecords().drawLineTo(p1);
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
        getShapeRecords().drawLine(x1, y1, x2, y2);
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
        getShapeRecords().drawLine(p0, p1);
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
        getShapeRecords().drawRectangle(x, y, width, height);
    }

    /**
     * Draws a rectangle specified by {@link java.awt.geom.Rectangle2D}
     * <p>
     * All coordinates are in twixels.
     *
     * @param r      specified rectangle
     */
    public void drawRectangle( Rectangle2D r ) {
        getShapeRecords().drawRectangle(r);
    }

    /**
     * Moves pen to the specified position.<p>
     * All coordinates are ABSOLUTE and are in twixels.
     *
     * @param x      new current X
     * @param y      new current Y
     */
    public void movePenTo( int x, int y ) {
        getShapeRecords().movePenTo(x, y);
    }

    /**
     * Moves pen to the specified point.<p>
     * All coordinates are ABSOLUTE and are in twixels!
     *
     * @param p     new pen position
     */
    public void movePenTo( Point2D p ) {
        getShapeRecords().movePenTo(p);
    }

    /**
     * Draw AWT Shape
     * <P>
     * All shape coordinates are in twixels!
     *
     * @param shape  AWT shape
     */
    public void drawAWTShape( java.awt.Shape shape ) {
        getShapeRecords().drawAWTShape(shape);
    }

    /**
     * Draw AWT Shape
     * <P>
     * All shape coordinates are in twixels!
     *
     * @param shape  AWT shape
     */
    public void drawAWTShape( java.awt.Shape shape, AffineTransform matrix ) {
        getShapeRecords().drawAWTShape(shape, matrix);
    }

    /**
     * Draw AWT PathIterator
     * <P>
     * All coordinates are in twixels!
     *
     * @param pi   AWT PathIterator
     */
    public void drawAWTPathIterator( java.awt.geom.PathIterator pi ) {
        getShapeRecords().drawAWTPathIterator(pi);
    }

    /**
     * Returns current pen position
     *
     * @return current pen position
     */
    public Point2D getCurrentPos() {
        return getShapeRecords().getCurrentPos();
    }

    /**
     * Returns first pen position (first moveTo)
     *
     * @return first pen position
     */
    public Point2D getFirstPos() {
        return getShapeRecords().getFirstPos();
    }

    /* --------------------------------------------------------------------------------------- */
    /*                                          AWT Stuff                                      */
    /* --------------------------------------------------------------------------------------- */

    //
    //      THE FOLLOWING CODE IS JUST AN EXPERIMENT!
    //      IT IS NOT INDENDED TO BE USED IN ANY WAY!
    //      IT IS IN A VERY FIRST STAGE OF DEVELOPMENT
    //


    private static class Painter {
        java.awt.Graphics2D g2;
        GeneralPath         gp_line;
        GeneralPath         gp_fill;
        java.awt.Paint[]    lstyle_paints;
        java.awt.Stroke[]   lstyle_strokes;
        java.awt.Paint[]    fstyle_paints;
        ShapeStyles         ss;
        int                 line_idx;
        int                 fill_idx;

        Painter( java.awt.Graphics2D g2, GeneralPath gp_fill, GeneralPath gp_line, ShapeStyles ss ) {
            this.g2 = g2;
            this.ss = ss;
            this.gp_line = gp_line;
            this.gp_fill = gp_fill;

            line_idx = -1;
            fill_idx = -1;
            lstyle_paints = new java.awt.Paint[ss.lineStyles.size()];
            lstyle_strokes = new java.awt.Stroke[ss.lineStyles.size()];
            fstyle_paints = new java.awt.Paint[ss.fillStyles.size()];
        }

        void paint_line() {
            if( line_idx >= 0 && gp_line != null ) {
                java.awt.Paint paint = lstyle_paints[line_idx];
                java.awt.Stroke stroke = lstyle_strokes[line_idx];
                if( paint == null ) {
                    LineStyle lstyle = ss.getLineStyle(line_idx);

                    int width = lstyle.getWidth();
                    //if( width < 20 ) width = 20;
                    stroke = new java.awt.BasicStroke(width);//, java.awt.BasicStroke.CAP_ROUND,
                    paint = lstyle.getColor().getAWTColor();

                    lstyle_paints[line_idx] = paint;
                    lstyle_strokes[line_idx] = stroke;
                }

                g2.setPaint(paint);
                g2.setStroke(stroke);

                g2.draw(gp_line);
                gp_line.reset();
            }
        }

        void paint_fill() {
            if( fill_idx >= 0 && gp_fill != null ) {
                java.awt.Paint paint = fstyle_paints[fill_idx];
                if( paint == null ) {
                    FillStyle fstyle = ss.getFillStyle(fill_idx);

                    switch( fstyle.getType() ) {
                        case FillStyle.SOLID: {
                            paint = fstyle.getColor().getAWTColor();
                            break;
                        }
                        case FillStyle.LINEAR_GRADIENT: {
                            Gradient grad = fstyle.getGraduent();
                            paint = grad.getColors()[0].getAWTColor();
                            break;
                        }
                    }
                    fstyle_paints[fill_idx] = paint;
                }

                if( paint != null ) g2.setPaint(paint);

                g2.fill(gp_fill);
                gp_fill.reset();
            }
        }

    }

    /**
     * Draws itself into specified graphics
     *
     * @param g      graphics object
     */
    public void draw( java.awt.Graphics2D g2 ) {
        IVVector blocks = new IVVector();

        StyleBlock sb = style_block;
        while( sb.prev != null ) {
            blocks.addElement(sb);
            sb = sb.prev;
        }
        blocks.addElement(sb);

        float x = 0f;
        float y = 0f;

        GeneralPath gp_line = null;
        GeneralPath gp_fill = null;
        for( int blk=blocks.size(); --blk>=0; ) {
            sb = (StyleBlock) blocks.elementAt(blk);
            ShapeRecords shaperecords = sb.shapeRecords;

            boolean isFills = sb.shapeStyles.fillStyles.size()>0;
            boolean isLines = sb.shapeStyles.lineStyles.size()>0;

            if( isFills && gp_fill == null ) gp_fill = new GeneralPath(GeneralPath.WIND_EVEN_ODD);
            if( isLines && gp_line == null ) gp_line = new GeneralPath(GeneralPath.WIND_EVEN_ODD);

            Painter painter = new Painter(g2, gp_fill, gp_line, sb.shapeStyles);

            IVVector records = shaperecords.getShapeRecords();
            boolean isGpEmpty = true;
            for( int r=0; r<records.size(); r++ ) {
                Object o = records.elementAt(r);
                if( o instanceof StyleChangeRecord ) {
                    StyleChangeRecord sr = (StyleChangeRecord) o;
                    int f = sr.getFlags();

                    boolean isGpEmpty1 = isGpEmpty;

                    if( (f&(StyleChangeRecord.FILLSTYLE1|StyleChangeRecord.FILLSTYLE0)) != 0 ) {
                        if( !isGpEmpty ) painter.paint_fill();
                        int idx = sr.getFillStyle0()-1;
                        if( idx < 0 ) idx = sr.getFillStyle1()-1;
                        painter.fill_idx = idx;
                        isGpEmpty1 = true;
                    }

                    if( (f&StyleChangeRecord.LINESTYLE) != 0 ) {
                        if( !isGpEmpty ) painter.paint_line();
                        painter.line_idx = sr.getLineStyle()-1;
                        isGpEmpty1 = true;
                    }
                    isGpEmpty = isGpEmpty1;

                    if( (f&StyleChangeRecord.MOVETO) != 0 ) {
                        x = sr.getDeltaX();
                        y = sr.getDeltaY();
                    }

                    if( isFills ) gp_fill.moveTo(x, y);
                    if( isLines ) gp_line.moveTo(x, y);

                } else if( o instanceof StrightEdgeRecord ) {
                    StrightEdgeRecord se = (StrightEdgeRecord) o;
                    x += se.getDeltaX();
                    y += se.getDeltaY();
                    if( isFills ) gp_fill.lineTo(x, y);
                    if( isLines ) gp_line.lineTo(x, y);
                    isGpEmpty = false;
                } else {
                    CurvedEdgeRecord ce = (CurvedEdgeRecord) o;
                    float xx = x+ce.getControlDeltaX();
                    float yy = y+ce.getControlDeltaY();
                    x = xx+ce.getAnchorDeltaX();
                    y = yy+ce.getAnchorDeltaY();
                    if( isFills ) gp_fill.quadTo(xx, yy, x, y);
                    if( isLines ) gp_line.quadTo(xx, yy, x, y);
                    isGpEmpty = false;
                }
            }
            if( !isGpEmpty ) {
                painter.paint_fill();
                painter.paint_line();
            }
        }
    }
}
