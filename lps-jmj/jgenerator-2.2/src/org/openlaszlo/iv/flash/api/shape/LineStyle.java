/*
 * $Id: LineStyle.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.parser.*;
import java.io.PrintStream;

/**
 * A line style represents a width and color of a line.
 * <P>
 * Notes:
 * <OL>
 * <LI>All lines in SWF have rounded joins and end-caps.
 *     Different join styles and end styles can be simulated with
 *     a very narrow shape that looks identical to the desired stroke.
 * <LI>SWF has no native support for dashed or dotted line styles.
 *     A dashed line can be simulated by breaking up the path into a series of short lines.
 * </OL>
 *
 * @author Dmitry Skavish
 */
public final class LineStyle extends FlashItem {

    private int width;              // width of this line style
    private Color color;            // color of this line style

    public LineStyle() {}

    /**
     * Creates line style of specified color and width
     *
     * @param width  width in twixels
     * @param color  line color
     */
    public LineStyle( int width, Color color ) {
        setWidth( width );
        setColor( color );
    }

    /**
     * Returns width of this linestyle.
     *
     * @return width of this linestyle in twixels.
     */
    public int getWidth() {
        return width;
    }

    /**
     * Sets width of this linestyle
     *
     * @param width  width of this linestyle in twixels
     */
    public void setWidth( int width ) {
        this.width = width;
    }

    /**
     * Returns color of this linestyle
     *
     * @return color of this linestyle
     */
    public Color getColor() {
        return color;
    }

    /**
     * Sets color of this linestyle
     *
     * @param color  specified color
     */
    public void setColor( Color color ) {
        this.color = color;
    }

    public static LineStyle parse( Parser p, boolean withAlpha ) {
        int width = p.getUWord();
        Color color = Color.parse(p, withAlpha);
        return new LineStyle(width, color);
    }

    public void write( FlashOutput fob ) {
        fob.writeWord(width);
        Shape shape = (Shape) fob.getUserData();
        if( shape.isWithAlpha() ) {
            color.writeRGBA(fob);
        } else {
            color.writeRGB(fob);
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.print( indent+"LineStyle: width="+width+" color=" );
        color.printContent(out, "");
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        ((LineStyle)item).width = width;
        ((LineStyle)item).color = (Color) color.getCopy(copier);
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new LineStyle(), copier );
    }
}
