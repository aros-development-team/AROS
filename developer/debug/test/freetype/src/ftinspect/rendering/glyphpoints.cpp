// glyphpoints.cpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#include "glyphpoints.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>


GlyphPoints::GlyphPoints(const QPen& onP,
                         const QPen& offP,
                         FT_Outline* outln)
: onPen(onP),
  offPen(offP),
  outline(outln)
{
  FT_BBox cbox;

  qreal halfPenWidth = qMax(onPen.widthF(), offPen.widthF()) / 2;

  FT_Outline_Get_CBox(outline, &cbox);

  bRect.setCoords(qreal(cbox.xMin) / 64 - halfPenWidth,
                  -qreal(cbox.yMax) / 64 - halfPenWidth,
                  qreal(cbox.xMax) / 64 + halfPenWidth,
                  -qreal(cbox.yMin) / 64 + halfPenWidth);
}


QRectF
GlyphPoints::boundingRect() const
{
  return bRect;
}


void
GlyphPoints::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget*)
{
  const qreal lod = option->levelOfDetailFromTransform(
                              painter->worldTransform());

  // don't draw points if magnification is too small
  if (lod >= 5)
  {
    // we want the same dot size regardless of the scaling;
    // for good optical results, the pen widths should be uneven integers

    // interestingly, using `drawPoint' doesn't work as expected:
    // the larger the zoom, the more horizontally stretched the dot appears
#if 0
    qreal origOnPenWidth = onPen.widthF();
    qreal origOffPenWidth = offPen.widthF();

    onPen.setWidthF(origOnPenWidth / lod);
    offPen.setWidthF(origOffPenWidth / lod);

    for (int i = 0; i < outline->n_points; i++)
    {
      if (outline->tags[i] & FT_CURVE_TAG_ON)
        painter->setPen(onPen);
      else
        painter->setPen(offPen);

      painter->drawPoint(QPointF(qreal(outline->points[i].x) / 64,
                                 -qreal(outline->points[i].y) / 64));
    }

    onPen.setWidthF(origOnPenWidth);
    offPen.setWidthF(origOffPenWidth);
#else
    QBrush onBrush(onPen.color());
    QBrush offBrush(offPen.color());

    painter->setPen(Qt::NoPen);

    qreal onRadius = onPen.widthF() / lod;
    qreal offRadius = offPen.widthF() / lod;

    for (int i = 0; i < outline->n_points; i++)
    {
      if (outline->tags[i] & FT_CURVE_TAG_ON)
      {
        painter->setBrush(onBrush);
        painter->drawEllipse(QPointF(qreal(outline->points[i].x) / 64,
                                     -qreal(outline->points[i].y) / 64),
                             onRadius,
                             onRadius);
      }
      else
      {
        painter->setBrush(offBrush);
        painter->drawEllipse(QPointF(qreal(outline->points[i].x) / 64,
                                     -qreal(outline->points[i].y) / 64),
                             offRadius,
                             offRadius);
      }
    }
#endif
  }
}


// end of glyphpoints.cpp
