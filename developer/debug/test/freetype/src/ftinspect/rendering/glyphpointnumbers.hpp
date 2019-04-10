// glyphpointnumbers.hpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#pragma once

#include <QGraphicsItem>
#include <QPen>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H


class GlyphPointNumbers
: public QGraphicsItem
{
public:
  GlyphPointNumbers(const QPen& onPen,
                    const QPen& offPen,
                    FT_Outline* outline);
  QRectF boundingRect() const;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget);

private:
  QPen onPen;
  QPen offPen;
  FT_Outline* outline;
  QRectF bRect;
};


// end of glyphpointnumbers.hpp
