/*****************************************************************************
 * SWFFile.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import org.openlaszlo.sc.ScriptCompiler;
import org.openlaszlo.utils.ChainedException;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.button.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.api.sound.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.api.shape.*;

import java.awt.Rectangle;
import java.awt.geom.Rectangle2D;
import java.awt.geom.AffineTransform;

import java.io.*;

import java.util.Properties;

import org.apache.log4j.*;

/** 
 * Extension of JGenerator FlashFile api used to 
 * constructs the Base SWF Library for an LZX output SWF.
 *
 * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
 */
class SWFFile extends FlashFile {

    public static final int TWIP = 20;
    public static final int KEYCODE_ENTER = 13;
    public static final int KEYCODE_TAB = 18;

    private Properties mProperties = null;

    /**
     * Return a ActionBlock from the given text.
     */
    protected DoAction actionBlock(String s) {
        return new DoAction(program(s));
    }

    /**
     * Return a program from the given text.
     */
    protected Program program(String s) {
        byte[] action = ScriptCompiler.compileToByteArray(s, mProperties);
        return new Program(action, 0, action.length);
    }


    /**
     * Export the given def with the given name
     * @param n name
     * @param def def
     */
    protected void export(String n, FlashDef def) {
        addDef(def);
        ExportAssets ea = new ExportAssets();
        ea.addAsset(n, def);
        Frame frame = getMainScript().getFrameAt(0);
        frame.addFlashObject(ea);
    }

    /**
     * Make a 100x100 rectangle
     */ 
    private Shape rectangle() {
        Shape shape = new Shape();
        shape.setFillStyle1( FillStyle.newSolid( AlphaColor.white ) );
        Rectangle r = new Rectangle(0, 0, 100*TWIP, 100*TWIP); 
        shape.drawRectangle( r );
        shape.setBounds( r );
        return shape;
    }

    /**
     * Create the LFC Base Library 
     * @param path to base LFC bytecode library
     * @param props properties affecting construction
     */
    public SWFFile(String path, Properties props) {

        mProperties = props;


        try {

            String s;
            Script movieClip, empty;
            ClipActions ca;
            Instance inst;
            DoAction block;
            Frame f0, f1;
            Program p;
            Button2 but;
            Shape shape;
            AffineTransform at = new AffineTransform();
            AffineTransform offScreen = new AffineTransform();
            int states;

            Script mainScript = new Script(1);
            mainScript.setMain();
            setMainScript(mainScript); 
            Frame frame = getMainScript().getFrameAt(0);
            Shape rectShape = rectangle();

            // 1. Button moved offscreen so that it's not vis. rectangle with actions
            but = new Button2();
            states = ButtonRecord.HitTest;
            but.addButtonRecord(new ButtonRecord(states, rectShape, 1, at, new CXForm()));
            // '18' is the key code for "tab", not sure why.
            but.addActionCondition(
                new ActionCondition(KEYCODE_TAB<<9, program("_root.LzKeys.gotKeyDown(9, 'extra');"))
                );
            offScreen.scale(.1, .1);
            offScreen.translate(-200*TWIP, -200*TWIP);
            // NOTE: mimic old laszlolibrary.swf; put at depth 9
            frame.addInstance(but, 9, offScreen, null);

            // 2. Movieclip called 'entercontrol' with 2 frames.
            movieClip = new Script(2);
            //  First frame of this movieclip has a stop
            movieClip.getFrameAt(0).addStopAction();
            //  Second frame of this movieclip has a button with an empty action
            but = new Button2();
            states = ButtonRecord.HitTest;
            but.addButtonRecord(new ButtonRecord(states, rectShape, 1, at, new CXForm()));
            but.addActionCondition(new ActionCondition(KEYCODE_ENTER<<9, new Program()));
            movieClip.getFrameAt(1).addInstance(but, 1, null, null);
            // NOTE: mimic old laszlolibrary.swf; put at depth 11
            frame.addInstance(movieClip, 11, offScreen, null, "entercontrol");

            // 3. movieclip called "frameupdate" which has two frames and some clip actions:
            movieClip = new Script(2);
            block = actionBlock("_root.LzIdle.onidle.sendEvent( getTimer() );");
            movieClip.getFrameAt(0).addFlashObject(block);
            movieClip.getFrameAt(1).addFlashObject(block);

            // NOTE: depth=2 required in order for this to work in swf6 for some
            // unknown reason!
            inst = frame.addInstance(movieClip, 2, null, null, "frameupdate");
            ca = new ClipActions();
            ca.setMask(ClipAction.KEY_DOWN|ClipAction.KEY_UP|ClipAction.MOUSE_DOWN|ClipAction.MOUSE_UP);
            s = "_root.LzKeys.gotKeyDown(Key.getCode())";
            ca.addAction(new ClipAction(ClipAction.KEY_DOWN, program(s)));
            s = "_root.LzKeys.gotKeyUp(Key.getCode())";
            ca.addAction(new ClipAction(ClipAction.KEY_UP, program(s)));
            s = "_root.LzModeManager.rawMouseEvent('onmousedown')";
            ca.addAction(new ClipAction(ClipAction.MOUSE_DOWN, program(s)));
            s = "_root.LzModeManager.rawMouseEvent('onmouseup')";
            ca.addAction(new ClipAction(ClipAction.MOUSE_UP, program(s)));
            inst.actions = ca;

            // 4. A movieclip with the export identifier "empty" that has 1 frame with nothing in it
            empty = movieClip = new Script(1);
            export("empty", movieClip);
             
            // 5. A movieclip with the export identifier "LzMask" that has 1 frame and three layers.
            // *First layer is a mask layer which has:
            //     A 100x100 square rectangle
            // *Second layer is an empty movieclip which is named "mask_sub"
            // *Third layer is a movieclip named "meas" which contains a 100x100 rectangle and these actions:
            // onClipEvent( load ){
            //     this._visible = false;
            // }
            // NOTE: mimic depths from old laszlolibrary.swf
            movieClip = new Script(1);
            export("LzMask", movieClip);
            f0 = movieClip.getFrameAt(0);

            // 3rd layer: NOT NEEDED according to adam.
            /* 
            movieClip = new Script(1);
            movieClip.getFrameAt(0).addInstance(rectShape, 1, at, null);
            inst = f0.addInstance(movieClip, 1, at, null, "meas");
            ca = new ClipActions();
            ca.setMask(ClipAction.LOAD);
            ca.addAction(new ClipAction(ClipAction.LOAD, program("this._visible = false;")));
            inst.actions = ca;
            */

            // 2nd layer
            f0.addInstance(empty, 4, null, null, "mask_sub");

            // 1st layer
            inst = f0.addInstance(rectShape, 3, at, null);
            inst.clip = 6; // NOTE: [2004-02-17 bloch] mimic depth from old laszlolibrary.swf
            

            // 6. A movieclip with the export identifier "LzMouseEvents" that has a 100x100 
            // button with no visible region and several actions
            movieClip = new Script(1);
            export("LzMouseEvents", movieClip);
            but = new Button2();
            states = ButtonRecord.HitTest;
            but.addButtonRecord(new ButtonRecord(states, rectShape, 1, at, new CXForm()));
            but.addActionCondition(ActionCondition.onPress(program(
                "_root.LzModeManager.handleMouseButton( myView, 'onmousedown')")));
            but.addActionCondition(ActionCondition.onRelease(program(
                "_root.LzModeManager.handleMouseButton( myView, 'onmouseup');" + 
                "_root.LzModeManager.handleMouseEvent( myView, 'onclick')")));
            but.addActionCondition(ActionCondition.onReleaseOutside(program(
                "_root.LzModeManager.handleMouseButton( myView, 'onmouseup');" + 
                "_root.LzModeManager.handleMouseEvent( myView, 'onmouseupoutside')")));
            but.addActionCondition(ActionCondition.onRollOver(program(
                "_root.LzModeManager.handleMouseEvent( myView, 'onmouseover')")));
            but.addActionCondition(ActionCondition.onRollOut(program(
                "_root.LzModeManager.handleMouseEvent( myView, 'onmouseout')")));
            but.addActionCondition(ActionCondition.onDragOut(program(
                "_root.LzModeManager.handleMouseEvent( myView, 'onmouseout');" + 
                "_root.LzModeManager.handleMouseEvent( myView, 'onmousedragout')")));
            but.addActionCondition(ActionCondition.onDragOver(program(
                "_root.LzModeManager.handleMouseEvent( myView, 'onmouseover');" + 
                "_root.LzModeManager.handleMouseEvent( myView, 'onmousedragin')")));
            movieClip.getFrameAt(0).addInstance(but, 1, at, null);

            // 7. A movieclip with the export identifier "swatch" which has a white 100x100 rectangle.
            movieClip = new Script(1);
            export("swatch", movieClip);
            f0 = movieClip.getFrameAt(0);
            f0.addInstance(rectShape, 1, at, null);

            // 8. A movieclip with the export identifier "LZkranker" which has 2 frames:
            String krank = mProperties.getProperty(CompilationEnvironment.KRANK_PROPERTY);
            if ("true".equals(krank)) {
                movieClip = new Script(2);
                export("__LZkranker", movieClip);
                f0 = movieClip.getFrameAt(0); 
                f0.addFlashObject(actionBlock("_root.LzSerializer.procStack();"));
                f1 = movieClip.getFrameAt(1); 
                f1.addFlashObject(actionBlock("_root.LzSerializer.procStack()"));
            }

            // 9. A movieclip with the export identifier "LZprofiler" which has 2 frames:
            String profile = mProperties.getProperty(CompilationEnvironment.PROFILE_PROPERTY);
            if ("true".equals(profile)) {
                movieClip = new Script(2);
                export("__LZprofiler", movieClip);
                block = actionBlock("_root.$lzprofiler.dump()");
                movieClip.getFrameAt(0).addFlashObject(block); 
                movieClip.getFrameAt(1).addFlashObject(block); 
            }

            // 10. The LFC ActionScript block, the only def in the incoming file 
            FlashFile oldLibrary = FlashFile.parse(path);
            IVVector objs = oldLibrary.getMainScript().getFrameAt(0);
            block = (DoAction)objs.elementAt(0);
            if (block == null) {
                throw new ChainedException("no DoAction block in " + path);
            }
            frame.addFlashObject(block);

            // 11. Add in end-of-LFC marker for qa
            block = actionBlock("global[ '$endOfLFCMarker' ];");
            frame.addFlashObject(block);

        } catch (IVException e) {
            throw new ChainedException(e);
        } catch (FileNotFoundException e) {
            throw new ChainedException(e);
        } catch (IOException e) {
            throw new ChainedException(e);
        }
    }

    /**
     * Create a SWFBaseLibrary that has no assets or logic in it
     */
    public SWFFile(Properties props) {
        mProperties = props;
    }

    /**
     * Add the preloader frame to 
     */
    void addPreloaderFrame(String preloadLibPath) {
                
        try {
            // Insert a blank frame for the preloader
            Timeline tm = getMainScript().getTimeline();
            tm.insertFrames(0, 1);
    
            // Make heartbeat movieclip
            Script hbClip = new Script(1);
            hbClip.getFrameAt(0);
            hbClip.setName("lzpreloader._heartbeat");
            addDef(hbClip);
    
            // Make preloader movieclip
            Script prelClip = new Script(1);
            prelClip.setName("lzpreloader");
            Frame frame = prelClip.getFrameAt(0);
            // Get preload lib
            FlashFile lib = FlashFile.parse(preloadLibPath);
            IVVector objs = lib.getMainScript().getFrameAt(0);
            DoAction block = (DoAction)objs.elementAt(0);
            frame.addFlashObject(block);
            // Add instance
            Instance inst = frame.addInstance(hbClip, 1, null, null, "_heartbeat");
            ClipActions ca = new ClipActions();
            ca.setMask(ClipAction.ENTER_FRAME);
            ca.addAction(new ClipAction(ClipAction.ENTER_FRAME, program("_parent.heartbeat()")));
            inst.actions = ca;
            // Export me
            export("lzpreloader", prelClip);

            // Insert me
            tm.getFrameAt(0).addInstance(prelClip, 1, null, null, "lzpreloader");
    
        } catch (IVException e) {
            throw new ChainedException(e);
        } catch (FileNotFoundException e) {
            throw new ChainedException(e);
        } catch (IOException e) {
            throw new ChainedException(e);
        }
    }
}
