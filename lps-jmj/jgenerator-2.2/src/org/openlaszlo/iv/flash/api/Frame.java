/*
 * $Id: Frame.java,v 1.10 2002/08/08 23:26:54 skavish Exp $
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

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.commands.*;

import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.context.Context;

import java.awt.geom.AffineTransform;
import java.awt.geom.Rectangle2D;
import java.io.*;
import java.util.*;

/**
 * A movie frame
 * <P>
 * A frame contains zero or more FlashObjects.
 * A frame can have a name (to be referenced in gotoFrame action).
 *
 * @author Dmitry Skavish
 * @see Timeline
 */
public final class Frame extends IVVector {

    private String name;
    private boolean is_anchor;     // flash mx

    /**
     * Creates empty frame
     */
    public Frame() {}

    /**
     * Creates frame of specified capacity
     *
     * @param capacity capacity of created frame
     */
    public Frame( int capacity ) {
        super( capacity );
    }

    /**
     * Creates frame from specified vector
     * <P>
     * Creates frame with capacity equal to the size of specified vector
     * and copies all data from the specified vector to this frame
     *
     * @param data   vector to copy from
     */
    public Frame( IVVector data ) {
        super( data );
    }

    /**
     * Returns name of this frame
     *
     * @return name of this frame or null
     */
    public String getName() {
        return name;
    }

    /**
     * Assings name to this frame
     *
     * @param name   new name of this frame
     */
    public void setName( String name ) {
        this.name = name;
    }

    /**
     * Sets whether this frame is named anchor or not
     * <P>
     * Only for Flash MX
     *
     * @param is_anchor this frame is named anchor if its true
     */
    public void setAnchor( boolean is_anchor ) {
        this.is_anchor = is_anchor;
    }

    /**
     * Returns whether this frame is named anchor or not
     * <P>
     * Only for Flash MX
     *
     * @return whether this frame is named anchor or not
     */
    public boolean isAnchor() {
        return is_anchor;
    }

    /**
     * Adds flash object's instance to this frame
     * <P>
     * Creates new instance and adds it to this frame using specified
     * depth, matrix and cxform.
     *
     * @param def    flash definition instance of which has to be added
     * @param depth  layer depth of the instance to be added
     * @param matrix transformation matrix for the instance (optional)
     * @param cxform transformation color for the instance (optional)
     * @return added instance
     * @see Instance
     */
    public Instance addInstance( FlashDef def, int depth, AffineTransform matrix, CXForm cxform ) {
        return addInstance(def, depth, matrix, cxform, null);
    }

    /**
     * Adds flash object's instance to this frame
     * <P>
     * Creates new instance and adds it to this frame using specified
     * depth, matrix, cxform and name.
     *
     * @param def    flash definition instance of which has to be added
     * @param depth  layer depth of the instance to be added
     * @param matrix transformation matrix for the instance (optional)
     * @param cxform transformation color for the instance (optional)
     * @param name   name of the instance (optional)
     * @return added instance
     * @see Instance
     */
    public Instance addInstance( FlashDef def, int depth, AffineTransform matrix, CXForm cxform, String name ) {
        Instance inst = new Instance();
        inst.def = def;
        inst.depth = depth;
        inst.matrix = matrix;
        inst.cxform = cxform;
        inst.name = name;
        addFlashObject( inst );
        return inst;
    }

    /**
     * Adds new instance of flash object which is already on a timeline with new transformation matrix
     * <P>
     * Creates new instance of an existing on a timeline flash object
     * and assigns specified transformation matrix to it.
     *
     * @param depth  layer depth of the existing instance
     * @param matrix transformation matrix for the instance (optional)
     * @return added instance
     */
    public Instance addInstance( int depth, AffineTransform matrix ) {
        return addInstance(depth, matrix, null);
    }

    /**
     * Adds new instance of flash object which is already on a timeline with new transformation matrix
     * <P>
     * Creates new instance of an existing on a timeline flash object
     * and assigns specified transformation and color matrixes to it.
     *
     * @param depth  layer depth of the existing instance
     * @param matrix transformation matrix for the instance (optional)
     * @param cxform color matrix for the instance (optional)
     * @return added instance
     */
    public Instance addInstance( int depth, AffineTransform matrix, CXForm cxform ) {
        Instance inst = new Instance();
        inst.depth = depth;
        inst.matrix = matrix;
        inst.cxform = cxform;
        inst.isMove = true;
        addFlashObject( inst );
        return inst;
    }

    /**
     * Adds specified instance into specified layer
     *
     * @param inst   specified instance
     * @param depth  layer depth
     * @return added instance
     */
    public Instance addInstance( Instance inst, int depth ) {
        inst.depth = depth;
        addFlashObject( inst );
        return inst;
    }

    /**
     * "Removes" flash object's instance from specified layer
     * <p>
     * Does not actually remove any instance from this frame, but rather adds
     * flash tag RemoveObject to this frame.
     *
     * @param depth layer depth of the existing instance to be removed
     * @return RemoveObject tag
     */
    public RemoveObject removeInstance( int depth ) {
        RemoveObject ro = new RemoveObject();
        ro.depth = depth;
        addFlashObject( ro );
        return ro;
    }

    /**
     * Adds specified flash object to this frame
     *
     * @param o      flash object to be added
     */
    public void addFlashObject( FlashObject o ) {
        addElement( o );
    }

    /**
     * Replaces flash object at specified index with new one
     *
     * @param o      new flash object to be replaced with
     * @param index  specified index
     */
    public void setFlashObjectAt( FlashObject o, int index ) {
        setElementAt( o, index );
    }

    /**
     * Returns flash object at specified index
     *
     * @param index  specified index
     * @return flash object at specified index
     */
    public FlashObject getFlashObjectAt( int index ) {
        return (FlashObject) elementAt( index );
    }

    /**
     * Removes flash object at specified index
     *
     * @param index  specified index
     * @return removed flash object
     */
    public FlashObject removeFlashObjectAt( int index ) {
        return (FlashObject) removeElementAt( index );
    }

    /**
     * Removes specified flash object from this frame
     *
     * @param o      specified flash object to be removed
     */
    public void remove( FlashObject o ) {
        removeElement( o );
    }

    /**
     * Writes content of this frame to flash buffer
     * <P>
     * Has to be used only when this frame is NOT from main timeline.
     *
     * @param fob    flash buffer to write to
     */
    public void write( FlashOutput fob ) {
        if( name != null ) writeFrameLabel(fob);
        super.write( fob );
        Tag.SHOWFRAME_TAG.write( fob );
    }

    /**
     * Writes content of this frame to flash buffer
     * <P>
     * Has to be used only when this frame is from main timeline.
     *
     * @param fob    flash buffer to write to
     * @param dc     dependencies collector
     */
    public void generate( FlashOutput fob, DepsCollector dc ) {
        if( name != null ) writeFrameLabel(fob);
        for( int i=0; i<top; i++ ) {
            FlashObject fo = (FlashObject) objects[i];
            fo.generate( fob, dc );
        }
        Tag.SHOWFRAME_TAG.write( fob );
    }

    /**
     * Collects dependencies of all the object from this frame
     *
     * @param dc     dependencies collector
     */
    public void collectDeps( DepsCollector dc ) {
        for( int i=0; i<top; i++ ) {
            FlashObject fo = (FlashObject) objects[i];
            fo.collectDeps(dc);
        }
    }

    /**
     * Processes all the scripts from this frame in the specified context
     *
     * @param file    flash file to be used for processing scripts
     * @param context context to be used for processing scripts
     * @exception IVException
     */
    public void process( FlashFile file, Context context ) throws IVException {
        name = context.apply( name );
        for( int i=0; i<top; i++ ) {
            FlashObject fo = (FlashObject) objects[i];
            file.processObject(fo, context);
        }
    }

    /**
     * Applies specified context to all the object from this frame
     *
     * @param context specified context
     */
    public void apply( Context context ) {
        name = context.apply(name);
        for( int i=0; i<top; i++ ) {
            FlashObject fo = (FlashObject) objects[i];
            fo.apply(context);
        }
    }

    /**
     * Performs all generator commands from this frame
     *
     * @param file    flash file to used for processing commands
     * @param context context to used for processing commands
     * @param parent  parent script
     * @param frame   this frame's number in the parent script's timelime
     * @exception IVException thrown if there were some errors during command processing
     */
    public void doCommand( FlashFile file, Context context, Script parent, int frame ) throws IVException {
        for( int i=0; i<top; i++ ) {
            FlashObject fo = (FlashObject) objects[i];
            if( !(fo instanceof Instance) ) continue;
            Instance instance = (Instance) fo;
            GenericCommand cmd = instance.command;

            if( cmd != null ) {
                try {
                    cmd.doCommand( file, context, parent, frame );
                } catch( Throwable t ) {
                    Log.logRB( Resource.ERRDOCMD, new Object[] {
                                    file.getFullName(), parent.getName(),
                                    String.valueOf(frame+1), cmd.getCommandName() }, t );

                    if( PropertyManager.showErrorsInline ) {
                        // create script with gray rectangle and text in place of the placeholder
                        Script script = new Script(1);
                        Frame fr = script.newFrame();
                        Shape shape = new Shape();
                        shape.setLineStyle( new LineStyle(20, AlphaColor.black) );
                        shape.setFillStyle0( FillStyle.newSolid( new AlphaColor( 150, 150, 150, 150 ) ) );
                        Rectangle2D r = GeomHelper.newRectangle(-1024, -1024, 2048, 2048);
                        shape.drawRectangle(r);
                        shape.setBounds(r);
                        fr.addInstance(shape, 1, null, null);

                        FlashFile defFile = file.getDefaultSymbolFile();
                        if( defFile != null ) {
                            Font font = defFile.getFont( "Arial" );
                            if( font != null ) {
                                String msg = cmd.getCommandName()+" : "+t.getMessage();
                                Text text = Text.newText();
                                TextItem item = new TextItem( msg, font, 8*20, AlphaColor.black );
                                text.addTextItem( item );
                                text.setBounds( (Rectangle2D) r.clone() );
                                AffineTransform m = AffineTransform.getTranslateInstance(-1024, -1024);
                                fr.addInstance(text, 2, m, null);
                            }
                        }
                        instance.def = script;
                        instance.command = null;
                    } else {
                        // just put empty shape as a definition for the instance
                        instance.def = Shape.newEmptyShape1();
                        instance.command = null;
                    }
                }
            }
        }
    }

    /**
     * Adds bounds of all the object from this frame to the specified rectangle
     *
     * @param rect   specified bounds
     */
    public void addBounds( Rectangle2D rect ) {
        for( int i=0; i<top; i++ ) {
            FlashObject fo = (FlashObject) objects[i];
            GeomHelper.add( rect, fo.getBounds() );
        }
    }

    /**
     * Appends all the objects from the specified frame to this one
     *
     * @param f      frame to be added to this one
     */
    public void append( Frame f ) {
        for( int i=0; i<f.size(); i++ ) {
            addFlashObject( f.getFlashObjectAt(i) );
        }
    }

    /**
     * Returns true if this frame is constant
     * <P>
     * Returns true if all the objects from this frame are constants.
     *
     * @return true - if this frame is constant
     */
    public boolean isConstant() {
        if( Util.hasVar(name) ) return false;
        for( int i=0; i<top; i++ ) {
            FlashObject fo = (FlashObject) objects[i];
            if( !fo.isConstant() ) return false;
        }
        return true;
    }

    /**
     * Creates a copy of this frame
     * <P>
     * The copy contains copies of all the objects
     *
     * @param copier copier to be used when copying
     * @return Frame casted to IVVector
     */
    public IVVector getCopy( ScriptCopier copier ) {
        Frame c = new Frame( size() );
        for( int i=0; i<top; i++ ) {
            FlashObject fo = (FlashObject) objects[i];
            c.setElementAt( fo.getCopy(copier), i );
        }
        c.setName( name );
        return c;
    }

    /**
     * Add a stop action to this frame. We do this a lot so it's nice to
     * have this code in one place.
     */
     public void addStopAction() {
        DoAction action = new DoAction();

        action.program = new Program();
        action.program.stop();

        addFlashObject( action );
     }

     private void writeFrameLabel( FlashOutput fob ) {
         fob.writeTag( Tag.FRAMELABEL, name.length()+(is_anchor?2:1) );
         fob.writeStringZ( name );
         if( is_anchor ) fob.writeByte(1);
     }

}

