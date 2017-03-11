#define DEBUG 1
#include <math.h>   // smallpt, a Path Tracer by Kevin Beason, 2008 
#include <stdlib.h>
#include <exec/types.h>
#include <aros/debug.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include "renderer.h"

struct Vec {        // Usage: time ./smallpt 5000 && xv image.ppm
    double x;
    double y;
    double z; // position, also color (r,g,b)
    
    Vec(double x_ = 0, double y_ = 0, double z_ = 0)
    {
        x = x_;
        y = y_;
        z = z_;
    } 

    Vec operator+(const Vec &b) const { return Vec(x + b.x, y + b.y, z + b.z); } 
    Vec operator-(const Vec &b) const { return Vec(x - b.x, y - b.y, z - b.z); } 
    Vec operator*(double b) const { return Vec(x * b, y * b, z * b); } 
    Vec mult(const Vec &b) const { return Vec(x * b.x, y * b.y, z * b.z); } 
    Vec& norm() { return *this = *this * (1/sqrt(x * x + y * y + z * z)); } 
    double dot(const Vec &b) const { return x * b.x + y * b.y + z * b.z; } // cross: 
    Vec operator%(Vec &b){ return Vec(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);} 
}; 

struct Ray {
    Vec o, d;
    Ray(Vec o_, Vec d_) : o(o_), d(d_) {}
};

enum Refl_t
{
    DIFF,
    SPEC,
    REFR
}; // material types, used in radiance()

struct Sphere
{
    double rad;  // radius
    Vec p, e, c; // position, emission, color
    Refl_t refl; // reflection type (DIFFuse, SPECular, REFRactive)
    Sphere(double rad_, Vec p_, Vec e_, Vec c_, Refl_t refl_) : rad(rad_), p(p_), e(e_), c(c_), refl(refl_) {}
    double intersect(const Ray &r) const
    {                     // returns distance, 0 if nohit
        Vec op = p - r.o; // Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0
        double t, eps = 1e-4, b = op.dot(r.d), det = b * b - op.dot(op) + rad * rad;
        if (det < 0)
            return 0;
        else
            det = sqrt(det);
        return (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0);
    } 
}; 

Sphere spheres[] = {//Scene: radius, position, emission, color, material 
   Sphere(1e5, Vec( 1e5+1,40.8,81.6), Vec(),Vec(.75,.25,.25),DIFF),//Left 
   Sphere(1e5, Vec(-1e5+99,40.8,81.6),Vec(),Vec(.25,.25,.75),DIFF),//Rght 
   Sphere(1e5, Vec(50,40.8, 1e5),     Vec(),Vec(.75,.75,.75),DIFF),//Back 
   Sphere(1e5, Vec(50,40.8,-1e5+170), Vec(),Vec(),           DIFF),//Frnt 
   Sphere(1e5, Vec(50, 1e5, 81.6),    Vec(),Vec(.75,.75,.75),DIFF),//Botm 
   Sphere(1e5, Vec(50,-1e5+81.6,81.6),Vec(),Vec(.75,.75,.75),DIFF),//Top 
   Sphere(16.5,Vec(27,16.5,47),       Vec(),Vec(1,1,1)*.999, SPEC),//Mirr 
   Sphere(16.5,Vec(73,16.5,78),       Vec(),Vec(1,1,1)*.999, REFR),//Glas 
   Sphere(600, Vec(50,681.6-.27,81.6),Vec(12,12,12),  Vec(), DIFF) //Lite 
 }; 

inline double clamp(double x)
{
    return x<0 ? 0 : x>1 ? 1 : x;
} 

inline int toInt(double x)
{
    return int(pow(clamp(x),1/2.2)*255+.5);
} 

inline bool intersect(const Ray &r, double &t, int &id)
{ 
    double n=sizeof(spheres)/sizeof(Sphere), d, inf=t=1e20; 
    for(int i=int(n);i--;) if((d=spheres[i].intersect(r))&&d<t){t=d;id=i;} 
    return t<inf; 
}

Vec radiance(const Ray &r, int depth, unsigned short *Xi)
{ 
    double t; // distance to intersection 
    int id=0; // id of intersected object 
    
    if (!intersect(r, t, id))
        return Vec(); // if miss, return black 
    
    const Sphere &obj = spheres[id];        // the hit object 
    
    Vec x=r.o+r.d*t, n=(x-obj.p).norm(), nl=n.dot(r.d)<0?n:n*-1, f=obj.c; 
    
    double p = f.x>f.y && f.x>f.z ? f.x : f.y>f.z ? f.y : f.z; // max refl 
    
    if (++depth>5) // 5
    {
        if (erand48(Xi)<p)
            f=f*(1/p);
        else return obj.e; //R.R. 
    }
    if (obj.refl == DIFF){                  // Ideal DIFFUSE reflection 
        double r1=2*M_PI*erand48(Xi), r2=erand48(Xi), r2s=sqrt(r2); 
        Vec w=nl, u=((fabs(w.x)>.1?Vec(0,1):Vec(1))%w).norm(), v=w%u; 
        Vec d = (u*cos(r1)*r2s + v*sin(r1)*r2s + w*sqrt(1-r2)).norm(); 
        return obj.e + f.mult(radiance(Ray(x,d),depth,Xi)); 
    } else if (obj.refl == SPEC)            // Ideal SPECULAR reflection 
        return obj.e + f.mult(radiance(Ray(x,r.d-n*2*n.dot(r.d)),depth,Xi)); 
    Ray reflRay(x, r.d-n*2*n.dot(r.d));     // Ideal dielectric REFRACTION 
    bool into = n.dot(nl)>0;                // Ray from outside going in? 
    double nc=1, nt=1.5, nnt=into?nc/nt:nt/nc, ddn=r.d.dot(nl), cos2t; 
    if ((cos2t=1-nnt*nnt*(1-ddn*ddn))<0)    // Total internal reflection 
        return obj.e + f.mult(radiance(reflRay,depth,Xi)); 
    Vec tdir = (r.d*nnt - n*((into?1:-1)*(ddn*nnt+sqrt(cos2t)))).norm(); 
    double a=nt-nc, b=nt+nc, R0=a*a/(b*b), c = 1-(into?-ddn:tdir.dot(n)); 
    double Re=R0+(1-R0)*c*c*c*c*c,Tr=1-Re,P=.25+.5*Re,RP=Re/P,TP=Tr/(1-P); 
    return obj.e + f.mult(depth>2 ? (erand48(Xi)<P ?   // Russian roulette 
        radiance(reflRay,depth,Xi)*RP:radiance(Ray(x,tdir),depth,Xi)*TP) : 
        radiance(reflRay,depth,Xi)*Re+radiance(Ray(x,tdir),depth,Xi)*Tr); 
}

extern "C"
void render_tile(int w, int h, int samps, int tile_x, int tile_y, ULONG *buffer, struct Task *gfxTask)
{
    Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).norm()); // cam pos, dir
    Vec cx = Vec(w * .5135 / h), cy = (cx % cam.d).norm() * .5135, r, *c = new Vec[32*32];

    for (int _y=tile_y * 32; _y < (tile_y + 1) * 32; _y++)
    {
        int y = h - _y - 1;
        for (unsigned short _x=tile_x * 32, Xi[3]={0,0,(UWORD)(y*y*y)}; _x < (tile_x + 1) * 32; _x++)   // Loop cols 
        {
            int x = _x; // w - _x - 1;
            for (int sy=0, i=(_y-tile_y*32)*32+_x-tile_x*32; sy<2; sy++)     // 2x2 subpixel rows 
            {
                for (int sx=0; sx<2; sx++, r=Vec())
                {        // 2x2 subpixel cols 
                    for (int s=0; s<samps; s++)
                    { 
                        double r1=2*erand48(Xi), dx=r1<1 ? sqrt(r1)-1: 1-sqrt(2-r1); 
                        double r2=2*erand48(Xi), dy=r2<1 ? sqrt(r2)-1: 1-sqrt(2-r2); 
                        Vec d = cx*( ( (sx+.5 + dx)/2 + x)/w - .5) + 
                                cy*( ( (sy+.5 + dy)/2 + y)/h - .5) + cam.d; 
                        r = r + radiance(Ray(cam.o+d*140,d.norm()),0,Xi)*(1./samps); 
                    } // Camera rays are pushed ^^^^^ forward to start in interior 
                    c[i] = c[i] + Vec(clamp(r.x),clamp(r.y),clamp(r.z))*.25; 
                } 
            }
        }
        int start_ptr = tile_y*32*w + tile_x*32;
        for (int yy=0; yy < 32; yy++)
        {
            for (int xx=0; xx < 32; xx++)
            {
                buffer[start_ptr+xx] = ((toInt(c[(xx+32*yy)].z) & 0xff) << 24) |
                    ((toInt(c[(xx+32*yy)].y) & 0xff) << 16) | ((toInt(c[(xx + 32*yy)].x) & 0xff) << 8) | 0xff;
            }
            start_ptr += w;
        }
        Signal(gfxTask, SIGBREAKF_CTRL_D);
    }

    delete[] c;
}

static inline struct MyMessage *AllocMsg(struct MinList *msgPool)
{
    if (IsMinListEmpty(msgPool))
        bug("!!! smallpt.cc - run out of free messages!\n");
    
    return (struct MyMessage *)REMHEAD(msgPool);
}

extern "C" void RenderTile(struct ExecBase *SysBase, struct MsgPort *masterPort)
{
    Vec *c = new Vec[TILE_SIZE*TILE_SIZE];
    struct MyMessage *msg;
    struct MyMessage *m;
    struct MsgPort *port = CreateMsgPort();
    struct MsgPort *syncPort = CreateMsgPort();
    struct MinList msgPool;

    FreeSignal(syncPort->mp_SigBit);
    syncPort->mp_SigBit = -1;
    syncPort->mp_Flags = PA_IGNORE;

    NEWLIST(&msgPool);

    D(bug("[SMP-SmallPT-Task] hello, msgport=%p\n", port));
    
    msg = (struct MyMessage *)AllocMem(sizeof(struct MyMessage) * 20, MEMF_PUBLIC | MEMF_CLEAR);
    for (int i=0; i < 20; i++)
        ADDHEAD(&msgPool, &msg[i]);

    if (port)
    {
        ULONG signals;
        BOOL doWork = TRUE;
        BOOL redraw = TRUE;
        
        m = AllocMsg(&msgPool);

        /* Tell renderer that we are bored and want to do some work */
        m->mm_Message.mn_Length = sizeof(msg);
        m->mm_Message.mn_ReplyPort = port;
        m->mm_Type = MSG_HUNGRY;
        PutMsg(masterPort, &m->mm_Message);

        D(bug("[SMP-SmallPT-Task] Just told renderer I'm hungry\n"));

        do {
            signals = Wait(SIGBREAKF_CTRL_C | (1 << port->mp_SigBit));

            if (signals & (1 << port->mp_SigBit))
            {
                while ((m = (struct MyMessage *)GetMsg(port)))
                {
                    if (m->mm_Message.mn_Node.ln_Type == NT_REPLYMSG)
                    {
                        ADDHEAD(&msgPool, m);
                        continue;
                    }

                    if (m->mm_Type == MSG_RENDERTILE)
                    {
                        struct tileWork *tile = m->mm_Body.RenderTile.tile;
                        int w = m->mm_Body.RenderTile.width;
                        int h = m->mm_Body.RenderTile.height;
                        int samps = m->mm_Body.RenderTile.numberOfSamples;
                        ULONG *buffer = m->mm_Body.RenderTile.buffer;
                        int tile_x = m->mm_Body.RenderTile.tile->x;
                        int tile_y = m->mm_Body.RenderTile.tile->y;
                        struct MsgPort *guiPort = m->mm_Body.RenderTile.guiPort;

                        ReplyMsg(&m->mm_Message);

                        Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).norm()); // cam pos, dir
                        Vec cx = Vec(w * .5135 / h), cy = (cx % cam.d).norm() * .5135, r;

                        for (int i=0; i < TILE_SIZE * TILE_SIZE; i++)
                            c[i] = Vec();

                        for (int _y=tile_y * 32; _y < (tile_y + 1) * 32; _y++)
                        {
                            int y = h - _y - 1;
                            for (unsigned short _x=tile_x * 32, Xi[3]={0,0,(UWORD)(y*y*y)}; _x < (tile_x + 1) * 32; _x++)   // Loop cols 
                            {
                                int x = _x; // w - _x - 1;
                                for (int sy=0, i=(_y-tile_y*32)*32+_x-tile_x*32; sy<2; sy++)     // 2x2 subpixel rows 
                                {
                                    for (int sx=0; sx<2; sx++, r=Vec())
                                    {        // 2x2 subpixel cols 
                                        for (int s=0; s<samps; s++)
                                        { 
                                            double r1=2*erand48(Xi), dx=r1<1 ? sqrt(r1)-1: 1-sqrt(2-r1); 
                                            double r2=2*erand48(Xi), dy=r2<1 ? sqrt(r2)-1: 1-sqrt(2-r2); 
                                            Vec d = cx*( ( (sx+.5 + dx)/2 + x)/w - .5) + 
                                                    cy*( ( (sy+.5 + dy)/2 + y)/h - .5) + cam.d; 
                                            r = r + radiance(Ray(cam.o+d*140,d.norm()),0,Xi)*(1./samps); 
                                        } // Camera rays are pushed ^^^^^ forward to start in interior 
                                        c[i] = c[i] + Vec(clamp(r.x),clamp(r.y),clamp(r.z))*.25; 
                                    } 
                                }
                            }
                            int start_ptr = tile_y*32*w + tile_x*32;
                            for (int yy=0; yy < 32; yy++)
                            {
                                for (int xx=0; xx < 32; xx++)
                                {
                                    buffer[start_ptr+xx] = ((toInt(c[(xx+32*yy)].z) & 0xff) << 24) |
                                        ((toInt(c[(xx+32*yy)].y) & 0xff) << 16) | ((toInt(c[(xx + 32*yy)].x) & 0xff) << 8) | 0xff;
                                }
                                start_ptr += w;
                            }

#if 1
                            if (redraw)
                            {
                                m = AllocMsg(&msgPool);
                                m->mm_Message.mn_Length = sizeof(struct MyMessage);
                                m->mm_Message.mn_ReplyPort = syncPort;
                                m->mm_Type = MSG_REDRAWTILE;
                                m->mm_Body.RedrawTile.TileX = tile_x;
                                m->mm_Body.RedrawTile.TileY = tile_y;
                                PutMsg(guiPort, &m->mm_Message);
                                redraw = FALSE;
                            }
                            else if ((m = (struct MyMessage *)GetMsg(syncPort)))
                            {
                                ADDHEAD(&msgPool, m);
                                redraw = TRUE;
                            }
#else
                            (void)syncPort;
                            Signal((struct Task *)guiPort->mp_SigTask, SIGBREAKF_CTRL_D);
#endif
                        }

                        Signal((struct Task *)guiPort->mp_SigTask, SIGBREAKF_CTRL_D);

                        m = AllocMsg(&msgPool);
                        m->mm_Message.mn_Length = sizeof(struct MyMessage);
                        m->mm_Message.mn_ReplyPort = port;
                        m->mm_Type = MSG_RENDERREADY;
                        m->mm_Body.RenderTile.tile = tile;
                        PutMsg(masterPort, &m->mm_Message);

                        m = AllocMsg(&msgPool);
                        m->mm_Message.mn_Length = sizeof(struct MyMessage);
                        m->mm_Message.mn_ReplyPort = port;
                        m->mm_Type = MSG_HUNGRY;
                        PutMsg(masterPort, &m->mm_Message);
                    }
                }
            }

            if (signals & SIGBREAKB_CTRL_C)
                doWork = FALSE;

        } while(doWork);
    }
    D(bug("[SMP-SmallPT-Task] cleaning up stuff\n"));
    FreeMem(msg, sizeof(struct MyMessage) * 20);
    DeleteMsgPort(port);
    DeleteMsgPort(syncPort);

    delete[] c;

    D(bug("[SMP-SmallPT-Task] sending signal to renderer task\n"));
    Signal((struct Task *)masterPort->mp_SigTask, SIGBREAKF_CTRL_C);
}
