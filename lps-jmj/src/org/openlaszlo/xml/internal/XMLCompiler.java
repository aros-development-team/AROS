/* ****************************************************************************
 * XMLCompiler.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml.internal;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.Reader;
import java.io.Writer;
import java.io.StringBufferInputStream;

import java.util.List;
import java.util.Stack;


import org.jdom.Attribute;
import org.jdom.Comment;
import org.jdom.Document;
import org.jdom.Element;
import org.jdom.JDOMException;
import org.jdom.input.SAXBuilder;

/**
 * Takes XML in various forms, and serializes it to actionscript, preserving all
 * heirarchy 
 *
 * @author Oliver Steele
 * @author Max Carlson
 * @version 1.0
 */
public class XMLCompiler {
    
    private static Stack counterstack = new Stack();
    private static Stack basestack = new Stack();
    
    private static String bas = "";

    /**
     * Compile XML to actionscript
     * 
     * @param XML string to compile
     * @return Actionscript representation of XML
     */
    public static String compile(String x) {
        // Compile a new element from scratch
        StringBuffer base = new StringBuffer();
        StringBufferInputStream sis = new StringBufferInputStream(x);
        SAXBuilder sb = new SAXBuilder();
        try {
            Document doc = sb.build(sis);
            Element e = doc.getRootElement();
            return compile(e);
        } catch (Exception ignored) {}
        
        return null;
    }

    /**
     * Compile XML to actionscript
     * 
     * @param Element e JDOM element to compile
     * @return Actionscript representation of XML
     */
    public static String compile(Element e) {
        return compile(e, Schema.DEFAULT_SCHEMA);
    }

    /**
     * Compile XML to actionscript
     * 
     * @param Element e JDOM element to compile
     * @param Schema schema Laszlo Schema to follow (what is var, function)
     * @return Actionscript representation of XML
     */
    public static String compile(Element e, Schema schema) {
        // Compile a new element from scratch
        StringBuffer base = new StringBuffer();
        return compile(e, schema, base);
    }
    
    /**
     * Compile XML to actionscript
     * 
     * @param Element e JDOM element to compile
     * @param Schema schema Laszlo Schema to follow (what is var, function)
     * @param StringBuffer base Base of current element - used when called 
     *  recursively
     * @return Actionscript representation of XML
     */
    public static String compile(Element e, Schema schema,
                                       StringBuffer base) {
        // Default to an empty string for the variable name - use name node 
        // in XML
        String varname = "";
        return compile(e, schema, base, varname);
    }   
    
    /**
     * Compile XML to actionscript
     * 
     * @param Element e JDOM element to compile
     * @param Schema schema Laszlo Schema to follow (what is var, function)
     * @param StringBuffer base Base of current element - used when called 
     *         recursively
     * @param String varname Variable name to use instead of element name
     * @return Actionscript representation of XML
     */
    public static String compile(Element e, Schema schema,
                                       StringBuffer base, String varname) {
        StringBuffer out = new StringBuffer();
    	
    	int lastcount = -1;
        
        
        /*
        // Find shortest available subsring - not necessary because of flash's 
        // name pools
        String name = (String)shorter.get(nodename);
        if (name == null) {
            String chunk = "";
            for (int i = 1; i < nodename.length(); i++) {
                chunk = nodename.substring(0, i);
                if (! shorter.containsKey(nodename) && ! used.containsKey(chunk) ) {
                    shorter.put(nodename, chunk);
                    used.put(chunk, "shorter");
                    break;
                }
            }
            out.append(chunk + " = '" + nodename + "';\n");
            name = chunk;
        }
        */
        
        if (base.length() <= 0) {
            // If we're just starting out, initialize everything
            counterstack = new Stack();
            counterstack.push(new Integer(-1));
            lastcount = counterstack.size();

            basestack = new Stack();
            basestack.push(base);
            
            bas = "";
            out.append("function x(n, a, t) {\nvar o = {};\no.n = n;\no.a = a;\no.c = [];\nif (t.length) {o.t = t;}\nreturn o;\n}\n");
        }   

        //out.append(counterstack.size() + ", " + lastcount + " > ");

        // Count up one for each unique base name - ensures there is no 
        // namespace collision in Actionscript namespace for elements with same 
        // names
        Integer in = new Integer(-1);
        in = (Integer)counterstack.pop();
        int inum = in.intValue();
        inum++;
        in = new Integer(inum);
        
        counterstack.push(in);
        
		if (base.length() > 0) {
        	base.append("[");
		    // Append the new number to the current base
		    base.append(in);
	        base.append("]");
	 	} else {
	 		base.append("root");
	 	}

        String name = e.getName();
        // Create an object to hold sub-values
        out.append(base + "=x('" + name + "'");

        // Add each attribute as an element of the 'attrs' object
        List attributes = e.getAttributes();
        //if (attributes.size() > 0) {
        	out.append(",{");
	        for (int i = 0; i < attributes.size(); i++) {
	            Attribute a = (Attribute)attributes.get(i);
	            
	            String val = null;
	            Schema.Type type = schema.getAttributeType(e, a.getName());
	            if (type == schema.STRING_TYPE) {
	                val = "'" + escapeQuote( a.getValue() ) + "'";
	            } else {
	                // schema.UNKNOWN_TYPE
	                try {
	                    float v = a.getFloatValue();
	                    val = escapeQuote( a.getValue() );
	                } catch (Exception ex) {
	                    val = "'" + escapeQuote( a.getValue() ) + "'";
	                }
	            }
	            
	            out.append(a.getName() + ":" + val);
	
	            if (i < attributes.size() - 1) {
	                out.append(",");
	            }
	        }
	        out.append("}");
		//}
        

        // Add textual data to 't' node if available
        String text = e.getTextTrim();
        if ((text != null) && (text.length() > 0)) {
            out.append(",'" + escapeQuote( text ) + "');\n");
        } else {
            out.append(");\n");
        }

        // Add each child element to the children object
        List nestedElements = e.getChildren();
        
        
        if (nestedElements.size() > 0) {
    	  	counterstack.push(new Integer(-1));        	
       	
        	for (int i = 0; i < nestedElements.size(); i++) {
	            Element c = (Element)nestedElements.get(i);
            	if (i > 0) {
	                StringBuffer temp = new StringBuffer(bas);
	                // compile child recursively, add to children object
	                out.append( compile(c, schema, temp) );
	            } else {
	                // Set up initial children object to hold child elements
	                basestack.push(bas + "");
   		   	        //out.append("in: " + bas + "\n");

	                base.append(".c");
	                bas = base + "";
	                out.append( compile(c, schema, base) );
	            }
	        }
	        
			if (counterstack.size() != lastcount) {
    	    	counterstack.pop();
    	    	bas = (String)basestack.pop();
    	    	//out.append("out: " + base + " : " + basestack.size() + ", " + lastcount + "\n");
    	    	lastcount = counterstack.size();
    		}
	 	}

        return out.toString();
    }

/*      
    /**
     * A utility class to allow execution from the command line
     * 
     * @param args[] a series of 3 arguments - [XML input document],
     *  [variable name], [AS output document]
  public static void main(String[] args) {
        if (args.length != 3) {
            System.out.println("Usage: XMLCompiler " +
                "[XML input document] " + "[variable name] " + "[AS output document]");
            System.exit(0);
        }
    
        try {
            // Create and load properties
            System.out.println("Reading XML from " + args[0]);
            
            // Load XML into JDOM Document
            SAXBuilder builder = new SAXBuilder();
            FileReader r = new FileReader(args[0]);
            Document doc = builder.build( r );
            
            System.out.println("\n\n---- Converting to Actionscript ----");
            
            Schema schema = Schema.DEFAULT_SCHEMA;
            StringBuffer out = new StringBuffer();
            
            // recurse on children - should be only one root element...
            List nestedElements = doc.getRootElement().getChildren();
            for (int i = 0; i < nestedElements.size(); i++) {
                Element c = (Element)nestedElements.get(i);
                out.append( compile(c, schema, new StringBuffer(), args[1]) );
            }      


            System.out.println("\n\n---- Writing to file ----");            
            FileWriter w = new FileWriter(args[2]);
            w.write(out + "");
            w.flush();
            w.close();
                    
            System.out.println(out);

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
 */
    
    /**
     * Escapes strings for inclusion inside actionscript strings,
     * ex: ' to \'
     * 
     * @param string to escape
     * @return an escaped string
     */
    private static String escapeQuote(String s) {
        String retvalue = s;
        if ( s.indexOf ("'") != -1 || s.indexOf ("\"") != -1 || s.indexOf ("\\") != -1) {
        StringBuffer hold = new StringBuffer();
        char c;
        for ( int i = 0; i < s.length(); i++ ) {
            if ( (c=s.charAt(i)) == '\'' ) {
                hold.append ("\\'");
            } else if ((c=s.charAt(i)) == '\"') {
                hold.append ("\\\"");
            } else if ((c=s.charAt(i)) == '\\') {
                hold.append ("\\\\");
            } else {
                hold.append(c);
            }
        }
        retvalue = hold.toString();
        }
        return retvalue;
    }
}
