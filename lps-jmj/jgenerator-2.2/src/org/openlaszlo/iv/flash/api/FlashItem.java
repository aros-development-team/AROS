/*
 * $Id: FlashItem.java,v 1.2 2002/02/15 23:44:27 skavish Exp $
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

import java.io.*;
import java.lang.reflect.Constructor;
import org.openlaszlo.iv.flash.util.*;

/**
 * Abstract base class for all Flash entities
 * <P>
 * All objects which can be parsed from swf file or generated into swf file are subclasses of this class
 * <P>
 * Object of this class can:<br>
 * <UL>
 * <LI>write itself to {@link FlashBuffer}
 * <LI>print itself to {@link PrintStream}
 * <LI>be copied through {@link ScriptCopier}
 * </UL>
 *
 * @author Dmitry Skavish
 */
public abstract class FlashItem {

    /**
     * Writes object to flash buffer
     *
     * @param fob    flash buffer to write
     */
    public abstract void write( FlashOutput fob );

    /**
     * Prints object to {@link PrintStream}
     *
     * @param out    stream to print
     * @param indent this string is printed before each line printed
     */
    public abstract void printContent( PrintStream out, String indent );

    /**
     * Copies this flash item into empty given one through {@link ScriptCopier}<br>
     * All subclasses of this class has to override this method if they have their own data
     *
     * @param item   item to copy to
     * @param copier copier
     * @return filled object
     * @see #getCopy
     */
    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        return item;
    }

    /**
     * Creates copy of this object using {@link ScriptCopier} class
     *
     * @param copier copier to use
     * @return copy of this object
     * @see #copyInto
     */
    public FlashItem getCopy( ScriptCopier copier ) {
        try {
            Constructor c = getClass().getConstructor( new Class [] {} );
            FlashItem o = (FlashItem) c.newInstance( new Object[] {} );
            return copyInto( o, copier );
        } catch( Exception e ) {
            return null;
        }
    }

    public String toString() {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(bos);
        printContent(ps, "");
        ps.flush();
        return bos.toString();
    }
}

