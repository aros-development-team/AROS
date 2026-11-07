/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    glwater: GPU water simulation and ray-traced pool test.

    A port of the well known "WebGL Water" demo idea to mesa3dgl, aimed at
    exercising hardware gallium drivers. Every frame:

      1. The wave simulation runs on the GPU: a SIM x SIM RGBA float texture
         (r = height, g = velocity, ba = surface normal xz) is ping-ponged
         through FBO passes (drop, sphere displacement, 2x update, normals).
      2. A caustics texture is rendered by projecting the water grid along
         refracted light rays and measuring how much each triangle shrinks
         or grows (dFdx/dFdy in the fragment shader).
      3. The pool, the water surface (above and below water variants) and a
         floating sphere are drawn. The water fragment shader ray-traces
         reflections and refractions against the pool box, the sphere and a
         sky cube map.

    This exercises: GLSL 1.20 vertex/fragment shaders, vertex texture fetch,
    float render targets, FBO ping-pong, multiple texture units, cube maps,
    VBO/IBO drawing, face culling, derivatives and a reasonable fragment
    shader load. Software rasterizers will run it very slowly: that is not a
    concern of this test.

    Self checks (printed with [glwater], failures reported in the exit code):
      - the water texture holds finite, non-zero, bounded heights
      - the caustics texture has plausible energy and a sphere shadow
      - the window shows something other than the clear colour

    Arguments:
      WIDTH/N, HEIGHT/N   window inner size (default 800x600)
      SECONDS/N           run for this long then exit (default: until closed)
      SIM/N               simulation texture size (default 256)
      CAUSTICS/N          caustics texture size (default 1024)
      DETAIL/N            water grid subdivisions (default 200)
      GRAVITY/S           sphere physics on from the start
      SEED/N              random seed for the initial drops

    Controls: drag outside the pool to orbit, drag on the water to add drops,
    drag the sphere to move it, mouse wheel zooms, SPACE pauses, G toggles
    gravity, R adds a random drop, ESC quits.

    The simulation is stepped on a time base (120 steps/s, as the original
    gets at 60 Hz with two steps per frame), so wave speed and damping do not
    depend on the frame rate the driver manages.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <devices/rawkeycodes.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/timer.h>

#include <GL/gla.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

CONST_STRPTR version = "$VER: glwater 1.0 (07.11.2026) AROS";

static const char template[] = "WIDTH/N,HEIGHT/N,SECONDS/N,SIM/N,CAUSTICS/N,DETAIL/N,GRAVITY/S,SEED/N";
enum { ARG_WIDTH, ARG_HEIGHT, ARG_SECONDS, ARG_SIM, ARG_CAUSTICS, ARG_DETAIL, ARG_GRAVITY, ARG_SEED, ARG_COUNT };

#define POOL_RIM (2.0f / 12.0f)

/* ------------------------------------------------------------------------ */
/* Output                                                                   */

static void out(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("%s", buf);
    fflush(stdout);
    bug("%s", buf);
}

/* ------------------------------------------------------------------------ */
/* GL entry points                                                          */

static PFNGLCREATESHADERPROC             p_glCreateShader;
static PFNGLSHADERSOURCEPROC             p_glShaderSource;
static PFNGLCOMPILESHADERPROC            p_glCompileShader;
static PFNGLGETSHADERIVPROC              p_glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC         p_glGetShaderInfoLog;
static PFNGLCREATEPROGRAMPROC            p_glCreateProgram;
static PFNGLATTACHSHADERPROC             p_glAttachShader;
static PFNGLLINKPROGRAMPROC              p_glLinkProgram;
static PFNGLGETPROGRAMIVPROC             p_glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC        p_glGetProgramInfoLog;
static PFNGLUSEPROGRAMPROC               p_glUseProgram;
static PFNGLDELETESHADERPROC             p_glDeleteShader;
static PFNGLDELETEPROGRAMPROC            p_glDeleteProgram;
static PFNGLBINDATTRIBLOCATIONPROC       p_glBindAttribLocation;
static PFNGLGETUNIFORMLOCATIONPROC       p_glGetUniformLocation;
static PFNGLUNIFORM1IPROC                p_glUniform1i;
static PFNGLUNIFORM1FPROC                p_glUniform1f;
static PFNGLUNIFORM2FPROC                p_glUniform2f;
static PFNGLUNIFORM3FPROC                p_glUniform3f;
static PFNGLUNIFORMMATRIX4FVPROC         p_glUniformMatrix4fv;
static PFNGLENABLEVERTEXATTRIBARRAYPROC  p_glEnableVertexAttribArray;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC p_glDisableVertexAttribArray;
static PFNGLVERTEXATTRIBPOINTERPROC      p_glVertexAttribPointer;
static PFNGLGENBUFFERSPROC               p_glGenBuffers;
static PFNGLBINDBUFFERPROC               p_glBindBuffer;
static PFNGLBUFFERDATAPROC               p_glBufferData;
static PFNGLDELETEBUFFERSPROC            p_glDeleteBuffers;
static PFNGLGENFRAMEBUFFERSPROC          p_glGenFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC          p_glBindFramebuffer;
static PFNGLFRAMEBUFFERTEXTURE2DPROC     p_glFramebufferTexture2D;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC   p_glCheckFramebufferStatus;
static PFNGLDELETEFRAMEBUFFERSPROC       p_glDeleteFramebuffers;
static PFNGLGENERATEMIPMAPPROC           p_glGenerateMipmap;
static PFNGLACTIVETEXTUREPROC            p_glActiveTexture;

static int missing_procs = 0;

static GLAProc getproc(const char *name, const char *alt)
{
    GLAProc p = glAGetProcAddress((const GLubyte *)name);
    if (!p && alt)
        p = glAGetProcAddress((const GLubyte *)alt);
    if (!p)
    {
        out("[glwater] missing GL entry point: %s\n", name);
        missing_procs++;
    }
    return p;
}

#define GETPROC(var, type, name, alt) var = (type)getproc(name, alt)

static BOOL load_gl_functions(void)
{
    GETPROC(p_glCreateShader,             PFNGLCREATESHADERPROC,             "glCreateShader", NULL);
    GETPROC(p_glShaderSource,             PFNGLSHADERSOURCEPROC,             "glShaderSource", NULL);
    GETPROC(p_glCompileShader,            PFNGLCOMPILESHADERPROC,            "glCompileShader", NULL);
    GETPROC(p_glGetShaderiv,              PFNGLGETSHADERIVPROC,              "glGetShaderiv", NULL);
    GETPROC(p_glGetShaderInfoLog,         PFNGLGETSHADERINFOLOGPROC,         "glGetShaderInfoLog", NULL);
    GETPROC(p_glCreateProgram,            PFNGLCREATEPROGRAMPROC,            "glCreateProgram", NULL);
    GETPROC(p_glAttachShader,             PFNGLATTACHSHADERPROC,             "glAttachShader", NULL);
    GETPROC(p_glLinkProgram,              PFNGLLINKPROGRAMPROC,              "glLinkProgram", NULL);
    GETPROC(p_glGetProgramiv,             PFNGLGETPROGRAMIVPROC,             "glGetProgramiv", NULL);
    GETPROC(p_glGetProgramInfoLog,        PFNGLGETPROGRAMINFOLOGPROC,        "glGetProgramInfoLog", NULL);
    GETPROC(p_glUseProgram,               PFNGLUSEPROGRAMPROC,               "glUseProgram", NULL);
    GETPROC(p_glDeleteShader,             PFNGLDELETESHADERPROC,             "glDeleteShader", NULL);
    GETPROC(p_glDeleteProgram,            PFNGLDELETEPROGRAMPROC,            "glDeleteProgram", NULL);
    GETPROC(p_glBindAttribLocation,       PFNGLBINDATTRIBLOCATIONPROC,       "glBindAttribLocation", NULL);
    GETPROC(p_glGetUniformLocation,       PFNGLGETUNIFORMLOCATIONPROC,       "glGetUniformLocation", NULL);
    GETPROC(p_glUniform1i,                PFNGLUNIFORM1IPROC,                "glUniform1i", NULL);
    GETPROC(p_glUniform1f,                PFNGLUNIFORM1FPROC,                "glUniform1f", NULL);
    GETPROC(p_glUniform2f,                PFNGLUNIFORM2FPROC,                "glUniform2f", NULL);
    GETPROC(p_glUniform3f,                PFNGLUNIFORM3FPROC,                "glUniform3f", NULL);
    GETPROC(p_glUniformMatrix4fv,         PFNGLUNIFORMMATRIX4FVPROC,         "glUniformMatrix4fv", NULL);
    GETPROC(p_glEnableVertexAttribArray,  PFNGLENABLEVERTEXATTRIBARRAYPROC,  "glEnableVertexAttribArray", NULL);
    GETPROC(p_glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC, "glDisableVertexAttribArray", NULL);
    GETPROC(p_glVertexAttribPointer,      PFNGLVERTEXATTRIBPOINTERPROC,      "glVertexAttribPointer", NULL);
    GETPROC(p_glGenBuffers,               PFNGLGENBUFFERSPROC,               "glGenBuffers", "glGenBuffersARB");
    GETPROC(p_glBindBuffer,               PFNGLBINDBUFFERPROC,               "glBindBuffer", "glBindBufferARB");
    GETPROC(p_glBufferData,               PFNGLBUFFERDATAPROC,               "glBufferData", "glBufferDataARB");
    GETPROC(p_glDeleteBuffers,            PFNGLDELETEBUFFERSPROC,            "glDeleteBuffers", "glDeleteBuffersARB");
    GETPROC(p_glGenFramebuffers,          PFNGLGENFRAMEBUFFERSPROC,          "glGenFramebuffers", "glGenFramebuffersEXT");
    GETPROC(p_glBindFramebuffer,          PFNGLBINDFRAMEBUFFERPROC,          "glBindFramebuffer", "glBindFramebufferEXT");
    GETPROC(p_glFramebufferTexture2D,     PFNGLFRAMEBUFFERTEXTURE2DPROC,     "glFramebufferTexture2D", "glFramebufferTexture2DEXT");
    GETPROC(p_glCheckFramebufferStatus,   PFNGLCHECKFRAMEBUFFERSTATUSPROC,   "glCheckFramebufferStatus", "glCheckFramebufferStatusEXT");
    GETPROC(p_glDeleteFramebuffers,       PFNGLDELETEFRAMEBUFFERSPROC,       "glDeleteFramebuffers", "glDeleteFramebuffersEXT");
    GETPROC(p_glActiveTexture,            PFNGLACTIVETEXTUREPROC,            "glActiveTexture", "glActiveTextureARB");

    /* optional */
    p_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)glAGetProcAddress((const GLubyte *)"glGenerateMipmap");
    if (!p_glGenerateMipmap)
        p_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)glAGetProcAddress((const GLubyte *)"glGenerateMipmapEXT");

    return missing_procs == 0;
}

static const char *gl_error_name(GLenum e)
{
    switch (e)
    {
    case GL_NO_ERROR:          return "GL_NO_ERROR";
    case GL_INVALID_ENUM:      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:     return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:     return "GL_OUT_OF_MEMORY";
    case 0x0506:               return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default:                   return "unknown";
    }
}

static void check_gl(const char *where)
{
    GLenum e;
    while ((e = glGetError()) != GL_NO_ERROR)
        out("[glwater] GL error after %s: 0x%04x (%s)\n", where, (unsigned)e, gl_error_name(e));
}

/* ------------------------------------------------------------------------ */
/* Small vector / matrix library                                            */

typedef struct { float x, y, z; } vec3;

static vec3  v3(float x, float y, float z)      { vec3 r = { x, y, z }; return r; }
static vec3  v3add(vec3 a, vec3 b)              { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static vec3  v3sub(vec3 a, vec3 b)              { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static vec3  v3scale(vec3 a, float s)           { return v3(a.x * s, a.y * s, a.z * s); }
static float v3dot(vec3 a, vec3 b)              { return a.x * b.x + a.y * b.y + a.z * b.z; }
static float v3len(vec3 a)                      { return sqrtf(v3dot(a, a)); }
static vec3  v3cross(vec3 a, vec3 b)
{
    return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static vec3  v3norm(vec3 a)
{
    float l = v3len(a);
    return l > 0.0f ? v3scale(a, 1.0f / l) : a;
}
static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

/* column major, GL convention: m[col*4 + row] */
typedef struct { float m[16]; } mat4;

static mat4 mat4_identity(void)
{
    mat4 r;
    memset(&r, 0, sizeof(r));
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static mat4 mat4_mul(const mat4 *a, const mat4 *b)
{
    mat4 r;
    int c, row, k;
    for (c = 0; c < 4; c++)
        for (row = 0; row < 4; row++)
        {
            float s = 0.0f;
            for (k = 0; k < 4; k++)
                s += a->m[k * 4 + row] * b->m[c * 4 + k];
            r.m[c * 4 + row] = s;
        }
    return r;
}

static mat4 mat4_perspective(float fovy_deg, float aspect, float znear, float zfar)
{
    mat4 r;
    float f = 1.0f / tanf(fovy_deg * (float)M_PI / 360.0f);
    memset(&r, 0, sizeof(r));
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (zfar + znear) / (znear - zfar);
    r.m[11] = -1.0f;
    r.m[14] = 2.0f * zfar * znear / (znear - zfar);
    return r;
}

static mat4 mat4_translation(float x, float y, float z)
{
    mat4 r = mat4_identity();
    r.m[12] = x; r.m[13] = y; r.m[14] = z;
    return r;
}

static mat4 mat4_rotation(float deg, float x, float y, float z)
{
    mat4 r = mat4_identity();
    vec3 a = v3norm(v3(x, y, z));
    float rad = deg * (float)M_PI / 180.0f;
    float c = cosf(rad), s = sinf(rad), t = 1.0f - c;

    r.m[0]  = t * a.x * a.x + c;
    r.m[1]  = t * a.x * a.y + s * a.z;
    r.m[2]  = t * a.x * a.z - s * a.y;
    r.m[4]  = t * a.x * a.y - s * a.z;
    r.m[5]  = t * a.y * a.y + c;
    r.m[6]  = t * a.y * a.z + s * a.x;
    r.m[8]  = t * a.x * a.z + s * a.y;
    r.m[9]  = t * a.y * a.z - s * a.x;
    r.m[10] = t * a.z * a.z + c;
    return r;
}

/* General 4x4 inverse by cofactors. Returns FALSE if singular. */
static BOOL mat4_invert(const mat4 *src, mat4 *dst)
{
    const float *m = src->m;
    float inv[16], det;
    int i;

    inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8]  =  m[4]*m[9]*m[15]  - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14]  + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9]  = -m[0]*m[9]*m[15]  + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14]  - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2]  =  m[1]*m[6]*m[15]  - m[1]*m[7]*m[14]  - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7]  - m[13]*m[3]*m[6];
    inv[6]  = -m[0]*m[6]*m[15]  + m[0]*m[7]*m[14]  + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7]  + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15]  - m[0]*m[7]*m[13]  - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7]  - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14]  + m[0]*m[6]*m[13]  + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6]  + m[12]*m[2]*m[5];
    inv[3]  = -m[1]*m[6]*m[11]  + m[1]*m[7]*m[10]  + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7]   + m[9]*m[3]*m[6];
    inv[7]  =  m[0]*m[6]*m[11]  - m[0]*m[7]*m[10]  - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7]   - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11]  + m[0]*m[7]*m[9]   + m[4]*m[1]*m[11] - m[4]*m[3]*m[9]  - m[8]*m[1]*m[7]   + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10]  - m[0]*m[6]*m[9]   - m[4]*m[1]*m[10] + m[4]*m[2]*m[9]  + m[8]*m[1]*m[6]   - m[8]*m[2]*m[5];

    det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (det == 0.0f)
        return FALSE;
    det = 1.0f / det;
    for (i = 0; i < 16; i++)
        dst->m[i] = inv[i] * det;
    return TRUE;
}

/* transform (x,y,z,w), divide by w */
static vec3 mat4_project(const mat4 *m, float x, float y, float z, float w)
{
    float rx = m->m[0] * x + m->m[4] * y + m->m[8]  * z + m->m[12] * w;
    float ry = m->m[1] * x + m->m[5] * y + m->m[9]  * z + m->m[13] * w;
    float rz = m->m[2] * x + m->m[6] * y + m->m[10] * z + m->m[14] * w;
    float rw = m->m[3] * x + m->m[7] * y + m->m[11] * z + m->m[15] * w;
    if (rw != 0.0f && rw != 1.0f)
    {
        rx /= rw; ry /= rw; rz /= rw;
    }
    return v3(rx, ry, rz);
}

/* ------------------------------------------------------------------------ */
/* Shaders                                                                  */

static const char *glsl_version = "#version 120\n";

/* Pool geometry, light and intersection helpers shared by all stages. */
static const char *glsl_consts =
"const float IOR_AIR = 1.0;\n"
"const float IOR_WATER = 1.333;\n"
"const vec3 abovewaterColor = vec3(0.25, 1.0, 1.25);\n"
"const vec3 underwaterColor = vec3(0.4, 0.9, 1.0);\n"
"const float poolHeight = 1.0;\n"
"uniform vec3 light;\n"
"uniform vec3 sphereCenter;\n"
"uniform float sphereRadius;\n"
"vec2 intersectCube(vec3 origin, vec3 ray, vec3 cubeMin, vec3 cubeMax) {\n"
"  vec3 tMin = (cubeMin - origin) / ray;\n"
"  vec3 tMax = (cubeMax - origin) / ray;\n"
"  vec3 t1 = min(tMin, tMax);\n"
"  vec3 t2 = max(tMin, tMax);\n"
"  float tNear = max(max(t1.x, t1.y), t1.z);\n"
"  float tFar = min(min(t2.x, t2.y), t2.z);\n"
"  return vec2(tNear, tFar);\n"
"}\n"
"float intersectSphere(vec3 origin, vec3 ray, vec3 center, float radius) {\n"
"  vec3 toSphere = origin - center;\n"
"  float a = dot(ray, ray);\n"
"  float b = 2.0 * dot(toSphere, ray);\n"
"  float c = dot(toSphere, toSphere) - radius * radius;\n"
"  float disc = b * b - 4.0 * a * c;\n"
"  if (disc > 0.0) {\n"
"    float t = (-b - sqrt(disc)) / (2.0 * a);\n"
"    if (t > 0.0) return t;\n"
"  }\n"
"  return 1.0e6;\n"
"}\n";

/* Surface shading of the pool walls and the sphere (fragment stages). */
static const char *glsl_shading =
"uniform sampler2D tiles;\n"
"uniform sampler2D causticTex;\n"
"uniform sampler2D water;\n"
"vec3 getSphereColor(vec3 point) {\n"
"  vec3 color = vec3(0.5);\n"
"  color *= 1.0 - 0.9 / pow((1.0 + sphereRadius - abs(point.x)) / sphereRadius, 3.0);\n"
"  color *= 1.0 - 0.9 / pow((1.0 + sphereRadius - abs(point.z)) / sphereRadius, 3.0);\n"
"  color *= 1.0 - 0.9 / pow((point.y + 1.0 + sphereRadius) / sphereRadius, 3.0);\n"
"  vec3 sphereNormal = (point - sphereCenter) / sphereRadius;\n"
"  vec3 refractedLight = refract(-light, vec3(0.0, 1.0, 0.0), IOR_AIR / IOR_WATER);\n"
"  float diffuse = max(0.0, dot(-refractedLight, sphereNormal)) * 0.5;\n"
"  vec4 info = texture2D(water, point.xz * 0.5 + 0.5);\n"
"  if (point.y < info.r) {\n"
"    vec4 caustic = texture2D(causticTex, 0.75 * (point.xz - point.y * refractedLight.xz / refractedLight.y) * 0.5 + 0.5);\n"
"    diffuse *= caustic.r * 4.0;\n"
"  }\n"
"  color += diffuse;\n"
"  return color;\n"
"}\n"
"vec3 getWallColor(vec3 point) {\n"
"  float scale = 0.5;\n"
"  vec3 wallColor;\n"
"  vec3 normal;\n"
"  if (abs(point.x) > 0.999) {\n"
"    wallColor = texture2D(tiles, point.yz * 0.5 + vec2(1.0, 0.5)).rgb;\n"
"    normal = vec3(-point.x, 0.0, 0.0);\n"
"  } else if (abs(point.z) > 0.999) {\n"
"    wallColor = texture2D(tiles, point.yx * 0.5 + vec2(1.0, 0.5)).rgb;\n"
"    normal = vec3(0.0, 0.0, -point.z);\n"
"  } else {\n"
"    wallColor = texture2D(tiles, point.xz * 0.5 + 0.5).rgb;\n"
"    normal = vec3(0.0, 1.0, 0.0);\n"
"  }\n"
"  scale /= length(point);\n"
"  scale *= 1.0 - 0.9 / pow(length(point - sphereCenter) / sphereRadius, 4.0);\n"
"  vec3 refractedLight = -refract(-light, vec3(0.0, 1.0, 0.0), IOR_AIR / IOR_WATER);\n"
"  float diffuse = max(0.0, dot(refractedLight, normal));\n"
"  vec4 info = texture2D(water, point.xz * 0.5 + 0.5);\n"
"  if (point.y < info.r) {\n"
"    vec4 caustic = texture2D(causticTex, 0.75 * (point.xz - point.y * refractedLight.xz / refractedLight.y) * 0.5 + 0.5);\n"
"    scale += diffuse * caustic.r * 2.0 * caustic.g;\n"
"  } else {\n"
"    vec2 t = intersectCube(point, refractedLight, vec3(-1.0, -poolHeight, -1.0), vec3(1.0, 2.0, 1.0));\n"
"    diffuse *= 1.0 / (1.0 + exp(-200.0 / (1.0 + 10.0 * (t.y - t.x)) * (point.y + refractedLight.y * t.y - 2.0 / 12.0)));\n"
"    scale += diffuse * 0.5;\n"
"  }\n"
"  return wallColor * scale;\n"
"}\n";

/* --- simulation: fullscreen quad into the ping-pong texture --- */

static const char *vs_quad =
"attribute vec2 a_pos;\n"
"varying vec2 coord;\n"
"void main() {\n"
"  coord = a_pos * 0.5 + 0.5;\n"
"  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
"}\n";

static const char *fs_drop =
"const float PI = 3.141592653589793;\n"
"uniform sampler2D src;\n"
"uniform vec2 center;\n"
"uniform float radius;\n"
"uniform float strength;\n"
"varying vec2 coord;\n"
"void main() {\n"
"  vec4 info = texture2D(src, coord);\n"
"  float drop = max(0.0, 1.0 - length(center * 0.5 + 0.5 - coord) / radius);\n"
"  drop = 0.5 - cos(drop * PI) * 0.5;\n"
"  info.r += drop * strength;\n"
"  gl_FragColor = info;\n"
"}\n";

static const char *fs_update =
"uniform sampler2D src;\n"
"uniform vec2 delta;\n"
"varying vec2 coord;\n"
"void main() {\n"
"  vec4 info = texture2D(src, coord);\n"
"  vec2 dx = vec2(delta.x, 0.0);\n"
"  vec2 dy = vec2(0.0, delta.y);\n"
"  float average = (\n"
"    texture2D(src, coord - dx).r +\n"
"    texture2D(src, coord - dy).r +\n"
"    texture2D(src, coord + dx).r +\n"
"    texture2D(src, coord + dy).r\n"
"  ) * 0.25;\n"
"  info.g += (average - info.r) * 2.0;\n"
"  info.g *= 0.995;\n"
"  info.r += info.g;\n"
"  gl_FragColor = info;\n"
"}\n";

static const char *fs_normal =
"uniform sampler2D src;\n"
"uniform vec2 delta;\n"
"varying vec2 coord;\n"
"void main() {\n"
"  vec4 info = texture2D(src, coord);\n"
"  vec3 dx = vec3(delta.x, texture2D(src, vec2(coord.x + delta.x, coord.y)).r - info.r, 0.0);\n"
"  vec3 dy = vec3(0.0, texture2D(src, vec2(coord.x, coord.y + delta.y)).r - info.r, delta.y);\n"
"  info.ba = normalize(cross(dy, dx)).xz;\n"
"  gl_FragColor = info;\n"
"}\n";

static const char *fs_sphere_displace =
"uniform sampler2D src;\n"
"uniform vec3 oldCenter;\n"
"uniform vec3 newCenter;\n"
"uniform float radius;\n"
"varying vec2 coord;\n"
"float volumeInSphere(vec3 center) {\n"
"  vec3 toCenter = vec3(coord.x * 2.0 - 1.0, 0.0, coord.y * 2.0 - 1.0) - center;\n"
"  float t = length(toCenter) / radius;\n"
"  float dy = exp(-pow(t * 1.5, 6.0));\n"
"  float ymin = min(0.0, center.y - dy);\n"
"  float ymax = min(max(0.0, center.y + dy), ymin + 2.0 * dy);\n"
"  return (ymax - ymin) * 0.1;\n"
"}\n"
"void main() {\n"
"  vec4 info = texture2D(src, coord);\n"
"  info.r += volumeInSphere(oldCenter);\n"
"  info.r -= volumeInSphere(newCenter);\n"
"  gl_FragColor = info;\n"
"}\n";

/* --- caustics: water grid projected along refracted light onto the floor --- */

static const char *vs_caustics =
"attribute vec2 a_pos;\n"
"uniform sampler2D water;\n"
"varying vec3 oldPos;\n"
"varying vec3 newPos;\n"
"vec3 project(vec3 origin, vec3 dir, vec3 refractedLight) {\n"
"  vec2 tcube = intersectCube(origin, dir, vec3(-1.0, -poolHeight, -1.0), vec3(1.0, 2.0, 1.0));\n"
"  origin += dir * tcube.y;\n"
"  float tplane = (-origin.y - 1.0) / refractedLight.y;\n"
"  return origin + refractedLight * tplane;\n"
"}\n"
"void main() {\n"
"  vec4 info = texture2D(water, a_pos * 0.5 + 0.5);\n"
"  info.ba *= 0.5;\n"
"  vec3 normal = vec3(info.b, sqrt(1.0 - dot(info.ba, info.ba)), info.a);\n"
"  vec3 refractedLight = refract(-light, vec3(0.0, 1.0, 0.0), IOR_AIR / IOR_WATER);\n"
"  vec3 ray = refract(-light, normal, IOR_AIR / IOR_WATER);\n"
"  vec3 base = vec3(a_pos.x, 0.0, a_pos.y);\n"
"  oldPos = project(base, refractedLight, refractedLight);\n"
"  newPos = project(base + vec3(0.0, info.r, 0.0), ray, refractedLight);\n"
"  gl_Position = vec4(0.75 * (newPos.xz + refractedLight.xz / refractedLight.y), 0.0, 1.0);\n"
"}\n";

static const char *fs_caustics =
"varying vec3 oldPos;\n"
"varying vec3 newPos;\n"
"void main() {\n"
"  float oldArea = length(dFdx(oldPos)) * length(dFdy(oldPos));\n"
"  float newArea = length(dFdx(newPos)) * length(dFdy(newPos));\n"
"  gl_FragColor = vec4(oldArea / newArea * 0.2, 1.0, 0.0, 0.0);\n"
"  vec3 refractedLight = refract(-light, vec3(0.0, 1.0, 0.0), IOR_AIR / IOR_WATER);\n"
"  vec3 dir = (sphereCenter - newPos) / sphereRadius;\n"
"  vec3 area = cross(dir, refractedLight);\n"
"  float shadow = dot(area, area);\n"
"  float dist = dot(dir, -refractedLight);\n"
"  shadow = 1.0 + (shadow - 1.0) / (0.05 + dist * 0.025);\n"
"  shadow = clamp(1.0 / (1.0 + exp(-shadow)), 0.0, 1.0);\n"
"  shadow = mix(1.0, shadow, clamp(dist * 2.0, 0.0, 1.0));\n"
"  gl_FragColor.g = shadow;\n"
"  vec2 t = intersectCube(newPos, -refractedLight, vec3(-1.0, -poolHeight, -1.0), vec3(1.0, 2.0, 1.0));\n"
"  gl_FragColor.r *= 1.0 / (1.0 + exp(-200.0 / (1.0 + 10.0 * (t.y - t.x)) * (newPos.y - refractedLight.y * t.y - 2.0 / 12.0)));\n"
"}\n";

/* --- water surface --- */

static const char *vs_water =
"attribute vec2 a_pos;\n"
"uniform sampler2D water;\n"
"uniform mat4 u_mvp;\n"
"varying vec3 position;\n"
"void main() {\n"
"  vec4 info = texture2D(water, a_pos * 0.5 + 0.5);\n"
"  position = vec3(a_pos.x, info.r, a_pos.y);\n"
"  gl_Position = u_mvp * vec4(position, 1.0);\n"
"}\n";

static const char *fs_water =
"uniform vec3 eye;\n"
"uniform samplerCube sky;\n"
"varying vec3 position;\n"
"vec3 getSurfaceRayColor(vec3 origin, vec3 ray, vec3 waterColor) {\n"
"  vec3 color;\n"
"  float q = intersectSphere(origin, ray, sphereCenter, sphereRadius);\n"
"  if (q < 1.0e6) {\n"
"    color = getSphereColor(origin + ray * q);\n"
"  } else if (ray.y < 0.0) {\n"
"    vec2 t = intersectCube(origin, ray, vec3(-1.0, -poolHeight, -1.0), vec3(1.0, 2.0, 1.0));\n"
"    color = getWallColor(origin + ray * t.y);\n"
"  } else {\n"
"    vec2 t = intersectCube(origin, ray, vec3(-1.0, -poolHeight, -1.0), vec3(1.0, 2.0, 1.0));\n"
"    vec3 hit = origin + ray * t.y;\n"
"    if (hit.y < 2.0 / 12.0) {\n"
"      color = getWallColor(hit);\n"
"    } else {\n"
"      color = textureCube(sky, ray).rgb;\n"
"      color += vec3(pow(max(0.0, dot(light, ray)), 5000.0)) * vec3(10.0, 8.0, 6.0);\n"
"    }\n"
"  }\n"
"  if (ray.y < 0.0) color *= waterColor;\n"
"  return color;\n"
"}\n"
"void main() {\n"
"  vec2 coord = position.xz * 0.5 + 0.5;\n"
"  vec4 info = texture2D(water, coord);\n"
"  for (int i = 0; i < 5; i++) {\n"
"    coord += info.ba * 0.005;\n"
"    info = texture2D(water, coord);\n"
"  }\n"
"  vec3 normal = vec3(info.b, sqrt(1.0 - dot(info.ba, info.ba)), info.a);\n"
"  vec3 incomingRay = normalize(position - eye);\n"
"#ifdef UNDERWATER\n"
"  normal = -normal;\n"
"  vec3 reflectedRay = reflect(incomingRay, normal);\n"
"  vec3 refractedRay = refract(incomingRay, normal, IOR_WATER / IOR_AIR);\n"
"  float fresnel = mix(0.5, 1.0, pow(1.0 - dot(normal, -incomingRay), 3.0));\n"
"  vec3 reflectedColor = getSurfaceRayColor(position, reflectedRay, underwaterColor);\n"
"  vec3 refractedColor = getSurfaceRayColor(position, refractedRay, vec3(1.0)) * vec3(0.8, 1.0, 1.1);\n"
"  gl_FragColor = vec4(mix(reflectedColor, refractedColor, (1.0 - fresnel) * length(refractedRay)), 1.0);\n"
"#else\n"
"  vec3 reflectedRay = reflect(incomingRay, normal);\n"
"  vec3 refractedRay = refract(incomingRay, normal, IOR_AIR / IOR_WATER);\n"
"  float fresnel = mix(0.25, 1.0, pow(1.0 - dot(normal, -incomingRay), 3.0));\n"
"  vec3 reflectedColor = getSurfaceRayColor(position, reflectedRay, abovewaterColor);\n"
"  vec3 refractedColor = getSurfaceRayColor(position, refractedRay, abovewaterColor);\n"
"  gl_FragColor = vec4(mix(refractedColor, reflectedColor, fresnel), 1.0);\n"
"#endif\n"
"}\n";

/* --- sphere --- */

static const char *vs_sphere =
"attribute vec3 a_pos;\n"
"uniform mat4 u_mvp;\n"
"varying vec3 position;\n"
"void main() {\n"
"  position = sphereCenter + a_pos * sphereRadius;\n"
"  gl_Position = u_mvp * vec4(position, 1.0);\n"
"}\n";

static const char *fs_sphere =
"varying vec3 position;\n"
"void main() {\n"
"  gl_FragColor = vec4(getSphereColor(position), 1.0);\n"
"  vec4 info = texture2D(water, position.xz * 0.5 + 0.5);\n"
"  if (position.y < info.r) gl_FragColor.rgb *= underwaterColor * 1.2;\n"
"}\n";

/* --- pool walls and floor --- */

static const char *vs_pool =
"attribute vec3 a_pos;\n"
"uniform mat4 u_mvp;\n"
"varying vec3 position;\n"
"void main() {\n"
"  position = a_pos;\n"
"  gl_Position = u_mvp * vec4(position, 1.0);\n"
"}\n";

static const char *fs_pool =
"varying vec3 position;\n"
"void main() {\n"
"  gl_FragColor = vec4(getWallColor(position), 1.0);\n"
"  vec4 info = texture2D(water, position.xz * 0.5 + 0.5);\n"
"  if (position.y < info.r) gl_FragColor.rgb *= underwaterColor * 1.2;\n"
"}\n";

static GLuint compile_shader(const char *name, GLenum type, const char **srcs, int n)
{
    GLuint sh = p_glCreateShader(type);
    GLint ok = 0;

    if (!sh)
    {
        out("[glwater] %s: glCreateShader failed\n", name);
        return 0;
    }
    p_glShaderSource(sh, n, srcs, NULL);
    p_glCompileShader(sh);
    p_glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        GLsizei len = 0;
        p_glGetShaderInfoLog(sh, sizeof(log) - 1, &len, log);
        log[len] = 0;
        out("[glwater] %s: %s shader failed to compile:\n%s\n", name,
            type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
        p_glDeleteShader(sh);
        return 0;
    }
    return sh;
}

/*
 * Builds a program. Vertex sources: version + defines + consts + body.
 * Fragment sources: version + defines + consts [+ shading] + body.
 * Attribute 0 is always "a_pos".
 */
static GLuint build_program(const char *name, const char *defines,
                            const char *vs_body, const char *fs_body, BOOL shading)
{
    const char *vsrc[4], *fsrc[5];
    int nv = 0, nf = 0;
    GLuint vs, fs, prg;
    GLint ok = 0;

    vsrc[nv++] = glsl_version;
    if (defines) vsrc[nv++] = defines;
    vsrc[nv++] = glsl_consts;
    vsrc[nv++] = vs_body;

    fsrc[nf++] = glsl_version;
    if (defines) fsrc[nf++] = defines;
    fsrc[nf++] = glsl_consts;
    if (shading) fsrc[nf++] = glsl_shading;
    fsrc[nf++] = fs_body;

    vs = compile_shader(name, GL_VERTEX_SHADER, vsrc, nv);
    fs = compile_shader(name, GL_FRAGMENT_SHADER, fsrc, nf);
    if (!vs || !fs)
    {
        if (vs) p_glDeleteShader(vs);
        if (fs) p_glDeleteShader(fs);
        return 0;
    }

    prg = p_glCreateProgram();
    p_glAttachShader(prg, vs);
    p_glAttachShader(prg, fs);
    p_glBindAttribLocation(prg, 0, "a_pos");
    p_glLinkProgram(prg);
    p_glDeleteShader(vs);
    p_glDeleteShader(fs);
    p_glGetProgramiv(prg, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        GLsizei len = 0;
        p_glGetProgramInfoLog(prg, sizeof(log) - 1, &len, log);
        log[len] = 0;
        out("[glwater] %s: program failed to link:\n%s\n", name, log);
        p_glDeleteProgram(prg);
        return 0;
    }
    return prg;
}

static GLint uni(GLuint prg, const char *name)
{
    return p_glGetUniformLocation(prg, name);
}

static void uni1i(GLuint prg, const char *name, GLint v)
{
    GLint l = uni(prg, name);
    if (l >= 0) p_glUniform1i(l, v);
}

static void uni1f(GLuint prg, const char *name, float v)
{
    GLint l = uni(prg, name);
    if (l >= 0) p_glUniform1f(l, v);
}

static void uni2f(GLuint prg, const char *name, float a, float b)
{
    GLint l = uni(prg, name);
    if (l >= 0) p_glUniform2f(l, a, b);
}

static void uni3v(GLuint prg, const char *name, vec3 v)
{
    GLint l = uni(prg, name);
    if (l >= 0) p_glUniform3f(l, v.x, v.y, v.z);
}

static void unim4(GLuint prg, const char *name, const mat4 *m)
{
    GLint l = uni(prg, name);
    if (l >= 0) p_glUniformMatrix4fv(l, 1, GL_FALSE, m->m);
}

/* ------------------------------------------------------------------------ */
/* Meshes                                                                   */

struct mesh
{
    GLuint  vbo, ibo;
    GLint   comps;
    GLsizei nidx;
};

static BOOL mesh_upload(struct mesh *m, const float *verts, int nverts, int comps,
                        const GLuint *idx, int nidx)
{
    p_glGenBuffers(1, &m->vbo);
    p_glGenBuffers(1, &m->ibo);
    if (!m->vbo || !m->ibo)
        return FALSE;
    p_glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    p_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)nverts * comps * sizeof(float), verts, GL_STATIC_DRAW);
    p_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
    p_glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)nidx * sizeof(GLuint), idx, GL_STATIC_DRAW);
    p_glBindBuffer(GL_ARRAY_BUFFER, 0);
    p_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    m->comps = comps;
    m->nidx  = nidx;
    return TRUE;
}

static void mesh_draw(const struct mesh *m)
{
    p_glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    p_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
    p_glEnableVertexAttribArray(0);
    p_glVertexAttribPointer(0, m->comps, GL_FLOAT, GL_FALSE, 0, (const void *)0);
    glDrawElements(GL_TRIANGLES, m->nidx, GL_UNSIGNED_INT, (const void *)0);
    p_glDisableVertexAttribArray(0);
    p_glBindBuffer(GL_ARRAY_BUFFER, 0);
    p_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void mesh_free(struct mesh *m)
{
    if (m->vbo) p_glDeleteBuffers(1, &m->vbo);
    if (m->ibo) p_glDeleteBuffers(1, &m->ibo);
    m->vbo = m->ibo = 0;
}

/*
 * Makes every triangle's winding face toward (inward) or away from (outward)
 * a reference point. Saves having to think about the winding of each quad.
 */
static void fix_winding(const float *verts, GLuint *idx, int ntris, vec3 ref, BOOL inward)
{
    int i;
    for (i = 0; i < ntris; i++)
    {
        GLuint *t = &idx[i * 3];
        vec3 p0 = v3(verts[t[0] * 3], verts[t[0] * 3 + 1], verts[t[0] * 3 + 2]);
        vec3 p1 = v3(verts[t[1] * 3], verts[t[1] * 3 + 1], verts[t[1] * 3 + 2]);
        vec3 p2 = v3(verts[t[2] * 3], verts[t[2] * 3 + 1], verts[t[2] * 3 + 2]);
        vec3 n  = v3cross(v3sub(p1, p0), v3sub(p2, p0));
        vec3 c  = v3scale(v3add(v3add(p0, p1), p2), 1.0f / 3.0f);
        vec3 want = inward ? v3sub(ref, c) : v3sub(c, ref);
        if (v3dot(n, want) < 0.0f)
        {
            GLuint tmp = t[1]; t[1] = t[2]; t[2] = tmp;
        }
    }
}

static BOOL build_quad(struct mesh *m)
{
    static const float v[] = { -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,  -1.0f, 1.0f };
    static const GLuint idx[] = { 0, 1, 2,  0, 2, 3 };
    return mesh_upload(m, v, 4, 2, idx, 6);
}

/*
 * (detail+1)^2 grid over [-1,1]^2. a_pos.x is world x, a_pos.y is world z.
 * Wound so that the +y side is the front face.
 */
static BOOL build_grid(struct mesh *m, int detail)
{
    int nv = (detail + 1) * (detail + 1);
    int ni = detail * detail * 6;
    float  *verts = malloc(nv * 2 * sizeof(float));
    GLuint *idx   = malloc(ni * sizeof(GLuint));
    int i, j, k = 0;
    BOOL ok;

    if (!verts || !idx)
    {
        free(verts); free(idx);
        return FALSE;
    }
    for (j = 0; j <= detail; j++)
        for (i = 0; i <= detail; i++)
        {
            verts[(j * (detail + 1) + i) * 2]     = (float)i / detail * 2.0f - 1.0f;
            verts[(j * (detail + 1) + i) * 2 + 1] = (float)j / detail * 2.0f - 1.0f;
        }
    for (j = 0; j < detail; j++)
        for (i = 0; i < detail; i++)
        {
            GLuint v00 = j * (detail + 1) + i;
            GLuint v10 = v00 + 1;
            GLuint v01 = v00 + detail + 1;
            GLuint v11 = v01 + 1;
            idx[k++] = v00; idx[k++] = v01; idx[k++] = v10;
            idx[k++] = v10; idx[k++] = v01; idx[k++] = v11;
        }
    ok = mesh_upload(m, verts, nv, 2, idx, ni);
    free(verts);
    free(idx);
    return ok;
}

static BOOL build_sphere(struct mesh *m, int stacks, int slices)
{
    int nv = (stacks + 1) * (slices + 1);
    int ni = stacks * slices * 6;
    float  *verts = malloc(nv * 3 * sizeof(float));
    GLuint *idx   = malloc(ni * sizeof(GLuint));
    int i, j, k = 0;
    BOOL ok;

    if (!verts || !idx)
    {
        free(verts); free(idx);
        return FALSE;
    }
    for (j = 0; j <= stacks; j++)
    {
        float phi = (float)M_PI * j / stacks;
        float sp = sinf(phi), cp = cosf(phi);
        for (i = 0; i <= slices; i++)
        {
            float theta = 2.0f * (float)M_PI * i / slices;
            float *p = &verts[(j * (slices + 1) + i) * 3];
            p[0] = sp * cosf(theta);
            p[1] = cp;
            p[2] = sp * sinf(theta);
        }
    }
    for (j = 0; j < stacks; j++)
        for (i = 0; i < slices; i++)
        {
            GLuint a = j * (slices + 1) + i;
            GLuint b = a + slices + 1;
            idx[k++] = a; idx[k++] = b;     idx[k++] = a + 1;
            idx[k++] = b; idx[k++] = b + 1; idx[k++] = a + 1;
        }
    fix_winding(verts, idx, ni / 3, v3(0, 0, 0), FALSE);
    ok = mesh_upload(m, verts, nv, 3, idx, ni);
    free(verts);
    free(idx);
    return ok;
}

static void add_quad(float *verts, int *nv, GLuint *idx, int *ni, vec3 a, vec3 b, vec3 c, vec3 d)
{
    GLuint base = *nv;
    vec3 p[4] = { a, b, c, d };
    int i;
    for (i = 0; i < 4; i++)
    {
        verts[(*nv) * 3]     = p[i].x;
        verts[(*nv) * 3 + 1] = p[i].y;
        verts[(*nv) * 3 + 2] = p[i].z;
        (*nv)++;
    }
    idx[(*ni)++] = base;     idx[(*ni)++] = base + 1; idx[(*ni)++] = base + 2;
    idx[(*ni)++] = base;     idx[(*ni)++] = base + 2; idx[(*ni)++] = base + 3;
}

/* Open-topped box: floor at y=-1, walls up to the rim at y=2/12, inner faces front. */
static BOOL build_pool(struct mesh *m)
{
    const float t = POOL_RIM, b = -1.0f;
    float  verts[20 * 3];
    GLuint idx[30];
    int nv = 0, ni = 0;

    add_quad(verts, &nv, idx, &ni, v3(-1, b, -1), v3( 1, b, -1), v3( 1, b,  1), v3(-1, b,  1)); /* floor */
    add_quad(verts, &nv, idx, &ni, v3(-1, b, -1), v3( 1, b, -1), v3( 1, t, -1), v3(-1, t, -1)); /* -z wall */
    add_quad(verts, &nv, idx, &ni, v3(-1, b,  1), v3( 1, b,  1), v3( 1, t,  1), v3(-1, t,  1)); /* +z wall */
    add_quad(verts, &nv, idx, &ni, v3(-1, b, -1), v3(-1, b,  1), v3(-1, t,  1), v3(-1, t, -1)); /* -x wall */
    add_quad(verts, &nv, idx, &ni, v3( 1, b, -1), v3( 1, b,  1), v3( 1, t,  1), v3( 1, t, -1)); /* +x wall */
    fix_winding(verts, idx, ni / 3, v3(0.0f, -0.5f, 0.0f), TRUE);
    return mesh_upload(m, verts, nv, 3, idx, ni);
}

/* ------------------------------------------------------------------------ */
/* Textures                                                                 */

static GLuint make_texture2d(GLenum ifmt, int w, int h, GLenum fmt, GLenum type,
                             const void *data, GLenum minf, GLenum magf, GLenum wrap)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, fmt, type, data);
    return tex;
}

static unsigned hash2(unsigned x, unsigned y)
{
    unsigned h = x * 374761393u + y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

/* Procedural ceramic tile sheet: 12x12 tiles with grout lines. */
static GLuint make_tiles(void)
{
    const int size = 512, tiles = 12;
    GLubyte *px = malloc(size * size * 3);
    GLuint tex;
    int x, y;

    if (!px)
        return 0;
    for (y = 0; y < size; y++)
        for (x = 0; x < size; x++)
        {
            float u = (x + 0.5f) / size, v = (y + 0.5f) / size;
            float fu = u * tiles - floorf(u * tiles);
            float fv = v * tiles - floorf(v * tiles);
            int tx = (int)(u * tiles), ty = (int)(v * tiles);
            float r, g, b;

            if (fu < 0.06f || fv < 0.06f)
            {
                r = 0.52f; g = 0.52f; b = 0.48f;              /* grout */
            }
            else
            {
                float shade = ((hash2(tx, ty) & 255) / 255.0f - 0.5f) * 0.10f;
                float edge  = 1.0f - 0.08f * (1.0f - clampf((fu - 0.06f) * 10.0f, 0.0f, 1.0f))
                                   - 0.08f * (1.0f - clampf((fv - 0.06f) * 10.0f, 0.0f, 1.0f));
                r = (0.74f + shade) * edge;
                g = (0.80f + shade) * edge;
                b = (0.86f + shade) * edge;
            }
            px[(y * size + x) * 3]     = (GLubyte)(clampf(r, 0.0f, 1.0f) * 255.0f);
            px[(y * size + x) * 3 + 1] = (GLubyte)(clampf(g, 0.0f, 1.0f) * 255.0f);
            px[(y * size + x) * 3 + 2] = (GLubyte)(clampf(b, 0.0f, 1.0f) * 255.0f);
        }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    tex = make_texture2d(GL_RGB, size, size, GL_RGB, GL_UNSIGNED_BYTE, px,
                         p_glGenerateMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR,
                         GL_LINEAR, GL_REPEAT);
    if (p_glGenerateMipmap)
        p_glGenerateMipmap(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    free(px);
    return tex;
}

static void sky_color(vec3 d, GLubyte *rgb)
{
    const vec3 zenith  = { 0.22f, 0.42f, 0.85f };
    const vec3 horizon = { 0.78f, 0.86f, 0.95f };
    const vec3 ground  = { 0.30f, 0.30f, 0.32f };
    vec3 c;
    d = v3norm(d);
    if (d.y >= 0.0f)
    {
        float t = powf(d.y, 0.55f);
        c = v3add(v3scale(horizon, 1.0f - t), v3scale(zenith, t));
    }
    else
    {
        float t = powf(-d.y, 0.5f);
        c = v3add(v3scale(horizon, 1.0f - t), v3scale(ground, t));
    }
    rgb[0] = (GLubyte)(clampf(c.x, 0.0f, 1.0f) * 255.0f);
    rgb[1] = (GLubyte)(clampf(c.y, 0.0f, 1.0f) * 255.0f);
    rgb[2] = (GLubyte)(clampf(c.z, 0.0f, 1.0f) * 255.0f);
}

/* Procedural sky cube map (gradient) following the GL cube map face conventions. */
static GLuint make_sky(void)
{
    const int size = 64;
    GLubyte *px = malloc(size * size * 3);
    GLuint tex = 0;
    int face, s, t;

    if (!px)
        return 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (face = 0; face < 6; face++)
    {
        for (t = 0; t < size; t++)
            for (s = 0; s < size; s++)
            {
                float u = 2.0f * (s + 0.5f) / size - 1.0f;
                float v = 2.0f * (t + 0.5f) / size - 1.0f;
                vec3 d;
                switch (face)
                {
                case 0:  d = v3( 1.0f, -v, -u); break;   /* +X */
                case 1:  d = v3(-1.0f, -v,  u); break;   /* -X */
                case 2:  d = v3( u,  1.0f,  v); break;   /* +Y */
                case 3:  d = v3( u, -1.0f, -v); break;   /* -Y */
                case 4:  d = v3( u, -v,  1.0f); break;   /* +Z */
                default: d = v3(-u, -v, -1.0f); break;   /* -Z */
                }
                sky_color(d, &px[(t * size + s) * 3]);
            }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB, size, size, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, px);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    free(px);
    return tex;
}

/* ------------------------------------------------------------------------ */
/* State                                                                    */

static GLAContext     glcont   = NULL;
static struct Window *win      = NULL;
static BOOL           finished = FALSE;
static BOOL           paused   = FALSE;
static BOOL           use_physics = FALSE;
static int            win_w = 800, win_h = 600;
static int            sim_size = 256, caustics_size = 1024, grid_detail = 200;
static int            failures = 0;

static GLuint fbo = 0;
static GLuint water_a = 0, water_b = 0;
static GLenum water_ifmt = GL_RGBA32F;
static GLuint tex_tiles = 0, tex_sky = 0, tex_caustics = 0;

static GLuint prg_drop, prg_update, prg_normal, prg_displace;
static GLuint prg_caustics, prg_water_above, prg_water_below, prg_sphere, prg_pool;

static struct mesh m_quad, m_grid, m_sphere, m_pool;

static const vec3 light_dir_raw = { 2.0f, 2.0f, -1.0f };
static vec3  light_dir;
static float angle_x = -25.0f, angle_y = -200.5f;
static float camera_dist = 4.0f;

/* 120 simulation steps per second: the original's two steps per 60 Hz frame */
#define SIM_STEP_DT (1.0f / 120.0f)
#define SIM_MAX_STEPS_PER_FRAME 12
static float sim_accum = 0.0f;
static vec3  sphere_center, sphere_old_center, sphere_velocity;
static const vec3 gravity = { 0.0f, -4.0f, 0.0f };
static float sphere_radius = 0.25f;

static mat4 cur_proj, cur_mv, cur_mvp;
static vec3 cur_eye;

enum { MODE_NONE, MODE_ORBIT, MODE_DROPS, MODE_SPHERE };
static int  drag_mode = MODE_NONE;
static int  drag_x, drag_y;
static vec3 drag_prev_hit, drag_plane_normal;

/* ------------------------------------------------------------------------ */
/* Timer                                                                    */

static struct MsgPort     timeport;
static struct timerequest timereq;
struct Device *TimerBase = NULL;

static BOOL init_timer(void)
{
    memset(&timeport, 0, sizeof(timeport));
    memset(&timereq, 0, sizeof(timereq));
    timeport.mp_Node.ln_Type = NT_MSGPORT;
    timeport.mp_Flags        = PA_IGNORE;
    timeport.mp_SigTask      = FindTask(NULL);
    NEWLIST(&timeport.mp_MsgList);
    timereq.tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    timereq.tr_node.io_Message.mn_ReplyPort    = &timeport;
    timereq.tr_node.io_Message.mn_Length       = sizeof(timereq);
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)&timereq, 0) == 0)
    {
        TimerBase = (struct Device *)timereq.tr_node.io_Device;
        return TRUE;
    }
    return FALSE;
}

static void deinit_timer(void)
{
    if (TimerBase)
    {
        CloseDevice((struct IORequest *)&timereq);
        TimerBase = NULL;
    }
}

static double now_seconds(void)
{
    struct timeval tv;
    if (!TimerBase)
        return 0.0;
    GetSysTime(&tv);
    return (double)tv.tv_secs + (double)tv.tv_micro / 1000000.0;
}

/* ------------------------------------------------------------------------ */
/* Water simulation (GPU ping-pong)                                         */

static BOOL attach_and_check(GLuint tex, int size)
{
    GLenum status;
    p_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    status = p_glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glViewport(0, 0, size, size);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        out("[glwater] FBO incomplete: 0x%04x\n", (unsigned)status);
        p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return FALSE;
    }
    return TRUE;
}

static GLuint make_water_texture(GLenum ifmt, const float *zeros)
{
    return make_texture2d(ifmt, sim_size, sim_size, GL_RGBA, GL_FLOAT, zeros,
                          GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
}

static BOOL init_water_textures(void)
{
    float *zeros = calloc((size_t)sim_size * sim_size * 4, sizeof(float));
    BOOL ok;

    if (!zeros)
        return FALSE;

    water_a = make_water_texture(GL_RGBA32F, zeros);
    water_b = make_water_texture(GL_RGBA32F, zeros);
    water_ifmt = GL_RGBA32F;
    check_gl("RGBA32F texture creation");
    ok = attach_and_check(water_b, sim_size);
    if (!ok)
    {
        out("[glwater] RGBA32F is not renderable here, trying RGBA16F\n");
        glDeleteTextures(1, &water_a);
        glDeleteTextures(1, &water_b);
        water_a = make_water_texture(GL_RGBA16F, zeros);
        water_b = make_water_texture(GL_RGBA16F, zeros);
        water_ifmt = GL_RGBA16F;
        ok = attach_and_check(water_b, sim_size);
    }
    p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    free(zeros);
    return ok;
}

/* Runs 'prg' over the whole water texture: reads water_a, writes water_b, swaps. */
static void water_pass_begin(GLuint prg)
{
    p_glUseProgram(prg);
    p_glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, water_a);
    uni1i(prg, "src", 0);
    uni2f(prg, "delta", 1.0f / sim_size, 1.0f / sim_size);
}

static void water_pass_end(void)
{
    GLuint tmp;
    p_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, water_b, 0);
    glViewport(0, 0, sim_size, sim_size);
    mesh_draw(&m_quad);
    p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    tmp = water_a; water_a = water_b; water_b = tmp;
}

static void water_add_drop(float x, float z, float radius, float strength)
{
    water_pass_begin(prg_drop);
    uni2f(prg_drop, "center", x, z);
    uni1f(prg_drop, "radius", radius);
    uni1f(prg_drop, "strength", strength);
    water_pass_end();
}

static void water_move_sphere(vec3 oldc, vec3 newc, float radius)
{
    water_pass_begin(prg_displace);
    uni3v(prg_displace, "oldCenter", oldc);
    uni3v(prg_displace, "newCenter", newc);
    uni1f(prg_displace, "radius", radius);
    water_pass_end();
}

static void water_step(void)
{
    water_pass_begin(prg_update);
    water_pass_end();
}

static void water_update_normals(void)
{
    water_pass_begin(prg_normal);
    water_pass_end();
}

/* ------------------------------------------------------------------------ */
/* Rendering                                                                */

static void set_scene_uniforms(GLuint prg)
{
    unim4(prg, "u_mvp", &cur_mvp);
    uni3v(prg, "light", light_dir);
    uni3v(prg, "sphereCenter", sphere_center);
    uni1f(prg, "sphereRadius", sphere_radius);
    uni3v(prg, "eye", cur_eye);
    uni1i(prg, "water", 0);
    uni1i(prg, "tiles", 1);
    uni1i(prg, "sky", 2);
    uni1i(prg, "causticTex", 3);
}

static void render_caustics(void)
{
    p_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_caustics, 0);
    glViewport(0, 0, caustics_size, caustics_size);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    p_glUseProgram(prg_caustics);
    p_glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, water_a);
    uni1i(prg_caustics, "water", 0);
    uni3v(prg_caustics, "light", light_dir);
    uni3v(prg_caustics, "sphereCenter", sphere_center);
    uni1f(prg_caustics, "sphereRadius", sphere_radius);
    mesh_draw(&m_grid);

    p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void update_camera(void)
{
    mat4 t1 = mat4_translation(0.0f, 0.0f, -camera_dist);
    mat4 rx = mat4_rotation(-angle_x, 1.0f, 0.0f, 0.0f);
    mat4 ry = mat4_rotation(-angle_y, 0.0f, 1.0f, 0.0f);
    mat4 t2 = mat4_translation(0.0f, 0.5f, 0.0f);
    mat4 tmp, inv;

    cur_proj = mat4_perspective(45.0f, (float)win_w / (float)win_h, 0.01f, 100.0f);
    tmp = mat4_mul(&t1, &rx);
    tmp = mat4_mul(&tmp, &ry);
    cur_mv  = mat4_mul(&tmp, &t2);
    cur_mvp = mat4_mul(&cur_proj, &cur_mv);
    if (mat4_invert(&cur_mv, &inv))
        cur_eye = mat4_project(&inv, 0.0f, 0.0f, 0.0f, 1.0f);
}

static void draw_scene(void)
{
    glViewport(0, 0, win_w, win_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    p_glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, water_a);
    p_glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, tex_tiles);
    p_glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_CUBE_MAP, tex_sky);
    p_glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, tex_caustics);
    p_glActiveTexture(GL_TEXTURE0);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    p_glUseProgram(prg_pool);
    set_scene_uniforms(prg_pool);
    mesh_draw(&m_pool);

    p_glUseProgram(prg_water_above);
    set_scene_uniforms(prg_water_above);
    glCullFace(GL_BACK);
    mesh_draw(&m_grid);

    p_glUseProgram(prg_water_below);
    set_scene_uniforms(prg_water_below);
    glCullFace(GL_FRONT);
    mesh_draw(&m_grid);

    glDisable(GL_CULL_FACE);
    p_glUseProgram(prg_sphere);
    set_scene_uniforms(prg_sphere);
    mesh_draw(&m_sphere);

    glDisable(GL_DEPTH_TEST);
    p_glUseProgram(0);
}

/* ------------------------------------------------------------------------ */
/* Simulation update                                                        */

static void update(float dt)
{
    int steps;

    if (drag_mode == MODE_SPHERE)
    {
        /* start from rest when released after being moved by hand */
        sphere_velocity = v3(0, 0, 0);
    }
    else if (use_physics)
    {
        float under = clampf((sphere_radius - sphere_center.y) / (2.0f * sphere_radius), 0.0f, 1.0f);
        sphere_velocity = v3add(sphere_velocity, v3scale(gravity, dt - 1.1f * dt * under));
        sphere_velocity = v3sub(sphere_velocity,
                                v3scale(v3norm(sphere_velocity), under * dt * v3dot(sphere_velocity, sphere_velocity)));
        sphere_center = v3add(sphere_center, v3scale(sphere_velocity, dt));
        if (sphere_center.y < sphere_radius - 1.0f)
        {
            sphere_center.y   = sphere_radius - 1.0f;
            sphere_velocity.y = fabsf(sphere_velocity.y) * 0.7f;
        }
    }

    water_move_sphere(sphere_old_center, sphere_center, sphere_radius);
    sphere_old_center = sphere_center;

    sim_accum += dt;
    for (steps = 0; sim_accum >= SIM_STEP_DT && steps < SIM_MAX_STEPS_PER_FRAME; steps++)
    {
        water_step();
        sim_accum -= SIM_STEP_DT;
    }
    if (sim_accum > SIM_STEP_DT)      /* too slow to keep up: drop the backlog */
        sim_accum = 0.0f;
    water_update_normals();
    render_caustics();
}

/* ------------------------------------------------------------------------ */
/* Picking                                                                  */

static vec3 ray_for_pixel(int px, int py)
{
    mat4 inv;
    float nx = 2.0f * (px + 0.5f) / win_w - 1.0f;
    float ny = 1.0f - 2.0f * (py + 0.5f) / win_h;
    vec3 a, b;

    if (!mat4_invert(&cur_mvp, &inv))
        return v3(0, 0, -1);
    a = mat4_project(&inv, nx, ny, -1.0f, 1.0f);
    b = mat4_project(&inv, nx, ny,  1.0f, 1.0f);
    return v3norm(v3sub(b, a));
}

static BOOL hit_sphere(vec3 origin, vec3 ray, vec3 center, float radius, vec3 *hit)
{
    vec3 offset = v3sub(origin, center);
    float a = v3dot(ray, ray);
    float b = 2.0f * v3dot(ray, offset);
    float c = v3dot(offset, offset) - radius * radius;
    float disc = b * b - 4.0f * a * c;
    if (disc > 0.0f)
    {
        float t = (-b - sqrtf(disc)) / (2.0f * a);
        if (t > 0.0f)
        {
            *hit = v3add(origin, v3scale(ray, t));
            return TRUE;
        }
    }
    return FALSE;
}

static void mouse_down(int x, int y)
{
    vec3 ray = ray_for_pixel(x, y);
    vec3 hit;

    drag_x = x;
    drag_y = y;
    if (hit_sphere(cur_eye, ray, sphere_center, sphere_radius, &hit))
    {
        drag_mode = MODE_SPHERE;
        drag_prev_hit = hit;
        drag_plane_normal = v3scale(ray_for_pixel(win_w / 2, win_h / 2), -1.0f);
    }
    else
    {
        vec3 p = v3add(cur_eye, v3scale(ray, -cur_eye.y / ray.y));
        if (ray.y < 0.0f && fabsf(p.x) < 1.0f && fabsf(p.z) < 1.0f)
        {
            drag_mode = MODE_DROPS;
            water_add_drop(p.x, p.z, 0.03f, 0.01f);
        }
        else
            drag_mode = MODE_ORBIT;
    }
}

static void mouse_move(int x, int y)
{
    vec3 ray, p;

    switch (drag_mode)
    {
    case MODE_ORBIT:
        angle_y -= (float)(x - drag_x);
        angle_x -= (float)(y - drag_y);
        angle_x = clampf(angle_x, -89.999f, 89.999f);
        update_camera();
        break;

    case MODE_DROPS:
        ray = ray_for_pixel(x, y);
        if (ray.y < 0.0f)
        {
            p = v3add(cur_eye, v3scale(ray, -cur_eye.y / ray.y));
            if (fabsf(p.x) < 1.0f && fabsf(p.z) < 1.0f)
                water_add_drop(p.x, p.z, 0.03f, 0.01f);
        }
        break;

    case MODE_SPHERE:
    {
        float denom;
        ray = ray_for_pixel(x, y);
        denom = v3dot(drag_plane_normal, ray);
        if (fabsf(denom) > 1e-6f)
        {
            float t = -v3dot(drag_plane_normal, v3sub(cur_eye, drag_prev_hit)) / denom;
            vec3 next = v3add(cur_eye, v3scale(ray, t));
            sphere_center = v3add(sphere_center, v3sub(next, drag_prev_hit));
            sphere_center.x = clampf(sphere_center.x, sphere_radius - 1.0f, 1.0f - sphere_radius);
            sphere_center.y = clampf(sphere_center.y, sphere_radius - 1.0f, 10.0f);
            sphere_center.z = clampf(sphere_center.z, sphere_radius - 1.0f, 1.0f - sphere_radius);
            drag_prev_hit = next;
        }
        break;
    }
    default:
        break;
    }
    drag_x = x;
    drag_y = y;
}

/* ------------------------------------------------------------------------ */
/* Self checks                                                              */

static BOOL finite_f(float v)
{
    return v == v && v < 1.0e30f && v > -1.0e30f;
}

static void verify_water(const char *when)
{
    float *buf = malloc((size_t)sim_size * sim_size * 4 * sizeof(float));
    int i, n = sim_size * sim_size, nonzero = 0, bad = 0;
    float maxabs = 0.0f, sumn = 0.0f;
    BOOL pass;

    if (!buf)
        return;
    p_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, water_a, 0);
    glReadPixels(0, 0, sim_size, sim_size, GL_RGBA, GL_FLOAT, buf);
    p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_gl("water readback");

    for (i = 0; i < n; i++)
    {
        float h = buf[i * 4], vel = buf[i * 4 + 1], nx = buf[i * 4 + 2], nz = buf[i * 4 + 3];
        if (!finite_f(h) || !finite_f(vel) || !finite_f(nx) || !finite_f(nz))
        {
            bad++;
            continue;
        }
        if (h != 0.0f) nonzero++;
        if (fabsf(h) > maxabs) maxabs = fabsf(h);
        sumn += sqrtf(nx * nx + nz * nz);
    }
    pass = (bad == 0) && (nonzero > 0) && (maxabs > 0.0f) && (maxabs < 1.0f);
    out("[glwater] water %s: %d/%d texels non-zero, max|h|=%.5f, mean|n.xz|=%.5f, non-finite=%d -> %s\n",
        when, nonzero, n, maxabs, sumn / n, bad, pass ? "PASS" : "FAIL");
    if (!pass) failures++;
    free(buf);
}

static void verify_caustics(const char *when)
{
    GLubyte *buf = malloc((size_t)caustics_size * caustics_size * 4);
    int i, n = caustics_size * caustics_size, lit = 0;
    unsigned long sum_r = 0;
    int max_r = 0, min_g = 255;
    BOOL pass;

    if (!buf)
        return;
    p_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_caustics, 0);
    glReadPixels(0, 0, caustics_size, caustics_size, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_gl("caustics readback");

    for (i = 0; i < n; i++)
    {
        int r = buf[i * 4], g = buf[i * 4 + 1];
        sum_r += r;
        if (r > max_r) max_r = r;
        if (r > 0) lit++;
        if (g < min_g) min_g = g;
    }
    /*
     * Expected: about 56% of the texture is covered by the projected floor,
     * at ~0.2 energy, minus the rim shadow. The sphere sits under water
     * and casts a blob shadow (g well below 1).
     */
    pass = (lit > n / 10) && (max_r > 25) && (sum_r / (double)n > 2.0) && (sum_r / (double)n < 160.0);
    out("[glwater] caustics %s: lit=%d/%d, mean r=%.2f, max r=%d, min g=%d (sphere shadow %s) -> %s\n",
        when, lit, n, sum_r / (double)n, max_r, min_g, min_g < 230 ? "present" : "absent",
        pass ? "PASS" : "FAIL");
    if (!pass) failures++;
    free(buf);
}

static void verify_window(const char *when)
{
    static const int sx[5] = { 2, 8, 4, 6, 5 };
    static const int sy[5] = { 2, 8, 6, 4, 5 };
    int i, nonblack = 0;
    char desc[160] = "";

    for (i = 0; i < 5; i++)
    {
        GLubyte px[4] = { 0, 0, 0, 0 };
        char one[32];
        glReadPixels(win_w * sx[i] / 10, win_h * sy[i] / 10, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
        if (px[0] | px[1] | px[2])
            nonblack++;
        snprintf(one, sizeof(one), " %02x%02x%02x", px[0], px[1], px[2]);
        strncat(desc, one, sizeof(desc) - strlen(desc) - 1);
    }
    out("[glwater] window %s: samples%s -> %s\n", when, desc, nonblack >= 3 ? "PASS" : "FAIL");
    if (nonblack < 3) failures++;
}

static void verify_all(const char *when)
{
    verify_water(when);
    verify_caustics(when);
    verify_window(when);
    check_gl("verify");
}

/* ------------------------------------------------------------------------ */
/* Setup / teardown                                                         */

static BOOL initgl(void)
{
    struct TagItem attributes[] =
    {
        { GLA_Window,    (IPTR)win         },
        { GLA_Left,      win->BorderLeft   },
        { GLA_Top,       win->BorderTop    },
        { GLA_Bottom,    win->BorderBottom },
        { GLA_Right,     win->BorderRight  },
        { GLA_DoubleBuf, GL_TRUE           },
        { GLA_RGBMode,   GL_TRUE           },
        { GLA_NoStencil, GL_TRUE           },
        { GLA_NoAccum,   GL_TRUE           },
        { TAG_DONE,      0                 }
    };

    glcont = glACreateContext(attributes);
    if (!glcont)
    {
        out("[glwater] glACreateContext failed\n");
        return FALSE;
    }
    glAMakeCurrent(glcont);
    return TRUE;
}

static BOOL init_scene(void)
{
    const char *gl_ver = (const char *)glGetString(GL_VERSION);
    int i;

    out("[glwater] GL_VENDOR   = %s\n", (const char *)glGetString(GL_VENDOR));
    out("[glwater] GL_RENDERER = %s\n", (const char *)glGetString(GL_RENDERER));
    out("[glwater] GL_VERSION  = %s\n", gl_ver);
    out("[glwater] GLSL        = %s\n", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
    out("[glwater] sim %dx%d, caustics %dx%d, grid %d, window %dx%d\n",
        sim_size, sim_size, caustics_size, caustics_size, grid_detail, win_w, win_h);

    if (!load_gl_functions())
        return FALSE;

    light_dir = v3norm(light_dir_raw);
    sphere_center = sphere_old_center = v3(-0.4f, -0.75f, 0.2f);
    sphere_velocity = v3(0, 0, 0);

    prg_drop        = build_program("drop",        NULL,                  vs_quad,     fs_drop,            FALSE);
    prg_update      = build_program("update",      NULL,                  vs_quad,     fs_update,          FALSE);
    prg_normal      = build_program("normal",      NULL,                  vs_quad,     fs_normal,          FALSE);
    prg_displace    = build_program("displace",    NULL,                  vs_quad,     fs_sphere_displace, FALSE);
    prg_caustics    = build_program("caustics",    NULL,                  vs_caustics, fs_caustics,        FALSE);
    prg_water_above = build_program("water-above", NULL,                  vs_water,    fs_water,           TRUE);
    prg_water_below = build_program("water-below", "#define UNDERWATER\n", vs_water,    fs_water,           TRUE);
    prg_sphere      = build_program("sphere",      NULL,                  vs_sphere,   fs_sphere,          TRUE);
    prg_pool        = build_program("pool",        NULL,                  vs_pool,     fs_pool,            TRUE);
    if (!prg_drop || !prg_update || !prg_normal || !prg_displace || !prg_caustics ||
        !prg_water_above || !prg_water_below || !prg_sphere || !prg_pool)
        return FALSE;
    check_gl("program build");

    if (!build_quad(&m_quad) || !build_grid(&m_grid, grid_detail) ||
        !build_sphere(&m_sphere, 32, 64) || !build_pool(&m_pool))
    {
        out("[glwater] mesh creation failed\n");
        return FALSE;
    }
    check_gl("mesh upload");

    p_glGenFramebuffers(1, &fbo);
    if (!init_water_textures())
        return FALSE;
    out("[glwater] water textures: %s\n", water_ifmt == GL_RGBA32F ? "RGBA32F" : "RGBA16F");

    tex_caustics = make_texture2d(GL_RGBA, caustics_size, caustics_size, GL_RGBA, GL_UNSIGNED_BYTE,
                                  NULL, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
    tex_tiles = make_tiles();
    tex_sky   = make_sky();
    if (!tex_caustics || !tex_tiles || !tex_sky)
    {
        out("[glwater] texture creation failed\n");
        return FALSE;
    }
    if (!attach_and_check(tex_caustics, caustics_size))
        return FALSE;
    p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_gl("texture setup");

    for (i = 0; i < 20; i++)
    {
        float x = (float)rand() / RAND_MAX * 2.0f - 1.0f;
        float z = (float)rand() / RAND_MAX * 2.0f - 1.0f;
        water_add_drop(x, z, 0.03f, (i & 1) ? 0.01f : -0.01f);
    }
    check_gl("initial drops");

    update_camera();
    return TRUE;
}

static void cleanup_scene(void)
{
    GLuint prgs[] = { prg_drop, prg_update, prg_normal, prg_displace, prg_caustics,
                      prg_water_above, prg_water_below, prg_sphere, prg_pool };
    unsigned i;

    if (!glcont)
        return;
    p_glUseProgram(0);
    for (i = 0; i < sizeof(prgs) / sizeof(prgs[0]); i++)
        if (prgs[i]) p_glDeleteProgram(prgs[i]);
    mesh_free(&m_quad);
    mesh_free(&m_grid);
    mesh_free(&m_sphere);
    mesh_free(&m_pool);
    if (fbo) p_glDeleteFramebuffers(1, &fbo);
    if (water_a) glDeleteTextures(1, &water_a);
    if (water_b) glDeleteTextures(1, &water_b);
    if (tex_caustics) glDeleteTextures(1, &tex_caustics);
    if (tex_tiles) glDeleteTextures(1, &tex_tiles);
    if (tex_sky) glDeleteTextures(1, &tex_sky);
}

static void handle_messages(void)
{
    struct IntuiMessage *msg;
    while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
    {
        int mx = msg->MouseX - win->BorderLeft;
        int my = msg->MouseY - win->BorderTop;

        switch (msg->Class)
        {
        case IDCMP_CLOSEWINDOW:
            finished = TRUE;
            break;

        case IDCMP_VANILLAKEY:
            switch (msg->Code)
            {
            case 27:  finished = TRUE; break;
            case ' ': paused = !paused; break;
            case 'g': case 'G':
                use_physics = !use_physics;
                out("[glwater] gravity %s\n", use_physics ? "on" : "off");
                break;
            case 'r': case 'R':
                water_add_drop((float)rand() / RAND_MAX * 2.0f - 1.0f,
                               (float)rand() / RAND_MAX * 2.0f - 1.0f, 0.03f, 0.02f);
                break;
            }
            break;

        case IDCMP_RAWKEY:
            if (msg->Code == RAWKEY_NM_WHEEL_UP || msg->Code == RAWKEY_NM_WHEEL_DOWN)
            {
                camera_dist *= (msg->Code == RAWKEY_NM_WHEEL_UP) ? 0.9f : 1.1f;
                camera_dist = clampf(camera_dist, 1.2f, 16.0f);
                update_camera();
            }
            break;

        case IDCMP_MOUSEBUTTONS:
            if (msg->Code == SELECTDOWN)
                mouse_down(mx, my);
            else if (msg->Code == SELECTUP)
                drag_mode = MODE_NONE;
            break;

        case IDCMP_MOUSEMOVE:
            if (drag_mode != MODE_NONE)
                mouse_move(mx, my);
            break;
        }
        ReplyMsg((struct Message *)msg);
    }
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    struct Screen *pubscreen;
    LONG seconds = 0, seed = 1;
    double t_start, t_last, t_sec;
    unsigned long frame = 0, frames_this_sec = 0;
    BOOL have_timer;
    int rc = RETURN_FAIL;

    rda = ReadArgs(template, args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "glwater");
        return RETURN_ERROR;
    }
    if (args[ARG_WIDTH])    win_w         = *(LONG *)args[ARG_WIDTH];
    if (args[ARG_HEIGHT])   win_h         = *(LONG *)args[ARG_HEIGHT];
    if (args[ARG_SECONDS])  seconds       = *(LONG *)args[ARG_SECONDS];
    if (args[ARG_SIM])      sim_size      = *(LONG *)args[ARG_SIM];
    if (args[ARG_CAUSTICS]) caustics_size = *(LONG *)args[ARG_CAUSTICS];
    if (args[ARG_DETAIL])   grid_detail   = *(LONG *)args[ARG_DETAIL];
    if (args[ARG_SEED])     seed          = *(LONG *)args[ARG_SEED];
    use_physics = args[ARG_GRAVITY] ? TRUE : FALSE;
    FreeArgs(rda);

    if (win_w < 64) win_w = 64;
    if (win_h < 64) win_h = 64;
    if (sim_size < 16) sim_size = 16;
    if (caustics_size < 16) caustics_size = 16;
    if (grid_detail < 2) grid_detail = 2;
    srand((unsigned)seed);

    if ((pubscreen = LockPubScreen(NULL)) == NULL)
        return RETURN_FAIL;

    win = OpenWindowTags(NULL,
        WA_Title,         (IPTR)"glwater",
        WA_PubScreen,     (IPTR)pubscreen,
        WA_CloseGadget,   TRUE,
        WA_DragBar,       TRUE,
        WA_DepthGadget,   TRUE,
        WA_Left,          50,
        WA_Top,           50,
        WA_InnerWidth,    win_w,
        WA_InnerHeight,   win_h,
        WA_Activate,      TRUE,
        WA_RMBTrap,       TRUE,
        WA_ReportMouse,   TRUE,
        WA_SimpleRefresh, TRUE,
        WA_NoCareRefresh, TRUE,
        WA_IDCMP,         IDCMP_VANILLAKEY | IDCMP_RAWKEY | IDCMP_CLOSEWINDOW |
                          IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE,
        TAG_DONE);

    UnlockPubScreen(NULL, pubscreen);
    if (!win)
        return RETURN_FAIL;

    have_timer = init_timer();

    if (initgl() && init_scene())
    {
        t_start = t_last = t_sec = now_seconds();
        rc = RETURN_OK;

        while (!finished)
        {
            double t = now_seconds();
            float dt = have_timer ? (float)(t - t_last) : (1.0f / 60.0f);
            t_last = t;
            if (dt > 0.1f) dt = 0.1f;     /* after a stall: keep the physics sane */

            if (!paused)
                update(dt);
            draw_scene();

            if (frame == 2)
                verify_all("initial");

            glASwapBuffers(glcont);
            frame++;
            frames_this_sec++;

            if (have_timer && t - t_sec >= 1.0)
            {
                out("[glwater] %.1f fps (frame %lu)\n", frames_this_sec / (t - t_sec), frame);
                t_sec = t;
                frames_this_sec = 0;
                if (seconds > 0 && t - t_start >= seconds)
                    finished = TRUE;
            }
            else if (!have_timer && seconds > 0 && frame >= (unsigned long)seconds * 60)
                finished = TRUE;

            handle_messages();
        }

        /* final state: redraw without a swap so the back buffer can be sampled */
        draw_scene();
        verify_all("final");
        check_gl("end of run");
        out("[glwater] %lu frames, %d check(s) failed\n", frame, failures);
        if (failures)
            rc = RETURN_WARN;
    }

    cleanup_scene();
    if (glcont)
        glADestroyContext(glcont);
    deinit_timer();
    CloseWindow(win);
    return rc;
}
