/*
 * $Id: CurvedEdgeRecord.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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
import java.io.PrintStream;
import org.openlaszlo.iv.flash.api.*;

/**
 * CurveEdge record.
 * <P>
 * SWF differs from most vector file formats by using Quadratic Bezier
 * curves rather than Cubic Bezier curves. PostScript uses Cubic Beziers,
 * as do most drawing applications, such as Illustrator, FreeHand and Corel Draw.
 * SWF uses Quadratic Bezier curves because they can be stored more compactly,
 * and can be rendered more efficiently.
 * <P>
 * A Quadratic Bezier curve has 3 points.  Two on-curve anchor points, and one off-curve
 * control point. A Cubic Bezier curve has 4 points.  Two on-curve anchor points,
 * and two off-curve control points.
 * <P>
 * The curved-edge record stores the edge as two X-Y deltas.
 * The three points that define the Quadratic Bezier are calculated like this:
 * <UL>
 * <LI>The first anchor point is the current drawing position.
 * <LI>The control point is the current drawing position + ControlDelta.
 * <LI>The last anchor point is the current drawing position + ControlDelta + AnchorDelta.
 * </UL>
 * The last anchor point becomes the current drawing position.
 *
 * @author Dmitry Skavish
 */
public final class CurvedEdgeRecord extends FlashItem {

    private int controlDeltaX;
    private int controlDeltaY;
    private int anchorDeltaX;
    private int anchorDeltaY;

    public CurvedEdgeRecord() {}

    public CurvedEdgeRecord( int controlDeltaX, int controlDeltaY, int anchorDeltaX, int anchorDeltaY ) {
        setControlDeltaX(controlDeltaX);
        setControlDeltaY(controlDeltaY);
        setAnchorDeltaX(anchorDeltaX);
        setAnchorDeltaY(anchorDeltaY);
    }

    public int getControlDeltaX() {
        return controlDeltaX;
    }
    public void setControlDeltaX( int controlDeltaX ) {
        this.controlDeltaX = controlDeltaX;
    }

    public int getControlDeltaY() {
        return controlDeltaY;
    }
    public void setControlDeltaY( int controlDeltaY ) {
        this.controlDeltaY = controlDeltaY;
    }

    public int getAnchorDeltaX() {
        return anchorDeltaX;
    }
    public void setAnchorDeltaX( int anchorDeltaX ) {
        this.anchorDeltaX = anchorDeltaX;
    }

    public int getAnchorDeltaY() {
        return anchorDeltaY;
    }
    public void setAnchorDeltaY( int anchorDeltaY ) {
        this.anchorDeltaY = anchorDeltaY;
    }

    public void write( FlashOutput fob ) {
        fob.writeBits(0x2, 2);
        int nBits = Util.getMinBitsS( Util.getMax(controlDeltaX, controlDeltaY, anchorDeltaX, anchorDeltaY) );
        if( nBits < 3 ) nBits = 3;
        fob.writeBits(nBits-2, 4);
        fob.writeBits(controlDeltaX, nBits);
        fob.writeBits(controlDeltaY, nBits);
        fob.writeBits(anchorDeltaX, nBits);
        fob.writeBits(anchorDeltaY, nBits);
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"    curve ("+controlDeltaX+","+controlDeltaY+","+anchorDeltaX+","+anchorDeltaY+")" );
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        ((CurvedEdgeRecord)item).controlDeltaX = controlDeltaX;
        ((CurvedEdgeRecord)item).controlDeltaY = controlDeltaY;
        ((CurvedEdgeRecord)item).anchorDeltaX = anchorDeltaX;
        ((CurvedEdgeRecord)item).anchorDeltaY = anchorDeltaY;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new CurvedEdgeRecord(), copier );
    }
}
