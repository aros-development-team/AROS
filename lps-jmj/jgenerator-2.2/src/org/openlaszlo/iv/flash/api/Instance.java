/*
 * $Id: Instance.java,v 1.2 2002/02/15 23:44:27 skavish Exp $
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

import java.awt.geom.*;

import java.io.PrintStream;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.commands.*;
import org.openlaszlo.iv.flash.context.Context;

/**
 * Instance of flash definition
 * <P>
 * This object represents flash file format's PlaceObject2 as well as PlaceObject tags.
 * <P>
 * PlaceObject2 can both add a character to the display list, and modify the attributes of a
 * character that is already on the display list.
 * <p>
 * The tag begins with a group of flags that indicate which fields are present in the tag.
 * The optional fields are <code>def</code>, <code>matrix</code>, <code>cxform</code>,
 * <code>ratio</code>, <code>clip</code> and <code>name</code>.
 * The <code>depth</code> field is the only field that is always required.
 * <p>
 * The depth value determines the stacking order of the character. Characters with lower depth values
 * are displayed underneath characters with higher depth values.  A depth value of 1 means the character
 * is displayed at the bottom of the stack.  There can be only one character at any given depth.
 * This means a character that is already on the display list can be identified by its depth alone.
 * (i.e. a <code>def</code> is not required).
 * <p>
 * Flag Move and flag HasCharacter indicate whether a new character is being added to the display list,
 * or a character already on the display list is being modified. The meaning of the flags is as follows:<br>
 * <ul>
 * <li> PlaceFlagMove = 0 and PlaceFlagHasCharacter = 1<br>
 *      A new character (with ID of CharacterId) is placed on the display list at the specified Depth.
 *      Other fields set the attributes of this new character.
 * <li> PlaceFlagMove = 1 and PlaceFlagHasCharacter = 0<br>
 *      The character at the specified Depth is modified. Other fields modify the attributes of this character.
 *      Because there can be only one character at any given depth, no CharacterId is required.
 * <li> PlaceFlagMove = 1 and PlaceFlagHasCharacter = 1<br>
 *      The character at the specified Depth is removed, and a new character (with ID of CharacterId) is
 *      placed at that depth. Other fields set the attributes of this new character.
 * </ul>
 * <p>
 * The optional fields in PlaceObject2 have the following meaning:<br>
 * <ul>
 * <li>The <code>def</code> field specifies the character to be added to the display list.
 *     It only used only when a new character is being added.  If a character that is already on
 *     the display list is being modified, the <code>def</code> field is absent.
 * <li>The <code>matrix</code> field specifies the position, scale and rotation of the character being added or modified.
 * <li>The <code>cxform</code> field specifies the color effect applied to the character being added or modified.
 * <li>The <code>ratio</code> field specifies a morph ratio for the character being added or modified.
 *     This field applies only to characters defined with DefineMorphShape,
 *     and controls how far the morph has progressed.
 *     A ratio of zero displays the character at the start of the morph. A ratio of 65535 displays the character
 *     at the end of the morph. For values between zero and 65535 the player interpolates between the start and end
 *     shapes, and displays an 'in-between' shape.
 * <li>The <code>clip</code> field specifies the top-most depth that will be masked by the character being added.
 *     A clip of zero indicates no clipping.
 * <li>The <code>name</code> field specifies a name for the character being added or modified. This field is typically used
 *     with sprite characters, and is used to identify the sprite for SetTarget actions.
 *     It allows the main movie (or other sprites) to perform actions inside the sprite.
 * </ul>
 *
 * @author Dmitry Skavish
 * @see RemoveObject
 */
public class Instance extends FlashObject {

    public static final int HAS_CLIPACTIONS  = 0x80;
    public static final int HAS_CLIP         = 0x40;
    public static final int HAS_NAME         = 0x20;
    public static final int HAS_RATIO        = 0x10;
    public static final int HAS_COLOR_TRANSF = 0x08;
    public static final int HAS_MATRIX       = 0x04;
    public static final int HAS_CHARACTER    = 0x02;
    public static final int MOVE             = 0x01;

    /**
     * true if this instance just for changing an object properties,
     * such as transformation etc.
     */
    public boolean isMove;

    /**
     * Transformation matrix
     */
    public AffineTransform matrix;

    /**
     * Color transformation matrix or null
     */
    public CXForm cxform;

    /**
     * Morphing ratio or -1
     */
    public int ratio = -1;

    /**
     * Clip layer number or -1
     */
    public int clip = -1;

    /**
     * Layer depth
     */
    public int depth;

    /**
     * Instance name or null
     */
    public String name;

    /**
     * Flash definition or null
     */
    public FlashDef def;

    /**
     * Clip actions or null
     */
    public ClipActions actions;

    /**
     * Generator command applied to this instance or null
     */
    public GenericCommand command;

    public Instance() {}

    public int getTag() {
        return Tag.PLACEOBJECT2;
    }

    /**
     * Parses PlaceObject tag
     *
     * @param p      parser
     * @return parser tag
     */
    public static Instance parse( Parser p ) {
        Instance o = new Instance();
        o.def = p.getDef(p.getUWord());
        o.depth = p.getUWord();
        o.matrix = p.getMatrix();
        if( p.getPos() < p.getTagEndPos() ) {
            o.cxform = CXForm.parse(p,false);
        }
        return o;
    }

    /**
     * Parses PlaceObject2 tag
     *
     * @param p      parser
     * @return parser tag
     */
    public static Instance parse2( Parser p ) {
        Instance o = new Instance();
        int flags = p.getUByte();
        o.depth = p.getUWord();
        if( (flags&HAS_CHARACTER) != 0 ) {
            o.def = p.getDef(p.getUWord());
        }
        if( (flags&HAS_MATRIX) != 0 ) {
            o.matrix = p.getMatrix();
        }
        if( (flags&HAS_COLOR_TRANSF) != 0 ) {
            o.cxform = CXForm.parse(p, true);
        }
        if( (flags&HAS_RATIO) != 0 ) {
            o.ratio = p.getUWord();
        }
        if( (flags&HAS_CLIP) != 0 ) {
            o.clip = p.getUWord();
        }
        if( (flags&HAS_NAME) != 0 ) {
            o.name = p.getString();
        }
        if( (flags&HAS_CLIPACTIONS) != 0 ) {
            o.actions = ClipActions.parse(p);
        }
        o.isMove = (flags&MOVE) != 0;
        return o;
    }


    /**
     * Returns true if this is instance of a script
     *
     * @return true if this is instance of a script
     */
    public boolean isScript() {
        return def instanceof Script;
    }

    /**
     * Returns flash definition as a Script
     *
     * @return definition as a Script
     */
    public Script getScript() {
        return (Script) def;
    }

    /**
     * Sets specified script as flash definition of this instance
     *
     * @param script specified script
     * @return specified script
     */
    public Script setScript( Script script ) {
        def = script;
        return script;
    }

    /**
     * Makes a copy of a script which is the flash definition of this instance
     *
     * @return copy of the script
     */
    public Script copyScript() {
        def = ((Script)def).copyScript();
        return (Script) def;
    }

    /**
     * Return true if there is generator command attached to this instance
     *
     * @return true if there is generator command attached to this instance
     */
    public boolean isCommand() {
        return command != null;
    }

    /**
     * Attaches generator command to this instance
     *
     * @param command generator command to be attached
     */
    public void setCommand( GenericCommand command ) {
        this.command = command;
    }

    /**
     * Returns attached generator command
     *
     * @return attached generator command
     */
    public GenericCommand getCommand() {
        return command;
    }

    /**
     * Returns first nested non-script of this instance
     * and its transformation matrix beginning from this instance
     *
     * @param m      collect transformations in this matrix
     * @return first nested non-script
     */
    public FlashDef getFirstNestedFlashDef( AffineTransform m ) {
        if( def == null ) return null;

        if( matrix != null ) m.concatenate(matrix);

        if( def instanceof Script ) {
            Script script = (Script) def;
            if( script.getFrameCount() > 0 ) {
                Frame frame = script.getFrameAt(0);
                for( int i=0; i<frame.size(); i++ ) {
                    FlashObject fo = frame.getFlashObjectAt(i);
                    if( fo instanceof Instance ) {
                        FlashDef df = ((Instance)fo).getFirstNestedFlashDef(m);
                        if( df != null ) return df;
                    } else if( fo instanceof FlashDef ) {
                        return (FlashDef) fo;
                    }
                }
            }
            return null;
        } else {
            return def;
        }
    }

    public void collectDeps( DepsCollector dc ) {
        // if this instance is mask (clip!=-1), then we need to get first nested
        // FlashDef, preserve its transformation and discard everything else
        if( clip != -1 && def instanceof Script ) {
            AffineTransform m = new AffineTransform();
            def = getFirstNestedFlashDef(m);
            matrix = m;
        }
        if( def != null ) {
            dc.addDep(def);
            //System.out.println( "Instance.collectDeps: def="+def.getID() );
        }
    }

    public void collectFonts( FontsCollector fc ) {
        if( def != null ) def.collectFonts( fc );
    }

    public void write( FlashOutput fob ) {
        int tagPos = fob.getPos();
        // quick guess: if name's length more than 8 or there are clip actions, we generate long tag
        boolean longtag = (name != null && name.length() > 8) || actions != null;
        if( longtag ) {
            fob.skip(6);
        } else {
            fob.skip(2);
        }
        int flags = (actions!= null? HAS_CLIPACTIONS : 0) |
                    (def    != null? HAS_CHARACTER   : 0) |
                    (matrix != null? HAS_MATRIX      : 0) |
                    (cxform != null? HAS_COLOR_TRANSF: 0) |
                    (name   != null? HAS_NAME        : 0) |
                    (ratio  != -1  ? HAS_RATIO       : 0) |
                    (clip   != -1  ? HAS_CLIP        : 0) |
                    (isMove        ? MOVE            : 0);
        fob.writeByte(flags);
        fob.writeWord(depth);
        if( def    != null ) fob.writeDefID(def);
        if( matrix != null ) fob.write(matrix);
        if( cxform != null ) cxform.write(fob);
        if( ratio  != -1   ) fob.writeWord(ratio);
        if( clip   != -1   ) fob.writeWord(clip);
        if( name   != null ) fob.writeStringZ(name);
        if( actions!= null ) actions.write(fob);

        if( longtag ) {
            fob.writeLongTagAt( Tag.PLACEOBJECT2, fob.getPos()-tagPos-6, tagPos );
        } else {
            fob.writeShortTagAt( Tag.PLACEOBJECT2, fob.getPos()-tagPos-2, tagPos );
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"Instance: depth="+depth );
        if( matrix != null ) out.println( indent+"     "+matrix.toString() );
        if( cxform != null ) cxform.printContent(out, indent+"    ");
        if( def != null ) out.println( indent+"    charID="+def.getID() );
        if( ratio != -1 ) out.println( indent+"    ratio="+ratio );
        if( clip != -1 ) out.println( indent+"    clip="+clip );
        if( name != null ) out.println( indent+"    name="+name );
        if( actions != null ) actions.printContent( out, indent+"    " );
        if( command != null ) command.printContent( out, indent );
    }

    protected boolean _isConstant() {
        if( isCommand() ) return false;
        if( name != null && Util.hasVar(name) ) return false;
        if( actions != null && !actions.isConstant() ) return false;
        if( def != null ) return def.isConstant();
        return true;
    }

    public void process( FlashFile file, Context context ) throws IVException {
        if( def != null && !isCommand() ) {
            def.process(file,context);
        }
    }

    public boolean isProcessed() {
        if( def != null && !isCommand() ) {
            return def.isProcessed();
        }
        return true;
    }

    public void setProcessed() {
        if( def != null && !isCommand() ) {
            def.setProcessed();
        }
    }

    public void apply( Context context ) {
        if( isCommand() ) return;
        super.apply(context);
        name = context.apply(name);
        if( actions != null ) actions.apply(context);
        if( def != null ) def.apply(context);
    }

    public Rectangle2D getBounds() {
        if( def == null ) return null;
        if( matrix == null ) return def.getBounds();

        return GeomHelper.calcBounds(matrix, def.getBounds());
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((Instance)item).matrix = matrix!=null? (AffineTransform) matrix.clone(): null;
        ((Instance)item).cxform = cxform!=null? (CXForm) cxform.getCopy(copier): null;
        ((Instance)item).isMove = isMove;
        ((Instance)item).ratio = ratio;
        ((Instance)item).clip = clip;
        ((Instance)item).depth = depth;
        ((Instance)item).name = name;
        ((Instance)item).actions = actions != null? (ClipActions) actions.getCopy( copier ): null;
        ((Instance)item).def = copier.copy(def);
        GenericCommand myCommand = command!=null? (GenericCommand) command.getCopy(copier): null;
        if( myCommand != null ) myCommand.setInstance((Instance)item);
        ((Instance)item).command = myCommand;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new Instance(), copier );
    }
}
