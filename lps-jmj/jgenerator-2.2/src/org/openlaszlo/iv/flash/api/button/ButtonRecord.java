/*
 * $Id: ButtonRecord.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
 *
 * ===========================================================================
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

package org.openlaszlo.iv.flash.api.button;

import java.awt.geom.AffineTransform;
import java.io.PrintStream;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;

public class ButtonRecord extends FlashItem {

    public static final int HitTest = 0x08;
    public static final int Down    = 0x04;
    public static final int Over    = 0x02;
    public static final int Up      = 0x01;

    private int states;
    private FlashDef def;
    private int layer;
    private AffineTransform matrix;
    private CXForm cxform;

    public ButtonRecord() {}

    public ButtonRecord( int states, FlashDef def, int layer, AffineTransform matrix, CXForm cxform ) {
        setStates( states );
        setDef( def );
        setLayer( layer );
        setMatrix( matrix );
        setCXForm( cxform );
    }

    public void setStates( int states ) { this.states = states; }
    public void setDef( FlashDef def ) { this.def = def; }
    public void setLayer( int layer ) { this.layer = layer; }
    public void setMatrix( AffineTransform matrix ) { this.matrix = matrix; }
    public void setCXForm( CXForm cxform ) { this.cxform = cxform; }

    public int getStates() { return states; }
    public FlashDef getDef() { return def; }
    public int getLayer() { return layer; }
    public AffineTransform getMatrix() { return matrix; }
    public CXForm getCXForm() { return cxform; }

    public void write( FlashOutput fob ) {
        fob.writeByte( states );
        fob.writeDefID( def );
        fob.writeWord( layer );
        fob.write( matrix );
        cxform.write( fob );
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"   ButtonRecord: charID="+def.getID()+" states="+states+" layer="+layer );
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        ButtonRecord br = new ButtonRecord();
        br.states = states;
        br.def = copier.copy(def);
        br.layer = layer;
        br.matrix = (AffineTransform) matrix.clone();
        br.cxform = (CXForm) cxform.getCopy(copier);
        return br;
    }

}

