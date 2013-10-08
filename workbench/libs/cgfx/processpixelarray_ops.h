
#include <exec/types.h>

void ProcessPixelArrayBrightnessFunc(struct RastPort *, struct Rectangle *, LONG,struct Library *);
void ProcessPixelArrayAlphaFunc(struct RastPort *, struct Rectangle *, UBYTE, struct Library *);
void ProcessPixelArrayTintFunc(struct RastPort *, struct Rectangle *, ULONG, struct Library *);
void ProcessPixelArrayBlurFunc(struct RastPort *, struct Rectangle *, struct Library *);
void ProcessPixelArrayColor2GreyFunc(struct RastPort *, struct Rectangle *, struct Library *);
void ProcessPixelArrayNegativeFunc(struct RastPort *, struct Rectangle *, struct Library *);
void ProcessPixelArrayNegativeFadeFunc(struct RastPort *, struct Rectangle *, struct Library *);
void ProcessPixelArrayTintFadeFunc(struct RastPort *, struct Rectangle *, struct Library *);
void ProcessPixelArrayGradientFunc(struct RastPort *, struct Rectangle *, BOOL, ULONG, ULONG, ULONG, BOOL, struct Library *);
void ProcessPixelArrayShiftRGBFunc(struct RastPort *, struct Rectangle *, struct Library *);
