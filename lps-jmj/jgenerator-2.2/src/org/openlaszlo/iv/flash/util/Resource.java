/*
 * $Id: Resource.java,v 1.9 2002/08/02 03:15:17 skavish Exp $
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

import java.util.*;

/**
 * Resources
 * <P>
 * Resource key has to have digit in first character from 0 to 4.
 * <ol start=0>
 * <li>FATAL
 * <li>ERROR
 * <li>WARN
 * <li>INFO
 * <li>DEBUG
 * </ol>
 *
 * @author Dmitry Skavish
 */
public class Resource extends ListResourceBundle {

    public static final String INFINITELOOP         = "0_000";
    public static final String GENCMDERR            = "2_001";
    public static final String UNKNOWNTAG           = "4_002";
    public static final String UNKNOWNGENTEXT       = "4_003";
    public static final String CMDNOTFOUND          = "1_004";
    public static final String CMDNOTCREATED        = "1_005";
    public static final String ERRCMDFILEREAD       = "1_006";
    public static final String CMDSCRIPTNOTFOUND    = "1_007";
    public static final String INVALRATEVALUE       = "1_008";
    public static final String INVALURL             = "1_009";
    public static final String ERRDATAREAD          = "1_00a";
    public static final String INVALDATASOURCE      = "1_00b";
    public static final String INVALALPHA           = "1_00c";
    public static final String COLNOTFOUNDCMD       = "1_00d";
    public static final String ERRPARSETEMPLATE     = "0_00e";
    public static final String CANTREADHEADER       = "0_00f";
    public static final String ILLEGALHEADER        = "0_010";
    public static final String FILETOOSHORT         = "0_011";
    public static final String FILETOOBIG           = "0_012";
    public static final String ERRWRITINGFILE       = "1_013";
    public static final String ERRREADINGFILE       = "1_014";
    public static final String FILENOTFOUND         = "1_015";
    public static final String UNKNOWNERROR         = "1_016";
    public static final String REENCODINGJPEG       = "3_018";
    public static final String INVLEXTERNALFONT     = "1_019";
    public static final String CLIPORDEFSYM         = "1_01a";
    public static final String ROWSORCOLS           = "1_01b";
    public static final String BORDERTOOTHICK       = "1_01c";
    public static final String PROCESSREQUEST       = "3_01d";
    public static final String ERRREADINGGIF        = "1_01e";
    public static final String ERRREADINGJPG        = "1_01f";
    public static final String ERRDOCMD             = "1_020";
    public static final String ERRREADINGPNG        = "1_021";
    public static final String UNSUPMEDIA           = "1_022";
    public static final String NOTEXT               = "1_023";
    public static final String CONNECTINGTO         = "3_024";
    public static final String RETRIEVINGCONTENT    = "3_025";
    public static final String INVLMP3LAYER         = "1_026";
    public static final String INVLMP3              = "1_027";
    public static final String INVLMP3FREQUENCY     = "1_028";
    public static final String NOGRAPHCONTEXT       = "1_029";
    public static final String NOMATCHINGCONTEXTS   = "1_02a";
    public static final String EXPECTSTDCONTEXT     = "1_02b";
    public static final String ERRSETPROPERTY       = "1_02e";
    public static final String CANTLOADPROPERTIES   = "1_02f";
    public static final String CANTLOADSYSPROPS     = "1_030";
    public static final String SQLQUERY             = "3_031";
    public static final String BADHIGHORLOW         = "1_032";
    public static final String PROCESSERROR         = "1_033";
    public static final String REQUESTFROM          = "3_034";
    public static final String COLNOTFOUNDCMD1      = "1_035";
    public static final String RESCALINGJPEG        = "3_036";
    public static final String JSERROR              = "1_037";
    public static final String SENDERROR            = "1_038";
    public static final String INLINECMDNOTFOUND    = "1_039";
    public static final String INLINECMDERROR       = "1_03a";
    public static final String CMDINSTNOTFOUND      = "1_03b";
    public static final String SERVERSTARTED        = "3_03c";
    public static final String ASASMREQFLASH5       = "2_03d";

    private static final Object[][] contents = {
        {INFINITELOOP        , "Infinite loop in script processing"},
        {GENCMDERR           , "Generator command ''{0}'' can be applied to instance only"},
        {UNKNOWNTAG          , "Unknown tag {0} found"},
        {UNKNOWNGENTEXT      , "Unknown generator text subtag {0} found"},
        {CMDNOTFOUND         , "Generator Command ''{0}'' not found"},
        {CMDNOTCREATED       , "Error creating Generator Command ''{0}''"},
        {ERRCMDFILEREAD      , "Error reading file ''{0}'' in command {1}"},
        {CMDSCRIPTNOTFOUND   , "Script ''{0}'' not found in command {1}"},
        {INVALRATEVALUE      , "Invalid rate {0} in command SetMovieParameters"},
        {INVALURL            , "Invalid url ''{0}''"},
        {ERRDATAREAD         , "Error reading data from datasource ''{0}'' in command {1}"},
        {INVALDATASOURCE     , "Invalid datasource ''{0}'' in command {1}"},
        {INVALALPHA          , "Invalid percent value {0} in command {1}"},
        {COLNOTFOUNDCMD      , "Command {0}: Column {1} not found in datasource {2}"},
        {COLNOTFOUNDCMD1     , "Command {0}: One of the following columns are required in datasource: {1}"},
        {ERRPARSETEMPLATE    , "Error parsing template ''{0}''"},
        {CANTREADHEADER      , "Cannot read the header of template ''{0}''"},
        {ILLEGALHEADER       , "Template ''{0}'' has illegal header - not a Shockwave Flash File"},
        {FILETOOSHORT        , "Template ''{0}'' size is too short"},
        {FILETOOBIG          , "Template ''{0}'' too big to fit into memory"},
        {ERRWRITINGFILE      , "Error writing file ''{0}''"},
        {ERRREADINGFILE      , "Error reading file ''{0}''"},
        {FILENOTFOUND        , "File ''{0}'' not found"},
        {UNKNOWNERROR        , "Unexpected exception caught, please send the log file to developers"},
        {REENCODINGJPEG      , "reEncoding JPEG image with quality {0}"},
        {RESCALINGJPEG       , "reScaling JPEG image with quality {2}, new width={0}, height={1}"},
        {INVLEXTERNALFONT    , "Cannot find any fonts in file ''{0}''"},
        {CLIPORDEFSYM        , "Table object has to have either Clip column or DefaultSymbol"},
        {ROWSORCOLS          , "Invalid number of rows or columns in command {0}"},
        {BORDERTOOTHICK      , "Border is too thick in Table command"},
        {PROCESSREQUEST      , "Processed file ''{0}'', output size {1} bytes, processing time {2}ms, total time {3}ms"},
        {ERRREADINGGIF       , "Error reading GIF stream"},
        {ERRREADINGJPG       , "Error reading JPEG stream"},
        {ERRDOCMD            , "Error occured in file ''{0}'', movie clip ''{1}'', frame ''{2}'', command ''{3}''"},
        {ERRREADINGPNG       , "Error reading PNG stream"},
        {UNSUPMEDIA          , "Unsupported media file ''{0}''"},
        {NOTEXT              , "No text for 'InsertText' command"},
        {CONNECTINGTO        , "Connecting to {0} ..."},
        {RETRIEVINGCONTENT   , "Retrieving content of {0} ..."},
        {INVLMP3LAYER        , "Invalid MP3 - only MPEG Audio Layer 3 supported"},
        {INVLMP3             , "Invalid MP3 header"},
        {INVLMP3FREQUENCY    , "Invalid MP3 frequency - must be 11025Hz, 22050Hz or 44100Hz"},
        {NOGRAPHCONTEXT      , "Current context has no GraphContext ancestor"},
        {NOMATCHINGCONTEXTS  , "No ancestor provided a match for ''{0}''"},
        {EXPECTSTDCONTEXT    , "Text (tabular) context has been expected"},
        {ERRSETPROPERTY      , "Please set property 'org.openlaszlo.iv.flash.installDir' to the directory where the JGenerator is installed"},
        {CANTLOADPROPERTIES  , "Cannot load properties file"},
        {CANTLOADSYSPROPS    , "Cannot retrieve system properties, using defaults"},
        {SQLQUERY            , "SQL Query \n   driver: {0}\n   url: {1}\n   user/password: {2}/{3}\n   query: {4}"},
        {BADHIGHORLOW        , "Command {0}: Either HIGH or LOW is not extreme value in row {1} of datasource {2}"},
        {PROCESSERROR        , "Error processing template ''{0}''"},
        {SENDERROR           , "Error writing to output stream during processing of template ''{0}''"},
        {REQUESTFROM         , "Processing request from ''{0}''"},
        {JSERROR             , "JavaScript: {0}"},
        {INLINECMDNOTFOUND   , "Inline command {0} not found"},
        {INLINECMDERROR      , "Error executing inline command {0}"},
        {CMDINSTNOTFOUND     , "Could not find an instance for command {0}. Set parameter 'instancename'."},
        {SERVERSTARTED       , "JGenerator servlet started."},
        {ASASMREQFLASH5      , "ASAssembler requires SWF version to be 5+"},
    };

    public Object[][] getContents() {
        return contents;
    }

    public static String get( String key ) {
        return bundle.getString(key);
    }

    private static ResourceBundle bundle;

    public static ResourceBundle getInstance() {
        return bundle;
    }

    static {
        bundle = ResourceBundle.getBundle( Resource.class.getName() );
    }

}

