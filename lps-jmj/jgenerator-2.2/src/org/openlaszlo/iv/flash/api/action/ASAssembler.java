/*
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

package org.openlaszlo.iv.flash.api.action;

import java.util.*;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.context.Context;

/**
 * ActionScript API assembler
 * <P>
 * Supports only Flash 5/MX actionscript.
 *
 * @author Dmitry Skavish
 */
public class ASAssembler {

    private HashMap pool = new HashMap();
    private HashMap labels = new HashMap();
    private IVVector forward_refs = new IVVector();
    private IVVector delayed_stack = new IVVector();
    private Program p = new Program();
    private Stack funcs = new Stack();

    /**
     * Creates new assembler
     * <P>
     * Requires flash version 5+
     *
     * @param file   flash file.
     */
    public ASAssembler( FlashFile file ) {
        if( file.getVersion() < 5 ) {
            Log.logRB(Resource.ASASMREQFLASH5);
        }
    }

    /**
     * Creates Program object from this assembler
     *
     * @return new Program object
     */
    public Program toProgram() {
        flush_stack();
        Program prog = new Program();
        writePool(prog);
        prog.body().writeFOB(p.body());
        return prog;
    }

    /**
     * Instructs the player to go to the next frame in the current movie.
     */
    public void nextFrame() {
        flush_stack();
        p.nextFrame();
    }

    /**
     * Instructs the player to go to the previous frame in the current movie.
     */
    public void prevFrame() {
        flush_stack();
        p.prevFrame();
    }

    /**
     * Instructs the player to start playing at the current frame.
     */
    public void play() {
        flush_stack();
        p.play();
    }

    /**
     * Instructs the player to stop playing the movie at the current frame.
     * @since flash 3
     */
    public void stop() {
        flush_stack();
        p.stop();
    }

    /**
     * Toggle the display between high and low quality.
     */
    public void toggleQuality() {
        flush_stack();
        p.toggleQuality();
    }

    /**
     * Instructs the player to stop playing all sounds.
     */
    public void stopSounds() {
        flush_stack();
        p.stopSounds();
    }

    public void trace() {
        flush_stack();
        p.body().writeByte(Actions.Trace);
    }

    /**
     * Adds two values. Can be used for strings as well.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>Pushes the result, A+B, to the stack.
     * </OL>
     */
    public void add() {
        flush_stack();
        p.body().writeByte(Actions.Add2);
    }

    /**
     * Subtracts two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>A is subtracted from B.
     * <LI>Pushes the result, A-B, to the stack.
     * </OL>
     */
    public void subtract() {
        flush_stack();
        p.subtract();
    }

    /**
     * Multiplies two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>The numbers are multiplied.
     * <LI>Pushes the result, A*B, to the stack.
     * </OL>
     */
    public void multiply() {
        flush_stack();
        p.multiply();
    }

    /**
     * Divides two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>B is divided by A.
     * <LI>Pushes the result, B/A, to the stack.
     * <LI>If A is zero, the result is the string #ERROR#.
     * </OL>
     * Note: When playing a Flash 5 .SWF, NaN, Infinity or –Infinity is pushed to the stack instead of #ERROR#.
     * @since flash 4
     */
    public void divide() {
        flush_stack();
        p.divide();
    }

    /**
     * Tests two values for equality. Can be used for strings as well.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>If the values are equal, a true is pushed to the stack.
     * <LI>Otherwise, a false is pushed to the stack.
     * </OL>
     */
    public void equal() {
        flush_stack();
        p.body().writeByte( Actions.Equals2 );
    }

    /**
     * Tests if a value is less than another value. Can be used for strings as well.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>The values are compared for equality.
     * <LI>If B < A, a true is pushed to the stack; otherwise, a false is pushed to the stack.
     * </OL>
     */
    public void less() {
        flush_stack();
        p.body().writeByte( Actions.Less2 );
    }

    /**
     * Performs a logical AND of two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>If both numbers are nonzero, a 1 is pushed to the stack; otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void logicalAnd() {
        flush_stack();
        p.logicalAnd();
    }

    /**
     * Performs a logical OR of two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>If either numbers is nonzero, a 1 is pushed to the stack; otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void logicalOr() {
        flush_stack();
        p.logicalOr();
    }

    /**
     * Performs a logical NOT of a number.
     * <P>
     * Note that in Macromedia Flash 5 .SWF files, the ActionNot action
     * converts its argument to a boolean, and pushes a result of type boolean.
     * In Macromedia Flash 4 .SWF files, the argument and result are numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>The numbers are compared for equality.
     * <LI>If the numbers are equal, a 1 (TRUE) is pushed to the stack.
     * <LI>Otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void logicalNot() {
        flush_stack();
        p.logicalNot();
    }

    /**
     * Tests two strings for equality.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are compared as strings. The comparison is case-sensitive.
     * <LI>If the strings are equal, a 1 (TRUE) is pushed to the stack.
     * <LI>Otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void stringEqual() {
        flush_stack();
        p.stringEqual();
    }

    /**
     * Computes the length of a string.
     * <P>
     * <OL>
     * <LI>Pops a string off the stack.
     * <LI>The length of the string is calculated and pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void stringLength() {
        flush_stack();
        p.stringLength();
    }

    /**
     * Tests if a string is less than another string.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>If B < A using a byte-by-byte comparison, a 1 is pushed to the stack;
     *     otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void stringLessThan() {
        flush_stack();
        p.stringLessThan();
    }

    /**
     * Extracts a substring from a string.
     * <P>
     * <OL>
     * <LI>Pops number <B>count</B> off the stack.
     * <LI>Pops number <B>index</B> off the stack.
     * <LI>Pops string <B>string</B> off the stack.
     * <LI>The substring of <B>string</B> starting at the <B>index</B>’th character and
     *     <B>count</B> characters in length is pushed to the stack.
     * <LI>If either <B>index</B> or <B>count</B> do not evaluate to integers, the result
     *     is the empty string.
     * </OL>
     * @since flash 4
     */
    public void subString() {
        flush_stack();
        p.subString();
    }

    /**
     * Concatenates two strings.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>The concatenation BA is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void addString() {
        flush_stack();
        p.addString();
    }

    /**
     * Computes the length of a string, multi-byte aware.
     * <P>
     * <OL>
     * <LI>Pops a string off the stack.
     * <LI>The length of the string in characters is calculated and pushed to the stack.
     * <LI>This is a multi-byte aware version of ActionStringLength.
     *     On systems with double-byte support, a double-byte character is counted as a single character.
     * </OL>
     * @since flash 4
     */
    public void mbLength() {
        flush_stack();
        p.mbLength();
    }

    /**
     * Converts ASCII to character code, multi-byte aware.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>The value is converted from a number to the corresponding character.
     *     If the character is a 16-bit value (>= 256), a double-byte character
     *     is constructed with the first byte containing the high-order byte,
     *     and the second byte containing the low-order byte.
     * <LI>The resulting character is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void mbChr() {
        flush_stack();
        p.mbChr();
    }

    /**
     * Converts character code to ASCII, multi-byte aware.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>The first character of value is converted to a numeric character code.
     *     If the first character of value is a double-byte character, a 16-bit value
     *     is constructed with the first byte as the high order byte and the second byte
     *     as the low order byte.
     * <LI>The resulting character code is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void mbOrd() {
        flush_stack();
        p.mbOrd();
    }

    /**
     * Extracts a substring from a string, multi-byte aware.
     * <P>
     * <OL>
     * <LI>Pops number <B>count</B> off the stack.
     * <LI>Pops number <B>index</B> off the stack.
     * <LI>Pops string <B>string</B> off the stack.
     * <LI>The substring of <B>string</B> starting at the <B>index</B>’th character and
     *     <B>count</B> characters in length is pushed to the stack.
     * <LI>If either <B>index</B> or <B>count</B> do not evaluate to integers, the result
     *     is the empty string.
     * <LI>This is a multi-byte aware version of ActionStringExtract. index and count are
     *     treated as character indices, counting double-byte characters as single characters.
     * </OL>
     * @since flash 4
     */
    public void mbSubString() {
        flush_stack();
        p.mbSubString();
    }

    /**
     * Converts to integer.
     * <P>
     * <OL>
     * <LI>Pops a value off the stack.
     * <LI>The value is converted to a number.
     * <LI>Next, any digits after the decimal point are discarded, resulting in an integer.
     * <LI>The resulting integer is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void toInt() {
        flush_stack();
        p.toInt();
    }

    /**
     * Converts character code to ASCII.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>The first character of value is converted to a numeric ASCII character code.
     * <LI>The resulting character code is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void ord() {
        flush_stack();
        p.ord();
    }

    /**
     * Converts ASCII to character code.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>The value is converted from a number to the corresponding ASCII character..
     * <LI>The resulting character is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void chr() {
        flush_stack();
        p.chr();
    }

    /**
     * Gets a movie property
     * <P>
     * <OL>
     * <LI>Pops index off the stack.
     * <LI>Pops target off the stack.
     * <LI>Retrieves the value of the property enumerated as index from
     *     the movie clip with target path target and pushes the value to the stack.
     * </OL>
     * @since flash 4
     */
    public void getProperty() {
        flush_stack();
        p.getProperty();
    }

    /**
     * Gets a movie property. Leaves property value on the stack.
     *
     * @param target   movie clip's target path
     * @param property property index
     */
    public void getProperty( String target, int property ) {
        push(target);
        push(property);
        getProperty();
    }

    /**
     * Sets a movie property.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>Pops index off the stack.
     * <LI>Pops target off the stack.
     * <LI>Sets the property enumerated as index in the movie clip
     *     with target path target to the value value.
     * </OL>
     * @since flash 4
     */
    public void setProperty() {
        flush_stack();
        p.setProperty();
    }

    /**
     * Sets property on specified movie clip.
     *
     * @param target   movie clip's target path
     * @param property property index
     * @param value    property value
     */
    public void setProperty( String target, int property, Object value ) {
        push(target);
        push(property);
        push(value);
        setProperty();
    }

    /**
     * Clones a movie clip.
     * <P>
     * <OL>
     * <LI>Pops depth off the stack (has to be added to 16384)
     * <LI>Pops target off the stack.
     * <LI>Pops source off the stack.
     * <LI>Duplicates movie clip source, giving the new instance
     *     the name target, at z-order depth depth.
     * </OL>
     * @since flash 4
     */
    public void duplicateClip() {
        flush_stack();
        p.cloneClip();
    }

    /**
     * Clones a movie clip.
     *
     * @param source source movie clip path
     * @param target target name
     * @param depth  depth (z-order)
     */
    public void duplicateClip( String source, String target, int depth ) {
        push(depth+16384);
        push(target);
        push(source);
        duplicateClip();
    }

    /**
     * Removes a movie clip.
     * <P>
     * <OL>
     * <LI>Pops target off the stack.
     * <LI>Removes the clone movie clip identified by target path target.
     * </OL>
     * @since flash 4
     */
    public void removeClip() {
        flush_stack();
        p.removeClip();
    }

    /**
     * Removes a movie clip.
     *
     * @param target target movie clip path
     */
    public void removeClip( String target ) {
        push(target);
        removeClip();
    }

    /**
     * Starts dragging a movie clip.
     * <P>
     * <OL>
     * <LI>Pops target off the stack. target identifies the movie clip to be dragged.
     * <LI>Pops lockcenter off the stack. If lockcenter evaluates to a nonzero value,
     *     the center of the dragged movie clip is locked to the mouse position.
     *     Otherwise, the movie clip moves relatively to the mouse position when
     *     the drag started.
     * <LI>Pops constrain off the stack.
     * <LI>If constrain evaluates to a nonzero value:
     *     <OL>
     *     <LI>Pops y2 off the stack.
     *     <LI>Pops x2 off the stack.
     *     <LI>Pops y1 off the stack.
     *     <LI>Pops x1 off the stack.
     *     </OL>
     * </OL>
     * @since flash 4
     */
    public void startDrag() {
        flush_stack();
        p.startDrag();
    }

    /**
     * Ends drag operation.
     * <P>
     * <OL>
     * <LI>Ends the drag operation in progress, if any.
     * </OL>
     * @since flash 4
     */
    public void endDrag() {
        flush_stack();
        p.endDrag();
    }

    /**
     * Calculates a random number.
     * <P>
     * <OL>
     * <LI>Pops maximum off the stack.
     * <LI>Calculates a random number, an integer in the range 0 ... (maximum-1)
     * <LI>This random number is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void random() {
        flush_stack();
        p.random();
    }

    /**
     * Calculates (pushes it to the stack) a random number in the range 0 .. (maximum-1)
     *
     * @param maximum maximum
     */
    public void random( int maximum ) {
        push(maximum);
        random();
    }

    /**
     * Reports milliseconds since player started.
     * <P>
     * <OL>
     * <LI>Calculates the number of milliseconds since the Player was started (an integer).
     * <LI>This number is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void getTimer() {
        flush_stack();
        p.getTimer();
    }

    /**
     * Instructs the player to go to the specified frame in the current movie.
     *
     * @param frame  frame number
     * @since flash 3
     */
    public void gotoFrame( int frame ) {
        flush_stack();
        p.gotoFrame(frame);
    }

    /**
     * Instructs the player to go to the specified frame in the current movie and play
     *
     * @param frame  frame number
     */
    public void gotoFrameAndPlay( int frame ) {
        p.gotoFrame(frame);
        play();
    }

    /**
     * Instructs the player to go to the specified frame in the current movie and stop
     *
     * @param frame  frame number
     */
    public void gotoFrameAndStop( int frame ) {
        gotoFrame(frame);
        stop();
    }

    /**
     * Go to frame and play, stack-based.
     * <P>
     * <OL>
     * <LI>Pops <b>frame</b> off the stack.
     * <LI>If <b>frame</b> is a number, the next frame of the movie to be displayed
     *     will be the <b>frame</b>’th frame in the current movie clip.
     * <LI>If <b>frame</b> is a string, <b>frame</b> is treated as a frame label.
     *     If the specified label exists in the current movie clip,
     *     the labeled frame will become the current frame. Otherwise, the action is ignored.
     * <LI>Either a frame or a number may be prefixed by a target path, e.g. /MovieClip:3 or /MovieClip:FrameLabel
     * </OL>
     * @since flash 4
     */
    public void gotoFrameAndPlay() {
        flush_stack();
        p.gotoFrameAndPlay();
    }

    /**
     * Go to frame and stop, stack-based.
     * <P>
     * <OL>
     * <LI>Pops <b>frame</b> off the stack.
     * <LI>If <b>frame</b> is a number, the next frame of the movie to be displayed
     *     will be the <b>frame</b>’th frame in the current movie clip.
     * <LI>If <b>frame</b> is a string, <b>frame</b> is treated as a frame label.
     *     If the specified label exists in the current movie clip,
     *     the labeled frame will become the current frame. Otherwise, the action is ignored.
     * <LI>Either a frame or a number may be prefixed by a target path, e.g. /MovieClip:3 or /MovieClip:FrameLabel
     * </OL>
     * @since flash 4
     */
    public void gotoFrameAndStop() {
        flush_stack();
        p.gotoFrameAndStop();
    }

    /**
     * Instructs the player to get the URL specified by UrlString.
     * The URL can be of any type, including an HTML file, an image
     * or another SWF movie. If the movie is playing in a browser,
     * the URL will be displayed in the frame specified by TargetString.
     * The special target names _level0 and _level1 are used to load another
     * SWF movie into levels 0 and 1 respectively.
     *
     * @param url    specified url
     * @param target target
     * @since flash 3
     */
    public void getURL( String url, String target ) {
        flush_stack();
        p.getURL(url, target);
    }

    /**
     * Get URL, stack-based.
     * <P>
     * <OL>
     * <LI>Pops window off the stack. window specifies the target window,
     *     which may be an empty string to indicate the current window.
     * <LI>Pops url off the stack. url which specifies the URL to be retrieved.
     * </OL>
     *
     * @param method Method specifies the method to use for the HTTP request. If (method and 0x40) != 0 then target is movie clip target,
     *               NOT browser window target!
     *               <UL>
     *               <LI>A value of 0 indicates that this is not a form request,
     *               so the movie clip’s variables should not be encoded and submitted.
     *               <LI>A value of 1 specifies a HTTP GET request.
     *               <LI>A value of 2 specifies a HTTP POST request.
     *               <LI>If method is 1 (GET) or 2 (POST), the variables in the current
     *               movie clip are submitted to the URL using the standard
     *               x-www-urlencoded encoding and the HTTP request method specified by method.
     *               </UL>
     */
    public void getURL( int method ) {
        flush_stack();
        p.getURL(method);
    }

    /**
     * Loads movie from specified url into specified level
     *
     * @param url    movie url
     * @param level  level. starts from 0
     * @param method <UL>
     *               <LI>0 - does not send variables
     *               <LI>1 - send variables using get
     *               <LI>2 - send variables using post
     *               </UL>
     */
    public void loadMovieNum( String url, int level, int method ) {
        push(url);
        push("_level"+level);
        getURL(method);
    }

    /**
     * Loads movie from specified url into specified level
     *
     * @param url    movie url
     * @param level  level. starts from 0
     */
    public void loadMovieNum( String url, int level ) {
        loadMovieNum(url, level, 0);
    }

    /**
     * Loads movie from specified url into specified target clip
     *
     * @param url    movie url
     * @param target target movie clip
     * @param method <UL>
     *               <LI>0 - does not send variables
     *               <LI>1 - send variables using get
     *               <LI>2 - send variables using post
     *               </UL>
     */
    public void loadMovie( String url, String target, int method ) {
        push(url);
        push(target);
        getURL(0x40+method);
    }

    /**
     * Loads movie from specified url into specified level
     *
     * @param url    movie url
     * @param target target movie clip
     */
    public void loadMovie( String url, String target ) {
        loadMovie(url, target, 0);
    }

    /**
     * Unloads movie from specified level
     *
     * @param level  level to unload the movie from
     */
    public void unloadMovieNum( int level ) {
        loadMovieNum(null, level);
    }

    /**
     * Unload movie previously loaded into specified target clip
     *
     * @param target target movie clip
     */
    public void unloadMovie( String target ) {
        loadMovie(null, target);
    }

    /**
     * Instructs the player to wait until the specified frame,
     * otherwise skip the specified number of actions.
     *
     * @param frame  specified frame
     * @param skip   specified number of actions to skip
     * @since flash 3
     */
    public void waitForFrame( int frame, int skip ) {
        flush_stack();
        p.waitForFrame(frame, skip);
    }

    /**
     * Instructs the player to change the context of subsequent actions,
     * so they apply to a named object (TargetName) rather than the current movie.
     * <P>
     * For example, the SetTarget action can be used to control the timeline of a sprite
     * object.  The following sequence of actions sends a sprite called "spinner" to the
     * first frame in its timeline:<BR>
     * <OL>
     * <LI>SetTarget "spinner"
     * <LI>GotoFrame zero
     * <LI>SetTarget "" (empty string)
     * <LI>End of actions. (Action code = 0)
     * </OL>
     * <P>
     * All actions following SetTarget "spinner" apply to the spinner
     * object until SetTarget "", which sets the action context back to
     * the current movie.
     * For a complete discussion of target names see DefineSprite.
     *
     * @param target name of the target
     * @since flash 3
     */
    public void setTarget( String target ) {
        flush_stack();
        p.setTarget(target);
    }

    /**
     * Instructs the player to go to frame associated with the specified label.
     * A label can be attached to a frame with the FrameLabel tag.
     *
     * @param label  specified frame label
     * @since flash 3
     */
    public void gotoFrameLabel( String label ) {
        flush_stack();
        p.gotoLabel(label);
    }

    /**
     * Pops a value from the stack.
     * @since flash 4
     */
    public void pop() {
        flush_stack();
        p.pop();
    }

    /**
     * Creates label at current position
     * <P>
     * The label can be used for forward and backward jumps
     * <P>
     * For example:
     * <code><PRE>
     *     as.getVar("i");
     *     as.getVar("n");
     *     as.equal();
     *     as.jumpIfTrue("end10");
     *     ......
     *     as.label("end10");
     * </PRE></CODE>
     *
     * @param name   name of label
     */
    public void label( String name ) {
        flush_stack();
        labels.put(name, new ASLabel(name, p.getPos()));
        for( int i=0; i<forward_refs.size(); i++ ) {
            ASLabel fref = (ASLabel) forward_refs.elementAt(i);
            if( fref.name.equals(name) ) {
                int offset = fref.offset;
                p.body().writeWordAt(p.getPos()-offset-2, offset);
            }
        }
    }

    /**
     * Jumps to specified label
     * <P>
     * Label has to be defined using 'label' method
     *
     * @param label_name label name
     */
    public void jump( String label_name ) {
        flush_stack();
        ASLabel l = (ASLabel) labels.get(label_name);
        if( l != null ) {
            int offset = l.offset;
            p.jump(offset-p.getPos()-5);
        } else {
            forward_refs.addElement(new ASLabel(label_name, p.getPos()+3));
            p.jump(0);
        }
    }

    /**
     * Pops top value off the stack and jumps to specified label if it's true
     * <P>
     * Label has to be defined using 'label' method
     *
     * @param label_name label name
     */
    public void jumpIfTrue( String label_name ) {
        flush_stack();
        ASLabel l = (ASLabel) labels.get(label_name);
        if( l != null ) {
            int offset = l.offset;
            p.jumpIfTrue(offset-p.getPos()-5);
        } else {
            forward_refs.addElement(new ASLabel(label_name, p.getPos()+3));
            p.jumpIfTrue(0);
        }
    }

    /**
     * Pushes a string to the stack.
     *
     * @param data   string to push
     */
    public void push( String data ) {
        delayed_stack.addElement(getString(data));
    }

    /**
     * Pushes a float to the stack.
     *
     * @param data   float to push
     */
    public void push( float data ) {
        delayed_stack.addElement(new Float(data));
    }

    /**
     * Pushes an int to the stack.
     *
     * @param data int to push
     */
    public void push( int data ) {
        delayed_stack.addElement(new Integer(data));
    }

    /**
     * Pushes an double to the stack.
     *
     * @param data double to push
     */
    public void push( double data ) {
        delayed_stack.addElement(new Double(data));
    }

    /**
     * Pushes an double to the stack.
     *
     * @param data double to push
     */
    public void push( boolean data ) {
        delayed_stack.addElement(new Boolean(data));
    }

    /**
     * Pushes an double to the stack.
     *
     * @param data double to push
     */
    public void push( Object data ) {
        if( data instanceof String ) push( (String)data );
        else if( data instanceof Expr ) ((Expr)data).eval(this);
        else delayed_stack.addElement(data);
    }

    /**
     * Sets a variable.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>Pops name off the stack, which is a string naming the variable to set.
     * <LI>Sets the variable name in the current execution context to value.
     * </OL>
     * A variable in another execution context may be referenced by prefixing the variable name with
     * the target path and a colon. For example: /A/B:FOO references variable FOO in movie
     * clip with target path /A/B.
     */
    public void setVar() {
        flush_stack();
        p.setVar();
    }

    /**
     * Sets value to a variable
     *
     * @param var1   name of the variable
     * @param data   new value
     */
    public void setVar(String var1, Object data) {
        push(var1);
        push(data);
        setVar();
    }

    /**
     * Defines local variable without init data
     *
     * @param var1   name of the variable
     */
    public void localVar( String var1 ) {
        push(var1);
        flush_stack();
        p.body().writeByte(Actions.DefineLocal2);
    }

    /**
     * Defines local variable with init value
     *
     * @param var1   name of the variable
     * @param data   init value
     */
    public void localVar( String var1, Object data ) {
        push(var1);
        push(data);
        flush_stack();
        p.body().writeByte(Actions.DefineLocal);
    }

    /**
     * Gets a variable’s value.
     * <P>
     * <OL>
     * <LI>Pops name off the stack, which is a string naming the variable to get.
     * <LI>Pushes the value of the variable to the stack.
     * </OL>
     */
    public void getVar() {
        flush_stack();
        p.eval();
    }

    /**
     * Pushes value of specified variable to the stack
     *
     * @param var1   variable name
     */
    public void getVar( String var1 ) {
        push(var1);
        flush_stack();
        p.eval();
    }

    /**
     * Pushes values of specified variables to the stack
     *
     * @param var1   variable name
     * @param var2   variable name
     */
    public void getVars( String var1, String var2 ) {
        push(var2);
        push(var1);
        getVar();
        swap();
        getVar();
    }

    /**
     * Swaps two topmost elements of the stack
     */
    public void swap() {
        flush_stack();
        p.body().writeByte(Actions.StackSwap);
    }

    /**
     * Duplicates top element of the stack
     */
    public void dup() {
        flush_stack();
        p.body().writeByte(Actions.PushDuplicate);
    }

    /**
     * Sets member to the specified value
     * <P>
     * <CODE><PRE>
     *     // _root.myclip.mysize = 10;
     *     as.setMember("_root.myclip.mysize", 10);
     *
     *     // _root.myclip.mysize = Math.sin(i);
     *     as.setMember("_root.myclip.mysize", new ASAssembler.Expr() {
     *         public void eval( ASAssembler as ) {
     *             as.callMethod("Math.sin", new ASAssembler.Expr() {
     *                 public void eval( ASAssembler as ) {
     *                     as.getVar("i");
     *                 }
     *             });
     *         }
     *     });
     * </PRE></CODE>
     *
     * @param mem    member name
     * @param value  specified value
     */
    public void setMember( String mem, Object value ) {
        IVVector t = parseMember(mem);
        if( t.size() == 1 ) {
            setVar(mem, value);
        } else {
            String iname = (String) t.elementAt(0);
            getVar(iname);
            int i=1;
            for( ; i<t.size()-1; i++ ) {
                push(t.elementAt(i));
                flush_stack();
                p.getMember();
            }
            push(t.elementAt(i));
            push(value);
            flush_stack();
            p.setMember();
        }
    }

    /**
     * Set member.
     * Stack state must be [ object, member name, value ]
     *
     * @since flash 5
     */
    public void setMember() {
        flush_stack();
        p.setMember();
    }

    /**
     * Pushes member value on the stack
     * <P>
     * <CODE><PRE>
     *     // sz = _root.myclip.mysize;
     *     as.push("sz");
     *     as.getMember("_root.myclip.mysize");
     *     as.setVar();
     *
     *     // var sz = _root.myclip.mysize;
     *     as.localVar("sz", new ASAssembler.Expr() {
     *         public void eval( ASAssembler as ) {
     *             as.getMember("_root.myclip.mysize");
     *         }
     *     });
     * </PRE></CODE>
     *
     * @param mem    member name
     */
    public void getMember( String mem ) {
        IVVector t = parseMember(mem);
        String iname = (String) t.elementAt(0);
        getVar(iname);
        if( t.size() > 1 ) {
            int i=1;
            for( ; i<t.size(); i++ ) {
                push(t.elementAt(i));
                flush_stack();
                p.getMember();
            }
        }
    }

    /**
     * Pushes element of an array into the stack
     * <P>
     * <CODE><PRE>
     *     // a = myarray[2];
     *     as.push("a");
     *     as.getArrayElement("myarray", 2);
     *     as.setVar();
     * </PRE></CODE>
     *
     * @param array_name name of array variable
     * @param index      index
     */
    public void getArrayElement( String array_name, int index ) {
        getVar(array_name);
        push(index);
        getMember();
    }

    /**
     * Pushes element of an array into the stack
     * <P>
     * <CODE><PRE>
     *     // a = myarray[i];
     *     as.push("a");
     *     as.getArrayElement("myarray", new ASAssembler.Expr() {
     *         public void eval( ASAssembler as ) {
     *             as.getVar("i");
     *         }
     *     });
     *     as.setVar();
     * </PRE></CODE>
     *
     * @param array_name name of array variable
     * @param index      index expression
     */
    public void getArrayElement( String array_name, Object index ) {
        getVar(array_name);
        push(index);
        getMember();
    }

    /**
     * Sets element of an array into the specified value
     * <P>
     * <CODE><PRE>
     *     // myarray[i] = 10;
     *     as.setArrayElement("myarray", new ASAssembler.Expr() {
     *         public void eval( ASAssembler as ) {
     *             as.getVar("i");
     *         }
     *     }, new Integer(10));
     * </PRE></CODE>
     *
     * @param array_name name of array variable
     * @param index      index expression
     */
    public void setArrayElement( String array_name, Object index, Object value ) {
        getVar(array_name);
        push(index);
        push(value);
        getMember();
    }

    /**
     * New Object
     */
    public void newObject() {
        flush_stack();
        p.newObject();
    }

    /**
     * Creates new object
     * <P>
     * <CODE><PRE>
     *     // ok = new Boolean(true);
     *     as.push("ok");
     *     as.newObject("Boolean", new Object[] {new Boolean(true)});
     *     as.setVar();
     * </PRE></CODE>
     *
     * @param class_name class name
     * @param args       arguments
     */
    public void newObject( String class_name, Object[] args ) {
        if( args == null || args.length == 0 ) {
            push(0);
        } else {
            for( int i=0; i<args.length; i++ ) {
                push(args[i]);
            }
            push(args.length);
        }
        newObject();
    }

    public void newObject( String class_name, Object arg ) {
        newObject(class_name, new Object[] {arg});
    }

    public void newObject( String class_name, int arg ) {
        newObject(class_name, new Integer(arg));
    }

    public void newObject( String class_name, double arg ) {
        newObject(class_name, new Double(arg));
    }

    public void newObject( String class_name, boolean arg ) {
        newObject(class_name, new Boolean(arg));
    }

    /**
     * Initialize object
     * <P>
     * <CODE><PRE>
     *     // a = {radius:5, angle:pi/2};
     *     as.push("a");
     *     as.initObject(
     *         new String[] {"radius", "angle"},
     *         new Object[] {
     *             new Integer(5),
     *             new ASAssembler.Expr() {
     *                 public void eval(ASAssembler as) {
     *                     as.getVar("pi");
     *                     as.push(2);
     *                     as.divide();
     *                 }
     *             }
     *         }
     *     );
     *     as.setVar();
     * </PRE></CODE>
     *
     * @param props  array of properties
     * @param values array of values
     */
    public void initObject( String[] props, Object[] values ) {
        for( int i=0; i<props.length; i++ ) {
            push(props[i]);
            push(values[i]);
        }
        push(props.length);
        flush_stack();
        p.body().writeByte(Actions.InitObject);
    }

    /**
     * Calls method with parameters
     * <P>
     * <CODE><PRE>
     *     // sn = Math.sin(10);
     *     as.push("sn");
     *     as.callMember("Math.sin", new Object[] {new Integer(10)});
     *     as.setVar();
     *
     *     // _root.myclip.play();
     *     as.callMember("_root.myclip.play", null);
     *     as.pop();       // remove resultat
     * </PRE></CODE>
     *
     * @param method method name
     * @param args   array of parameters
     */
    public void callMethod( String method, Object[] args ) {
        IVVector t = parseMember(method);
        if( t.size() == 1 ) {
            callFunction(method, args);
        } else {
            if( args == null || args.length == 0 ) {
                push(0);
            } else {
                for( int i=0; i<args.length; i++ ) {
                    push(args[i]);
                }
                push(args.length);
            }
            String iname = (String) t.elementAt(0);
            getVar(iname);
            int i=1;
            for( ; i<t.size()-1; i++ ) {
                push(t.elementAt(i));
                flush_stack();
                p.getMember();
            }
            push(t.elementAt(i));
            callMethod();
        }
    }

    public void callMethod( String method, Object arg ) {
        callMethod(method, new Object[] {arg});
    }

    public void callMethod( String method, int arg ) {
        callMethod(method, new Integer(arg));
    }

    public void callMethod( String method, double arg ) {
        callMethod(method, new Double(arg));
    }

    public void callMethod( String method, boolean arg ) {
        callMethod(method, new Boolean(arg));
    }

    /**
     * Calls a method of an object.
     * Stack state must be [ arguments, argCount, object, methodName ]
     *
     * @since flash 5
     */
    public void callMethod() {
        flush_stack();
        p.callMethod();
    }

    /**
     * Get member.
     * Stack state must be [ object, member name ]
     *
     * @since flash 5
     */
    public void getMember() {
        flush_stack();
        p.getMember();
    }

    /**
     * Calls function with parameters
     * <P>
     * <CODE><PRE>
     *     // sn = myfunc(10);
     *     as.push("sn");
     *     as.callFunction("myfunc", new Object[] {new Integer(10)});
     *     as.setVar();
     * </PRE></CODE>
     *
     * @param func_name function name
     * @param args      array of parameters
     */
    public void callFunction( String func_name, Object[] args ) {
        if( args == null || args.length == 0 ) {
            push(0);
        } else {
            for( int i=0; i<args.length; i++ ) {
                push(args[i]);
            }
            push(args.length);
        }
        push(func_name);
        flush_stack();
        p.callFunction();
    }

    public void callFunction( String func_name, Object arg ) {
        callFunction(func_name, new Object[] {arg});
    }

    public void callFunction( String func_name, int arg ) {
        callFunction(func_name, new Integer(arg));
    }

    public void callFunction( String func_name, double arg ) {
        callFunction(func_name, new Double(arg));
    }

    public void callFunction( String func_name, boolean arg ) {
        callFunction(func_name, new Boolean(arg));
    }

    /**
     * Defines new function
     * <P>
     * <CODE><PRE>
     *     // function printmsg(msg, name) {
     *     //    trace(msg+", "+name);
     *     // }
     *     as.defineFunction("printmsg", new String[] {"msg", "name"});
     *         as.getVar("name");
     *         as.push(" = ");
     *         as.getVar("msg");
     *         as.add();
     *         as.add();
     *         as.trace();
     *     as.endFunction();
     * </PRE></CODE>
     *
     * @param func_name function name
     * @param parms     parameter names
     */
    public void defineFunction( String func_name, String[] parms ) {
        flush_stack();
        FlashBuffer fob = p.body();
        fob.writeByte(Actions.DefineFunction|0x80);     // 0x80 - with length
        int pos = fob.getPos();
        fob.writeWord(0);       // size - will be stored later
        fob.writeStringZ(func_name);
        int num = parms==null? 0: parms.length;
        fob.writeWord(num);
        for( int i=0; i<num; i++ ) fob.writeStringZ(parms[i]);
        fob.writeWordAt(fob.getSize()-pos, pos);
        funcs.push(p);
        p = new Program();
    }

    /**
     * End of function definition
     *
     * @see defineFunction
     */
    public void endFunction() {
        flush_stack();

        Program prev_p = (Program) funcs.pop();
        prev_p.body().writeWord(p.body().getSize());
        prev_p.body().writeFOB(p.body());

        p = prev_p;
    }

    /**
     * Function/method return statement
     */
    public void funcReturn() {
        flush_stack();
        p.body().writeByte(Actions.Return);
    }

    /**
     * Nested expression
     * <P>
     * Usage:
     * <P>
     * <CODE><PRE>
     *     // _root.myclip.mysize = i*10;
     *     as.setMember("_root.myclip.mysize", new ASAssembler.Expr() {
     *         public void eval( ASAssembler as ) {
     *             as.push(10);
     *             as.getVar("i");
     *             as.multiply();
     *         }
     *     });
     *
     *     // var d = f*108;
     *     as.localVar("d", new ASAssembler.Expr() {
     *         public void eval( ASAssembler as ) {
     *             as.push(108);
     *             as.getVar("f");
     *             as.multiply();
     *         }
     *     });
     * </PRE></CODE>
     */
    public abstract static class Expr {
        public abstract void eval( ASAssembler as );
    }

    ///////////////////////////////////////////////////////////////////////////////////////////

    private IVVector t = new IVVector();

    private IVVector parseMember( String m ) {
        t.reset();
        int idx = 0;
        for(;;) {
            int idx2 = m.indexOf('.', idx);
            if( idx2 < 0 ) {
                if( idx == 0 ) t.addElement(m);
                else t.addElement(m.substring(idx));
                break;
            }
            t.addElement(m.substring(idx, idx2));
            idx = ++idx2;
        }
        return t;
    }

    /**
     * Returns index of specified string in contant pool
     *
     * @param s      specified string
     * @return index of specified string wrapped in Short
     */
    private Short getString( String s ) {
        Short i = (Short) pool.get(s);
        if( i == null ) {
            i = new Short((short) pool.size());
            pool.put(s, i);
        }
        return i;
    }

    /**
     * Writes constant pool
     *
     * @param p      program to write contant pool
     */
    private void writePool( Program p ) {
        if( pool.size() == 0 ) return;
        String[] ss = new String[pool.size()];
        Set set = pool.entrySet();
        for( Iterator it = set.iterator(); it.hasNext(); ) {
            Map.Entry entry = (Map.Entry) it.next();
            String s = (String) entry.getKey();
            int idx = ((Short)entry.getValue()).intValue();
            ss[idx] = s;
        }
        p.addConstantPool(ss);
    }

    /**
     * Flushes push stack
     */
    private void flush_stack() {
        int n = delayed_stack.size();
        if( n == 0 ) return;
        if( n == 1 ) {
            p.push(delayed_stack.elementAt(0));
        } else {
            Object[] data = new Object[n];
            delayed_stack.copyInto(data);
            p.push(data);
        }
        delayed_stack.reset();
    }

    private static class ASLabel {
        String name;
        int offset;
        ASLabel( String name, int offset ) {
            this.name = name;
            this.offset = offset;
        }
    }

}

