/* -*- mode: JDE; c-basic-offset: 2; -*- */

/***
 * Instructions.java
 *
 * Description: SWF Action Code instruction opcode constants
 */

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.sc;
import java.io.*;
import java.nio.*;
import java.util.*;
import org.openlaszlo.sc.Actions.Action;
import org.openlaszlo.sc.Values.*;

public class Instructions {
  private static String runtime;

  public static void setRuntime(String target) {
    runtime = target.intern();
  }

  public static String getRuntime() {
    return runtime;
  }
  // Reverse lookup, for disassembly and compiling assembler

  public static Map ActionNames;

  // Action -> String
  public static String actionName(Action op) {
    if (ActionNames == null) {
      HashMap names = new HashMap();
      // TODO [2004-03-01 ptw] Surely there is a better way to iterate
      // over the entries of a hash table?
      for (Iterator i = Actions.items().iterator(); i.hasNext(); ) {
        Action value = (Action)((Map.Entry)i.next()).getValue();
        String name = value.name;
        if (name.equals(name.toUpperCase())) {
          name = name.toLowerCase();
        } else {
          name = name.substring(0, 1).toLowerCase() + name.substring(1);
        }
        names.put(value, name);
      }
      ActionNames = names;
    }
    String name = (String)ActionNames.get(op);
    return name;
  }

  /*
   * Instructions
   *
   * The code generator creates instructions, which are passed to the
   * assembler for pretty-printing or bytecode generation.  Each
   * instruction knows how to print itself, and how to add its bytes to a
   * byte array.
   */

  public static Map NameInstruction = new HashMap();

  // TODO [2004-03-01 ptw] Surely there is a better way
  public static List items() {
    List l = new ArrayList();
    for (Iterator i = NameInstruction.entrySet().iterator(); i.hasNext(); ) {
        Instruction value = (Instruction)((Map.Entry)i.next()).getValue();
        l.add(value);
    }
    return l;
  }

  /***
   * Registers
   */
  public static class Register implements Serializable {
    public String name;
    public byte regno;

    public static Set AUTO_REG = new LinkedHashSet();
    static {
      AUTO_REG.add("this");
      AUTO_REG.add("arguments");
      AUTO_REG.add("super");
      AUTO_REG.add("_root");
      AUTO_REG.add("_parent");
      AUTO_REG.add("_global");
    }

    public Register(String name) {
      this.name = name;
      this.regno = (byte)-1;
    }

    static public Register make(String name) {
      if (AUTO_REG.contains(name)) {
        return new AutoRegister(name);
      } else {
        return new Register(name);
      }
    }

    public String toString() {
      return "r:" + regno + "='" + name + "'";
    }
  }

  // Hard-wired registers that are automatically filled in,
  // according to the regflags
  public static class AutoRegister extends Register {
    public short flag;
    public short notFlag;

    public static Map FLAGS = new HashMap();
    static {
      FLAGS.put("this", new Integer(0x1));
      FLAGS.put("arguments", new Integer(0x4));
      FLAGS.put("super", new Integer(0x10));
      FLAGS.put("_root", new Integer(0x40));
      FLAGS.put("_parent", new Integer(0x80));
      FLAGS.put("_global", new Integer(0x100));
    }

    public static Map NOT_FLAGS = new HashMap();
    static {
      NOT_FLAGS.put("this", new Integer(0x2));
      NOT_FLAGS.put("arguments", new Integer(0x8));
      NOT_FLAGS.put("super", new Integer(0x20));
      NOT_FLAGS.put("_root", new Integer(0x0));
      NOT_FLAGS.put("_parent", new Integer(0x0));
      NOT_FLAGS.put("_global", new Integer(0x0));
    }

    // TODO: [2004-03-29 ptw] These flags turn off the creation of
    // this, arguments, and super in the activation record if they are
    // not used in the function body.  Need to compute that from the
    // free references in the unlikely case that one of these is not
    // registered but still used (e.g., closed over).
    public static short DEFAULT_FLAGS = (short)(0x2 | 0x8 | 0x20);

    public AutoRegister(String name) {
      super(name);
      flag = ((Integer)FLAGS.get(name)).shortValue();
      notFlag = ((Integer)NOT_FLAGS.get(name)).shortValue();
    }
  }


  /***
   * The abstract superclass of instructions.  Subclasses are
   *  ConcreteInstruction and PseudoInstruction.
   */
  public abstract static class Instruction implements Serializable {
    protected static final boolean[] curriedInstructions = new boolean[256];
    
    // default is to return null, meaning no information
    public StackModel updateStackModel(StackModel model) {
      return null;
    }

    public static Instruction curry(Action op) {
      Instruction inst;
      if (op == Actions.PUSH) {
        inst = new PUSHInstruction(op);
      } else if (op == Actions.DefineFunction) {
        inst = new DefineFunctionInstruction(op);
      } else if (op == Actions.DefineFunction2) {
        inst = new DefineFunction2Instruction(op);
      } else if (TargetInstruction.OPCODES.contains(op)) {
        inst = new TargetInstruction(op);
      } else {
        inst = new ConcreteInstruction(op);
      }
      NameInstruction.put(actionName(op), inst);
      curriedInstructions[Actions.opcodeIndex(op.opcode)] = true;
      return inst;
    }

    public abstract void writeBytes(ByteBuffer bytes, Map constants);

    // Python interface(s)
    public boolean getHasTarget() {
      return this instanceof TargetInstruction;
    }

    public boolean getIsLabel() {
      return this instanceof LABELInstruction;
    }

    public boolean getIsPush() {
      return this instanceof PUSHInstruction;
    }

    public boolean getIsBranchIfFalse() {
      return this instanceof BranchIfFalseInstruction;
    }

    public Instruction __findattr__(String name) {
      if (NameInstruction.containsKey(name)) {
        return (Instruction)NameInstruction.get(name);
      }
      return null;
    }

    public abstract Instruction __call__();

    public abstract Instruction __call__(Object arg);

    public abstract Instruction __call__(Object[] args);
  }

  /***
   * Represents an instruction backed by bytecode.
   */
  public static class ConcreteInstruction extends Instruction {
    public Action op;
    public List args;

    protected ConcreteInstruction(Action op) {
      this(op, null);
    }

    protected ConcreteInstruction(Action op, List args) {
      super();
      this.op = op;
      // Copy the arglist so it can be munged (e.g., push merging)
      if (args != null) this.args = new ArrayList(args);
    }

    public Object readResolve() {
        if (curriedInstructions[Actions.opcodeIndex(op.opcode)] && this.args == null)
            return (Instruction) NameInstruction.get(actionName(op));
        return this;
    }
    
    public Instruction __call__() {
      assert (! this.op.args);
      return this;
    }

    public Instruction __call__(Object arg) {
      assert this.op.args;
      return new ConcreteInstruction(this.op, Collections.singletonList(arg));
    }

    public Instruction __call__(Object[] args) {
      assert this.op.args;
      return new ConcreteInstruction(this.op, Arrays.asList(args));
    }

    public String toString() {
      Action op = this.op;
      String s = actionName(op);
      if (this.args != null) {
        s += " " + this.argsString();
      }
      return s;
    }

    public StackModel updateStackModel(StackModel model) {
      Action op = this.op;
      int arity = op.arity;
      int returns = op.returns;
      // Compute the arity for vararg instructions from the stack model
      // TODO: [2002-11-24 ptw] separate classes rather than re-dispatching
      if (op == Actions.CallFunction ||
          op == Actions.NEW) {
          if (model.size() >= 2 && Values.isInteger(model.get(-2))) {
            arity = ((Number)model.get(-2)).intValue() + 2;
            returns = 1;
          } else {
            arity = -1;
            returns = -1;
          }
      } else if (op == Actions.CallMethod ||
                 op == Actions.NewMethod) {
          if (model.size() >= 3 && Values.isInteger(model.get(-3))) {
            arity = ((Number)model.get(-3)).intValue() + 3;
            returns = 1;
          } else {
            arity = -1;
            returns = -1;
          }
      } else if (op == Actions.InitArray) {
          if (model.size() >=1 && Values.isInteger(model.get(-1))) {
            arity = ((Number)model.get(-1)).intValue() + 1;
            returns = 1;
          } else {
            arity = -1;
            returns = -1;
          }
      } else if (op == Actions.InitObject) {
          if (model.size() >=1 && Values.isInteger(model.get(-1))) {
            arity = ((Number)model.get(-1)).intValue() * 2 + 1;
            returns = 1;
          } else {
            arity = -1;
            returns = -1;
          }
      }
      // System.out.println(this.toString() + " arity: " + arity + " returns: " + returns);
      // Adjust the stack model according to the instruction arity
      if (arity != -1 && returns != -1) {
        // TODO [2002-12-24 ptw] For now, we just invalidate any
        // information in the model. We could interpret some
        // instructions to update the model, but it is only used
        // for calls right now
        if (model.size() >= arity) {
          model.removeRange(( - arity), model.size());
          model.addAll(Collections.nCopies(returns, null));
          return model;
        } else {
          // Needed operands outside of this block
          return null;
        }
      } else {
        return null;
      }
    }

    public String argsString() {
      Action op = this.op;
      if (op == Actions.SetRegister) {
        return "r:" + this.args.get(0).toString();
      } else {
        StringBuffer s = new StringBuffer();
        for (Iterator i = this.args.iterator(); i.hasNext(); ) {
          Object next = i.next();
          s.append("'" + next.toString() + "'");
          if (i.hasNext()) s.append(" ");
        }
        return s.toString();
      }
    }

    /***
     * Appends the bytecode to bytes, resolving constant references
     * against constants.
     */
    public void writeBytes(ByteBuffer bytes, Map constants) {
      bytes.order(ByteOrder.LITTLE_ENDIAN);
      assert bytes.order() == ByteOrder.LITTLE_ENDIAN;
      bytes.put(this.op.opcode);
      // Flash has two types of instructions: single-byte instructions,
      // which have their low bit set, and multi-byte instructions, in which
      // the opcode byte is followed by a two-byte length word.
      if ((this.op.opcode & 0x80) != 0) {
        int offset = bytes.position();
        bytes.putShort((short)0);
        if (this.op.args) {
          this.writeArgs(bytes, constants);
          int size = bytes.position() - offset - 2;
          if (size > ((1<<16)-1)) {
              throw new CompilerException("offset out of range in " + this);
          }
          bytes.putShort(offset, (short)size);
        }
      }
    }

    public void writeArgs(ByteBuffer bytes, Map constants) {
      Action op = this.op;
      List args = this.args;
      if (op == Actions.CONSTANTS) {
        int n = args.size();
        if (n > ((1<<16)-1)) {
            throw new CompilerException("too many arguments in " + this);
        }
        bytes.putShort((short)n);
        try {
          for (Iterator i = args.iterator(); i.hasNext(); ) {
            String encoding = "Cp1252";
            if (runtime.equals("swf6") || runtime.equals("swf7")) {
              encoding = "UTF-8";
            }
            bytes.put(((String)i.next()).getBytes(encoding));
            bytes.put((byte)0);
          }
        } catch (UnsupportedEncodingException e) {
          assert false : "this can't happen";
        }
      } else if (op == Actions.SetRegister) {
        Integer regno = ((Integer)args.get(0));
        if (regno.intValue() != regno.byteValue()) {
          throw new CompilerException("invalid register number");
        }
        bytes.put(regno.byteValue());
      } else {
        throw new CompilerException("unimplemented: code generation for " + this.toString());
      }
    }

    // This is an upper bound estimate for most instructions
    public int argsBytes() {
      Action op = this.op;
      if (op == Actions.CONSTANTS) {
        int b = 2;
        for (Iterator i = this.args.iterator(); i.hasNext(); ) {
          b += ((String)i.next()).length() + 1;
        }
        return b;
      } else if (op == Actions.SetRegister) {
        return 1;
      } else {
        return 0;
      }
    }
  }


  /***
   * Target instructions have a target.  During code generation, this is
   * a label (an arbitrary object, currently a string).  During assembly,
   * this is replaced by an integer offset.
   */

  public static class TargetInstruction extends ConcreteInstruction {
    public short targetOffset;

    // These are all the actions that have this type.
    public static Set OPCODES = new HashSet();
    static {
      OPCODES.add(Actions.BRANCH);
      OPCODES.add(Actions.BranchIfTrue);
      OPCODES.add(Actions.DefineFunction);
      OPCODES.add(Actions.WITH);
      OPCODES.add(Actions.DefineFunction2);
    }

    protected TargetInstruction(Action op) {
      this(op, null);
    }

    protected TargetInstruction(Action op, List args) {
      this(op, args, (short)0);
    }

    protected TargetInstruction(Action op, List args, short targetOffset) {
      super(op, args);
      this.targetOffset = targetOffset;
    }

    public TargetInstruction makeTargetInstruction(List args) {
      return new TargetInstruction(this.op, args);
    }

    public Instruction __call__(Object arg) {
      return makeTargetInstruction(Collections.singletonList(arg));
    }

    public Instruction __call__(Object[] args) {
      return makeTargetInstruction(Arrays.asList(args));
    }

    public Object getTarget() {
      return this.args.get(0);
    }

    public TargetInstruction replaceTarget(Object target) {
      TargetInstruction replace = makeTargetInstruction(this.args);
      replace.args.set(0, target);
      return replace;
    }

    public void writeArgs(ByteBuffer bytes, Map pool) {
      bytes.putShort(this.targetOffset);
    }

    public int argsBytes() {
      return 2;
    }
  }


  public static class DefineFunctionInstruction extends TargetInstruction {

    protected DefineFunctionInstruction(Action op) {
      this(op, null);
    }

    protected DefineFunctionInstruction(Action op, List args) {
      this(op, args, (short)0);
    }

    protected DefineFunctionInstruction(Action op, List args, short targetOffset) {
      super(op, args);
      assert op == Actions.DefineFunction;
      this.targetOffset = targetOffset;
    }

    public TargetInstruction makeTargetInstruction(List args) {
      return new DefineFunctionInstruction(Actions.DefineFunction, args);
    }

    public void writeArgs(ByteBuffer bytes, Map pool) {
      List args = this.args;
      String fname = (String)args.get(1);
      if (fname == null) fname = "";
      List fnargs = args.subList(2, args.size());
      int nargs = fnargs.size();
      bytes.put(fname.getBytes());
      bytes.put((byte)0);
      if (nargs != (short)nargs) {
        throw new CompilerException("too many arguments");
      }
      bytes.putShort((short)nargs);
      for (Iterator i = fnargs.iterator(); i.hasNext(); ) {
        bytes.put(((String)i.next()).getBytes());
        bytes.put((byte)0);
      }
      bytes.putShort(this.targetOffset);
    }

    public int argsBytes() {
      List args = this.args;
      String fname = (String)args.get(1);
      if (fname == null) fname = "";
      args = args.subList(2, args.size());
      int b = fname.length() + 1 + 2; // Geez, why not just size!?!?
      for (Iterator i = args.iterator(); i.hasNext(); ) {
        b += ((String)i.next()).length() + 1;
      }
      b += 2;
      return b;
    }

    public String toString() {
      StringBuffer b = new StringBuffer();
      b.append("function ");
      Object name = this.args.get(1);
      if (name != null) {
        b.append(name.toString());
      }
      b.append("(");
      List args = this.args.subList(2, this.args.size());
      for (Iterator i = args.iterator(); i.hasNext(); ) {
        Object next = i.next();
        // --- better way to escape strings?
        if (next instanceof String) {
          b.append("'" + next.toString() + "'");
        } else {
          b.append(next.toString());
        }
        if (i.hasNext()) b.append(", ");
      }
      b.append(")");
      return b.toString();
    }
  }


  /*
   * Flash 7 DefineFunction2 Instruction
   *
   * Similar to function, but parameters can be assigned to
   * 'registers'
   * Output format: string name, short nargs, byte nregs, short
   * regflags, [byte regno, string arg]*, short ninstrs
   */
  public static class DefineFunction2Instruction extends DefineFunctionInstruction {

    protected DefineFunction2Instruction(Action op) {
      this(op, null);
    }

    protected DefineFunction2Instruction(Action op, List args) {
      this(op, args, (short)0);
    }

    protected DefineFunction2Instruction(Action op, List args, short targetOffset) {
      super(op, args);
      assert op == Actions.DefineFunction2;
      this.targetOffset = targetOffset;
    }

    public TargetInstruction makeTargetInstruction(List args) {
      return new DefineFunction2Instruction(Actions.DefineFunction2, args);
    }

    public void writeArgs(ByteBuffer bytes, Map pool) {
      List args = this.args;
      String fname = (String)args.get(1);
      if (fname == null) fname = "";
      byte nregs = ((Integer)args.get(2)).byteValue();
      List fnargs = args.subList(3, args.size());
      // string name
      bytes.put(fname.getBytes());
      bytes.put((byte)0);
      // short nargs (will be back-patched)
      short nargs = 0;
      int nargsPos = bytes.position();
      bytes.putShort(nargs);
      // byte nregs
      bytes.put(nregs);
      // short regflags (will be back-patched)
      short regflags = AutoRegister.DEFAULT_FLAGS;
      int regflagsPos = bytes.position();
      bytes.putShort((short)0);
      for (Iterator i = fnargs.iterator(); i.hasNext(); ) {
        Object arg = i.next();
        if (arg instanceof Register) {
          Register reg = (Register)arg;
          nregs++;
          if (reg instanceof AutoRegister) {
            AutoRegister areg = (AutoRegister)reg;
            regflags &= ~(areg.notFlag);
            regflags |= areg.flag;
          } else {
            nargs++;
            // byte regno
            bytes.put(reg.regno);
            // string arg
            bytes.put(reg.name.getBytes());
            bytes.put((byte)0);
          }
        } else if (arg instanceof String) {
          nargs++;
          // byte regno
          // 0 means not registered
          bytes.put((byte)0);
          // string arg
          bytes.put(((String)arg).getBytes());
          bytes.put((byte)0);
        } else {
          System.out.println("Unknown arg: " + arg);
        }
      }
      // Backpatch nregs and flags
      bytes.putShort(nargsPos, nargs);
      bytes.putShort(regflagsPos, regflags);
      // short ninstrs
      bytes.putShort(this.targetOffset);
    }

    public int argsBytes() {
      List args = this.args;
      String fname = (String)args.get(1);
      if (fname == null) fname = "";
      args = args.subList(2, args.size());
      int b = fname.length() + 1 + 2 + 1 + 2; // Geez, why not just size!?!?
      for (Iterator i = args.iterator(); i.hasNext(); ) {
        Object arg = i.next();
        if (arg instanceof Register) {
          if (arg instanceof AutoRegister) {
            ;
          } else {
            b += 1+ ((Register)arg).name.length() + 1;
          }
        } else {
          b += ((String)arg).length() + 1;
        }
      }
      b += 2;
      return b;
    }

    public String toString() {
      StringBuffer b = new StringBuffer();
      b.append("function2 ");
      Object name = this.args.get(1);
      if (name != null) {
        b.append(name.toString());
      }
      List args = this.args.subList(3, this.args.size());
      b.append("(");
      for (Iterator i = args.iterator(); i.hasNext(); ) {
        Object next = i.next();
        if (next instanceof AutoRegister) {
          continue;
        }
        if (next instanceof String) {
          // --- better way to escape strings?
          b.append("'" + next.toString() + "'");
        } else {
          b.append(next.toString());
        }
        if (i.hasNext()) b.append(", ");
      }
      b.append(") (");
      for (Iterator i = args.iterator(); i.hasNext(); ) {
        Object next = i.next();
        boolean firsttime = true;
        if (next instanceof AutoRegister) {
          if (firsttime) {
            firsttime = false;
          } else {
            b.append(", ");
          }
          b.append(next.toString());
        }
      }
      b.append(")");
      return b.toString();
    }
  }

  public static class PUSHInstruction extends ConcreteInstruction {
    private int cachedArgsBytes;

    protected PUSHInstruction(Action op) {
      this(op, null);
    }

    protected PUSHInstruction(Action op, List args) {
      super(Actions.PUSH, args);
      assert op == Actions.PUSH;
      this.cachedArgsBytes = -1;
    }

    public PUSHInstruction(List args) {
      this(Actions.PUSH, args);
    }

    // Python interfaces
    public Instruction __call__(Object arg) {
      return new PUSHInstruction(Collections.singletonList(arg));
    }

    public Instruction __call__(Object[] args) {
      return new PUSHInstruction(Arrays.asList(args));
    }

    public boolean isVolatile() {
      for (Iterator i = this.args.iterator(); i.hasNext(); ) {
        if (Values.isRegister(i.next())) {
          return true;
        }
      }
      return false;
    }

    public int argsBytes() {
      if (this.cachedArgsBytes == -1) {
        this.cachedArgsBytes = this.computeArgsBytes(this.args);
      }
      return this.cachedArgsBytes;
    }

    public StackModel updateStackModel(StackModel model) {
      model.notePush(this, this.argsModel());
      return model;
    }

    // Return the args in a format for the stack model.  Only
    // numbers and strings are modelled (for vararg instructions)
    private List argsModel() {
      List model = new ArrayList();
      for (Iterator i = this.args.iterator(); i.hasNext(); ) {
        Object v = i.next();
        if (v instanceof String ||
            v instanceof Number) {
          model.add(v);
        } else {
          model.add(null);
        }
      }
      return model;
    }

    public boolean merge(PUSHInstruction other, StackModel model) {
      // Limit of multi-byte instruction length
      if (this.argsBytes() + other.argsBytes() >= 65536) {
        return false;
      }
      // We know they are cached now
      this.cachedArgsBytes += other.cachedArgsBytes;
      // Merged args are inserted after the args that are still on
      // the stack
      int i = model.pushDepth(this);
      this.args.addAll(i, other.args);
      model.notePush(this, other.argsModel());
      return true;
    }

    // Can only make this optimization if the last value is still on
    // the stack
    public boolean dup(StackModel model) {
      int i = model.pushDepth(this);
      if (i > 0) {
        Object last = this.args.get(i - 1);
        if (model.size() >= 1 && model.get(-1) == last) {
          List lastargs = Collections.singletonList(last);
          int lastBytes = this.computeArgsBytes(lastargs);
          // Limit of multi-byte instruction length
          if (this.argsBytes() + lastBytes >= 65536){
            return false;
          }
          // Must have been cached above
          this.cachedArgsBytes += lastBytes;
          this.args.addAll(i, lastargs);
          model.notePush(this, lastargs);
          return true;
        } else {
//           System.out.println("dup.i: " + i);
//           System.out.println("dup.model: " + model);
          return false;
        }
      } else {
        return false;
      }
    }

    public void writeArgs(ByteBuffer bytes, Map constants) {
      for (Iterator i = this.args.iterator(); i.hasNext(); ) {
        Object o = i.next();

        // Numbers are written as integers or floats if there is no
        // loss of precision, otherwise as doubles
        // FIXME [2002-02-25 ptw] Use a language with dynamic dispatch
        if (o instanceof Number) {
          Number n = (Number)o;
          if (n.doubleValue() == n.intValue()) {
            bytes.put(PushTypes.Integer);
            bytes.putInt(n.intValue());
          } else if (n.doubleValue() == n.floatValue()) {
            bytes.put(PushTypes.Float);
            bytes.putFloat(n.floatValue());
          } else {
            bytes.put(PushTypes.Double);
            // SWF has the opposite word order
            long lb = Double.doubleToRawLongBits(n.doubleValue());
            bytes.putInt((int)(lb >> 32));
            bytes.putInt((int)lb);
          }
        } else if (o instanceof Value) {
          Value v = (Value)o;
          bytes.put(v.type);
          if (v instanceof ParameterizedValue) {
            bytes.put(((ParameterizedValue)v).value);
          }
        } else if (o instanceof String) {
          String s = (String)o;
          if (constants != null && constants.containsKey(s)) {
            int index = ((Integer)constants.get(s)).intValue();
            if (index < 1<<8) {
              bytes.put(PushTypes.CONSTANT_INDEX8);
              bytes.put((byte)index);
            } else {
              assert index < 1<<16;
              bytes.put(PushTypes.CONSTANT_INDEX16);
              bytes.putShort((short)index);
            }
          } else {
            bytes.put(PushTypes.String);
            bytes.put(s.getBytes());
            bytes.put((byte)0);
          }
        }
      }
    }

    // compute an upper bound for the size of the arguments.  cf., computePushArg
    public int computeArgsBytes(List args) {
      // 'exact' answer
      //       int b = 0;
      //       for (Iterator i = args.iterator(); i.hasNext(); ) {
      //         b += computePushArg(i.next()).size;
      //       }
      //       return b;
      // quick upper bound instead:
      int b = 0;
      for (Iterator i = args.iterator(); i.hasNext(); ) {
        Object o = i.next();

        if (o instanceof Number) {
          Number n = (Number)o;
          if (n.doubleValue() == n.intValue()) {
            b += 1 + 4;
          } else if (n.doubleValue() == n.floatValue()) {
            b += 1 + 4;
          } else {
            b += 1 + 8;
          }
        } else if (o instanceof Value) {
          Value v = (Value)o;
          b += 1;
          if (v instanceof ParameterizedValue) {
            b += 1;
          }
        } else if (o instanceof String) {
          String s = (String)o;
          b += 1 + s.length() + 1;
        }
      }
      return b;
    }

    // Python interface to PUSHInstruction constants
    public List getargs() {
      return this.args;
    }
  }

  // Pseudo-instructions used during assembly

  /***
   * Represents an instruction that isn't backed by bytecode.
   * PseudoInstructions are resolved by the assembler.
   */
  public static class PseudoInstruction extends Instruction {
    public Object name;

    public PseudoInstruction(Object name) {
      this.name = name;
    }

    public String toString() {
      return this.name.toString();
    }

    public void writeBytes(ByteBuffer bytes, Map constants) {
      ;
    }

    public int argsBytes() {
      return 0;
    }

    public Instruction __call__() {
      assert false;
      return null;
    }

    public Instruction __call__(Object arg) {
      assert false;
      return null;
    }

    public Instruction __call__(Object[] args) {
      assert false;
      return null;
    }
  }

  /***
   * A BranchIfFalse pseudo-instruction is used to simplify the code
   * generator.  It is replaced by (NOT, BranchIfTrue) when the code
   * generator emits to the assembler
   */
  public static class BranchIfFalseInstruction extends PseudoInstruction {

    protected BranchIfFalseInstruction(Object target) {
      super(target);
    }

    public Instruction __call__(Object target) {
      return new BranchIfFalseInstruction(target);
    }

    public Instruction __call__(int target) {
      return new BranchIfFalseInstruction(new Integer(target));
    }

    public String toString() {
      return "branchIfFalse " + this.name;
    }

    // Python interface
    public Object getTarget() {
      return this.name;
    }
  }

  /***
   * A LABEL pseudo-instruction of the form @code{LABEL(n)} is added to
   * the instruction sequence at the target for a label.  (Label targets
   * are used for conditional expressions, control structures, and the
   * ends of definitions and @code{with} statements.)
   */
  public static class LABELInstruction extends PseudoInstruction {

    protected LABELInstruction(Object name) {
      super(name);
    }

    public Instruction __call__(Object name) {
      return new LABELInstruction(name);
    }

    public Instruction __call__(int target) {
      return new LABELInstruction(new Integer(target));
    }

    public String toString() {
      return super.toString() + ":";
    }
  }

  public static class COMMENTInstruction extends PseudoInstruction {

    protected COMMENTInstruction(String comment) {
      super(comment);
    }

    public Instruction __call__(Object comment) {
      return new COMMENTInstruction((String)comment);
    }

    public String toString() {
      return ";; " + super.toString();
    }

    // Allow PUSHInstruction to optimize across this.
    public StackModel updateStackModel(StackModel model) {
      return model;
    }
  }

  // Print running statistics about the current assembly state
  public static class CHECKPOINTInstruction extends COMMENTInstruction {
    protected CHECKPOINTInstruction(String message) {
      super(message);
    }

    public Instruction __call__(Object message) {
      return new CHECKPOINTInstruction((String)message);
    }

    public void writeBytes(ByteBuffer bytes, Map constants) {
      System.out.println(super.toString() + "\t" + bytes.position());
    }
  }

  // Pass arbitrary bytes into the object
  public static class BLOBInstruction extends PseudoInstruction {
    byte[] blob;

    protected BLOBInstruction(String repr, byte[] blob) {
      super(repr);
      this.blob = blob;
    }

    public Instruction __call__(String repr, byte[] blob) {
      return new BLOBInstruction(repr, blob);
    }

    public void writeBytes(ByteBuffer bytes, Map constants) {
      bytes.put(this.blob);
    }

    public int argsBytes() {
      return (this.blob.length);
    }
  }

  // For each symbol in the actions package, create a symbol with the
  // same name in this package.  If the instruction takes arguments,
  // the symbol names a constructor, otherwise it names an instance.
  // (This matches standard algebraic datatype syntax in functional
  // languages, and it ends up being more convenient and easier to use
  // than it sounds: for example, POP and BRANCH('label') are both
  // instructions.)

  public static Instruction NONE                     = Instruction.curry(Actions.NONE);
  public static Instruction NextFrame                = Instruction.curry(Actions.NextFrame);
  public static Instruction PreviousFrame            = Instruction.curry(Actions.PreviousFrame);
  public static Instruction PLAY                     = Instruction.curry(Actions.PLAY);
  public static Instruction STOP                     = Instruction.curry(Actions.STOP);
  public static Instruction ToggleQuality            = Instruction.curry(Actions.ToggleQuality);
  public static Instruction StopSounds               = Instruction.curry(Actions.StopSounds);
  public static Instruction NumericAdd               = Instruction.curry(Actions.NumericAdd);
  public static Instruction SUBTRACT                 = Instruction.curry(Actions.SUBTRACT);
  public static Instruction MULTIPLY                 = Instruction.curry(Actions.MULTIPLY);
  public static Instruction DIVIDE                   = Instruction.curry(Actions.DIVIDE);
  public static Instruction OldEquals                = Instruction.curry(Actions.OldEquals);
  public static Instruction OldLessThan              = Instruction.curry(Actions.OldLessThan);
  public static Instruction LogicalAnd               = Instruction.curry(Actions.LogicalAnd);
  public static Instruction LogicalOr                = Instruction.curry(Actions.LogicalOr);
  public static Instruction NOT                      = Instruction.curry(Actions.NOT);
  public static Instruction StringEqual              = Instruction.curry(Actions.StringEqual);
  public static Instruction StringLength             = Instruction.curry(Actions.StringLength);
  public static Instruction SUBSTRING                = Instruction.curry(Actions.SUBSTRING);
  public static Instruction POP                      = Instruction.curry(Actions.POP);
  public static Instruction INT                      = Instruction.curry(Actions.INT);
  public static Instruction GetVariable              = Instruction.curry(Actions.GetVariable);
  public static Instruction SetVariable              = Instruction.curry(Actions.SetVariable);
  public static Instruction SetTargetExpression      = Instruction.curry(Actions.SetTargetExpression);
  public static Instruction StringConcat             = Instruction.curry(Actions.StringConcat);
  public static Instruction GetProperty              = Instruction.curry(Actions.GetProperty);
  public static Instruction SetProperty              = Instruction.curry(Actions.SetProperty);
  public static Instruction DuplicateMovieClip       = Instruction.curry(Actions.DuplicateMovieClip);
  public static Instruction RemoveClip               = Instruction.curry(Actions.RemoveClip);
  public static Instruction TRACE                    = Instruction.curry(Actions.TRACE);
  public static Instruction StartDragMovie           = Instruction.curry(Actions.StartDragMovie);
  public static Instruction StopDragMovie            = Instruction.curry(Actions.StopDragMovie);
  public static Instruction StringLessThan           = Instruction.curry(Actions.StringLessThan);
  public static Instruction RANDOM                   = Instruction.curry(Actions.RANDOM);
  public static Instruction MBLENGTH                 = Instruction.curry(Actions.MBLENGTH);
  public static Instruction ORD                      = Instruction.curry(Actions.ORD);
  public static Instruction CHR                      = Instruction.curry(Actions.CHR);
  public static Instruction GetTimer                 = Instruction.curry(Actions.GetTimer);
  public static Instruction MBSUBSTRING              = Instruction.curry(Actions.MBSUBSTRING);
  public static Instruction MBORD                    = Instruction.curry(Actions.MBORD);
  public static Instruction MBCHR                    = Instruction.curry(Actions.MBCHR);
  public static Instruction GotoFrame                = Instruction.curry(Actions.GotoFrame);
  public static Instruction GetUrl                   = Instruction.curry(Actions.GetUrl);
  public static Instruction WaitForFrame             = Instruction.curry(Actions.WaitForFrame);
  public static Instruction SetTarget                = Instruction.curry(Actions.SetTarget);
  public static Instruction GotoLabel                = Instruction.curry(Actions.GotoLabel);
  public static Instruction WaitForFrameExpression   = Instruction.curry(Actions.WaitForFrameExpression);
  public static Instruction PUSH                     = Instruction.curry(Actions.PUSH);
  public static Instruction BRANCH                   = Instruction.curry(Actions.BRANCH);
  public static Instruction GetURL2                  = Instruction.curry(Actions.GetURL2);
  public static Instruction BranchIfTrue             = Instruction.curry(Actions.BranchIfTrue);
  public static Instruction CallFrame                = Instruction.curry(Actions.CallFrame);
  public static Instruction GotoExpression           = Instruction.curry(Actions.GotoExpression);

  // Flash 5
  public static Instruction DELETE                   = Instruction.curry(Actions.DELETE);
  public static Instruction DELETE2                  = Instruction.curry(Actions.DELETE2);
  public static Instruction VarEquals                = Instruction.curry(Actions.VarEquals);
  public static Instruction CallFunction             = Instruction.curry(Actions.CallFunction);
  public static Instruction RETURN                   = Instruction.curry(Actions.RETURN);
  public static Instruction MODULO                   = Instruction.curry(Actions.MODULO);
  public static Instruction NEW                      = Instruction.curry(Actions.NEW);
  public static Instruction VAR                      = Instruction.curry(Actions.VAR);
  public static Instruction InitArray                = Instruction.curry(Actions.InitArray);
  public static Instruction InitObject               = Instruction.curry(Actions.InitObject);
  public static Instruction TypeOf                   = Instruction.curry(Actions.TypeOf);
  public static Instruction TargetPath               = Instruction.curry(Actions.TargetPath);
  public static Instruction ENUMERATE                = Instruction.curry(Actions.ENUMERATE);
  public static Instruction ADD                      = Instruction.curry(Actions.ADD);
  public static Instruction LessThan                 = Instruction.curry(Actions.LessThan);
  public static Instruction EQUALS                   = Instruction.curry(Actions.EQUALS);
  public static Instruction ObjectToNumber           = Instruction.curry(Actions.ObjectToNumber);
  public static Instruction ObjectToString           = Instruction.curry(Actions.ObjectToString);
  public static Instruction DUP                      = Instruction.curry(Actions.DUP);
  public static Instruction SWAP                     = Instruction.curry(Actions.SWAP);
  public static Instruction GetMember                = Instruction.curry(Actions.GetMember);
  public static Instruction SetMember                = Instruction.curry(Actions.SetMember);
  public static Instruction Increment                = Instruction.curry(Actions.Increment);
  public static Instruction Decrement                = Instruction.curry(Actions.Decrement);
  public static Instruction CallMethod               = Instruction.curry(Actions.CallMethod);
  public static Instruction NewMethod                = Instruction.curry(Actions.NewMethod);
  public static Instruction BitwiseAnd               = Instruction.curry(Actions.BitwiseAnd);
  public static Instruction BitwiseOr                = Instruction.curry(Actions.BitwiseOr);
  public static Instruction BitwiseXor               = Instruction.curry(Actions.BitwiseXor);
  public static Instruction ShiftLeft                = Instruction.curry(Actions.ShiftLeft);
  public static Instruction ShiftRight               = Instruction.curry(Actions.ShiftRight);
  public static Instruction UShiftRight              = Instruction.curry(Actions.UShiftRight);
  public static Instruction SetRegister              = Instruction.curry(Actions.SetRegister);
  public static Instruction CONSTANTS                = Instruction.curry(Actions.CONSTANTS);
  public static Instruction WITH                     = Instruction.curry(Actions.WITH);
  public static Instruction DefineFunction           = Instruction.curry(Actions.DefineFunction);

  // Flash 6
  public static Instruction InstanceOf               = Instruction.curry(Actions.InstanceOf);
  public static Instruction EnumerateValue           = Instruction.curry(Actions.EnumerateValue);
  public static Instruction StrictEquals             = Instruction.curry(Actions.StrictEquals);
  public static Instruction GreaterThan              = Instruction.curry(Actions.GreaterThan);
  public static Instruction StringGreaterThan        = Instruction.curry(Actions.StringGreaterThan);

  // Flash 7
  public static Instruction DefineFunction2          = Instruction.curry(Actions.DefineFunction2);

  // Psuedo-instructions
  public static Instruction BranchIfFalse            = new BranchIfFalseInstruction("");
  public static Instruction LABEL                    = new LABELInstruction("");
  public static Instruction COMMENT                  = new COMMENTInstruction("");
  public static Instruction CHECKPOINT               = new CHECKPOINTInstruction("");
  public static Instruction BLOB                     = new BLOBInstruction("", null);
  static {
    NameInstruction.put("BranchIfFalse", BranchIfFalse);
    NameInstruction.put("LABEL", LABEL);
    NameInstruction.put("COMMENT", COMMENT);
    NameInstruction.put("CHECKPOINT", CHECKPOINT);
    NameInstruction.put("BLOB", BLOB);
  }
}
