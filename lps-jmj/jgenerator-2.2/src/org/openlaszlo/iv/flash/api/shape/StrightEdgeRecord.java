/*
 * $Id: StrightEdgeRecord.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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
 * Straight-edge record.
 * <P>
 * The straight-edge record stores the edge as an X-Y delta.
 * The delta is added to the current drawing position,
 * and this becomes the new drawing position.
 * The edge is rendered between the old and new drawing positions.
 * <P>
 * Straight-edge records support three types of line:
 * <OL>
 * <LI>General lines.
 * <LI>Horizontal lines.
 * <LI>Vertical lines.
 * </OL>
 * General lines store both X & Y deltas, the horizontal and vertical
 * lines store only the X delta and Y delta respectively.
 *
 * @author Dmitry Skavish
 */
public final class StrightEdgeRecord extends FlashItem {

    public static final int GENERAL_LINE = 0;
    public static final int VERT_LINE    = 1;
    public static final int HORIZ_LINE   = 2;

    private int type;         // type of this record
    private int deltaX;       // delta X
    private int deltaY;       // delta Y

    public StrightEdgeRecord() {}

    public int getType() {
        return type;
    }

    public void setType( int type ) {
        this.type = type;
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

    /**
     * Creates general line
     *
     * @param deltaX new X delta
     * @param deltaY new Y delta
     * @return general line
     */
    public static StrightEdgeRecord newLine( int deltaX, int deltaY ) {
        StrightEdgeRecord sr = new StrightEdgeRecord();
        sr.setType( GENERAL_LINE );
        sr.setDeltaX( deltaX );
        sr.setDeltaY( deltaY );
        return sr;
    }

    /**
     * Creates horizontal line
     *
     * @param deltaX new X delta
     * @return horizontal line
     */
    public static StrightEdgeRecord newHLine( int deltaX ) {
        StrightEdgeRecord sr = new StrightEdgeRecord();
        sr.setType( HORIZ_LINE );
        sr.setDeltaX( deltaX );
        return sr;
    }

    /**
     * Creates vertical line
     *
     * @param deltaY new Y delta
     * @return vertical line
     */
    public static StrightEdgeRecord newVLine( int deltaY ) {
        StrightEdgeRecord sr = new StrightEdgeRecord();
        sr.setType( VERT_LINE );
        sr.setDeltaY( deltaY );
        return sr;
    }

    public void write( FlashOutput fob ) {
        fob.writeBits(0x3, 2);
        switch( type ) {
            case GENERAL_LINE: {
                int nBits = Util.getMinBitsS( Util.getMax(deltaX, deltaY) );
                if( nBits < 3 ) nBits = 3;
                fob.writeBits(nBits-2, 4);
                fob.writeBit(1);
                fob.writeBits(deltaX, nBits);
                fob.writeBits(deltaY, nBits);
                break;
            }
            case VERT_LINE: {
                int nBits = Util.getMinBitsS( deltaY );
                if( nBits < 3 ) nBits = 3;
                fob.writeBits(nBits-2, 4);
                fob.writeBit(0);
                fob.writeBit(1);
                fob.writeBits(deltaY, nBits);
                break;
            }
            case HORIZ_LINE: {
                int nBits = Util.getMinBitsS( deltaX );
                if( nBits < 3 ) nBits = 3;
                fob.writeBits(nBits-2, 4);
                fob.writeBit(0);
                fob.writeBit(0);
                fob.writeBits(deltaX, nBits);
                break;
            }
        }
    }

    public void printContent( PrintStream out, String indent ) {
        switch( type ) {
            case GENERAL_LINE:
                out.println( indent+"    line ("+deltaX+","+deltaY+")" );
                break;
            case VERT_LINE:
                out.println( indent+"    vline ("+deltaY+")" );
                break;
            case HORIZ_LINE:
                out.println( indent+"    hline ("+deltaX+")" );
                break;
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        ((StrightEdgeRecord)item).type = type;
        ((StrightEdgeRecord)item).deltaX = deltaX;
        ((StrightEdgeRecord)item).deltaY = deltaY;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new StrightEdgeRecord(), copier );
    }
}
