/*
 * $Id: ActionCondition.java,v 1.3 2002/08/06 23:55:01 skavish Exp $
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
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.action.*;

public class ActionCondition extends FlashItem {

    // Button state transitions triggering an action.
    public static int OverDownToIdle    = 0x0100;   // DragOut 1
    public static int IdleToOverDown    = 0x0080;   // DragOver 1
    public static int OutDownToIdle     = 0x0040;   // Release Outside
    public static int OutDownToOverDown = 0x0020;   // DragOver 2
    public static int OverDownToOutDown = 0x0010;   // DragOut 2
    public static int OverDownToOverUp  = 0x0008;   // Release
    public static int OverUpToOverDown  = 0x0004;   // Press
    public static int OverUpToIdle      = 0x0002;   // RollOut
    public static int IdleToOverUp      = 0x0001;   // RollOver

    private int condition;
    private Program program;

    public ActionCondition() {}

    public ActionCondition( int condition, Program program ) {
        this.condition = condition;
        this.program = program;
    }

    /**
     * Creates action condition on all "out" events: RollOut, DragOut
     *
     * @param program program to be executed on the event
     * @return action condition
     */
    public static ActionCondition onOut( Program program ) {
        return new ActionCondition(OverUpToIdle|OverDownToIdle|OverDownToOutDown, program);
    }

    /**
     * Creates action condition on all "over" events: RollOver, DragOver
     *
     * @param program program to be executed on the event
     * @return action condition
     */
    public static ActionCondition onOver( Program program ) {
        return new ActionCondition(IdleToOverUp|IdleToOverDown|OutDownToOverDown, program);
    }

    /**
     * Creates action condition on RollOut event
     *
     * @param program program to be executed on the event
     * @return action condition
     */
    public static ActionCondition onRollOut( Program program ) {
        return new ActionCondition(OverUpToIdle, program);
    }

    /**
     * Creates action condition on RollOver event
     *
     * @param program program to be executed on the event
     * @return action condition
     */
    public static ActionCondition onRollOver( Program program ) {
        return new ActionCondition(IdleToOverUp, program);
    }

    /**
     * Creates action condition on Press event
     *
     * @param program program to be executed on the event
     * @return action condition
     */
    public static ActionCondition onPress( Program program ) {
        return new ActionCondition(OverUpToOverDown, program);
    }

    /**
     * Creates action condition on DragOut event
     *
     * @param program program to be executed on the event
     * @return action condition
     */
    public static ActionCondition onDragOut( Program program ) {
        return new ActionCondition(OverDownToIdle|OverDownToOutDown, program);
    }

    /**
     * Creates action condition on DragOver event
     *
     * @param program program to be executed on the event
     * @return action condition
     */
    public static ActionCondition onDragOver( Program program ) {
        return new ActionCondition(IdleToOverDown|OutDownToOverDown, program);
    }

    /**
     * Creates action condition on ReleaseOutside event
     *
     * @param program program to be executed on the event
     * @return action condition
     */
    public static ActionCondition onReleaseOutside( Program program ) {
        return new ActionCondition(OutDownToIdle, program);
    }

    /**
     * Creates action condition on Release event
     *
     * @param program program to be executed on the event
     * @return action condition
     */
    public static ActionCondition onRelease( Program program ) {
        return new ActionCondition(OverDownToOverUp, program);
    }

    /**
     * Creates action condition on key press
     *
     * @param ch      key code to catch
     * @param program program to be executed on the event
     * @return action condition
     */
    public static ActionCondition onKeyPress( char ch, Program program ) {
        return new ActionCondition((ch<<1)&0xfe, program);
    }

    public void setCondition( int condition ) { this.condition = condition; }
    public void setProgram( Program program ) { this.program = program; }

    public int getCondition() { return condition; }
    public Program getProgram() { return program; }

    public void write( FlashOutput fob ) {
        fob.writeWord(condition);
        program.write(fob);
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"    ActionCondition: condition="+Util.w2h(condition) );
        program.printContent(out, indent+"        ");
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return new ActionCondition( condition, (Program) program.getCopy(copier) );
    }

}

