/***
 * Actions.java
 */

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.sc;
import java.io.*;
import java.util.*;

/*
 * Single-word instructions are CAPS (to minimize the chance of
 * collisions with client names).  Multi-word instructions are
 * CamelCase.
 *
 * Names (modulo case) are intended to be the same as those used by
 * flasm.  This aids debugging, and will also insure that the inline
 * assembler assembles the same language as flasm, when this is added.
 * There are probably some names that don't yet match flasm, especially
 * outside the Flash 5 opcode set.
 */

/***
 * Information about ActionScript bytecodes
 * -1 for arity or returns means "don't know"
 */
public class Actions {
    protected static final int ACTIONS_SIZE = 256;
    /* name -> Action */
    public static final HashMap actions = new HashMap();
    protected static Action[] actionArray = new Action[ACTIONS_SIZE];
    
    /* Returns set of Action */
    public static Set items() {
        return actions.entrySet();
    }
    
    static final int opcodeIndex(byte opcode) {
        return (int) opcode - (int) java.lang.Byte.MIN_VALUE;
    }
    
    protected static Action find(byte opcode) {
        Action action = actionArray[opcodeIndex(opcode)];
        if (action == null) throw new RuntimeException("null action");
        return action;
    }
    
    public static class Action implements Serializable {
        public final String name;
        public final byte opcode;
        public final boolean args;
        public final int arity;
        public final int returns;
        
        public Action(String name, byte opcode) {
            this(name, opcode, -1, -1);
        }
        
        public Action(String name, byte opcode, int arity, int returns) {
            this(name, opcode, arity, returns, (opcode & 0x80) != 0);
        }
        
        public Action(String name, byte opcode, int arity, int returns, boolean args) {
            super();
            this.name = name;
            this.opcode = opcode;
            this.args = args;
            this.arity = arity;
            this.returns = returns;
            assert actions.get(name) == null;
            actions.put(name, this);
            int index = opcodeIndex(opcode);
            assert actionArray[index] == null;
            actionArray[index] = this;
        }
        
        public Object writeReplace() {
            return ActionReplacement.make(this.opcode);
        }
    }
    
    // This object is used to replace a serialized Action.
    protected static class ActionReplacement implements Externalizable {
        // class implementation
        protected static final ActionReplacement[] array = new ActionReplacement[ACTIONS_SIZE];
        
        // return an interned object for this opcode
        static ActionReplacement make(byte opcode) {
            ActionReplacement object = array[opcodeIndex(opcode)];
            if (object == null)
                object = new ActionReplacement(opcode);
            return object;
        }
        
        // instance implementation
        protected byte opcode;
        
        // The java serializer calls this.  It has to be public.
        public ActionReplacement() {
        }
        
        protected ActionReplacement(byte opcode) {
            assert array[opcodeIndex(opcode)] == null;
            this.opcode = opcode;
            array[opcodeIndex(opcode)] = this;
        }
        
        public void writeExternal(ObjectOutput out)
            throws IOException
        {
            out.write(this.opcode);
        }
        
        public void readExternal(ObjectInput in)
            throws IOException
        {
            this.opcode = (byte) in.read();
        }
        
        public Object readResolve() throws ObjectStreamException {
            return Actions.find(this.opcode);
        }
    }
    
    public static Action NONE                     = new Action("NONE", (byte)0x00, 0, 0);
    public static Action NextFrame                = new Action("NextFrame", (byte)0x04);
    public static Action PreviousFrame            = new Action("PreviousFrame", (byte)0x05);
    public static Action PLAY                     = new Action("PLAY", (byte)0x06);
    public static Action STOP                     = new Action("STOP", (byte)0x07);
    public static Action ToggleQuality            = new Action("ToggleQuality", (byte)0x08);
    public static Action StopSounds               = new Action("StopSounds", (byte)0x09);
    public static Action NumericAdd               = new Action("NumericAdd", (byte)0x0A, 2, 1);
    public static Action SUBTRACT                 = new Action("SUBTRACT", (byte)0x0B, 2, 1);
    public static Action MULTIPLY                 = new Action("MULTIPLY", (byte)0x0C, 2, 1);
    public static Action DIVIDE                   = new Action("DIVIDE", (byte)0x0D, 2, 1);
    public static Action OldEquals                = new Action("OldEquals", (byte)0x0E, 2, 1);
    public static Action OldLessThan              = new Action("OldLessThan", (byte)0x0F, 2, 1);
    public static Action LogicalAnd               = new Action("LogicalAnd", (byte)0x10, 2, 1);
    public static Action LogicalOr                = new Action("LogicalOr", (byte)0x11, 2, 1);
    public static Action NOT                      = new Action("NOT", (byte)0x12, 1, 1);
    public static Action StringEqual              = new Action("StringEqual", (byte)0x13, 2, 1);
    public static Action StringLength             = new Action("StringLength", (byte)0x14, 1, 1);
    public static Action SUBSTRING                = new Action("SUBSTRING", (byte)0x15);
    public static Action POP                      = new Action("POP", (byte)0x17, 1, 0);
    public static Action INT                      = new Action("INT", (byte)0x18, 1, 1);
    public static Action GetVariable              = new Action("GetVariable", (byte)0x1C, 1, 1);
    public static Action SetVariable              = new Action("SetVariable", (byte)0x1D, 2, 0);
    public static Action SetTargetExpression      = new Action("SetTargetExpression", (byte)0x20);
    public static Action StringConcat             = new Action("StringConcat", (byte)0x21, 2, 1);
    public static Action GetProperty              = new Action("GetProperty", (byte)0x22, 2, 1);
    public static Action SetProperty              = new Action("SetProperty", (byte)0x23, 3, 0);
    public static Action DuplicateMovieClip       = new Action("DuplicateMovieClip", (byte)0x24);
    public static Action RemoveClip               = new Action("RemoveClip", (byte)0x25);
    public static Action TRACE                    = new Action("TRACE", (byte)0x26, 1, 0);
    public static Action StartDragMovie           = new Action("StartDragMovie", (byte)0x27);
    public static Action StopDragMovie            = new Action("StopDragMovie", (byte)0x28);
    public static Action StringLessThan           = new Action("StringLessThan", (byte)0x29, 2, 1);
    public static Action RANDOM                   = new Action("RANDOM", (byte)0x30, 0, 1);
    public static Action MBLENGTH                 = new Action("MBLENGTH", (byte)0x31, 1, 1);
    public static Action ORD                      = new Action("ORD", (byte)0x32, 1, 1);
    public static Action CHR                      = new Action("CHR", (byte)0x33, 1, 1);
    public static Action GetTimer                 = new Action("GetTimer", (byte)0x34, 0, 1);
    public static Action MBSUBSTRING              = new Action("MBSUBSTRING", (byte)0x35);
    public static Action MBORD                    = new Action("MBORD", (byte)0x36, 1, 1);
    public static Action MBCHR                    = new Action("MBCHR", (byte)0x37, 1, 1);
    public static Action GotoFrame                = new Action("GotoFrame", (byte)0x81); 
    public static Action GetUrl                   = new Action("GetUrl", (byte)0x83); 
    public static Action WaitForFrame             = new Action("WaitForFrame", (byte)0x8A); 
    public static Action SetTarget                = new Action("SetTarget", (byte)0x8B); 
    public static Action GotoLabel                = new Action("GotoLabel", (byte)0x8C); 
    public static Action WaitForFrameExpression   = new Action("WaitForFrameExpression", (byte)0x8D); 
    public static Action PUSH                     = new Action("PUSH", (byte)0x96, 0, 1);
    public static Action BRANCH                   = new Action("BRANCH", (byte)0x99);
    public static Action GetURL2                  = new Action("GetURL2", (byte)0x9A, 2, 0, false);
    public static Action BranchIfTrue             = new Action("BranchIfTrue", (byte)0x9D);
    public static Action CallFrame                = new Action("CallFrame", (byte)0x9E);
    public static Action GotoExpression           = new Action("GotoExpression", (byte)0x9F);
    /*
     * Flash 5
     */
    public static Action DELETE                   = new Action("DELETE", (byte)0x3A, 2, 1); // That's right delete takes 2 args
    public static Action DELETE2                  = new Action("DELETE2", (byte)0x3B, 1, 1); // and delete2 takes 1 arg
    public static Action VarEquals                = new Action("VarEquals", (byte)0x3C, 2, 0);
    public static Action CallFunction             = new Action("CallFunction", (byte)0x3D); // <fn> <#args> ...
    public static Action RETURN                   = new Action("RETURN", (byte)0x3E);
    public static Action MODULO                   = new Action("MODULO", (byte)0x3F, 2, 1);
    public static Action NEW                      = new Action("NEW", (byte)0x40); // <fn> <#args> ...
    public static Action VAR                      = new Action("VAR", (byte)0x41, 1, 0);
    public static Action InitArray                = new Action("InitArray", (byte)0x42); // <#elts> ...
    public static Action InitObject               = new Action("InitObject", (byte)0x43); // <#pairs> ...
    public static Action TypeOf                   = new Action("TypeOf", (byte)0x44, 1, 1);
    public static Action TargetPath               = new Action("TargetPath", (byte)0x45);
    public static Action ENUMERATE                = new Action("ENUMERATE", (byte)0x46);
    public static Action ADD                      = new Action("ADD", (byte)0x47, 2, 1);
    public static Action LessThan                 = new Action("LessThan", (byte)0x48, 2, 1);
    public static Action EQUALS                   = new Action("EQUALS", (byte)0x49, 2, 1);
    public static Action ObjectToNumber           = new Action("ObjectToNumber", (byte)0x4A, 1, 1);
    public static Action ObjectToString           = new Action("ObjectToString", (byte)0x4B, 1, 1);
    public static Action DUP                      = new Action("DUP", (byte)0x4C, 1, 2);
    public static Action SWAP                     = new Action("SWAP", (byte)0x4D, 2, 2);
    public static Action GetMember                = new Action("GetMember", (byte)0x4E, 2, 1);
    public static Action SetMember                = new Action("SetMember", (byte)0x4F, 3, 0);
    public static Action Increment                = new Action("Increment", (byte)0x50, 1, 1);
    public static Action Decrement                = new Action("Decrement", (byte)0x51, 1, 1);
    public static Action CallMethod               = new Action("CallMethod", (byte)0x52); // <meth> <obj> <#args> ...
    public static Action NewMethod                = new Action("NewMethod", (byte)0x53); // <meth> <obj> <//args> ...
    public static Action BitwiseAnd               = new Action("BitwiseAnd", (byte)0x60, 2, 1);
    public static Action BitwiseOr                = new Action("BitwiseOr", (byte)0x61, 2, 1);
    public static Action BitwiseXor               = new Action("BitwiseXor", (byte)0x62, 2, 1);
    public static Action ShiftLeft                = new Action("ShiftLeft", (byte)0x63, 2, 1);
    public static Action ShiftRight               = new Action("ShiftRight", (byte)0x64, 2, 1);
    public static Action UShiftRight              = new Action("UShiftRight", (byte)0x65, 2, 1);
    public static Action SetRegister              = new Action("SetRegister", (byte)0x87, 1, 1); 
    public static Action CONSTANTS                = new Action("CONSTANTS", (byte)0x88); 
    public static Action WITH                     = new Action("WITH", (byte)0x94); 
    public static Action DefineFunction           = new Action("DefineFunction", (byte)0x9B); 
    
    /*
     * Flash 6
     */
    public static Action InstanceOf               = new Action("InstanceOf", (byte)0x54, 2, 1);
    public static Action EnumerateValue           = new Action("EnumerateValue", (byte)0x55);
    public static Action StrictEquals             = new Action("StrictEquals", (byte)0x66, 2, 1);
    public static Action GreaterThan              = new Action("GreaterThan", (byte)0x67, 2, 1);
    public static Action StringGreaterThan        = new Action("StringGreaterThan", (byte)0x68, 2, 1);
    public static Action StrictMode               = new Action("StrictMode", (byte)0x89, 0, 0);
    
    /*
     * Flash 7
     */
    public static Action CAST                     = new Action("CAST", (byte)0x2b);
    public static Action IMPLEMENTS               = new Action("IMPLEMENTS", (byte)0x2c);
    public static Action EXTENDS                  = new Action("EXTENDS", (byte)0x69);
    public static Action DefineFunction2          = new Action("DefineFunction2", (byte)0x8e);
    public static Action TRY                      = new Action("TRY", (byte)0x8f);
    public static Action THROW                    = new Action("THROW", (byte)0x2a);
}
