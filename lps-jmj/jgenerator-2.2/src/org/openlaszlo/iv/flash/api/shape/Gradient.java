/*
 * $Id: Gradient.java,v 1.1 2002/02/15 23:46:26 skavish Exp $
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
 * Gradient
 * <P>
 * Here is an overview of the Macromedia Flash (SWF) gradient model:<BR>
 * <UL>
 * <LI>There are two styles of gradient - Linear and Radial.
 * <LI>Each gradient has its own transformation matrix, and can be transformed independently of its parent shape.
 * <LI>A gradient can have up to eight control points.  Colors are interpolated between the control
 *     points to create the color ramp.
 * <LI>Each control point is defined by a ratio and an RGBA color.  The ratio determines the position of
 *     the control point in the gradient, the RGBA value determines its color.
 * </UL>
 *
 * @author Dmitry Skavish
 */
public final class Gradient extends FlashItem {

    private int nGrads;
    private int[] ratios = new int[8];
    private Color[] colors = new Color[8];

    /**
     * Creates empty gradient
     */
    public Gradient() {
        nGrads = 0;
    }

    /**
     * Creates gradient with one transition
     *
     * @param ratio  specified retio
     * @param color  specified color
     */
    public Gradient( int ratio, Color color ) {
        nGrads = 0;
        addTransition( ratio, color );
    }

    public Color[] getColors() {
        return colors;
    }

    public int[] getRatios() {
        return ratios;
    }

    public int getNumber() {
        return nGrads;
    }

    /**
     * Adds transition
     *
     * @param ratio  the transition's ratio
     * @param color  the transition's color
     */
    public void addTransition( int ratio, Color color ) {
        ratios[nGrads] = ratio;
        colors[nGrads] = color;
        nGrads++;
    }

    public static Gradient parse( Parser p, boolean withAlpha ) {
        Gradient g = new Gradient();
        int nGrads = p.getUByte();
        for( int i=0; i<nGrads; i++ ) {
            int ratio = p.getUByte();
            Color color = Color.parse(p, withAlpha);
            g.addTransition(ratio, color);
        }
        return g;
    }

    public void write( FlashOutput fob ) {
        fob.writeByte(nGrads);
        if( fob.getUserData() instanceof Shape ) {
            if( ((Shape)fob.getUserData()).isWithAlpha() ) {
                for( int i=0; i<nGrads; i++ ) {
                    fob.writeByte( ratios[i] );
                    colors[i].writeRGBA(fob);
                }
            } else {
                for( int i=0; i<nGrads; i++ ) {
                    fob.writeByte( ratios[i] );
                    colors[i].writeRGB(fob);
                }
            }
        } else {
            for( int i=0; i<nGrads; i++ ) {
                fob.writeByte( ratios[i] );
                colors[i].write(fob);
            }
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"Gradient: num="+nGrads );
        for( int i=0; i<nGrads; i++ ) {
            out.println( indent+"    ratio["+i+"]="+ratios[i] );
            out.print(   indent+"    color["+i+"]=" ); colors[i].printContent(out, "");
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        ((Gradient)item).nGrads = nGrads;
        int[] r = new int[8];
        System.arraycopy(ratios, 0, r, 0, 8);
        Color[] c = new Color[8];
        for( int i=0; i<nGrads; i++ ) {
            c[i] = (Color) colors[i].getCopy(copier);
        }
        ((Gradient)item).ratios = r;
        ((Gradient)item).colors = c;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new Gradient(), copier );
    }
}
