/*
 * $Id: CXForm.java,v 1.3 2002/08/12 21:18:28 skavish Exp $
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

package org.openlaszlo.iv.flash.api;

import java.io.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;

/**
 * Class defines a simple transform that can be applied to the color space of a graphic object.<BR>
 * There are two types of transform possible:
 * <P>
 * 1. Multiplication Transforms<BR>
 * 2. Addition Transforms<BR>
 * <P>
 * Multiplication transforms multiply the red, green, and blue components by an 8.8 fixed-point multiplication term.
 * The fixed-point representation of 1.0 is 0x100 or 256 decimal.
 * Therefore, the result of a multiplication operation should be divided by 256.<BR>
 * <P>
 * For any color (R,G,B,A) the transformed color (R', G', B', A') is calculated as follows:
 * <P>
 * R' = (R * RedMultTerm)    / 256<BR>
 * G' = (G * GreenMultTerm)  / 256<BR>
 * B' = (B * BlueMultTerm)   / 256<BR>
 * A' = (A * AlphaMultTerm)  / 256<br>
 * <P>
 * Addition transforms simply add an addition term (positive or negative) to the red, green and blue components of
 * the object being displayed.  If the result is greater than 255, the result is clamped to 255.
 * If the result is less than zero, the result is clamped to zero.
 * <P>
 * For any color (R,G,B,A) the transformed color (R', G', B', A') is calculated as follows:
 * <P>
 * R' = max(0, min(R + RedAddTerm,    255))<BR>
 * G' = max(0, min(G + GreenAddTerm,  255))<BR>
 * B' = max(0, min(B + BlueAddTerm,   255))<BR>
 * A' = max(0, min(A + AlphaAddTerm,  255))<br>
 * <P>
 * Addition and Multiplication transforms can be combined as below.
 * The multiplication operation is performed first.
 * <P>
 * R' = max(0, min(((R * RedMultTerm)    / 256) + RedAddTerm,    255))<BR>
 * G' = max(0, min(((G * GreenMultTerm)  / 256) + GreenAddTerm,  255))<BR>
 * B' = max(0, min(((B * BlueMultTerm)   / 256) + BlueAddTerm,   255))<BR>
 * A' = max(0, min(((A * AlphaMultTerm)  / 256) + AlphaAddTerm,  255))<br>
 *
 * @author Dmitry Skavish
 */
public final class CXForm extends FlashItem {

    private int rAdd;
    private int gAdd;
    private int bAdd;
    private int aAdd;
    private int rMul;
    private int gMul;
    private int bMul;
    private int aMul;
    private boolean withAlpha;

    public CXForm() {}

    /**
     * Creates color transformation
     *
     * @param withAlpha if true creates transformation with alpha
     */
    public CXForm( boolean withAlpha ) {
        this.withAlpha = withAlpha;
    }

    public int getRedAdd()   {
        return rAdd;
    }
    public int getGreenAdd() {
        return gAdd;
    }
    public int getBlueAdd()  {
        return bAdd;
    }
    public int getAlphaAdd() {
        return aAdd;
    }

    public int getRedMul()   {
        return rMul;
    }
    public int getGreenMul() {
        return gMul;
    }
    public int getBlueMul()  {
        return bMul;
    }
    public int getAlphaMul() {
        return aMul;
    }

    public void setRedAdd( int v )   {
        rAdd = v;
    }
    public void setGreenAdd( int v ) {
        gAdd = v;
    }
    public void setBlueAdd( int v )  {
        bAdd = v;
    }
    public void setAlphaAdd( int v ) {
        aAdd = v;
    }

    public void setRedMul( int v )   {
        rMul = v;
    }
    public void setGreenMul( int v ) {
        gMul = v;
    }
    public void setBlueMul( int v )  {
        bMul = v;
    }
    public void setAlphaMul( int v ) {
        aMul = v;
    }

    public void setWithAlpha( boolean withAlpha ) {
        this.withAlpha = withAlpha;
    }

    private boolean hasAdd() {
        return rAdd != 0 || gAdd != 0 || bAdd != 0 || aAdd != 0;
    }

    private boolean hasMul() {
        return rMul != 256 || gMul != 256 || bMul != 256 || aMul != 256;
    }

    /**
     * Creates transformation which being applied to colors reduces or increases brightness
     *
     * @param f         value between -1.0 and 1.0<br>
     *                  from -1 to 0 - reduce brightness<br>
     *                  from 0 to 1  - increase brightness
     * @param withAlpha if true this transformation will be with alpha
     * @return new transformation
     */
    public static CXForm newBrightness( double f, boolean withAlpha ) {
        CXForm cx = newIdentity(withAlpha);
        if( f < 0.0 ) {
            int ff = (int) ((1.0+f)*256.0);
            cx.setRedMul(ff);
            cx.setGreenMul(ff);
            cx.setBlueMul(ff);
        } else {
            int ff = (int) (f*256.0);
            cx.setRedAdd(ff);
            cx.setGreenAdd(ff);
            cx.setBlueAdd(ff);
            ff = 256-ff;
            cx.setRedMul(ff);
            cx.setGreenMul(ff);
            cx.setBlueMul(ff);
        }
        return cx;
    }

    /**
     * Creates identity transformation (no tranformation at all)
     *
     * @param withAlpha if true then create transformation with alpha
     * @return default transformation
     */
    public static CXForm newIdentity( boolean withAlpha ) {
        CXForm cx = new CXForm( withAlpha );
        cx.initDefault();
        return cx;
    }

    /**
     * Creates transformation which being to applied to colors sets their alpha to specified value
     *
     * @param alpha  alpha in range 0..256
     * @return transformation with specified alpha
     */
    public static CXForm newAlpha( int alpha ) {
        CXForm cx = new CXForm(true);
        cx.initDefault();
        cx.aMul = alpha;
        return cx;
    }

    private void initDefault() {
        rMul = gMul = bMul = aMul = 256;
        rAdd = gAdd = bAdd = aAdd = 0;
    }

    /**
     * Creates linear interpolation of the two specified transformation
     *
     * @param t      coefficient of the interpolation [0..1]
     * @param c1     first matrix
     * @param c2     second matrix
     * @return interpolation of two specified matrixes
     */
    public static CXForm interLinear( double t, CXForm c1, CXForm c2 ) {
        double t1 = 1.0-t;
        CXForm n = new CXForm();
        n.withAlpha = c1.withAlpha||c2.withAlpha;
        n.rAdd = (int) (c1.rAdd*t1+c2.rAdd*t);
        n.gAdd = (int) (c1.gAdd*t1+c2.gAdd*t);
        n.bAdd = (int) (c1.bAdd*t1+c2.bAdd*t);
        n.aAdd = (int) (c1.aAdd*t1+c2.aAdd*t);
        n.rMul = (int) (c1.rMul*t1+c2.rMul*t);
        n.gMul = (int) (c1.gMul*t1+c2.gMul*t);
        n.bMul = (int) (c1.bMul*t1+c2.bMul*t);
        n.aMul = (int) (c1.aMul*t1+c2.aMul*t);
        return n;

    }
    /**
     * Transforms specified color
     *
     * @param color  specified color to be transformed
     * @return new transformed color
     */
    public AlphaColor transform( AlphaColor color ) {
        int r = Math.max(0, Math.min(((color.getRed()*rMul)/256)+rAdd, 255));
        int g = Math.max(0, Math.min(((color.getGreen()*gMul)/256)+gAdd, 255));
        int b = Math.max(0, Math.min(((color.getBlue()*bMul)/256)+bAdd, 255));
        int a = Math.max(0, Math.min(((color.getAlpha()*aMul)/256)+aAdd, 255));

        return new AlphaColor(r, g, b, a);
    }

    /**
     * Transforms specified color
     *
     * @param color  specified color to be transformed
     * @return new transformed color
     */
    public Color transform( Color color ) {
        if( color instanceof AlphaColor ) return transform((AlphaColor) color);

        int r = Math.max(0, Math.min(((color.getRed()*rMul)/256)+rAdd, 255));
        int g = Math.max(0, Math.min(((color.getGreen()*gMul)/256)+gAdd, 255));
        int b = Math.max(0, Math.min(((color.getBlue()*bMul)/256)+bAdd, 255));

        return new Color(r, g, b);
    }

    public static CXForm parse( Parser p, boolean withAlpha ) {
        p.initBits();
        CXForm f = new CXForm();
        f.withAlpha = withAlpha;
        boolean hasAdd = p.getBool();
        boolean hasMul = p.getBool();
        int nBits = p.getBits(4);
        if( hasMul ) {
            f.rMul = p.getSBits(nBits);
            f.gMul = p.getSBits(nBits);
            f.bMul = p.getSBits(nBits);
            if( withAlpha ) f.aMul = p.getSBits(nBits);
            else f.aMul = 256;
        } else {
            f.rMul = f.gMul = f.bMul = f.aMul = 256;
        }
        if( hasAdd ) {
            f.rAdd = p.getSBits(nBits);
            f.gAdd = p.getSBits(nBits);
            f.bAdd = p.getSBits(nBits);
            if( withAlpha ) f.aAdd = p.getSBits(nBits);
            else f.aAdd = 0;
        } else {
            f.rAdd = f.gAdd = f.bAdd = f.aAdd = 0;
        }
        return f;
    }

    public void write( FlashOutput fob ) {
        fob.initBits();
        if( withAlpha ) writeWithAlpha( fob );
        else writeNoAlpha( fob );
        fob.flushBits();
    }

    protected final void writeWithAlpha( FlashOutput fob ) {
        int nBits;
        boolean hasAdd = hasAdd();
        boolean hasMul = hasMul();
        if( hasAdd && hasMul ) {
            int max1 = Util.getMax(rAdd, gAdd, bAdd, aAdd);
            int max2 = Util.getMax(rMul, gMul, bMul, aMul);
            nBits = Util.getMinBitsS( Util.getMax(max1, max2) );
            fob.writeBits(0x3, 2);
        } else if( hasAdd ) {
            nBits = Util.getMinBitsS( Util.getMax(rAdd, gAdd, bAdd, aAdd) );
            fob.writeBits(0x2, 2);
        } else if( hasMul ) {
            nBits = Util.getMinBitsS( Util.getMax(rMul, gMul, bMul, aMul) );
            fob.writeBits(0x1, 2);
        } else {
            nBits = 0;
            fob.writeBits(0x0, 2);
        }
        fob.writeBits(nBits, 4);
        if( hasMul ) {
            fob.writeBits(rMul, nBits);
            fob.writeBits(gMul, nBits);
            fob.writeBits(bMul, nBits);
            fob.writeBits(aMul, nBits);
        }
        if( hasAdd ) {
            fob.writeBits(rAdd, nBits);
            fob.writeBits(gAdd, nBits);
            fob.writeBits(bAdd, nBits);
            fob.writeBits(aAdd, nBits);
        }
    }

    protected final void writeNoAlpha( FlashOutput fob ) {
        int nBits;
        boolean hasAdd = hasAdd();
        boolean hasMul = hasMul();
        if( hasAdd && hasMul ) {
            int max1 = Util.getMax(rAdd, gAdd, bAdd);
            int max2 = Util.getMax(rMul, gMul, bMul);
            nBits = Util.getMinBitsS( Util.getMax(max1, max2) );
            fob.writeBits(0x3, 2);
        } else if( hasAdd ) {
            nBits = Util.getMinBitsS( Util.getMax(rAdd, gAdd, bAdd) );
            fob.writeBits(0x2, 2);
        } else if( hasMul ) {
            nBits = Util.getMinBitsS( Util.getMax(rMul, gMul, bMul) );
            fob.writeBits(0x1, 2);
        } else {
            nBits = 0;
            fob.writeBits(0x0, 2);
        }
        fob.writeBits(nBits, 4);
        if( hasMul ) {
            fob.writeBits(rMul, nBits);
            fob.writeBits(gMul, nBits);
            fob.writeBits(bMul, nBits);
        }
        if( hasAdd ) {
            fob.writeBits(rAdd, nBits);
            fob.writeBits(gAdd, nBits);
            fob.writeBits(bAdd, nBits);
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"CXForm: withAlpha="+withAlpha );
        out.println( indent+"    Add: |0x"+Util.b2h(rAdd)+",0x"+Util.b2h(gAdd)+",0x"+Util.b2h(bAdd)+",0x"+Util.b2h(aAdd)+"|" );
        out.println( indent+"    Mul: |0x"+Util.w2h(rMul)+",0x"+Util.w2h(gMul)+",0x"+Util.w2h(bMul)+",0x"+Util.w2h(aMul)+"|" );
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((CXForm)item).rAdd  = rAdd;
        ((CXForm)item).gAdd  = gAdd;
        ((CXForm)item).bAdd  = bAdd;
        ((CXForm)item).aAdd  = aAdd;
        ((CXForm)item).rMul = rMul;
        ((CXForm)item).gMul = gMul;
        ((CXForm)item).bMul = bMul;
        ((CXForm)item).aMul = aMul;
        ((CXForm)item).withAlpha = withAlpha;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new CXForm(), copier );
    }
}

