/***************************************************************************
 SimpleMail - Copyright (C) 2000 Hynek Schlawack and Sebastian Bauer

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
***************************************************************************/
#ifndef SM__DATATYPESCACHE_H
#define SM__DATATYPESCACHE_H

/* struct dt_node; */

/*void dt_init(void);*/
/*void dt_cleanup(void);*/
struct dt_node *dt_load_picture(char *filename, struct Screen *scr);
void dt_dispose_picture(struct dt_node *node);

int dt_width(struct dt_node *node);
int dt_height(struct dt_node *node);
void dt_put_on_rastport(struct dt_node *node, struct RastPort *rp, int x, int y);
void dt_put_on_rastport_tiled(struct dt_node *node, struct RastPort *rp, int x1, int y1, int x2, int y2, int xoffset, int yoffset);

#endif
