BOOL AllocExtLayerInfo(struct Layer_Info * li, struct LayersBase *LayersBase);
void FreeExtLayerInfo(struct Layer_Info * li, struct LayersBase *LayersBase);
ULONG InitLIExtra(struct Layer_Info * li, struct LayersBase * LayersBase);


struct Layer *FindBehindLayer(struct Layer_Info *li, WORD pri, BOOL behind);

struct ClipRect *AllocClipRect(struct Layer *L, struct LayersBase *LayersBase);
void FreeClipRect(struct ClipRect *CR, struct Layer *L, struct LayersBase *LayersBase);

BOOL PointInRegion(struct Region *region, WORD x, WORD y);
void TranslateRegion(struct Region *region, WORD dx, WORD dy);



