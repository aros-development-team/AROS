/*
 * $Id: ScriptCopier.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

package org.openlaszlo.iv.flash.util;

import java.io.*;
import java.util.*;
import org.openlaszlo.iv.flash.api.*;

/**
 * This class helps in copying of flash objects.
 * <P>
 * It remembers all copied objects and makes sure
 * that an object will not be copied twice.
 * All flash objects can be constant or not. Constant object is such
 * an object which is invariant to generator processing, for example if an
 * object represents some text and this text does not contain generator
 * variable, then this object is constant, otherwise it is considered
 * to be non-constant.<BR>
 * So when an object is to be copied it's checked whether it's a constant
 * or not. If the object is constant it's not copied, but rather it's reference
 * is used. If the object is not a constant then it is copied using the same guidelines,
 * i.e. if the object contains non-constant object, they will not be copied.
 *
 * @author Dmitry Skavish
 */
public final class ScriptCopier {

    private Hashtable hashTable = new Hashtable();

    public ScriptCopier() {}

    /**
     * Copies the specified object.
     * <P>
     * Returns copy of the specified object or the object itself
     * depending on whether the object is constant or not.
     * Objects are not copied twice, i.e. all copied objects are
     * remembered and on second request "remembered" object will be
     * returned.
     *
     * @param def    object to be copied
     * @return the object's copy
     */
    public FlashDef copy( FlashDef def ) {
        if( def == null ) return null;
        if( def.isConstant() ) return def;
        FlashDef myDef = (FlashDef) hashTable.get(def);
        if( myDef != null ) return myDef;
        myDef = (FlashDef) def.getCopy(this);
        hashTable.put(def, myDef);
        return myDef;
    }

    /**
     * Returns remembered copy of this object or null.
     *
     * @param def    object remembered copy of which is requested
     * @return remembered copy of the object or null
     */
    public FlashDef get( FlashDef def ) {
        if( def == null ) return null;
        FlashDef myDef = (FlashDef) hashTable.get(def);
        if( myDef == null ) return def;
        return myDef;
    }

}
