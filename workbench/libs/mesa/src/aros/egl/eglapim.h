#ifndef EGLAPIM_H
#define EGLAPIM_H
EGLint meglGetError ();
EGLDisplay meglGetDisplay (EGLNativeDisplayType display_id);
EGLBoolean meglInitialize (EGLDisplay dpy, EGLint * major, EGLint * minor);
EGLBoolean meglTerminate (EGLDisplay dpy);
const char * meglQueryString (EGLDisplay dpy, EGLint name);
EGLBoolean meglGetConfigs (EGLDisplay dpy, EGLConfig * configs, EGLint config_size, EGLint * num_config);
EGLBoolean meglChooseConfig (EGLDisplay dpy, const EGLint * attrib_list, EGLConfig * configs, EGLint config_size, EGLint * num_config);
EGLBoolean meglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint * value);
EGLSurface meglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint * attrib_list);
EGLSurface meglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config, const EGLint * attrib_list);
EGLSurface meglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint * attrib_list);
EGLBoolean meglDestroySurface (EGLDisplay dpy, EGLSurface surface);
EGLBoolean meglQuerySurface (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint * value);
EGLBoolean meglBindAPI (EGLenum api);
EGLenum meglQueryAPI ();
EGLBoolean meglWaitClient ();
EGLBoolean meglReleaseThread ();
EGLSurface meglCreatePbufferFromClientBuffer (EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint * attrib_list);
EGLBoolean meglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
EGLBoolean meglBindTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer);
EGLBoolean meglReleaseTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer);
EGLBoolean meglSwapInterval (EGLDisplay dpy, EGLint interval);
EGLContext meglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint * attrib_list);
EGLBoolean meglDestroyContext (EGLDisplay dpy, EGLContext ctx);
EGLBoolean meglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
EGLContext meglGetCurrentContext ();
EGLSurface meglGetCurrentSurface (EGLint readdraw);
EGLDisplay meglGetCurrentDisplay ();
EGLBoolean meglQueryContext (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint * value);
EGLBoolean meglWaitGL ();
EGLBoolean meglWaitNative (EGLint engine);
EGLBoolean meglSwapBuffers (EGLDisplay dpy, EGLSurface surface);
EGLBoolean meglCopyBuffers (EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target);
#endif
