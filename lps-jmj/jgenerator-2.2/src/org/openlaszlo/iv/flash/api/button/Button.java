/*
 * $Id: Button.java,v 1.3 2002/03/31 19:47:26 skavish Exp $
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

import java.io.PrintStream;
import java.awt.geom.*;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.context.Context;

/**
 * Class represents Flash Button
 */
public class Button extends FlashDef {

    protected IVVector buttonRecords = new IVVector();
    protected IVVector conditions = new IVVector(4);
    protected ButtonCXForm buttonCXForm;
    protected ButtonSound buttonSound;
    protected boolean isProcessed;

    public Button() {}

    public int getTag() {
        return Tag.DEFINEBUTTON;
    }

    public void setButtonCXForm( ButtonCXForm bcxf ) {
        this.buttonCXForm = bcxf;
    }

    public ButtonCXForm getButtonCXForm() {
        return this.buttonCXForm;
    }

    public void setButtonSound( ButtonSound bsnd ) {
        this.buttonSound = bsnd;
    }

    public ButtonSound getButtonSound() {
        return this.buttonSound;
    }

    public void addButtonRecord( ButtonRecord br ) {
        buttonRecords.addElement(br);
    }

    public IVVector getButtonRecords() {
        return buttonRecords;
    }

    public void setButtonRecords( IVVector buttonRecords ) {
        this.buttonRecords = buttonRecords;
    }

    public void addActionCondition( ActionCondition ac ) {
        conditions.addElement(ac);
    }

    public IVVector getActionConditions() {
        return conditions;
    }

    public void setActionConditions( IVVector conditions ) {
        this.conditions = conditions;
    }

    public static Button parse( Parser p ) {
        Button o = new Button();
//        o.tagCode = p.getTagCode();
        o.setID( p.getUWord() );
        o.parseButtonRecords(p,false);
        o.addActionCondition( new ActionCondition(0,new Program(p.getBuf(), p.getPos(), p.getTagEndPos())) );
        return o;
    }

    protected void parseButtonRecords( Parser p, boolean withAlpha ) {
        for(;;) {
            int flags = p.getUByte();
            if( flags == 0 ) break;
            ButtonRecord br = new ButtonRecord();
            br.setStates( flags );
            br.setDef( p.getDef( p.getUWord() ) );
            br.setLayer( p.getUWord() );
            br.setMatrix( p.getMatrix() );
            br.setCXForm( CXForm.parse(p, withAlpha) );
            addButtonRecord(br);
        }
    }

    protected void writeButtonRecords( FlashOutput fob ) {
        for( int i=0; i<buttonRecords.size(); i++ ) {
            ButtonRecord t = (ButtonRecord) buttonRecords.elementAt(i);
            t.write(fob);
        }
        fob.writeByte(0);
    }

    public void collectDeps( DepsCollector dc ) {
        //System.out.println( "Button collectDeps: ID="+getID() );
        for( int i=0; i<buttonRecords.size(); i++ ) {
            ButtonRecord t = (ButtonRecord) buttonRecords.elementAt(i);
            //System.out.println( "    collected: ID="+t.getDef().getID() );
            dc.addDep( t.getDef() );
        }
        if( buttonSound != null ) buttonSound.collectDeps(dc);
    }

    public void collectFonts( FontsCollector fc ) {
        for( int i=0; i<buttonRecords.size(); i++ ) {
            ButtonRecord t = (ButtonRecord) buttonRecords.elementAt(i);
            t.getDef().collectFonts(fc);
        }
    }

    /**
     * Returns this button's bounds
     *
     * @return bounds of this button
     */
    public Rectangle2D getBounds() {
        Rectangle2D rect = null;
        Rectangle2D bounds = GeomHelper.newRectangle();
        for( int i=0; i<buttonRecords.size(); i++ ) {
            ButtonRecord t = (ButtonRecord) buttonRecords.elementAt(i);
            // uncomment this if only UP and Hit shapes should be considered
            //if( (t.getStates()&(ButtonRecord.Up|ButtonRecord.HitTest)) == 0 ) continue;
            GeomHelper.calcBounds(t.getMatrix(), t.getDef().getBounds(), bounds);
            rect = GeomHelper.add( rect, bounds );
        }
        return rect;
    }

    public void write( FlashOutput fob ) {
        int pos = fob.getPos();
        fob.skip(6);
        fob.writeDefID( this );
        writeButtonRecords(fob);
        ((ActionCondition)conditions.elementAt(0)).getProgram().write(fob);
        fob.writeLongTagAt(getTag(), fob.getPos()-pos-6, pos);
        writeExternals( fob );
    }

    protected void writeExternals( FlashOutput fob ) {
        if( buttonCXForm != null ) {
            buttonCXForm.setButton( this );
            buttonCXForm.write(fob);
        }
        if( buttonSound != null ) {
            buttonSound.setButton( this );
            buttonSound.write(fob);
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"Button("+Tag.tagNames[getTag()]+"): id="+getID() );
        for( int i=0; i<buttonRecords.size(); i++ ) {
            ButtonRecord t = (ButtonRecord) buttonRecords.elementAt(i);
            t.printContent(out, indent);
        }
        for( int i=0; i<conditions.size(); i++ ) {
            ActionCondition ac = (ActionCondition) conditions.elementAt(i);
            ac.printContent(out, indent);
        }
    }

    protected boolean _isConstant() {
        for( int i=0; i<buttonRecords.size(); i++ ) {
            ButtonRecord t = (ButtonRecord) buttonRecords.elementAt(i);
            if( !t.getDef().isConstant() ) return false;
        }
        for( int i=0; i<conditions.size(); i++ ) {
            ActionCondition ac = (ActionCondition) conditions.elementAt(i);
            if( !ac.getProgram().isConstant() ) return false;
        }
        return true;
    }

    public void process( FlashFile file, Context context ) throws IVException {
        for( int i=0; i<buttonRecords.size(); i++ ) {
            ButtonRecord t = (ButtonRecord) buttonRecords.elementAt(i);
            FlashDef def = t.getDef();
            if( !isProcessed() ) {
                file.processObject(def, context);
            }
        }
    }

    public boolean isProcessed() {
        return isProcessed;
    }

    public void setProcessed() {
        this.isProcessed = true;
    }

    public void apply( Context context ) {
        super.apply( context );
        for( int i=0; i<buttonRecords.size(); i++ ) {
            ButtonRecord t = (ButtonRecord) buttonRecords.elementAt(i);
            t.getDef().apply(context);
        }
        for( int i=0; i<conditions.size(); i++ ) {
            ActionCondition ac = (ActionCondition) conditions.elementAt(i);
            ac.getProgram().apply(context);
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((Button)item).buttonRecords = buttonRecords.getCopy(copier);
        ((Button)item).conditions = conditions.getCopy(copier);
        ((Button)item).buttonCXForm = buttonCXForm != null? (ButtonCXForm)buttonCXForm.getCopy(copier): null;
        ((Button)item).buttonSound = buttonSound != null? (ButtonSound)buttonSound.getCopy(copier): null;
        ((Button)item).isProcessed = isProcessed;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new Button(), copier );
    }

}

