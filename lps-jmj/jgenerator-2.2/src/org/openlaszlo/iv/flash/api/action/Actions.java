/*
 * $Id: Actions.java,v 1.3 2002/06/17 03:13:05 skavish Exp $
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

package org.openlaszlo.iv.flash.api.action;

import java.io.PrintStream;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;

/**
 * Enumeration of all known actions
 */
public class Actions {

    public static final int None                     = 0x00;
    public static final int NextFrame                = 0x04;
    public static final int PrevFrame                = 0x05;
    public static final int Play                     = 0x06;
    public static final int Stop                     = 0x07;
    public static final int ToggleQuality            = 0x08;
    public static final int StopSounds               = 0x09;
    public static final int Add                      = 0x0A;
    public static final int Subtract                 = 0x0B;
    public static final int Multiply                 = 0x0C;
    public static final int Divide                   = 0x0D;
    public static final int Equal                    = 0x0E;
    public static final int LessThan                 = 0x0F;
    public static final int LogicalAnd               = 0x10;
    public static final int LogicalOr                = 0x11;
    public static final int LogicalNot               = 0x12;
    public static final int StringEqual              = 0x13;
    public static final int StringLength             = 0x14;
    public static final int SubString                = 0x15;
    public static final int Pop                      = 0x17;
    public static final int Int                      = 0x18;
    public static final int Eval                     = 0x1C;
    public static final int SetVariable              = 0x1D;
    public static final int SetTargetExpression      = 0x20;
    public static final int StringConcat             = 0x21;
    public static final int GetProperty              = 0x22;
    public static final int SetProperty              = 0x23;
    public static final int DuplicateClip            = 0x24;
    public static final int RemoveClip               = 0x25;
    public static final int Trace                    = 0x26;
    public static final int StartDragMovie           = 0x27;
    public static final int StopDragMovie            = 0x28;
    public static final int StringLessThan           = 0x29;
    public static final int Random                   = 0x30;
    public static final int MBLength                 = 0x31;
    public static final int Ord                      = 0x32;
    public static final int Chr                      = 0x33;
    public static final int GetTimer                 = 0x34;
    public static final int MBSubString              = 0x35;
    public static final int MBOrd                    = 0x36;
    public static final int MBChr                    = 0x37;
    public static final int GotoFrame                = 0x81; // frame num (WORD)
    public static final int GetURL                   = 0x83; // url (STR), window (STR)
    public static final int WaitForFrame             = 0x8A; // frame needed (WORD), actions to skip (BYTE)
    public static final int SetTarget                = 0x8B; // name (STR)
    public static final int GotoLabel                = 0x8C; // name (STR)
    public static final int WaitForFrameExpression   = 0x8D; // frame needed on stack, actions to skip (BYTE)
    public static final int PushData                 = 0x96;
    public static final int Jump                     = 0x99;
    public static final int GetURL2                  = 0x9A;
    public static final int JumpIfTrue               = 0x9D;
    public static final int CallFrame                = 0x9E;
    public static final int GotoExpression           = 0x9F;

    // Flash 5 actions
    public static final int Delete                   = 0x3A;
    public static final int Delete2                  = 0x3B;
    public static final int DefineLocal              = 0x3C;
    public static final int CallFunction             = 0x3D;
    public static final int Return                   = 0x3E;
    public static final int Modulo                   = 0x3F;
    public static final int NewObject                = 0x40;
    public static final int DefineLocal2             = 0x41;
    public static final int InitArray                = 0x42;
    public static final int InitObject               = 0x43;
    public static final int TypeOf                   = 0x44;
    public static final int TargetPath               = 0x45;
    public static final int Enumerate                = 0x46;
    public static final int Add2                     = 0x47;
    public static final int Less2                    = 0x48;
    public static final int Equals2                  = 0x49;
    public static final int ObjectToNumber           = 0x4A;
    public static final int ObjectToString           = 0x4B;
    public static final int PushDuplicate            = 0x4C;
    public static final int StackSwap                = 0x4D;
    public static final int GetMember                = 0x4E;
    public static final int SetMember                = 0x4F;
    public static final int Increment                = 0x50;
    public static final int Decrement                = 0x51;
    public static final int CallMethod               = 0x52;
    public static final int NewMethod                = 0x53;
    public static final int BitAnd                   = 0x60;
    public static final int BitOr                    = 0x61;
    public static final int BitXor                   = 0x62;
    public static final int BitLShift                = 0x63;
    public static final int BitRShift                = 0x64;
    public static final int BitURShift               = 0x65;
    public static final int StoreRegister            = 0x87; // register number (byte)
    public static final int ConstantPool             = 0x88; // num (WORD), pool (STR*num)
    public static final int With                     = 0x94; // withBlock (STR)
    public static final int DefineFunction           = 0x9B; // name (STR), parmsNum (WORD), parms (STR*parmsNum), codeSize (WORD), code (UI8*codeSize)

    public static final String[] actionsNames = {
        "None",
        "NextFrame",
        "PrevFrame",
        "Play",
        "Stop",
        "ToggleQuality",
        "StopSounds",
        "Add",
        "Subtract",
        "Multiply",
        "Divide",
        "Equal",
        "LessThan",
        "LogicalAnd",
        "LogicalOr",
        "LogicalNot",
        "StringEqual",
        "StringLength",
        "SubString",
        "Pop",
        "Int",
        "Eval",
        "SetVariable",
        "SetTargetExpression",
        "StringConcat",
        "GetProperty",
        "SetProperty",
        "DuplicateClip",
        "RemoveClip",
        "Trace",
        "StartDragMovie",
        "StopDragMovie",
        "StringLessThan",
        "Random",
        "MBLength",
        "Ord",
        "Chr",
        "GetTimer",
        "MBSubString",
        "MBOrd",
        "MBChr",
        "GotoFrame",
        "GetURL",
        "WaitForFrame",
        "SetTarget",
        "GotoLabel",
        "WaitForFrameExpression",
        "PushData",
        "Jump",
        "GetURL2",
        "JumpIfTrue",
        "CallFrame",
        "GotoExpression",
        "Delete",
        "Delete2",
        "DefineLocal",
        "CallFunction",
        "Return",
        "Modulo",
        "NewObject",
        "DefineLocal2",
        "InitArray",
        "InitObject",
        "TypeOf",
        "TargetPath",
        "Enumerate",
        "Add2",
        "Less2",
        "Equals2",
        "ObjectToNumber",
        "ObjectToString",
        "PushDuplicate",
        "StackSwap",
        "GetMember",
        "SetMember",
        "Increment",
        "Decrement",
        "CallMethod",
        "NewMethod",
        "BitAnd",
        "BitOr",
        "BitXor",
        "BitLShift",
        "BitRShift",
        "BitURShift",
        "StoreRegister",
        "ConstantPool",
        "With",
        "DefineFunction",
    };

    // names and indexes have to be in the same order
    public static final int[] actionsIndexes = {
        None,
        NextFrame,
        PrevFrame,
        Play,
        Stop,
        ToggleQuality,
        StopSounds,
        Add,
        Subtract,
        Multiply,
        Divide,
        Equal,
        LessThan,
        LogicalAnd,
        LogicalOr,
        LogicalNot,
        StringEqual,
        StringLength,
        SubString,
        Pop,
        Int,
        Eval,
        SetVariable,
        SetTargetExpression,
        StringConcat,
        GetProperty,
        SetProperty,
        DuplicateClip,
        RemoveClip,
        Trace,
        StartDragMovie,
        StopDragMovie,
        StringLessThan,
        Random,
        MBLength,
        Ord,
        Chr,
        GetTimer,
        MBSubString,
        MBOrd,
        MBChr,
        GotoFrame,
        GetURL,
        WaitForFrame,
        SetTarget,
        GotoLabel,
        WaitForFrameExpression,
        PushData,
        Jump,
        GetURL2,
        JumpIfTrue,
        CallFrame,
        GotoExpression,
        Delete,
        Delete2,
        DefineLocal,
        CallFunction,
        Return,
        Modulo,
        NewObject,
        DefineLocal2,
        InitArray,
        InitObject,
        TypeOf,
        TargetPath,
        Enumerate,
        Add2,
        Less2,
        Equals2,
        ObjectToNumber,
        ObjectToString,
        PushDuplicate,
        StackSwap,
        GetMember,
        SetMember,
        Increment,
        Decrement,
        CallMethod,
        NewMethod,
        BitAnd,
        BitOr,
        BitXor,
        BitLShift,
        BitRShift,
        BitURShift,
        StoreRegister,
        ConstantPool,
        With,
        DefineFunction,
    };

    public static String getActionName( int action ) {
        for( int i=0; i<actionsIndexes.length; i++ ) {
            if( action == actionsIndexes[i] ) return actionsNames[i];
        }
        return "<unknown>";
    }

}

