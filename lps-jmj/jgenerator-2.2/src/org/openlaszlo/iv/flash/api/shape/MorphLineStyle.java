/*
 * $Id: MorphLineStyle.java,v 1.1 2002/02/15 23:46:26 skavish Exp $
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
 *
 * @author Dmitry Skavish
 * @see LineStyle
 */
public final class MorphLineStyle extends FlashItem {

    private LineStyle style_start;
    private LineStyle style_end;

    public MorphLineStyle() {}

    /**
     * Creates line style of specified color and width
     *
     */
    public MorphLineStyle( int width_start, Color color_start, int width_end, Color color_end ) {
        style_start = new LineStyle(width_start, color_start);
        style_end   = new LineStyle(width_end, color_end);
    }

    public MorphLineStyle( LineStyle style_start, LineStyle style_end ) {
        this.style_start = style_start;
        this.style_end = style_end;
    }

    /**
     * Returns start width of this linestyle.
     *
     * @return start width of this linestyle
     */
    public int getWidthStart() {
        return style_start.getWidth();
    }

    /**
     * Sets start width of this linestyle
     *
     * @param width  width of this linestyle
     */
    public void setWidthStart( int width ) {
        style_start.setWidth(width);
    }

    /**
     * Returns end width of this linestyle.
     *
     * @return width of this linestyle
     */
    public int getWidthEnd() {
        return style_end.getWidth();
    }

    /**
     * Sets end width of this linestyle
     *
     * @param width  width of this linestyle
     */
    public void setWidthEnd( int width ) {
        style_end.setWidth(width);
    }

    /**
     * Returns start color of this linestyle
     *
     * @return color of this linestyle
     */
    public Color getColorStart() {
        return style_start.getColor();
    }

    /**
     * Sets start color of this linestyle
     *
     * @param color  specified color
     */
    public void setColorStart( Color color ) {
        style_start.setColor(color);
    }

    /**
     * Returns end color of this linestyle
     *
     * @return color of this linestyle
     */
    public Color getColorEnd() {
        return style_end.getColor();
    }

    /**
     * Sets end color of this linestyle
     *
     * @param color  specified color
     */
    public void setColorEnd( Color color ) {
        style_end.setColor(color);
    }

    public static MorphLineStyle parse( Parser p ) {
        int width_start = p.getUWord();
        int width_end = p.getUWord();
        Color color_start = AlphaColor.parse(p);
        Color color_end = AlphaColor.parse(p);
        return new MorphLineStyle(width_start, color_start, width_end, color_end);
    }

    public void write( FlashOutput fob ) {
        fob.writeWord(getWidthStart());
        fob.writeWord(getWidthEnd());
        getColorStart().writeRGBA(fob);
        getColorEnd().writeRGBA(fob);
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"MorphLineStyle: start:" );
        style_start.printContent(out, indent+"     ");
        out.println( indent+"                end:" );
        style_end.printContent(out, indent+"     ");
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        ((MorphLineStyle)item).style_start = (LineStyle) style_start.getCopy(copier);
        ((MorphLineStyle)item).style_end = (LineStyle) style_end.getCopy(copier);
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new MorphLineStyle(), copier );
    }
}

