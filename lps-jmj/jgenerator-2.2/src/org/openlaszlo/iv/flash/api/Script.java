/*
 * $Id: Script.java,v 1.13 2002/08/08 23:26:54 skavish Exp $
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

/*
 * 12/11/2001 fixed bug in getBounds(), now it understands masks
 *
 */

package org.openlaszlo.iv.flash.api;

import java.io.*;
import java.util.*;
import java.awt.geom.Rectangle2D;
import java.awt.geom.AffineTransform;

import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.commands.*;
import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.api.sound.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.button.*;
import org.openlaszlo.iv.flash.context.Context;
import org.openlaszlo.iv.flash.context.FakeContext;
import org.openlaszlo.iv.flash.context.StandardContext;

/**
 * Flash movie clip
 * <p>
 * Contains a timeline, a background color and generator commands of script level.
 * <P>
 * Excerpt from flash file format specs:
 * <P>
 * A script corresponds to a 'movie clip' in the Flash user-interface.
 * It is a movie contained within a movie, and supports many of the features of a regular Flash movie, including:<br>
 * <ul>
 * <li>Most of the control tags that can be used in the main movie.
 * <li>A timeline that can stop, start and play independently of the main movie.
 * <li>A streaming sound track that is automatically mixed with the main sound track.
 * </ul>
 * <p>
 * A script object is defined with a DefineSprite tag.  It consists of a character ID, a frame count, and a
 * series of control tags.  Definition tags (such as DefineShape) are not allowed in the DefineSprite tag.
 * All the characters referred to by control tags in the sprite must be defined outside the sprite,
 * and before the DefineSprite tag.
 * <p>
 * Once defined, a script is displayed with a PlaceObject2 tag in the main movie.  The transform (specified in
 * PlaceObject) is concatenated with the transforms of objects placed inside the script.  These objects behave
 * like 'children' of the script, so when the script is moved, the objects inside the script move also.
 * Similarly, when the script is scaled or rotated, the child objects are also scaled or rotated.
 * A script object stops playing automatically when it is removed from the display list.
 *
 * @author Dmitry Skavish
 */
public final class Script extends FlashDef {

    private Timeline timeline;              // a timeline of the script
    private boolean isProcessed = false;    // true if this script is already processed by jgenerator
    private IVVector gl_commands;           // global generator commands, like SetEnvironment etc
    private IVVector zero_frame;            // zero frame, it usually contains some sound heads etc
    private SetBackgroundColor bkgColor;    // background color of the script

    /**
     * Creates empty script
     */
    public Script() {}

    /**
     * Creates empty script with specified initial capacity
     *
     * @param frameNumber initial capacity of this script in frames
     * @see #Script(int, int)
     */
    public Script( int frameNumber ) {
        this( 0, frameNumber );
    }

    /**
     * Creates empty script with specified initial capacity and ID
     * <P>
     * The capacity specifies the capacity of this script's
     * timeline (in frames). It does not specify how many frames
     * this script will eventually have.
     *
     * @param objID       ID of this script, if it's -1, then this is a main script
     * @param frameNumber initial capacity of this script in frames
     */
    public Script( int objID, int frameNumber ) {
        timeline = new Timeline( frameNumber+1 );
        setID( objID );
    }

    /**
     * Returns true if this script is a main script
     * <P>
     * Main script is a script which represents main flash timeline
     *
     * @return true if this is a main script
     */
    public boolean isMain() {
        return getID() == -1;
    }

    /**
     * Sets this script to be main
     */
    public void setMain() {
        setID( -1 );
    }

    /**
     * Sets this script to be NOT main script
     */
    public void resetMain() {
        setID( 0 );
    }

    /**
     * Returns this script's timeline
     *
     * @return script's timeline
     */
    public Timeline getTimeline() {
        return timeline;
    }

    /**
     * Writes content of this script to flash buffer
     * <P>
     * This method has to be used only if this script is main script.
     *
     * @param fob    flash buffer to write
     * @see #isMain
     * @see #write
     */
    public void generate( FlashOutput fob ) {
        int frameCount = timeline.getFrameCount();
        fob.writeWord( frameCount );

        // collect fonts and make sure there is only one instance of each font
        FontsCollector fc = new FontsCollector();
        collectFonts( fc );

        // Create a slot for license key first
        int licenseLen = 32;
        fob.writeTag( Tag.SERIALNUMBER, licenseLen );
        for (int i=0; i < licenseLen; i++)
            fob.writeByte( 0x78 ); // x

        // write background color 
        if( bkgColor != null ) bkgColor.write( fob );

        mergeFonts( fc.getFonts() );

        // generate timeline in right order (definitions first)
        timeline.generate(fob, new DepsCollector(fc));

        Tag.END_TAG.write( fob );
    }

    /**
     * Writes content of this script to flash buffer
     * <P>
     * This method has to be used only if this script is main script.
     *
     * @param fob    flash buffer to write
     * @param fc1    
     * @param pfc             preloader fonts
     * @param hasPreloader    true if preloader exists
     * @see #isMain
     * @see #write
     */
    public void generate( FlashOutput fob, FontsCollector fc1, 
                          FontsCollector pfc, boolean hasPreloader ) {
        int frameCount = timeline.getFrameCount();
        fob.writeWord( frameCount );

        // write background color first
        if( bkgColor != null ) bkgColor.write( fob );

        // Fonts in fc1 includes fonts declared in the Laszlo app as well as fonts that
        // are used in SWF assets that are imported.
        //
        // Fonts in pfc are fonts that are used in SWF assets that are imported in the preloader/splash.
        // FIXME: [2004-03-09 bloch] If a font name is used in a SWF asset that's in the preloader *and*
        // a laszlo font name, then there might be a prob
        // here since only the preloader version will show up and if the font needed merging, some 
        // text will show up blank.
        mergeFonts( fc1.getFonts() );
        if (hasPreloader) {
            mergeFonts( pfc.getFonts() ); 
        }

        IVVector fonts; 
        int frame = 0;
        // Assume that preloader is only one frame long in the main timeline
        // starting on frame 0.
        if (hasPreloader) {
            // add preloader fonts here
            fonts = pfc.getFonts();
            for(int i = 0; i < fonts.size(); i++) {
                FontDef fontDef = (FontDef)fonts.elementAt(i);
                fontDef.write( fob );
            }

            // write out preloader at frame 0 and place the rest of the app in
            // frame 1.
            Frame fo = (Frame) timeline.elementAt(frame++);
            fo.generate( fob, new DepsCollector(new FontsCollector()) );
        }

        // Place the rest of the fonts 
        fonts = fc1.getFonts();
        for(int i = 0; i < fonts.size(); i++) {
            FontDef fontDef = (FontDef)fonts.elementAt(i);
            fontDef.write( fob );
        }

        // generate timeline in right order, but starting with
        // frame 1
        timeline.generate(fob, new DepsCollector(new FontsCollector()), frame);

        Tag.END_TAG.write( fob );
    }

    /**
     * Writes content of this script to flash buffer
     * <P>
     * This method has to be used only if this script is NOT a main script.
     *
     * @param fob    flash buffer to write
     * @see #isMain
     * @see #generate
     */
    public void write( FlashOutput fob ) {
        int start = fob.getPos();     // save for length calculating
        fob.skip( 6 );              // 6 - long tag
        fob.writeDefID( this );
        int frameCount = timeline.getFrameCount();
        fob.writeWord( frameCount );

        // write background color first
        if( bkgColor != null ) bkgColor.write( fob );

        // write "zero frame"
        if( zero_frame != null ) zero_frame.write( fob );

        // write timeline
        timeline.write( fob );

        // write end tag
        Tag.END_TAG.write( fob );

        int size = fob.getPos()-start-6;  // 6 - long tag
        fob.writeLongTagAt( Tag.DEFINESPRITE, size, start );
    }

    /**
     * Parses script
     *
     * @param p      parser
     * @param isMain true if script to be parsed is main
     * @return new parsed script
     * @exception IVException
     */
    public static Script parse( Parser p, boolean isMain ) throws IVException {
        int objID = -1;
        if( !isMain ) {
            objID = p.getUWord();
        }

        // number of frames
        int frameNum = p.getUWord();
        // create script
        Script sc = new Script( objID, frameNum );

        Timeline tl = sc.getTimeline();
        IVVector tFrame = new IVVector();
        sc.zero_frame = tFrame;
        String frameName = null;
        boolean is_anchor = false;

        for(;;) {
            int code = p.getTag();

            // check for tag
            switch( code ) {
                /* Control tags         */
                case Tag.SHOWFRAME: {
                    Frame f = new Frame( tFrame );
                    f.setName( frameName );
                    f.setAnchor(is_anchor);
                    frameName = null;
                    is_anchor = false;
                    tl.addFrame( f );
                    tFrame.reset();
                    break;
                }
                case Tag.END:
                    return sc;
                case Tag.PLACEOBJECT:
                    tFrame.addElement( Instance.parse( p ) );
                    break;
                case Tag.PLACEOBJECT2: {
                    Instance instance = Instance.parse2(p);
                    GenericCommand cmd = GenericCommand.checkAndParseMX(instance);
                    if( cmd != null && cmd.isGlobal() ) {
                        sc.addGlobalCommand(cmd);
                    } else {
                        tFrame.addElement(instance);
                    }
                    break;
                }
                case Tag.REMOVEOBJECT:
                    tFrame.addElement( RemoveObject.parse( p ) );
                    break;
                case Tag.REMOVEOBJECT2:
                    tFrame.addElement( RemoveObject.parse2( p ) );
                    break;
                case Tag.SETBKGCOLOR:
                    sc.setBackgroundColor( SetBackgroundColor.parse(p) );
                    break;
                case Tag.STARTSOUND:
                    tFrame.addElement( StartSound.parse(p) );
                    break;
                case Tag.DOACTION:
                    tFrame.addElement( DoAction.parse(p) );
                    break;
                case Tag.INITCLIPACTION:
                    tFrame.addElement( InitClipAction.parse(p) );
                    break;
                case Tag.TEMPLATECOMMAND: {
                    GenericCommand cmd = GenericCommand.parse(p);
                    if( cmd == null ) break;
                    if( !cmd.isGlobal() ) {
                        // this is not a global command, search backward for corresponding instance
                        int j=tl.getFrameCount();
                        IVVector myFrame = tFrame;
                        timelineLoop:
                        for(;;) {
                            for( int i=myFrame.size(); --i>=0; ) {
                                Object o = myFrame.elementAt(i);
                                if( o instanceof Instance ) {
                                    Instance inst = (Instance) o;
                                    if( inst.depth != cmd.getDepth() ) continue;
                                    cmd.setInstance(inst);
                                    inst.setCommand(cmd);
                                    break timelineLoop;
                                }
                            }
                            if( --j < 0 ) break;
                            myFrame = tl.getFrameAt(j);
                        };
                        if( j<0 ) {
                            Log.logRB(Resource.GENCMDERR, new Object[] {cmd.getCommandName()});
                        }
                    } else {
                        sc.addGlobalCommand( cmd );
                    }
                    break;
                }
                /* Info tags            */
                case Tag.FLASHGENERATOR:
                    p.getFile().setTemplate(true);
                    break;
                case Tag.PROTECT:
                    tFrame.addElement( p.newUnknownTag() );
                    break;
                case Tag.SERIALNUMBER:
                    break;
                case Tag.FRAMELABEL:
                    //Util.dump(p.getBuf(), p.getTagDataPos(), p.getTagEndPos()-p.getTagDataPos(), System.out);
                    frameName = p.getString();
                    if( p.getPos() < p.getTagEndPos() ) {
                        is_anchor = p.getUByte()==1;
                    }
                    break;
                case Tag.GENERATORTEXT: {
                    int id = p.getUWord();
                    Text text = (Text) p.getDef(id);
                    text.parseGenText(p);
                    break;
                }
                case Tag.NAMECHARACTER: {
                    int id = p.getUWord();
                    String name = p.getString();
                    FlashDef def = p.getDef(id);
                    def.setName(name);
                    p.addDefToLibrary( name, def );
                    break;
                }
                case Tag.FREECHARACTER:
                    tFrame.addElement( FreeCharacter.parse( p ) );
                    break;
                /* Streaming sound tags */
                case Tag.SOUNDSTREAMHEAD:
                case Tag.SOUNDSTREAMHEAD2:
                    tFrame.addElement( SoundStreamHead.parse( p ) );
                    break;
                case Tag.SOUNDSTREAMBLOCK:
                    tFrame.addElement( SoundStreamBlock.parse( p ) );
                    break;
                /* Definitions tags     */
                case Tag.DEFINESPRITE:
                    p.addDef( Script.parse( p, false ) );
                    break;
                case Tag.DEFINEMOVIE:
                    p.addDef( QTMovie.parse(p) );
                    break;
                case Tag.DEFINESHAPE:
                case Tag.DEFINESHAPE2:
                case Tag.DEFINESHAPE3:
                    p.addDef( LazyShape.parse( p ) );
                    break;
                case Tag.DEFINEMORPHSHAPE:
                    p.addDef( LazyMorphShape.parse( p ) );
                    break;
                case Tag.DEFINESOUND:
                    p.addDef( LazySound.parse( p ) );
                    break;
                case Tag.DEFINEFONT:
                    p.addDef( FontDef.parse( p ) );
                    break;
                case Tag.DEFINEFONTINFO:
                    FontDef.parseFontInfoTag( p );
                    break;
                case Tag.DEFINEFONTINFO2:
                    FontDef.parseFontInfoTag2( p );
                    break;
                case Tag.DEFINEFONT2:
                    p.addDef( FontDef.parse2( p ) );
                    break;
                case Tag.EXTERNALFONT: {
                    FlashDef def = FontDef.parseExternalFontTag( p );
                    if( def != null ) p.addDef( def );
                    break;
                }
                case Tag.DEFINETEXT:
                    p.addDef( Text.parse( p, false ) );
                    break;
                case Tag.DEFINETEXT2:
                    p.addDef( Text.parse( p, true ) );
                    break;
                case Tag.DEFINEEDITTEXT:
                    p.addDef( TextField.parse( p ) );
                    break;
                case Tag.DEFINEBUTTON:
                    p.addDef( Button.parse( p ) );
                    break;
                case Tag.DEFINEBUTTON2:
                    p.addDef( Button2.parse2( p ) );
                    break;
                case Tag.DEFINEBUTTONCXFORM:
                    ButtonCXForm.parse(p);  // this guy will add itself to a button, we need to do nothing
                    break;
                case Tag.DEFINEBUTTONSOUND:
                    ButtonSound.parse(p);  // this guy will add itself to a button, we need to do nothing
                    break;
                case Tag.DEFINEBITS:
                case Tag.DEFINEBITSJPEG2:
                case Tag.DEFINEBITSJPEG3:
                    p.addDef( JPEGBitmap.parse(p) );
                    break;
                case Tag.DEFINEBITSLOSSLESS:
                case Tag.DEFINEBITSLOSSLESS2:
                    p.addDef( LLBitmap.parse(p) );
                    break;
                case Tag.JPEGTABLES:
                    JPEGBitmap.parseJPegTables(p);
                    break;
                    /* Flash 5 tags         */
                case Tag.ENABLEDEBUGGER:
                    tFrame.addElement( p.newUnknownTag() );
                    break;
                case Tag.EXPORTASSETS: {
                    ExportAssets ea = ExportAssets.parse(p);
                    if( ea != null ) tFrame.addElement(ea);
                    break;
                }
                case Tag.PATHSAREPOSTSCRIPT:
                    break;
                case Tag.IMPORTASSETS:
                    tFrame.addElement( ImportAssets.parse(p) );
                    break;
                    /* Unknown tags parsed here   */
                default:
                    Log.logRB(Resource.UNKNOWNTAG, new Object[] {Util.b2h(code)});
                    tFrame.addElement( p.newUnknownTag() );
                    break;
            }
            p.skipLastTag();
        }
    }

    /**
     * Returns background color of this script
     *
     * @return background color of this script
     */
    public SetBackgroundColor getBackgroundColor() {
        return bkgColor;
    }

    /**
     * Sets new background color
     *
     * @param bkgColor new color
     */
    public void setBackgroundColor( SetBackgroundColor bkgColor ) {
        this.bkgColor = bkgColor;
    }

    protected void addGlobalCommand( GenericCommand cmd ) {
        if( gl_commands == null ) gl_commands = new IVVector();
        gl_commands.addElement( cmd );
    }

    /**
     * Removes global commands from this script which have to appear
     * only once in a file. For example: MovieSetCommand
     * <p>
     * Usually all templates loaded into another one need to have these global
     * commands stripped out
     */
    public void removeFileDepGlobalCommands() {
        if( gl_commands == null ) return;
        for( int i=0; i<gl_commands.size(); i++ ) {
            GenericCommand cmd = (GenericCommand) gl_commands.elementAt(i);
            if( cmd instanceof MovieSetCommand /*||*/ ) {
                gl_commands.removeElementAt(i);
                i--;
            }
        }
    }

    /**
     * Finds nested script's instance by name
     * <P>
     * Searches instance of a script in this script
     * and in all nested scripts by instance's name.
     *
     * @param name   name of the instance to be searched
     * @return found Script or null
     */
    public Instance findInstance( String name ) {
        for( int i=0; i<timeline.getFrameCount(); i++ ) {
            Frame frame = timeline.getFrameAt(i);
            for( int j=0; j<frame.size(); j++ ) {
                FlashObject fo = frame.getFlashObjectAt(j);
                if( !(fo instanceof Instance) ) continue;
                Instance inst = (Instance) fo;
                if( inst.name != null && inst.name.equals( name ) ) return inst;
                if( inst.isScript() ) {
                    inst = inst.getScript().findInstance(name);
                    if( inst != null ) return inst;
                }
            }
        }
        return null;
    }

    /**
     * Processes this script in the specified context and flash file
     *
     * @param file    file to be used when processing this script
     * @param context context to be used when processing this script
     * @exception IVException
     */
    public void process( FlashFile file, Context context ) throws IVException {
        if( isProcessed() ) return;

        FakeContext fakeContext = null;
        Context localContext = null;

        // execute global commands (if any)
        if( gl_commands != null ) {
            fakeContext = new FakeContext(context);
            for( int i=0; i<gl_commands.size(); i++ ) {
                GenericCommand cmd = (GenericCommand) gl_commands.elementAt(i);
                try {
                    cmd.doCommand(file, localContext!=null?localContext:fakeContext, this, 0);
                    localContext = fakeContext.getContext();
                } catch( Exception e ) {
                    Log.logRB(Resource.ERRDOCMD, new Object[] {file.getFullName(), getName(), "0", cmd.getCommandName()}, e);
                }
            }
        }

        if( localContext == null ) {
            localContext = new StandardContext();
        }
        localContext.setParent(context);

        // perform generator commands
        timeline.doCommand( file, localContext, this );

        // process inner scripts
        timeline.process( file, localContext );

        // apply context to the rest
        apply( localContext );
    }

    /**
     * Returns true if this script was already processed
     *
     * @return true if this script was already processed
     */
    public boolean isProcessed() {
        return isProcessed;
    }

    /**
     * Marks this script as processed
     */
    public void setProcessed() {
        isProcessed = true;
    }

    /**
     * Creates a copy of this script
     *
     * @return copy of this script
     */
    public Script copyScript() {
        return (Script) getCopy( new ScriptCopier() );
    }

    /**
     * Appends specified script to the end of this script
     * <p>
     * Removes all hanging instances of this script, i.e. for every instance
     * which is still on the timeline by the of this script put RemoveObject
     * tag in the last frame of the timeline. Then adds all frames of the specified
     * script to the end of this one.
     *
     * @param sc     script to be appended
     */
    public void appendScript( Script sc ) {
        Timeline scTm = sc.getTimeline();

        Frame lastFrame = newFrame();
        removeAllInstances(lastFrame);
        Frame firstFrame = scTm.getFrameAt(0);
        lastFrame.setName( firstFrame.getName() );
        lastFrame.append( firstFrame );

        for( int i=1; i<scTm.getFrameCount(); i++ ) {
            Frame frame = scTm.getFrameAt(i);
            timeline.addFrame(frame);
        }
    }

    /**
     * Removes all hanging instances of the timeline in the specified frame
     * <p>
     * Traverses this script and for every hanging instance (instance which is
     * still on the timeline by the last frame) puts RemoveObject tag in the specified frame.
     *
     * @param lastFrame frame to put RemoveObject tags in
     */
    public void removeAllInstances( Frame lastFrame ) {
        IVVector layers = getOccupiedLayers();
        for( int i=0; i<layers.size(); i++ ) {
            Instance inst = (Instance) layers.elementAt(i);
            if( inst == null ) continue;
            lastFrame.removeInstance(i);
        }
    }

    /**
     * Calculates bounds of this script
     * <p>
     * Takes masks into account too
     *
     * @return bounds of this script
     */
    public Rectangle2D getBounds() {
        Rectangle2D rect = null;
        IVVector layers = new IVVector();
        int[] masks  = null;
        for( int i=0; i<timeline.getFrameCount(); i++ ) {
            Frame frame = timeline.getFrameAt(i);
            for( int k=0; k<frame.size(); k++ ) {
                FlashObject fo = frame.getFlashObjectAt(k);
                if( fo instanceof Instance ) {
                    Instance inst = (Instance) fo;
                    int layer = inst.depth;
                    Rectangle2D bounds = null;
                    FlashDef def = inst.def;
                    if( def != null ) {
                        bounds = def.getBounds();
                        layers.setElementAt(bounds, layer);
                    } else if( inst.matrix != null ) {
                        bounds = (Rectangle2D) layers.elementAt(layer);
                    }

                    // check for mask
                    if( masks!=null && masks.length>layer && masks[layer]!=0 ) continue;

                    if( inst.matrix != null && bounds != null ) {
                        bounds = GeomHelper.calcBounds( inst.matrix, bounds );
                    }
                    rect = GeomHelper.add( rect, bounds );

                    if( inst.clip > 0 ) {
                        int clip = inst.clip;
                        if( masks == null ) {
                            masks = new int[clip+10];
                        } else if( masks.length <= clip ) {
                            int[] masks1 = new int[clip+10];
                            System.arraycopy(masks, 0, masks1, 0, masks.length);
                            masks = masks1;
                        }
                        masks[layer] = clip;
                        for( int m=layer+1; m<=clip; m++ ) {
                            masks[m] = -1;
                        }
                    }
                } else if( fo instanceof RemoveObject ) {
                    RemoveObject ro = (RemoveObject) fo;
                    int layer = ro.depth;
                    if( masks!=null && masks.length>layer && masks[layer]>0 ) {
                        // mask is to be removed, clear it
                        int clip = masks[layer];
                        for( int m=layer; m<=clip; m++ ) {
                            masks[m] = 0;
                        }
                    }
                    layers.setElementAt(null, layer);
                } else {
                    rect = GeomHelper.add( rect, fo.getBounds() );
                }
            }
        }
        if( rect == null ) {
            rect = GeomHelper.newRectangle();
        }
        return rect;
    }

    /**
     * Reserves specified number of layers beginning from the specified one
     * <p>
     * Traverses the timeline and moves all the instances which are on layers
     * greater than the specified one by the specified number.
     * <P>
     * For example if we have instances A, B, C, D on layers 2,3,4,5,
     * then if we call reserveLayers(3,2) we will have:
     * A on 2, B on 5, C on 6, D on 7, so there will be two available layers depths: 3 and 4.
     *
     * @param from   reserve layers beginning from this one
     * @param num    number of layers to be reserved
     * @return 'from' layer
     */
    public int reserveLayers( int from, int num ) {
        int cnt = timeline.getFrameCount();
        for( int i=0; i<cnt; i++ ) {
            Frame frame = timeline.getFrameAt(i);
            int fsz = frame.size();
            for( int k=0; k<fsz; k++ ) {
                FlashObject fo = frame.getFlashObjectAt(k);
                if( fo instanceof Instance ) {
                    Instance inst = (Instance) fo;
                    if( inst.depth >= from ) inst.depth += num;
                    if( inst.clip >= from ) inst.clip += num;
                } else if( fo instanceof RemoveObject ) {
                    RemoveObject ro = (RemoveObject) fo;
                    if( ro.depth >= from ) ro.depth += num;
                }
            }
        }
        return from;
    }

    /**
     * Returns maximum layer's depth of this script
     *
     * @return maximum depth
     */
    public int getMaxDepth() {
        int max = 0;
        for( int i=0; i<timeline.getFrameCount(); i++ ) {
            Frame frame = timeline.getFrameAt(i);
            for( int k=0; k<frame.size(); k++ ) {
                FlashObject fo = frame.getFlashObjectAt(k);
                if( fo instanceof Instance ) {
                    Instance inst = (Instance) fo;
                    if( inst.depth > max ) max = inst.depth;
                    if( inst.clip > max ) max = inst.clip;
                }
            }
        }
        return max;
    }

    /**
     * Creates linear interpolation of the two specified matrixes
     *
     * @param t      coefficient of the interpolation [0..1]
     * @param startMatrix first matrix
     * @param endMatrix   second matrix
     * @return interpolation of the two specified matrixes
     */
    public static AffineTransform interLinear(
                double t, AffineTransform startMatrix, AffineTransform endMatrix )
    {
        double t1 = 1.0-t;
        double m00 = startMatrix.getScaleX()*t1 + endMatrix.getScaleX()*t;
        double m11 = startMatrix.getScaleY()*t1 + endMatrix.getScaleY()*t;
        double m01 = startMatrix.getShearX()*t1 + endMatrix.getShearX()*t;
        double m10 = startMatrix.getShearY()*t1 + endMatrix.getShearY()*t;
        double m02 = startMatrix.getTranslateX()*t1 + endMatrix.getTranslateX()*t;
        double m12 = startMatrix.getTranslateY()*t1 + endMatrix.getTranslateY()*t;
        return new AffineTransform(m00, m10, m01, m11, m02, m12);
    }

    /**
     * Adds simple motion and color tweening to this script
     *
     * @param def        symbol to be tweened
     * @param layer      layer on which to place the symbol and do the tweening
     * @param startFrame start frame
     * @param startMatrix start transformation matrix
     * @param startCXF   start color matrix (optional)
     * @param endFrame   end frame (included)
     * @param endMatrix  end transformation matrix
     * @param endCXF     end color matrix (optional)
     * @return last frame
     */
    public Frame addTweening( FlashDef def, int layer,
                             int startFrame, AffineTransform startMatrix, CXForm startCXF,
                             int endFrame, AffineTransform endMatrix, CXForm endCXF )
    {
        return addTweening(def, layer, getFrameAt(startFrame), endFrame-startFrame,
                           startMatrix, startCXF, endMatrix, endCXF);
    }

    /**
     * Adds simple motion and color tweening to this script
     *
     * @param def        symbol to be tweened
     * @param layer      layer on which to place the symbol and do the tweening
     * @param startFrame start frame
     * @param startMatrix
     *                   start transformation matrix
     * @param startCXF   start color matrix (optional)
     * @param endFrame   end frame (included)
     * @param endMatrix  end transformation matrix
     * @param endCXF     end color matrix (optional)
     * @param name       instance name
     * @return last frame
     */
    public Frame addTweening( FlashDef def, int layer,
                             int startFrame, AffineTransform startMatrix, CXForm startCXF,
                             int endFrame, AffineTransform endMatrix, CXForm endCXF, String name )
    {
        return addTweening(def, layer, getFrameAt(startFrame), endFrame-startFrame,
                           startMatrix, startCXF, endMatrix, endCXF, name);
    }

    /**
     * Adds simple motion and color tweening to this script
     *
     * @param def       symbol to be tweened
     * @param layer     layer on which to place the symbol and do the tweening
     * @param frame     first frame to start the tweening
     * @param num       number of frames for the tweening (in addition to the first frame)
     * @param startMatrix start transformation matrix
     * @param startCXF  start color matrix (optional)
     * @param endMatrix end transformation matrix
     * @param endCXF    end color matrix (optional)
     * @return last frame
     */
    public Frame addTweening( FlashDef def, int layer, Frame frame, int num,
                             AffineTransform startMatrix, CXForm startCXF,
                             AffineTransform endMatrix, CXForm endCXF )
    {
        return addTweening(def, layer, frame, num, startMatrix, startCXF, endMatrix, endCXF, null);
    }

    /**
     * Adds simple motion and color tweening to this script
     *
     * @param def       symbol to be tweened
     * @param layer     layer on which to place the symbol and do the tweening
     * @param frame     first frame to start the tweening
     * @param num       number of frames for the tweening (in addition to the first frame)
     * @param startMatrix start transformation matrix
     * @param startCXF  start color matrix (optional)
     * @param endMatrix end transformation matrix
     * @param endCXF    end color matrix (optional)
     * @param name      instance name (optional), is set on first instance
     * @return last frame
     */
    public Frame addTweening( FlashDef def, int layer, Frame frame, int num,
                             AffineTransform startMatrix, CXForm startCXF,
                             AffineTransform endMatrix, CXForm endCXF, String name )
    {
        int startFrame = getFrameIndex(frame);
        int endFrame = startFrame+num;
        frame.addInstance(def, layer, startMatrix, startCXF, name);
        for( int i=1; i<num; i++ ) {
            double t = (double)i/num;
            AffineTransform matrix = interLinear(t, startMatrix, endMatrix);
            CXForm cxform = startCXF!=null? CXForm.interLinear(t, startCXF, endCXF): null;
            frame = getFrameAt(startFrame+i);
            frame.addInstance(layer, matrix, cxform);
        }
        if( startFrame != endFrame ) {
            frame = getFrameAt(endFrame);
            frame.addInstance(layer, endMatrix, endCXF);
        }
        return frame;
    }

    /**
     * Fades out everything on the timeline during the specified number of frames
     *
     * @param num    number of frames
     */
    public void fadeOut( int num ) {
        IVVector layers = getOccupiedLayers();
        int startFrame = getFrameCount()-1;;
        CXForm startcx = CXForm.newAlpha(256);
        CXForm endcx = CXForm.newAlpha(0);
        for( int i=0; i<layers.size(); i++ ) {
            Instance inst = (Instance) layers.elementAt(i);
            if( inst == null ) continue;
            for( int j=1; j<=num; j++ ) {
                double t = (double)j/num;
                CXForm cxform = CXForm.interLinear(t, startcx, endcx);
                Frame frame = getFrameAt(startFrame+j);
                frame.addInstance(i, null, cxform);
            }
        }
    }

    /**
     * Returns vector of all occupied layers by the end of this script
     *
     * @return vector which contains Instances at 'layer' indexes
     */
    public IVVector getOccupiedLayers() {
        IVVector layers = new IVVector();
        for( int i=0; i<timeline.getFrameCount(); i++ ) {
            Frame frame = timeline.getFrameAt(i);
            for( int k=0; k<frame.size(); k++ ) {
                FlashObject fo = frame.getFlashObjectAt(k);
                if( fo instanceof Instance ) {
                    Instance inst = (Instance) fo;
                    int layer = inst.depth;
                    layers.setElementAt(inst, layer);
                } else if( fo instanceof RemoveObject ) {
                    RemoveObject ro = (RemoveObject) fo;
                    int layer = ro.depth;
                    layers.setElementAt(null, layer);
                }
            }
        }
        return layers;
    }

    /**
     * Returns number of frames in this script
     *
     * @return number of frames
     */
    public int getFrameCount() {
        return timeline.getFrameCount();
    }

    /**
     * Returns frame at specified index
     * <p>
     * If frame does not exist, creates it at specified index and fills everything
     * in between with empty frames
     *
     * @param frameNum  frame number
     * @return frame at specified index
     */
    public Frame getFrameAt( int frameNum ) {
        if( timeline.getFrameCount() <= frameNum ) {
            for( int i=timeline.getFrameCount(); i<=frameNum; i++ ) newFrame();
        }
        return timeline.getFrameAt(frameNum);
    }

    /**
     * Returns index of specified frame in the timeline or -1
     *
     * @param frame  specified frame
     * @return index of specified frame in the timeline or -1
     */
    public int getFrameIndex( Frame frame ) {
        return timeline.getFrameIndex(frame);
    }

    /**
     * Creates new frame and adds it to the end of the timeline
     *
     * @return new added frame
     */
    public Frame newFrame() {
        return timeline.newFrame();
    }

    /**
     * Returns last frame of this script
     *
     * @return last frame of this script
     */
    public Frame getLastFrame() {
        return timeline.getFrameAt( getFrameCount()-1 );
    }

    public void apply( Context context ) {
        if( isConstant() || isProcessed() ) return;
        timeline.apply(context);
    }

    public boolean isConstant() {
        return timeline.isConstant();
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((Script)item).zero_frame = zero_frame!=null? zero_frame.getCopy(copier): null;
        ((Script)item).timeline = (Timeline) timeline.getCopy(copier);
        ((Script)item).isProcessed = isProcessed;
        if( gl_commands != null ) {
            IVVector v = new IVVector( gl_commands.size() );
            for( int i=0; i<gl_commands.size(); i++ ) {
                GenericCommand cmd = (GenericCommand) gl_commands.elementAt(i);
                v.addElement( cmd.getCopy(null) );
            }
            ((Script)item).gl_commands = v;
        }
        ((Script)item).bkgColor = bkgColor!=null? (SetBackgroundColor)bkgColor.getCopy(copier): null;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new Script(), copier );
    }

    public void collectDeps( DepsCollector dc ) {
        //System.out.println( "Script.collectDeps" );
        timeline.collectDeps(dc);
    }

    public void collectFonts( FontsCollector fc ) {
        for( int i=0; i<timeline.getFrameCount(); i++ ) {
            Frame frame = timeline.getFrameAt(i);
            for( int k=0; k<frame.size(); k++ ) {
                FlashObject fo = frame.getFlashObjectAt(k);
                fo.collectFonts( fc );
            }
        }
    }

    public int getTag() {
        return Tag.DEFINESPRITE;
    }

    public void printContent( PrintStream out, String indent ) {
        String id = isMain()? "main": Integer.toString(getID());
        out.println( indent+"Script: id="+id+" frames="+timeline.getFrameCount()+" name='"+getName()+"'" );
        if( zero_frame != null ) zero_frame.printContent( out, indent+"    " );
        timeline.printContent( out, indent+"    " );
        out.println( indent+"End Script("+id+") name='"+getName()+"'" );
    }

    /**
     * Merges the same fonts into one
     *
     * @param fonts  vector of fonts (FontDefs)
     */
    protected void mergeFonts( IVVector fonts ) {
        // vector of Object[] {(String)font_key, (IVVector) text_blocks}
        IVVector fonts_blocks = new IVVector();

        // hashtable of FontDef by their font key
        Hashtable fonts_map = new Hashtable();

        // iterate all the fonts and merge the same ones
        for( int i=0; i<fonts.size(); i++ ) {
            FontDef fdef = (FontDef) fonts.elementAt(i);
            Font font = fdef.getFont();
            //System.out.println( "Script.mergeFonts: font="+font.fontKey );

            // check for the same font in font cache, if found and smaller then remove from the cache
            Font font2 = FontCache.getFont(font.fontKey);
            if( font2 != null && font2 != font ) {
                if( font.isLargeThan(font2) ) {
                    //System.out.println( "Script.mergeFonts: removing smaller font from cache" );
                    // remove old font from cache
                    FontCache.removeFont(font2.fontKey);
                    // add new one
                    FontCache.addFont(font.fontKey, font);
                }
            }

            FontDef prev_fdef = (FontDef) fonts_map.get(font.fontKey);
            if( prev_fdef == null ) {
                //System.out.println( "Script.mergeFonts: there is no previous font" );
                fonts_map.put(font.fontKey, fdef);
            } else {
                Font font_prev = prev_fdef.getFont();
                //System.out.println( "Script.mergeFonts: there is previous font and they are "+(font_prev==font?"":"not ")+"equal" );
                if( font_prev != font ) {
                    //System.out.println( "Script.mergeFonts: merge fonts" );
                    Font font3 = FontDef.mergeFonts(font_prev, font);
                    if( font3 == font ) {
                        //System.out.println( "Script.mergeFonts: font3==font" );
                        prev_fdef.setFont(font);
                        fonts_blocks.addElement(new Object[] {font_prev, prev_fdef.getTextBlocks()});
                        fonts_map.remove(font.fontKey);
                        fonts_map.put(font.fontKey, fdef);
                        fonts.removeElement(prev_fdef);
                    } else {
                        //System.out.println( "Script.mergeFonts: font3==font_prev" );
                        fdef.setFont(font_prev);
                        fonts_blocks.addElement(new Object[] {font, fdef.getTextBlocks()});
                        fonts.removeElement(fdef);
                    }
                    i--;
                }
            }
        }

        for( int i=0; i<fonts_blocks.size(); i++ ) {
            Object[] objs = (Object[]) fonts_blocks.elementAt(i);
            Font old_font = (Font) objs[0];
            String fontKey = old_font.fontKey;
            IVVector text_blocks = (IVVector) objs[1];
            FontDef fontDef = (FontDef) fonts_map.get(fontKey);
            fontDef.addTextBlocks( text_blocks );
            Font new_font = fontDef.getFont();
            //System.out.println( "updating textblocks for font "+fontKey );
            for( int j=0; j<text_blocks.size(); j++ ) {
                TextBlock tblock = (TextBlock) text_blocks.elementAt(j);
                tblock.layout();
                tblock.changeFont(old_font, new_font);
            }
        }
    }

    /**
     * zero_frame IVVector accessor
     *
     * @return IVVector
     */
    public IVVector getZero_frame() {
        return zero_frame;
    }

}
