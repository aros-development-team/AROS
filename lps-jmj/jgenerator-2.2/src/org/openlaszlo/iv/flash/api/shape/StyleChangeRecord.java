/*
 * $Id: StyleChangeRecord.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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
import java.io.PrintStream;

/**
 * Stylechange record.
 * <P>
 * The style change record is a non-edge record. It can be used to:
 * <UL>
 * <LI>Select a fill or line style for drawing
 * <LI>Move the current drawing position (without drawing)
 * <LI>Replace the current fill and line style arrays with a new set of styles
 * </UL>
 * Because fill and line styles often change at the start of a new path,
 * it is useful to perform more than one action in a single record.
 * For example, say a DefineShape tag defines a red circle and a blue square.
 * After the circle is closed, it is necessary to move the drawing position,
 * and replace the red fill with the blue fill. The style change record can achieve
 * this with a single shape-record.
 *
 * @author Dmitry Skavish
 */
public final class StyleChangeRecord extends FlashItem {

    public static final int NEW_STYLES = 0x10;
    public static final int LINESTYLE  = 0x08;
    public static final int FILLSTYLE1 = 0x04;
    public static final int FILLSTYLE0 = 0x02;
    public static final int MOVETO     = 0x01;

    private int flags;
    private int deltaX;
    private int deltaY;
    private int fillStyle0;
    private int fillStyle1;
    private int lineStyle;

    public StyleChangeRecord() {}

    public int getFlags() {
        return flags;
    }

    public void setFlags( int flags ) {
        this.flags = flags;
    }

    public void addFlags( int flags ) {
        this.flags |= flags;
    }

    public int getDeltaX() {
        return deltaX;
    }

    public void setDeltaX( int deltaX ) {
        this.deltaX = deltaX;
    }

    public int getDeltaY() {
        return deltaY;
    }

    public void setDeltaY( int deltaY ) {
        this.deltaY = deltaY;
    }

    public int getFillStyle0() {
        return fillStyle0;
    }

    public void setFillStyle0( int fillStyle0 ) {
        this.fillStyle0 = fillStyle0;
    }

    public int getFillStyle1() {
        return fillStyle1;
    }

    public void setFillStyle1( int fillStyle1 ) {
        this.fillStyle1 = fillStyle1;
    }

    public int getLineStyle() {
        return lineStyle;
    }

    public void setLineStyle( int lineStyle ) {
        this.lineStyle = lineStyle;
    }

    public void write( FlashOutput fob ) {
        write(fob, 0, 0);
    }

    public void write( FlashOutput fob, int nFillBits, int nLineBits ) {
        fob.writeBits(flags, 6);
        if( (flags&MOVETO) != 0 ) {
            int nBits = Util.getMinBitsS( Util.getMax(deltaX, deltaY) );
            fob.writeBits(nBits, 5);
            fob.writeBits(deltaX, nBits);
            fob.writeBits(deltaY, nBits);
        }
        if( (flags&FILLSTYLE0) != 0 ) {
            fob.writeBits(fillStyle0, nFillBits);
        }
        if( (flags&FILLSTYLE1) != 0 ) {
            fob.writeBits(fillStyle1, nFillBits);
        }
        if( (flags&LINESTYLE) != 0 ) {
            fob.writeBits(lineStyle, nLineBits);
        }
    }

    public void printContent( PrintStream out, String indent ) {
        if( (flags&MOVETO) != 0 ) {
            out.println( indent+"    moveto ("+deltaX+","+deltaY+")" );
        }
        if( (flags&FILLSTYLE0) != 0 ) {
            out.println( indent+"    fillStyle0 "+fillStyle0 );
        }
        if( (flags&FILLSTYLE1) != 0 ) {
            out.println( indent+"    fillStyle1 "+fillStyle1 );
        }
        if( (flags&LINESTYLE) != 0 ) {
            out.println( indent+"    lineStyle "+lineStyle );
        }
        if( (flags&NEW_STYLES) != 0 ) {
            out.println( indent+"    newstyles" );
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        ((StyleChangeRecord)item).deltaX = deltaX;
        ((StyleChangeRecord)item).deltaY = deltaY;
        ((StyleChangeRecord)item).fillStyle0 = fillStyle0;
        ((StyleChangeRecord)item).fillStyle1 = fillStyle1;
        ((StyleChangeRecord)item).lineStyle = lineStyle;
        ((StyleChangeRecord)item).flags = flags;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new StyleChangeRecord(), copier );
    }
}
