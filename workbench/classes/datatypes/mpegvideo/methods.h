
extern IPTR DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg);
extern IPTR DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg);
extern IPTR DT_SetMethod(struct IClass *cl, Object *o, struct opSet *msg);
extern IPTR DT_Write(struct IClass *cl, Object *o, struct dtWrite *dtw);
extern IPTR DT_LoadFrame(struct IClass *cl, Object *o, struct adtFrame *alf);
extern IPTR DT_UnLoadFrame(struct IClass *cl, Object *o, struct adtFrame *alf);
