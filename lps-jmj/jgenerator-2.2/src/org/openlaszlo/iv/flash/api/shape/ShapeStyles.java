/*
 * $Id: ShapeStyles.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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
 * This object represents collection of shape styles.
 *
 * @author Dmitry Skavish
 */
public final class ShapeStyles extends FlashItem {

    /**
     * Vector of {@link FillStyle}s records
     */
    public IVVector fillStyles;

    /**
     * Vector of {@link LineStyle}s records.
     */
    public IVVector lineStyles;

    /**
     * Creates empty shapestyle
     */
    public ShapeStyles() {
        this(new IVVector(), new IVVector());
    }

    public ShapeStyles( IVVector fillStyles, IVVector lineStyles ) {
        this.fillStyles = fillStyles;
        this.lineStyles = lineStyles;
    }

    /**
     * Adds new fillstyle to this array.
     *
     * @param fs     new fillstyle to be added
     * @return index of added fillstyle
     */
    public int addFillStyle( FillStyle fs ) {
        fillStyles.addElement(fs);
        return fillStyles.size();
    }

    /**
     * Adds new linestyle to this array.
     *
     * @param ls     new linestyle to be added
     * @return index of added linestyle
     */
    public int addLineStyle( LineStyle ls ) {
        lineStyles.addElement(ls);
        return lineStyles.size();
    }

    /**
     * Returns index of specified fillstyle
     *
     * @param fs     fillstyle which index is to be returned
     * @return index of specified fillstyle or -1
     */
    public int getFillStyleIndex( FillStyle fs ) {
        for( int i=0; i<fillStyles.size(); i++ ) {
            FillStyle ffs = (FillStyle) fillStyles.elementAt(i);
            if( ffs == fs ) return i+1;
        }
        return -1;
    }

    /**
     * Returns index of specified linestyle
     *
     * @param ls     linestyle which index is to be returned
     * @return index of specified linestyle or -1
     */
    public int getLineStyleIndex( LineStyle ls ) {
        for( int i=0; i<lineStyles.size(); i++ ) {
            LineStyle lls = (LineStyle) lineStyles.elementAt(i);
            if( lls == ls ) return i+1;
        }
        return -1;
    }

    /**
     * Returns line style by its index
     *
     * @param index  index of line style to be returned
     * @return line style at specified index
     */
    public LineStyle getLineStyle( int index ) {
        return (LineStyle) lineStyles.elementAt(index);
    }

    /**
     * Returns fill style by its index
     *
     * @param index  index of fill style to be returned
     * @return fill style at specified index
     */
    public FillStyle getFillStyle( int index ) {
        return (FillStyle) fillStyles.elementAt(index);
    }

    /**
     * Returns maximum number of bits required to hold fillstyles' indexes
     *
     * @return maximum number of bits required to hold fillstyles' indexes
     */
    public int calcNFillBits() {
        return Util.getMinBitsU( fillStyles.size() );
    }

    /**
     * Returns maximum number of bits required to hold linestyles' indexes
     *
     * @return maximum number of bits required to hold linestyles' indexes
     */
    public int calcNLineBits() {
        return Util.getMinBitsU( lineStyles.size() );
    }

    public void collectDeps( DepsCollector dc ) {
        int fillStyleSize = fillStyles.size();
        for( int i=0; i<fillStyleSize; i++ ) {
            FillStyle fillStyle = (FillStyle) fillStyles.elementAt(i);
            if( fillStyle.getBitmap() != null ) dc.addDep(fillStyle.getBitmap());
        }
    }

    public static ShapeStyles parse( Parser p, boolean withAlpha ) {
        // Get the number of fills.
        int nFills = p.getUByte();
        if( nFills == 255 ) {
            nFills = p.getUWord();
        }
        IVVector fillStyles = new IVVector(nFills);

        // Get each of the fill style.
        for( int i=0; i<nFills; i++ ) {
            fillStyles.addElement( FillStyle.parse(p, withAlpha) );
        }

        int nLines = p.getUByte();
        if( nLines == 255 ) {
            nLines = p.getUWord();
        }
        IVVector lineStyles = new IVVector(nLines);

        // Get each of the line styles.
        for( int i=0; i<nLines; i++ ) {
            lineStyles.addElement( LineStyle.parse(p, withAlpha) );
        }

        return new ShapeStyles(fillStyles, lineStyles);
    }

    public void write( FlashOutput fob ) {
        int fillStyleSize = fillStyles.size();
        if( fillStyleSize > 254 ) {
            fob.writeByte(255);
            fob.writeWord(fillStyleSize);
        } else {
            fob.writeByte(fillStyleSize);
        }
        fillStyles.write(fob);
        int lineStyleSize = lineStyles.size();
        if( lineStyleSize > 254 ) {
            fob.writeByte(255);
            fob.writeWord(lineStyleSize);
        } else {
            fob.writeByte(lineStyleSize);
        }
        lineStyles.write(fob);
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"ShapeStyles:" );
        fillStyles.printContent(out, indent+"    ");
        lineStyles.printContent(out, indent+"    ");
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        ((ShapeStyles)item).fillStyles = fillStyles.getCopy(copier);
        ((ShapeStyles)item).lineStyles = lineStyles.getCopy(copier);
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new ShapeStyles(null, null), copier );
    }
}
