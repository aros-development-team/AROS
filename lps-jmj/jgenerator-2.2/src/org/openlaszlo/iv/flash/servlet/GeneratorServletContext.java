/*
 * $Id: GeneratorServletContext.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

package org.openlaszlo.iv.flash.servlet;

import java.io.*;
import java.net.*;
import java.util.*;

import javax.servlet.*;
import javax.servlet.http.*;

/**
 * Generator servlet context<P>
 *
 * When servlet is invoked it stores its environment along
 * with HttpRequest and HttpResponse in this context using
 * current thread as a key, so that later on it can be retrieved
 * from for example Java datasources.<P>
 *
 * Example:<P>
 * <PRE><CODE>
 *     GeneratorServletContext context = GeneratorServletContext.getContext();
 *     HttpServletResponse response = context.getHttpServletResponse();
 *     HttpServletRequest request = context.getHttpServletRequest();
 *     HttpSession session = request.getSession();
 * </CODE></PRE>
 *
 * @author Dmitry Skavish
 */
public class GeneratorServletContext {

    private static Hashtable contexts = new Hashtable();

    private HttpServletRequest request;
    private HttpServletResponse response;
    private ServletConfig config;

    public static GeneratorServletContext getContext() {
        return (GeneratorServletContext) contexts.get( Thread.currentThread() );
    }

    public static GeneratorServletContext createContext() {
        GeneratorServletContext context = new GeneratorServletContext();
        contexts.put( Thread.currentThread(), context );
        return context;
    }

    public static void destroyContext() {
        contexts.remove( Thread.currentThread() );
    }


    public HttpServletRequest getHttpServletRequest() {
        return request;
    }

    public HttpServletResponse getHttpServletResponse() {
        return response;
    }

    public ServletConfig getServletConfig() {
        return config;
    }

    public void setHttpServletRequest( HttpServletRequest request ) {
        this.request = request;
    }

    public void setHttpServletResponse( HttpServletResponse response ) {
        this.response = response;
    }

    public void setServletConfig( ServletConfig config ) {
        this.config = config;
    }
}


