/* ****************************************************************************
 * ASTLiteral.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.sc.parser;

public class ASTLiteral extends SimpleNode {

    private Object mValue = null;

    public ASTLiteral(int id) {
        super(id);
    }

    public ASTLiteral(Parser p, int id) {
        super(p, id);
    }

    public static Node jjtCreate(int id) {
        return new ASTLiteral(id);
    }

    public static Node jjtCreate(Parser p, int id) {
        return new ASTLiteral(p, id);
    }

    // Added
    public ASTLiteral(Object value) {
        mValue = value;
    }

    public Object getValue() {
        return mValue;
    }

    static final int hexval(char c) {
        switch(c) {
        case '0' :
            return 0;
        case '1' :
            return 1;
        case '2' :
            return 2;
        case '3' :
            return 3;
        case '4' :
            return 4;
        case '5' :
            return 5;
        case '6' :
            return 6;
        case '7' :
            return 7;
        case '8' :
            return 8;
        case '9' :
            return 9;

        case 'a' :
        case 'A' :
            return 10;
        case 'b' :
        case 'B' :
            return 11;
        case 'c' :
        case 'C' :
            return 12;
        case 'd' :
        case 'D' :
            return 13;
        case 'e' :
        case 'E' :
            return 14;
        case 'f' :
        case 'F' :
            return 15;
        }

        throw new RuntimeException("Illegal hex or unicode constant");
        // Should never come here
    }
    
    static final int octval(char c) {
        switch(c) {
        case '0' :
            return 0;
        case '1' :
            return 1;
        case '2' :
            return 2;
        case '3' :
            return 3;
        case '4' :
            return 4;
        case '5' :
            return 5;
        case '6' :
            return 6;
        case '7' :
            return 7;
        case '8' :
            return 8;
        case '9' :
            return 9;

        case 'a' :
        case 'A' :
            return 10;
        case 'b' :
        case 'B' :
            return 11;
        case 'c' :
        case 'C' :
            return 12;
        case 'd' :
        case 'D' :
            return 13;
        case 'e' :
        case 'E' :
            return 14;
        case 'f' :
        case 'F' :
            return 15;
        }
        
        throw new RuntimeException("Illegal octal constant");
        // Should never come here
    }
  
    public void setStringValue(String image) {
        int l = image.length();
        StringBuffer sb = new StringBuffer(l);
        for (int i=0; i<l; i++) {
            char c = image.charAt(i);
            if ((c == '\\') && (i+1<l)){
                i++;
                c = image.charAt(i);
                if (c=='n') c='\n';
                else if (c=='b') c = '\b';
                else if (c=='f') c = '\f';
                else if (c=='r') c = '\r';
                else if (c=='t') c = '\t';
                else if (c =='x') {
                    c = (char)(hexval(image.charAt(i+1)) << 4 |
                               hexval(image.charAt(i+1)));
                    i +=2;
                } else if (c =='u') {
                    c = (char)(hexval(image.charAt(i+1)) << 12 |
                               hexval(image.charAt(i+2)) << 8 |
                               hexval(image.charAt(i+3)) << 4 |
                               hexval(image.charAt(i+4)));
                    i +=4;
                } else if (c >='0' && c <= '7') {
                    c = (char)(octval(image.charAt(i)));
                    if ((image.length()>i) && 
                        (image.charAt(i+1)>='0') && (image.charAt(i+1)<='7')) {
                        i++;
                        c = (char) ((c<<4) | octval(image.charAt(i)));
                    }
                }
            }
            sb.append(c);
        }
        mValue = sb.toString();
    }
    
    public void setDecimalValue(String image) {
        try {
            mValue = new Long(Long.parseLong(image));
        } catch (NumberFormatException e) {
            mValue = new Double(image);
        }
    }
    
    public void setOctalValue(String image) {
        try {
            String imageWithout0 = image.substring(1);          
            mValue = new Long(Long.parseLong(imageWithout0,8));
        } catch (NumberFormatException e) {
            mValue = new Double(image);
        }
    }
    
    public void setHexValue(String image) {
        try {
            String imageWithout0x = image.substring(2);
            mValue = new Long(Long.parseLong(imageWithout0x,16));
        } catch (NumberFormatException e) {
            mValue = new Double(image);
        }
    }
    
    public void setFloatingPointValue(String image) {
        mValue = new Double(image);
    }
    
    public void setBooleanValue(boolean value) {
        mValue = new Boolean(value);
    }
    
    public void setNullValue() {
        mValue = null;
    }
  
    public String toString() {
        if (mValue == null) {
            return "null";
        }
        return "Literal(" + mValue.toString() + ")";
    }

}
