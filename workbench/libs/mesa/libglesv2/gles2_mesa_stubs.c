/*
 * gles2_mesa_stubs.c — GLES2/3 forwarding stubs for AROS
 *
 * Replaces the ANGLE-based libGLESv2.a with stubs that forward all
 * GL ES 2.0/3.x calls through glAGetProcAddress() → mesa3dgl.library's
 * dispatch table.  This ensures that when code #includes <GLES2/gl2.h>
 * and calls glCreateShader() etc. directly (link-time resolution), the
 * calls end up in Mesa's softpipe — not ANGLE's dead stubs.
 *
 * Each function resolves its real implementation on first call via
 * glAGetProcAddress(), then caches the pointer for subsequent calls.
 *
 * Build:
 *   x86_64-aros-gcc -mcmodel=large -mno-red-zone -c gles2_mesa_stubs.c \
 *     -I$SDK/include -o gles2_mesa_stubs.o
 *   x86_64-aros-ar rcs libGLESv2.a gles2_mesa_stubs.o
 */

#include <GL/gl.h>
#include <GL/gla.h>
#include <proto/gl.h>
#include <proto/exec.h>

extern void kprintf(const char *, ...);

/*
 * Resolve a GL function pointer via glAGetProcAddress → mesa3dgl.library.
 * Cached in a static local — resolved once per function per process.
 */
static inline void *_gles2_resolve(const char *name) {
    void *fn = (void *)glAGetProcAddress((const GLubyte *)name);
    if (!fn)
        kprintf("[GLESv2 stub] FATAL: %s not found in mesa3dgl!\n", name);
    return fn;
}

/* ── GL ES 2.0 core functions ────────────────────────────────────────── */

/* Function pointer typedefs — avoids comma-in-macro issues */
typedef void     (*pfn_v_e)(GLenum);
typedef void     (*pfn_v_uu)(GLuint, GLuint);
typedef void     (*pfn_v_uuc)(GLuint, GLuint, const GLchar *);
typedef void     (*pfn_v_eu)(GLenum, GLuint);
typedef void     (*pfn_v_ffff)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void     (*pfn_v_ee)(GLenum, GLenum);
typedef void     (*pfn_v_eeee)(GLenum, GLenum, GLenum, GLenum);
typedef void     (*pfn_v_elpve)(GLenum, GLsizeiptr, const void *, GLenum);
typedef void     (*pfn_v_elpv)(GLenum, GLintptr, GLsizeiptr, const void *);
typedef GLenum   (*pfn_e_e)(GLenum);
typedef void     (*pfn_v_b)(GLbitfield);
typedef void     (*pfn_v_f)(GLfloat);
typedef void     (*pfn_v_i)(GLint);
typedef void     (*pfn_v_bbbb)(GLboolean, GLboolean, GLboolean, GLboolean);
typedef void     (*pfn_v_u)(GLuint);
typedef void     (*pfn_v_eieiiive)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void *);
typedef void     (*pfn_v_eiiiiiieve)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void *);
typedef void     (*pfn_v_eieiiiii)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
typedef void     (*pfn_v_eiiiiiis)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
typedef GLuint   (*pfn_u_v)(void);
typedef GLuint   (*pfn_u_e)(GLenum);
typedef void     (*pfn_v_sp)(GLsizei, const GLuint *);
typedef void     (*pfn_v_spu)(GLsizei, GLuint *);
typedef void     (*pfn_v_eii)(GLenum, GLint, GLsizei);
typedef void     (*pfn_v_eiev)(GLenum, GLsizei, GLenum, const void *);
typedef void     (*pfn_v_ff)(GLfloat, GLfloat);
typedef void     (*pfn_v_v)(void);
typedef void     (*pfn_v_eeeu)(GLenum, GLenum, GLenum, GLuint);
typedef void     (*pfn_v_eeeui)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef void     (*pfn_v_uusspspep)(GLuint, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLchar *);
typedef void     (*pfn_v_ussu)(GLuint, GLsizei, GLsizei *, GLuint *);
typedef GLint    (*pfn_i_uc)(GLuint, const GLchar *);
typedef void     (*pfn_v_ep)(GLenum, GLboolean *);
typedef void     (*pfn_v_eep)(GLenum, GLenum, GLint *);
typedef GLenum   (*pfn_e_v)(void);
typedef void     (*pfn_v_efp)(GLenum, GLfloat *);
typedef void     (*pfn_v_eeep)(GLenum, GLenum, GLenum, GLint *);
typedef void     (*pfn_v_eip)(GLenum, GLint *);
typedef void     (*pfn_v_uep)(GLuint, GLenum, GLint *);
typedef void     (*pfn_v_ussp)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void     (*pfn_v_eepp)(GLenum, GLenum, GLint *, GLint *);
typedef const GLubyte *(*pfn_cp_e)(GLenum);
typedef void     (*pfn_v_uifp)(GLuint, GLint, GLfloat *);
typedef void     (*pfn_v_uiip)(GLuint, GLint, GLint *);
typedef void     (*pfn_v_uevp)(GLuint, GLenum, void **);
typedef GLboolean (*pfn_b_u)(GLuint);
typedef GLboolean (*pfn_b_e)(GLenum);
typedef void     (*pfn_v_eei)(GLenum, GLenum, GLint);
typedef void     (*pfn_v_sevev)(GLsizei, const GLuint *, GLenum, const void *, GLsizei);
typedef void     (*pfn_v_usccsip)(GLuint, GLsizei, const GLchar *const *, const GLint *);
typedef void     (*pfn_v_eiu)(GLenum, GLint, GLuint);
typedef void     (*pfn_v_eee)(GLenum, GLenum, GLenum);
typedef void     (*pfn_v_eiieiiieev)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
typedef void     (*pfn_v_eef)(GLenum, GLenum, GLfloat);
typedef void     (*pfn_v_eefp)(GLenum, GLenum, const GLfloat *);
typedef void     (*pfn_v_eeip)(GLenum, GLenum, const GLint *);
typedef void     (*pfn_v_eeiu)(GLenum, GLenum, GLint, GLuint);
typedef void     (*pfn_v_uefp)(GLuint, GLenum, GLfloat *);
typedef void     (*pfn_v_ei)(GLenum, GLint);
typedef void     (*pfn_v_eiiiiiieev)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *);
typedef void     (*pfn_v_if)(GLint, GLfloat);
typedef void     (*pfn_v_isfp)(GLint, GLsizei, const GLfloat *);
typedef void     (*pfn_v_ii)(GLint, GLint);
typedef void     (*pfn_v_isip)(GLint, GLsizei, const GLint *);
typedef void     (*pfn_v_iff)(GLint, GLfloat, GLfloat);
typedef void     (*pfn_v_iii)(GLint, GLint, GLint);
typedef void     (*pfn_v_ifff)(GLint, GLfloat, GLfloat, GLfloat);
typedef void     (*pfn_v_iiii)(GLint, GLint, GLint, GLint);
typedef void     (*pfn_v_iffff)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void     (*pfn_v_iiiii)(GLint, GLint, GLint, GLint, GLint);
typedef void     (*pfn_v_isbfp)(GLint, GLsizei, GLboolean, const GLfloat *);
typedef void     (*pfn_v_uf)(GLuint, GLfloat);
typedef void     (*pfn_v_ufp)(GLuint, const GLfloat *);
typedef void     (*pfn_v_uff)(GLuint, GLfloat, GLfloat);
typedef void     (*pfn_v_ufff)(GLuint, GLfloat, GLfloat, GLfloat);
typedef void     (*pfn_v_uffff)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void     (*pfn_v_uiebsv)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
typedef void     (*pfn_v_iiss)(GLint, GLint, GLsizei, GLsizei);
typedef void     (*pfn_v_iisseeev)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
typedef void     (*pfn_v_eess)(GLenum, GLenum, GLsizei, GLsizei);
typedef void     (*pfn_v_fb)(GLfloat, GLboolean);
typedef void     (*pfn_v_b2)(GLboolean);
typedef void     (*pfn_v_sep)(GLsizei, const GLenum *);
typedef const GLubyte *(*pfn_cp_eu)(GLenum, GLuint);
typedef void     (*pfn_v_esess)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
typedef void     (*pfn_v_iiiiiiiiibe)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
typedef void     (*pfn_v_eiiisssieev)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
typedef void     (*pfn_v_eiiiiissieev)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void *);
typedef void     (*pfn_v_ev)(GLenum, void *);

void glActiveTexture(GLenum texture) {
    static pfn_v_e fn; if (!fn) fn = (pfn_v_e)_gles2_resolve("glActiveTexture");
    if (fn) fn(texture);
}

void glAttachShader(GLuint program, GLuint shader) {
    static pfn_v_uu fn; if (!fn) fn = (pfn_v_uu)_gles2_resolve("glAttachShader");
    if (fn) fn(program, shader);
}

void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) {
    static pfn_v_uuc fn; if (!fn) fn = (pfn_v_uuc)_gles2_resolve("glBindAttribLocation");
    if (fn) fn(program, index, name);
}

void glBindBuffer(GLenum target, GLuint buffer) {
    static pfn_v_eu fn; if (!fn) fn = (pfn_v_eu)_gles2_resolve("glBindBuffer");
    if (fn) fn(target, buffer);
}

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    static pfn_v_eu fn; if (!fn) fn = (pfn_v_eu)_gles2_resolve("glBindFramebuffer");
    if (fn) fn(target, framebuffer);
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    static pfn_v_eu fn; if (!fn) fn = (pfn_v_eu)_gles2_resolve("glBindRenderbuffer");
    if (fn) fn(target, renderbuffer);
}

void glBindTexture(GLenum target, GLuint texture) {
    static pfn_v_eu fn; if (!fn) fn = (pfn_v_eu)_gles2_resolve("glBindTexture");
    if (fn) fn(target, texture);
}

void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    static pfn_v_ffff fn; if (!fn) fn = (pfn_v_ffff)_gles2_resolve("glBlendColor");
    if (fn) fn(red, green, blue, alpha);
}

void glBlendEquation(GLenum mode) {
    static pfn_v_e fn; if (!fn) fn = (pfn_v_e)_gles2_resolve("glBlendEquation");
    if (fn) fn(mode);
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    static pfn_v_ee fn; if (!fn) fn = (pfn_v_ee)_gles2_resolve("glBlendEquationSeparate");
    if (fn) fn(modeRGB, modeAlpha);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    static pfn_v_ee fn; if (!fn) fn = (pfn_v_ee)_gles2_resolve("glBlendFunc");
    if (fn) fn(sfactor, dfactor);
}

void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    static pfn_v_eeee fn; if (!fn) fn = (pfn_v_eeee)_gles2_resolve("glBlendFuncSeparate");
    if (fn) fn(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    static pfn_v_elpve fn; if (!fn) fn = (pfn_v_elpve)_gles2_resolve("glBufferData");
    if (fn) fn(target, size, data, usage);
}

void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    static pfn_v_elpv fn; if (!fn) fn = (pfn_v_elpv)_gles2_resolve("glBufferSubData");
    if (fn) fn(target, offset, size, data);
}

GLenum glCheckFramebufferStatus(GLenum target) {
    static pfn_e_e fn; if (!fn) fn = (pfn_e_e)_gles2_resolve("glCheckFramebufferStatus");
    return fn ? fn(target) : 0;
}

void glClear(GLbitfield mask) {
    static pfn_v_b fn; if (!fn) fn = (pfn_v_b)_gles2_resolve("glClear");
    if (fn) fn(mask);
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    static pfn_v_ffff fn; if (!fn) fn = (pfn_v_ffff)_gles2_resolve("glClearColor");
    if (fn) fn(red, green, blue, alpha);
}

void glClearDepthf(GLfloat d) {
    static pfn_v_f fn; if (!fn) fn = (pfn_v_f)_gles2_resolve("glClearDepthf");
    if (fn) fn(d);
}

void glClearStencil(GLint s) {
    static pfn_v_i fn; if (!fn) fn = (pfn_v_i)_gles2_resolve("glClearStencil");
    if (fn) fn(s);
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    static pfn_v_bbbb fn; if (!fn) fn = (pfn_v_bbbb)_gles2_resolve("glColorMask");
    if (fn) fn(red, green, blue, alpha);
}

void glCompileShader(GLuint shader) {
    static pfn_v_u fn; if (!fn) fn = (pfn_v_u)_gles2_resolve("glCompileShader");
    if (fn) fn(shader);
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
    GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
    static pfn_v_eieiiive fn; if (!fn) fn = (pfn_v_eieiiive)_gles2_resolve("glCompressedTexImage2D");
    if (fn) fn(target, level, internalformat, width, height, border, imageSize, data);
}

void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
    GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
    static pfn_v_eiiiiiieve fn; if (!fn) fn = (pfn_v_eiiiiiieve)_gles2_resolve("glCompressedTexSubImage2D");
    if (fn) fn(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat,
    GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    static pfn_v_eieiiiii fn; if (!fn) fn = (pfn_v_eieiiiii)_gles2_resolve("glCopyTexImage2D");
    if (fn) fn(target, level, internalformat, x, y, width, height, border);
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
    GLint x, GLint y, GLsizei width, GLsizei height) {
    static pfn_v_eiiiiiis fn; if (!fn) fn = (pfn_v_eiiiiiis)_gles2_resolve("glCopyTexSubImage2D");
    if (fn) fn(target, level, xoffset, yoffset, x, y, width, height);
}

GLuint glCreateProgram(void) {
    static pfn_u_v fn; if (!fn) fn = (pfn_u_v)_gles2_resolve("glCreateProgram");
    return fn ? fn() : 0;
}

GLuint glCreateShader(GLenum type) {
    static pfn_u_e fn; if (!fn) fn = (pfn_u_e)_gles2_resolve("glCreateShader");
    return fn ? fn(type) : 0;
}

void glCullFace(GLenum mode) {
    static pfn_v_e fn; if (!fn) fn = (pfn_v_e)_gles2_resolve("glCullFace");
    if (fn) fn(mode);
}

void glDeleteBuffers(GLsizei n, const GLuint *buffers) {
    static pfn_v_sp fn; if (!fn) fn = (pfn_v_sp)_gles2_resolve("glDeleteBuffers");
    if (fn) fn(n, buffers);
}

void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
    static pfn_v_sp fn; if (!fn) fn = (pfn_v_sp)_gles2_resolve("glDeleteFramebuffers");
    if (fn) fn(n, framebuffers);
}

void glDeleteProgram(GLuint program) {
    static pfn_v_u fn; if (!fn) fn = (pfn_v_u)_gles2_resolve("glDeleteProgram");
    if (fn) fn(program);
}

void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
    static pfn_v_sp fn; if (!fn) fn = (pfn_v_sp)_gles2_resolve("glDeleteRenderbuffers");
    if (fn) fn(n, renderbuffers);
}

void glDeleteShader(GLuint shader) {
    static pfn_v_u fn; if (!fn) fn = (pfn_v_u)_gles2_resolve("glDeleteShader");
    if (fn) fn(shader);
}

void glDeleteTextures(GLsizei n, const GLuint *textures) {
    static pfn_v_sp fn; if (!fn) fn = (pfn_v_sp)_gles2_resolve("glDeleteTextures");
    if (fn) fn(n, textures);
}

void glDepthFunc(GLenum func) {
    static pfn_v_e fn; if (!fn) fn = (pfn_v_e)_gles2_resolve("glDepthFunc");
    if (fn) fn(func);
}

void glDepthMask(GLboolean flag) {
    static pfn_v_b2 fn; if (!fn) fn = (pfn_v_b2)_gles2_resolve("glDepthMask");
    if (fn) fn(flag);
}

void glDepthRangef(GLfloat n, GLfloat f) {
    static pfn_v_ff fn; if (!fn) fn = (pfn_v_ff)_gles2_resolve("glDepthRangef");
    if (fn) fn(n, f);
}

void glDetachShader(GLuint program, GLuint shader) {
    static pfn_v_uu fn; if (!fn) fn = (pfn_v_uu)_gles2_resolve("glDetachShader");
    if (fn) fn(program, shader);
}

void glDisable(GLenum cap) {
    static pfn_v_e fn; if (!fn) fn = (pfn_v_e)_gles2_resolve("glDisable");
    if (fn) fn(cap);
}

void glDisableVertexAttribArray(GLuint index) {
    static pfn_v_u fn; if (!fn) fn = (pfn_v_u)_gles2_resolve("glDisableVertexAttribArray");
    if (fn) fn(index);
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    static pfn_v_eii fn; if (!fn) fn = (pfn_v_eii)_gles2_resolve("glDrawArrays");
    if (fn) fn(mode, first, count);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    static pfn_v_eiev fn; if (!fn) fn = (pfn_v_eiev)_gles2_resolve("glDrawElements");
    if (fn) fn(mode, count, type, indices);
}

void glEnable(GLenum cap) {
    static pfn_v_e fn; if (!fn) fn = (pfn_v_e)_gles2_resolve("glEnable");
    if (fn) fn(cap);
}

void glEnableVertexAttribArray(GLuint index) {
    static pfn_v_u fn; if (!fn) fn = (pfn_v_u)_gles2_resolve("glEnableVertexAttribArray");
    if (fn) fn(index);
}

void glFinish(void) {
    static pfn_v_v fn; if (!fn) fn = (pfn_v_v)_gles2_resolve("glFinish");
    if (fn) fn();
}

void glFlush(void) {
    static pfn_v_v fn; if (!fn) fn = (pfn_v_v)_gles2_resolve("glFlush");
    if (fn) fn();
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    static pfn_v_eeeu fn; if (!fn) fn = (pfn_v_eeeu)_gles2_resolve("glFramebufferRenderbuffer");
    if (fn) fn(target, attachment, renderbuffertarget, renderbuffer);
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    static pfn_v_eeeui fn; if (!fn) fn = (pfn_v_eeeui)_gles2_resolve("glFramebufferTexture2D");
    if (fn) fn(target, attachment, textarget, texture, level);
}

void glFrontFace(GLenum mode) {
    static pfn_v_e fn; if (!fn) fn = (pfn_v_e)_gles2_resolve("glFrontFace");
    if (fn) fn(mode);
}

void glGenBuffers(GLsizei n, GLuint *buffers) {
    static pfn_v_spu fn; if (!fn) fn = (pfn_v_spu)_gles2_resolve("glGenBuffers");
    if (fn) fn(n, buffers);
}

void glGenerateMipmap(GLenum target) {
    static pfn_v_e fn; if (!fn) fn = (pfn_v_e)_gles2_resolve("glGenerateMipmap");
    if (fn) fn(target);
}

void glGenFramebuffers(GLsizei n, GLuint *framebuffers) {
    static pfn_v_spu fn; if (!fn) fn = (pfn_v_spu)_gles2_resolve("glGenFramebuffers");
    if (fn) fn(n, framebuffers);
}

void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    static pfn_v_spu fn; if (!fn) fn = (pfn_v_spu)_gles2_resolve("glGenRenderbuffers");
    if (fn) fn(n, renderbuffers);
}

void glGenTextures(GLsizei n, GLuint *textures) {
    static pfn_v_spu fn; if (!fn) fn = (pfn_v_spu)_gles2_resolve("glGenTextures");
    if (fn) fn(n, textures);
}

void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize,
    GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    static pfn_v_uusspspep fn; if (!fn) fn = (pfn_v_uusspspep)_gles2_resolve("glGetActiveAttrib");
    if (fn) fn(program, index, bufSize, length, size, type, name);
}

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize,
    GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    static pfn_v_uusspspep fn; if (!fn) fn = (pfn_v_uusspspep)_gles2_resolve("glGetActiveUniform");
    if (fn) fn(program, index, bufSize, length, size, type, name);
}

void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) {
    static pfn_v_ussu fn; if (!fn) fn = (pfn_v_ussu)_gles2_resolve("glGetAttachedShaders");
    if (fn) fn(program, maxCount, count, shaders);
}

GLint glGetAttribLocation(GLuint program, const GLchar *name) {
    static pfn_i_uc fn; if (!fn) fn = (pfn_i_uc)_gles2_resolve("glGetAttribLocation");
    return fn ? fn(program, name) : -1;
}

void glGetBooleanv(GLenum pname, GLboolean *data) {
    static pfn_v_ep fn; if (!fn) fn = (pfn_v_ep)_gles2_resolve("glGetBooleanv");
    if (fn) fn(pname, data);
}

void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    static pfn_v_eep fn; if (!fn) fn = (pfn_v_eep)_gles2_resolve("glGetBufferParameteriv");
    if (fn) fn(target, pname, params);
}

GLenum glGetError(void) {
    static pfn_e_v fn; if (!fn) fn = (pfn_e_v)_gles2_resolve("glGetError");
    return fn ? fn() : 0x0500; /* GL_INVALID_ENUM */
}

void glGetFloatv(GLenum pname, GLfloat *data) {
    static pfn_v_efp fn; if (!fn) fn = (pfn_v_efp)_gles2_resolve("glGetFloatv");
    if (fn) fn(pname, data);
}

void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    static pfn_v_eeep fn; if (!fn) fn = (pfn_v_eeep)_gles2_resolve("glGetFramebufferAttachmentParameteriv");
    if (fn) fn(target, attachment, pname, params);
}

void glGetIntegerv(GLenum pname, GLint *data) {
    static pfn_v_eip fn; if (!fn) fn = (pfn_v_eip)_gles2_resolve("glGetIntegerv");
    if (fn) fn(pname, data);
}

void glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
    static pfn_v_uep fn; if (!fn) fn = (pfn_v_uep)_gles2_resolve("glGetProgramiv");
    if (fn) fn(program, pname, params);
}

void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    static pfn_v_ussp fn; if (!fn) fn = (pfn_v_ussp)_gles2_resolve("glGetProgramInfoLog");
    if (fn) fn(program, bufSize, length, infoLog);
}

void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    static pfn_v_eep fn; if (!fn) fn = (pfn_v_eep)_gles2_resolve("glGetRenderbufferParameteriv");
    if (fn) fn(target, pname, params);
}

void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
    static pfn_v_uep fn; if (!fn) fn = (pfn_v_uep)_gles2_resolve("glGetShaderiv");
    if (fn) fn(shader, pname, params);
}

void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    static pfn_v_ussp fn; if (!fn) fn = (pfn_v_ussp)_gles2_resolve("glGetShaderInfoLog");
    if (fn) fn(shader, bufSize, length, infoLog);
}

void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) {
    static pfn_v_eepp fn; if (!fn) fn = (pfn_v_eepp)_gles2_resolve("glGetShaderPrecisionFormat");
    if (fn) fn(shadertype, precisiontype, range, precision);
}

void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) {
    static pfn_v_ussp fn; if (!fn) fn = (pfn_v_ussp)_gles2_resolve("glGetShaderSource");
    if (fn) fn(shader, bufSize, length, source);
}

const GLubyte *glGetString(GLenum name) {
    static pfn_cp_e fn; if (!fn) fn = (pfn_cp_e)_gles2_resolve("glGetString");
    return fn ? fn(name) : (const GLubyte *)"";
}

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
    static pfn_v_eefp fn; if (!fn) fn = (pfn_v_eefp)_gles2_resolve("glGetTexParameterfv");
    if (fn) fn(target, pname, params);
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) {
    static pfn_v_eep fn; if (!fn) fn = (pfn_v_eep)_gles2_resolve("glGetTexParameteriv");
    if (fn) fn(target, pname, params);
}

void glGetUniformfv(GLuint program, GLint location, GLfloat *params) {
    static pfn_v_uifp fn; if (!fn) fn = (pfn_v_uifp)_gles2_resolve("glGetUniformfv");
    if (fn) fn(program, location, params);
}

void glGetUniformiv(GLuint program, GLint location, GLint *params) {
    static pfn_v_uiip fn; if (!fn) fn = (pfn_v_uiip)_gles2_resolve("glGetUniformiv");
    if (fn) fn(program, location, params);
}

GLint glGetUniformLocation(GLuint program, const GLchar *name) {
    static pfn_i_uc fn; if (!fn) fn = (pfn_i_uc)_gles2_resolve("glGetUniformLocation");
    return fn ? fn(program, name) : -1;
}

void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params) {
    static pfn_v_uefp fn; if (!fn) fn = (pfn_v_uefp)_gles2_resolve("glGetVertexAttribfv");
    if (fn) fn(index, pname, params);
}

void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params) {
    static pfn_v_uep fn; if (!fn) fn = (pfn_v_uep)_gles2_resolve("glGetVertexAttribiv");
    if (fn) fn(index, pname, params);
}

void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer) {
    static pfn_v_uevp fn; if (!fn) fn = (pfn_v_uevp)_gles2_resolve("glGetVertexAttribPointerv");
    if (fn) fn(index, pname, pointer);
}

void glHint(GLenum target, GLenum mode) {
    static pfn_v_ee fn; if (!fn) fn = (pfn_v_ee)_gles2_resolve("glHint");
    if (fn) fn(target, mode);
}

GLboolean glIsBuffer(GLuint buffer) {
    static pfn_b_u fn; if (!fn) fn = (pfn_b_u)_gles2_resolve("glIsBuffer");
    return fn ? fn(buffer) : GL_FALSE;
}

GLboolean glIsEnabled(GLenum cap) {
    static pfn_b_e fn; if (!fn) fn = (pfn_b_e)_gles2_resolve("glIsEnabled");
    return fn ? fn(cap) : GL_FALSE;
}

GLboolean glIsFramebuffer(GLuint framebuffer) {
    static pfn_b_u fn; if (!fn) fn = (pfn_b_u)_gles2_resolve("glIsFramebuffer");
    return fn ? fn(framebuffer) : GL_FALSE;
}

GLboolean glIsProgram(GLuint program) {
    static pfn_b_u fn; if (!fn) fn = (pfn_b_u)_gles2_resolve("glIsProgram");
    return fn ? fn(program) : GL_FALSE;
}

GLboolean glIsRenderbuffer(GLuint renderbuffer) {
    static pfn_b_u fn; if (!fn) fn = (pfn_b_u)_gles2_resolve("glIsRenderbuffer");
    return fn ? fn(renderbuffer) : GL_FALSE;
}

GLboolean glIsShader(GLuint shader) {
    static pfn_b_u fn; if (!fn) fn = (pfn_b_u)_gles2_resolve("glIsShader");
    return fn ? fn(shader) : GL_FALSE;
}

GLboolean glIsTexture(GLuint texture) {
    static pfn_b_u fn; if (!fn) fn = (pfn_b_u)_gles2_resolve("glIsTexture");
    return fn ? fn(texture) : GL_FALSE;
}

void glLineWidth(GLfloat width) {
    static pfn_v_f fn; if (!fn) fn = (pfn_v_f)_gles2_resolve("glLineWidth");
    if (fn) fn(width);
}

void glLinkProgram(GLuint program) {
    static pfn_v_u fn; if (!fn) fn = (pfn_v_u)_gles2_resolve("glLinkProgram");
    if (fn) fn(program);
}

void glPixelStorei(GLenum pname, GLint param) {
    static pfn_v_ei fn; if (!fn) fn = (pfn_v_ei)_gles2_resolve("glPixelStorei");
    if (fn) fn(pname, param);
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
    static pfn_v_ff fn; if (!fn) fn = (pfn_v_ff)_gles2_resolve("glPolygonOffset");
    if (fn) fn(factor, units);
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
    GLenum format, GLenum type, void *pixels) {
    static pfn_v_iisseeev fn; if (!fn) fn = (pfn_v_iisseeev)_gles2_resolve("glReadPixels");
    if (fn) fn(x, y, width, height, format, type, pixels);
}

void glReleaseShaderCompiler(void) {
    static pfn_v_v fn; if (!fn) fn = (pfn_v_v)_gles2_resolve("glReleaseShaderCompiler");
    if (fn) fn();
}

void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    static pfn_v_eess fn; if (!fn) fn = (pfn_v_eess)_gles2_resolve("glRenderbufferStorage");
    if (fn) fn(target, internalformat, width, height);
}

void glSampleCoverage(GLfloat value, GLboolean invert) {
    static pfn_v_fb fn; if (!fn) fn = (pfn_v_fb)_gles2_resolve("glSampleCoverage");
    if (fn) fn(value, invert);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    static pfn_v_iiss fn; if (!fn) fn = (pfn_v_iiss)_gles2_resolve("glScissor");
    if (fn) fn(x, y, width, height);
}

void glShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryformat,
    const void *binary, GLsizei length) {
    static pfn_v_sevev fn; if (!fn) fn = (pfn_v_sevev)_gles2_resolve("glShaderBinary");
    if (fn) fn(count, shaders, binaryformat, binary, length);
}

void glShaderSource(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length) {
    static pfn_v_usccsip fn; if (!fn) fn = (pfn_v_usccsip)_gles2_resolve("glShaderSource");
    if (fn) fn(shader, count, string, length);
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    static pfn_v_eiu fn; if (!fn) fn = (pfn_v_eiu)_gles2_resolve("glStencilFunc");
    if (fn) fn(func, ref, mask);
}

void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
    static pfn_v_eeiu fn; if (!fn) fn = (pfn_v_eeiu)_gles2_resolve("glStencilFuncSeparate");
    if (fn) fn(face, func, ref, mask);
}

void glStencilMask(GLuint mask) {
    static pfn_v_u fn; if (!fn) fn = (pfn_v_u)_gles2_resolve("glStencilMask");
    if (fn) fn(mask);
}

void glStencilMaskSeparate(GLenum face, GLuint mask) {
    static pfn_v_eu fn; if (!fn) fn = (pfn_v_eu)_gles2_resolve("glStencilMaskSeparate");
    if (fn) fn(face, mask);
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    static pfn_v_eee fn; if (!fn) fn = (pfn_v_eee)_gles2_resolve("glStencilOp");
    if (fn) fn(fail, zfail, zpass);
}

void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    static pfn_v_eeee fn; if (!fn) fn = (pfn_v_eeee)_gles2_resolve("glStencilOpSeparate");
    if (fn) fn(face, sfail, dpfail, dppass);
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat,
    GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
    static pfn_v_eiieiiieev fn; if (!fn) fn = (pfn_v_eiieiiieev)_gles2_resolve("glTexImage2D");
    if (fn) fn(target, level, internalformat, width, height, border, format, type, pixels);
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    static pfn_v_eef fn; if (!fn) fn = (pfn_v_eef)_gles2_resolve("glTexParameterf");
    if (fn) fn(target, pname, param);
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
    static pfn_v_eefp fn; if (!fn) fn = (pfn_v_eefp)_gles2_resolve("glTexParameterfv");
    if (fn) fn(target, pname, params);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    static pfn_v_eei fn; if (!fn) fn = (pfn_v_eei)_gles2_resolve("glTexParameteri");
    if (fn) fn(target, pname, param);
}

void glTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
    static pfn_v_eeip fn; if (!fn) fn = (pfn_v_eeip)_gles2_resolve("glTexParameteriv");
    if (fn) fn(target, pname, params);
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
    GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    static pfn_v_eiiiiiieev fn; if (!fn) fn = (pfn_v_eiiiiiieev)_gles2_resolve("glTexSubImage2D");
    if (fn) fn(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glUniform1f(GLint location, GLfloat v0) {
    static pfn_v_if fn; if (!fn) fn = (pfn_v_if)_gles2_resolve("glUniform1f");
    if (fn) fn(location, v0);
}

void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
    static pfn_v_isfp fn; if (!fn) fn = (pfn_v_isfp)_gles2_resolve("glUniform1fv");
    if (fn) fn(location, count, value);
}

void glUniform1i(GLint location, GLint v0) {
    static pfn_v_ii fn; if (!fn) fn = (pfn_v_ii)_gles2_resolve("glUniform1i");
    if (fn) fn(location, v0);
}

void glUniform1iv(GLint location, GLsizei count, const GLint *value) {
    static pfn_v_isip fn; if (!fn) fn = (pfn_v_isip)_gles2_resolve("glUniform1iv");
    if (fn) fn(location, count, value);
}

void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    static pfn_v_iff fn; if (!fn) fn = (pfn_v_iff)_gles2_resolve("glUniform2f");
    if (fn) fn(location, v0, v1);
}

void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
    static pfn_v_isfp fn; if (!fn) fn = (pfn_v_isfp)_gles2_resolve("glUniform2fv");
    if (fn) fn(location, count, value);
}

void glUniform2i(GLint location, GLint v0, GLint v1) {
    static pfn_v_iii fn; if (!fn) fn = (pfn_v_iii)_gles2_resolve("glUniform2i");
    if (fn) fn(location, v0, v1);
}

void glUniform2iv(GLint location, GLsizei count, const GLint *value) {
    static pfn_v_isip fn; if (!fn) fn = (pfn_v_isip)_gles2_resolve("glUniform2iv");
    if (fn) fn(location, count, value);
}

void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    static pfn_v_ifff fn; if (!fn) fn = (pfn_v_ifff)_gles2_resolve("glUniform3f");
    if (fn) fn(location, v0, v1, v2);
}

void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
    static pfn_v_isfp fn; if (!fn) fn = (pfn_v_isfp)_gles2_resolve("glUniform3fv");
    if (fn) fn(location, count, value);
}

void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    static pfn_v_iiii fn; if (!fn) fn = (pfn_v_iiii)_gles2_resolve("glUniform3i");
    if (fn) fn(location, v0, v1, v2);
}

void glUniform3iv(GLint location, GLsizei count, const GLint *value) {
    static pfn_v_isip fn; if (!fn) fn = (pfn_v_isip)_gles2_resolve("glUniform3iv");
    if (fn) fn(location, count, value);
}

void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    static pfn_v_iffff fn; if (!fn) fn = (pfn_v_iffff)_gles2_resolve("glUniform4f");
    if (fn) fn(location, v0, v1, v2, v3);
}

void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
    static pfn_v_isfp fn; if (!fn) fn = (pfn_v_isfp)_gles2_resolve("glUniform4fv");
    if (fn) fn(location, count, value);
}

void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    static pfn_v_iiiii fn; if (!fn) fn = (pfn_v_iiiii)_gles2_resolve("glUniform4i");
    if (fn) fn(location, v0, v1, v2, v3);
}

void glUniform4iv(GLint location, GLsizei count, const GLint *value) {
    static pfn_v_isip fn; if (!fn) fn = (pfn_v_isip)_gles2_resolve("glUniform4iv");
    if (fn) fn(location, count, value);
}

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    static pfn_v_isbfp fn; if (!fn) fn = (pfn_v_isbfp)_gles2_resolve("glUniformMatrix2fv");
    if (fn) fn(location, count, transpose, value);
}

void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    static pfn_v_isbfp fn; if (!fn) fn = (pfn_v_isbfp)_gles2_resolve("glUniformMatrix3fv");
    if (fn) fn(location, count, transpose, value);
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    static pfn_v_isbfp fn; if (!fn) fn = (pfn_v_isbfp)_gles2_resolve("glUniformMatrix4fv");
    if (fn) fn(location, count, transpose, value);
}

void glUseProgram(GLuint program) {
    static pfn_v_u fn; if (!fn) fn = (pfn_v_u)_gles2_resolve("glUseProgram");
    if (fn) fn(program);
}

void glValidateProgram(GLuint program) {
    static pfn_v_u fn; if (!fn) fn = (pfn_v_u)_gles2_resolve("glValidateProgram");
    if (fn) fn(program);
}

void glVertexAttrib1f(GLuint index, GLfloat x) {
    static pfn_v_uf fn; if (!fn) fn = (pfn_v_uf)_gles2_resolve("glVertexAttrib1f");
    if (fn) fn(index, x);
}

void glVertexAttrib1fv(GLuint index, const GLfloat *v) {
    static pfn_v_ufp fn; if (!fn) fn = (pfn_v_ufp)_gles2_resolve("glVertexAttrib1fv");
    if (fn) fn(index, v);
}

void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
    static pfn_v_uff fn; if (!fn) fn = (pfn_v_uff)_gles2_resolve("glVertexAttrib2f");
    if (fn) fn(index, x, y);
}

void glVertexAttrib2fv(GLuint index, const GLfloat *v) {
    static pfn_v_ufp fn; if (!fn) fn = (pfn_v_ufp)_gles2_resolve("glVertexAttrib2fv");
    if (fn) fn(index, v);
}

void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    static pfn_v_ufff fn; if (!fn) fn = (pfn_v_ufff)_gles2_resolve("glVertexAttrib3f");
    if (fn) fn(index, x, y, z);
}

void glVertexAttrib3fv(GLuint index, const GLfloat *v) {
    static pfn_v_ufp fn; if (!fn) fn = (pfn_v_ufp)_gles2_resolve("glVertexAttrib3fv");
    if (fn) fn(index, v);
}

void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    static pfn_v_uffff fn; if (!fn) fn = (pfn_v_uffff)_gles2_resolve("glVertexAttrib4f");
    if (fn) fn(index, x, y, z, w);
}

void glVertexAttrib4fv(GLuint index, const GLfloat *v) {
    static pfn_v_ufp fn; if (!fn) fn = (pfn_v_ufp)_gles2_resolve("glVertexAttrib4fv");
    if (fn) fn(index, v);
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, const void *pointer) {
    static pfn_v_uiebsv fn; if (!fn) fn = (pfn_v_uiebsv)_gles2_resolve("glVertexAttribPointer");
    if (fn) fn(index, size, type, normalized, stride, pointer);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    static pfn_v_iiss fn; if (!fn) fn = (pfn_v_iiss)_gles2_resolve("glViewport");
    if (fn) fn(x, y, width, height);
}

/* ── GL ES 3.0 additions commonly used by WebKit ────────────────────── */

void glBindVertexArray(GLuint array) {
    static pfn_v_u fn; if (!fn) fn = (pfn_v_u)_gles2_resolve("glBindVertexArray");
    if (fn) fn(array);
}

void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {
    static pfn_v_sp fn; if (!fn) fn = (pfn_v_sp)_gles2_resolve("glDeleteVertexArrays");
    if (fn) fn(n, arrays);
}

void glGenVertexArrays(GLsizei n, GLuint *arrays) {
    static pfn_v_spu fn; if (!fn) fn = (pfn_v_spu)_gles2_resolve("glGenVertexArrays");
    if (fn) fn(n, arrays);
}

void glDrawBuffers(GLsizei n, const GLenum *bufs) {
    static pfn_v_sep fn; if (!fn) fn = (pfn_v_sep)_gles2_resolve("glDrawBuffers");
    if (fn) fn(n, bufs);
}

void glReadBuffer(GLenum src) {
    static pfn_v_e fn; if (!fn) fn = (pfn_v_e)_gles2_resolve("glReadBuffer");
    if (fn) fn(src);
}

const GLubyte *glGetStringi(GLenum name, GLuint index) {
    static pfn_cp_eu fn; if (!fn) fn = (pfn_cp_eu)_gles2_resolve("glGetStringi");
    return fn ? fn(name, index) : (const GLubyte *)"";
}

void glRenderbufferStorageMultisample(GLenum target, GLsizei samples,
    GLenum internalformat, GLsizei width, GLsizei height) {
    static pfn_v_esess fn; if (!fn) fn = (pfn_v_esess)_gles2_resolve("glRenderbufferStorageMultisample");
    if (fn) fn(target, samples, internalformat, width, height);
}

void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
    GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    static pfn_v_iiiiiiiiibe fn; if (!fn) fn = (pfn_v_iiiiiiiiibe)_gles2_resolve("glBlitFramebuffer");
    if (fn) fn(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void glTexImage3D(GLenum target, GLint level, GLint internalformat,
    GLsizei width, GLsizei height, GLsizei depth, GLint border,
    GLenum format, GLenum type, const void *pixels) {
    static pfn_v_eiiisssieev fn; if (!fn) fn = (pfn_v_eiiisssieev)_gles2_resolve("glTexImage3D");
    if (fn) fn(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
    GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) {
    static pfn_v_eiiiiissieev fn; if (!fn) fn = (pfn_v_eiiiiissieev)_gles2_resolve("glTexSubImage3D");
    if (fn) fn(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

/* ── OES extension used by OWB ───────────────────────────────────────── */

void glEGLImageTargetTexture2DOES(GLenum target, void *image) {
    static pfn_v_ev fn; if (!fn) fn = (pfn_v_ev)_gles2_resolve("glEGLImageTargetTexture2DOES");
    if (fn) fn(target, image);
}
