/*
**
** $Id$
**  anim.datatype 1.12
**
*/

extern IPTR DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg);
extern IPTR DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg);
extern IPTR DT_FrameBox(struct IClass *cl, Object *o, struct dtFrameBox *msg);
extern IPTR DT_SetMethod(struct IClass *cl, Object *o, struct opSet *msg);
extern IPTR DT_Write(struct IClass *cl, Object *o, struct dtWrite *dtw);
extern IPTR DT_LoadFrame(struct IClass *cl, Object *o, struct adtFrame *alf);
extern IPTR DT_UnLoadFrame(struct IClass *cl, Object *o, struct adtFrame *alf);
extern IPTR DT_Stop(struct IClass *cl, Object *o, struct opSet *msg);
extern IPTR DT_Pause(struct IClass *cl, Object *o, struct opSet *msg);
extern IPTR DT_Start(struct IClass *cl, Object *o, struct adtStart *msg);
