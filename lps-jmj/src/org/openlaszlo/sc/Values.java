/***
 * Values.java
 *
 * Description: SWF Action Code instruction opcode constants
 */

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.sc;
import java.io.*;

public class Values {
  /***
   * These do double duty as identifiers, and as the typecodes
   * that are used by PUSH.
   */
  public static class PushTypes {
    public static final byte String = 0;
    public static final byte Float = 1;
    public static final byte Null = 2;
    public static final byte Undefined = 3;
    public static final byte Register = 4;
    public static final byte Boolean = 5;
    public static final byte Integer = 7;
    public static final byte Double = 6;
    public static final byte CONSTANT_INDEX8 = 8;
    public static final byte CONSTANT_INDEX16 = 9;
  }

  // TODO: [2004-03-24 ptw] Generalize to any number of registers
  public static Value r0 = Value.make("r:0", PushTypes.Register, (byte)0);
  public static Value r1 = Value.make("r:1", PushTypes.Register, (byte)1);
  public static Value r2 = Value.make("r:2", PushTypes.Register, (byte)2);
  public static Value r3 = Value.make("r:3", PushTypes.Register, (byte)3);

  public static Value Register(int n) {
    switch (n) {
      case 0:
        return r0;
      case 1:
        return r1;
      case 2:
        return r2;
      case 3:
        return r3;
      default:
        return Value.make("r:" + n, PushTypes.Register, (byte)n);
    }
  }

  public static boolean isRegister(Object v) {
    return v instanceof Value && ((Value)v).type == PushTypes.Register;
  }

  public static boolean isInteger(Object v) {
    return v instanceof Number && ((Number)v).intValue() == ((Number)v).doubleValue();
  }

  /***
   * Runtime Types
   */

  public static class Value implements Serializable {
    public String name;
    public byte type;

    public static Value make(String name, byte type) {
      return new Value(name, type);
    }

    public static Value make(String name, byte type, byte value) {
      return new ParameterizedValue(name, type, value);
    }

    private Value(String name, byte type) {
      this.name = name.intern();
      this.type = type;
    }

    public String toString() {
      return this.name;
    }

    public Object readResolve() throws ObjectStreamException {
        switch (this.type) {
        case PushTypes.Undefined:
            return Values.Undefined;
        case PushTypes.Null:
            return Values.Null;
        default:
            return this;
        }
    }
  }

  public static class ParameterizedValue extends Value {
    public byte value;

    private ParameterizedValue(String name, byte type, byte value) {
      super(name, type);
      this.value = value;
    }

    public Object readResolve() throws ObjectStreamException {
        switch (this.type) {
        case PushTypes.Boolean:
            return this.value == 0 ? Values.False : Values.True;
        case PushTypes.Register:
            return Values.Register(this.value);
        default:
            return super.readResolve();
        }
    }
  }

  public static Value True = Value.make("TRUE", PushTypes.Boolean, (byte)1);
  public static Value False = Value.make("FALSE", PushTypes.Boolean, (byte)0);
  public static Value Undefined = Value.make("UNDEF", PushTypes.Undefined);
  public static Value Null = Value.make("NULL", PushTypes.Null);

  // Integers are represented as instances of Long

  // Strings are represented as instances of String
}

