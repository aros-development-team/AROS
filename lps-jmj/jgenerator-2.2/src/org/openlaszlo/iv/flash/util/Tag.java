/*
 * $Id: Tag.java,v 1.4 2002/05/30 05:04:56 skavish Exp $
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

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.parser.FixedTag;

/**
 * Enumeration of flash file format tags.
 *
 * @author Dmitry Skavish
 */
public final class Tag {

    public static final int END                  = 0;
    public static final int SHOWFRAME            = 1;
    public static final int DEFINESHAPE          = 2;
    public static final int FREECHARACTER        = 3;
    public static final int PLACEOBJECT          = 4;
    public static final int REMOVEOBJECT         = 5;
    public static final int DEFINEBITS           = 6;
    public static final int DEFINEBUTTON         = 7;
    public static final int JPEGTABLES           = 8;
    public static final int SETBKGCOLOR          = 9;
    public static final int DEFINEFONT           = 10;
    public static final int DEFINETEXT           = 11;
    public static final int DOACTION             = 12;
    public static final int DEFINEFONTINFO       = 13;
    public static final int DEFINESOUND          = 14;
    public static final int STARTSOUND           = 15;
    //public static final int                      = 16;
    public static final int DEFINEBUTTONSOUND    = 17;
    public static final int SOUNDSTREAMHEAD      = 18;
    public static final int SOUNDSTREAMBLOCK     = 19;
    public static final int DEFINEBITSLOSSLESS   = 20;
    public static final int DEFINEBITSJPEG2      = 21;
    public static final int DEFINESHAPE2         = 22;
    public static final int DEFINEBUTTONCXFORM   = 23;
    public static final int PROTECT              = 24;
    public static final int PATHSAREPOSTSCRIPT   = 25;
    public static final int PLACEOBJECT2         = 26;
    //public static final int                      = 27;
    public static final int REMOVEOBJECT2        = 28;
    //public static final int                      = 29;
    //public static final int                      = 30;
    //public static final int                      = 31;
    public static final int DEFINESHAPE3         = 32;
    public static final int DEFINETEXT2          = 33;
    public static final int DEFINEBUTTON2        = 34;
    public static final int DEFINEBITSJPEG3      = 35;
    public static final int DEFINEBITSLOSSLESS2  = 36;
    public static final int DEFINEEDITTEXT       = 37;
    public static final int DEFINEMOVIE          = 38;
    public static final int DEFINESPRITE         = 39;
    public static final int NAMECHARACTER        = 40;
    public static final int SERIALNUMBER         = 41;
    public static final int GENERATORTEXT        = 42;
    public static final int FRAMELABEL           = 43;
    //public static final int                      = 44;
    public static final int SOUNDSTREAMHEAD2     = 45;
    public static final int DEFINEMORPHSHAPE     = 46;
    //public static final int                      = 47;
    public static final int DEFINEFONT2          = 48;
    public static final int TEMPLATECOMMAND      = 49;
    //public static final int                      = 50;
    public static final int FLASHGENERATOR       = 51;
    public static final int EXTERNALFONT         = 52;
    //public static final int                      = 53;
    //public static final int                      = 54;
    //public static final int                      = 55;
    public static final int EXPORTASSETS         = 56;  // Flash 5
    public static final int IMPORTASSETS         = 57;  // Flash 5
    public static final int ENABLEDEBUGGER       = 58;  // Flash 5
    public static final int INITCLIPACTION       = 59;  // Flash 6
    //public static final int                      = 60;
    //public static final int                      = 61;
    public static final int DEFINEFONTINFO2      = 62;  // Flash 6

    public static final String[] tagNames = {
        "End",
        "ShowFrame",
        "DefineShape",
        "FreeCharacter",
        "PlaceObject",
        "RemoveObject",
        "DefineBits",
        "DefineButton",
        "JPEGTables",
        "SetBackgroundColor",
        "DefineFont",
        "DefineText",
        "DoAction",
        "DefineFontInfo",
        "DefineSound",
        "StartSound",
        "Unknown",
        "DefineButtonSound",
        "SoundStreamHead",
        "SoundStreamBlock",
        "DefineBitsLossLess",
        "DefineBitsJPEG",
        "DefineShape2",
        "DefineButtonCXForm",
        "Protect",
        "PathsArePostscript",
        "PlaceObject2",
        "Unknown",
        "RemoveObject2",
        "Unknown",
        "Unknown",
        "Unknown",
        "DefineShape3",
        "DefineText2",
        "DefineButton2",
        "DefineBitsJPEG3",
        "DefineBitsLossLess2",
        "DefineEditText",
        "DefineMovie",
        "DefineSprite",
        "NameCharacter",
        "SerialNumber",
        "GeneratorText",
        "FrameLabel",
        "Unknown",
        "SoundStreamHead2",
        "DefineMorphShape",
        "Unknown",
        "DefineFont2",
        "TemplateCommand",
        "Unknown",
        "FlashGenerator",
        "ExternalFont",
        "Unknown",
        "Unknown",
        "Unknown",
        "ExportAssets",
        "ImportAssets",
        "EnableDebugger",
        "InitClipAction",
        "Unknown",
        "Unknown",
        "DefineFontInfo2",
    };

    public static final FlashObject END_TAG       = new FixedTag( END );
    public static final FlashObject SHOWFRAME_TAG = new FixedTag( SHOWFRAME );

}
