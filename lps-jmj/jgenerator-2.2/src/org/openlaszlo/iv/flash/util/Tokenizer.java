/*
 * $Id: Tokenizer.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

/**
 * Simple tokenizer of generator text datasources.
 *
 * @author Dmitry Skavish
 */
public final class Tokenizer {

    private String line;
    private int cur;
    private int length;

    public Tokenizer( String line ) {
        this.line = line;
        cur = 0;
        length = line.length();
    }

    private boolean skipBlanks() {
        while( cur < length && line.charAt(cur) == ' ' ) cur++;
        return cur >= length;
    }

    public String getToken() {
        if( skipBlanks() ) return null;

        boolean quotes;
        int start;

        char ch = line.charAt(cur);
        if( ch == '\"' ) {
            quotes = true;
            start = ++cur;
            if( cur >= length ) return null;
            ch = line.charAt(cur);
        } else {
            quotes = false;
            start = cur;
        }
        int end;

    mainLoop:
        for(;;) {
            switch( ch ) {
                case '\\':
                    cur++;
                    if( cur >= length ) {
                        end = cur;
                        break mainLoop;
                    }
                    break;
                case '\"':
                    if( quotes ) {
                        end = cur++;
                        if( skipBlanks() ) break mainLoop;
                        if( line.charAt(cur) == ',' ) cur++;
                        break mainLoop;
                    }
                    break;
                case ',':
                    if( !quotes ) {
                        end = cur++;
                        break mainLoop;
                    }
                    break;
                default:
                    break;
            }
            cur++;
            if( cur >= length ) {
                end = cur;
                break;
            }
            ch = line.charAt(cur);
        }

        // trimming
        while( start < end && line.charAt(end-1) == ' ' ) end--;

        return line.substring(start, end);
    }

}
