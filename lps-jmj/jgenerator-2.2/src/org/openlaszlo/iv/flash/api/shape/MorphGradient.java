/*
 * $Id: MorphGradient.java,v 1.1 2002/02/15 23:46:26 skavish Exp $
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
import org.openlaszlo.iv.flash.parser.Parser;
import java.io.PrintStream;

/**
 * MorphGradient
 *
 * @author Dmitry Skavish
 * @see Gradient
 */
public final class MorphGradient extends FlashItem {

    private int nGrads;
    private int[] ratios_start = new int[8];
    private int[] ratios_end = new int[8];
    private Color[] colors_start = new Color[8];
    private Color[] colors_end = new Color[8];

    /**
     * Creates empty gradient
     */
    public MorphGradient() {}

    /**
     * Creates gradient with one transition
     *
     * @param ratio_start start ratio
     * @param color_start start color
     * @param ratio_end end ratio
     * @param color_end end color
     */
    public MorphGradient( int ratio_start, Color color_start, int ratio_end, Color color_end ) {
        nGrads = 0;
        addTransition( ratio_start, color_start, ratio_end, color_end );
    }

    public Color[] getColorsStart() {
        return colors_start;
    }

    public int[] getRatiosStart() {
        return ratios_start;
    }

    public Color[] getColorsEnd() {
        return colors_end;
    }

    public int[] getRatiosEnd() {
        return ratios_end;
    }

    public int getNumber() {
        return nGrads;
    }

    /**
     * Adds transition
     *
     * @param ratio_start start ratio
     * @param color_start start color
     * @param ratio_end end ratio
     * @param color_end end color
     */
    public void addTransition( int ratio_start, Color color_start, int ratio_end, Color color_end ) {
        ratios_start[nGrads] = ratio_start;
        colors_start[nGrads] = color_start;
        ratios_end[nGrads] = ratio_end;
        colors_end[nGrads] = color_end;
        nGrads++;
    }

    public static MorphGradient parse( Parser p ) {
        MorphGradient g = new MorphGradient();
        int nGrads = p.getUByte();
        for( int i=0; i<nGrads; i++ ) {
            int ratio_start = p.getUByte();
            AlphaColor color_start = AlphaColor.parse(p);
            int ratio_end = p.getUByte();
            AlphaColor color_end = AlphaColor.parse(p);
            g.addTransition(ratio_start, color_start, ratio_end, color_end);
        }
        return g;
    }

    public void write( FlashOutput fob ) {
        fob.writeByte(nGrads);
        for( int i=0; i<nGrads; i++ ) {
            fob.writeByte( ratios_start[i] );
            colors_start[i].writeRGBA(fob);
            fob.writeByte( ratios_end[i] );
            colors_end[i].writeRGBA(fob);
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"MorphGradient: num="+nGrads );
        for( int i=0; i<nGrads; i++ ) {
            out.println( indent+"    ratio_start["+i+"]="+ratios_start[i] );
            out.print(   indent+"    color_start["+i+"]=" ); colors_start[i].printContent(out, "");
            out.println( indent+"    ratio_end["+i+"]="+ratios_end[i] );
            out.print(   indent+"    color_end["+i+"]=" ); colors_end[i].printContent(out, "");
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        for( int i=0; i<nGrads; i++ ) {
            ((MorphGradient)item).addTransition(
                ratios_start[i], colors_start[i], ratios_end[i], colors_end[i]);
        }
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new MorphGradient(), copier );
    }
}

