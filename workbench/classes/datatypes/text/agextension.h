#ifndef TEXTDT_AG_EXTENSION_H
#define TEXTDT_AG_EXTENSION_H

#define AG_LINK_ADDWIDTH   2


/* setup amigaguide specific text.datatype instance fields */
void PrepareAGExtension(struct Text_Data *td);

/* draw amigaguide link */
void DrawLineBevel(struct Text_Data *td, struct RastPort *rp, 
                   struct Line *line, BOOL text);

/* check and adjust marked link line to new y top pos */
BOOL CheckMarkedLink(struct Text_Data *td, LONG y);

/* handles mouse event for amigaguide links. Returns 1 if the
   event belongs to a link, 0 otherwise. */
int HandleMouseLink(struct Text_Data *td, struct RastPort *rp, 
                    LONG x, LONG y, LONG code);

/* send double clicked word to object */
void TriggerWord(struct Text_Data *td, UBYTE *word);

/* handle amigaguide specific trigger functions */
void DT_AGTrigger(struct IClass *cl, Object *o, struct dtTrigger *msg);

#endif
