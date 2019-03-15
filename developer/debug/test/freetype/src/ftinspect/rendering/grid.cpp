// grid.cpp

// Copyright (C) 2016-2017 by Werner Lemberg.


#include "grid.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>


Grid::Grid(const QPen& gridP,
           const QPen& axisP)
: gridPen(gridP),
  axisPen(axisP)
{
 // empty
}


QRectF
Grid::boundingRect() const
{
  // XXX fix size

  // no need to take care of pen width
  return QRectF(-100, -100,
                200, 200);
}


// XXX call this in a `myQDraphicsView::drawBackground' derived method
//     to always fill the complete viewport

void
Grid::paint(QPainter* painter,
            const QStyleOptionGraphicsItem* option,
            QWidget*)
{
  const qreal lod = option->levelOfDetailFromTransform(
                              painter->worldTransform());

  painter->setPen(gridPen);

  // don't mark pixel center with a cross if magnification is too small
  if (lod > 20)
  {
    int halfLength = 1;

    // cf. QSpinBoxx
    if (lod > 640)
      halfLength = 6;
    else if (lod > 320)
      halfLength = 5;
    else if (lod > 160)
      halfLength = 4;
    else if (lod > 80)
      halfLength = 3;
    else if (lod > 40)
      halfLength = 2;

    for (qreal x = -100; x < 100; x++)
      for (qreal y = -100; y < 100; y++)
      {
        painter->drawLine(QLineF(x + 0.5, y + 0.5 - halfLength / lod,
                                 x + 0.5, y + 0.5 + halfLength / lod));
        painter->drawLine(QLineF(x + 0.5 - halfLength / lod, y + 0.5,
                                 x + 0.5 + halfLength / lod, y + 0.5));
      }
  }

  // don't draw grid if magnification is too small
  if (lod >= 5)
  {
    // XXX fix size
    for (int x = -100; x <= 100; x++)
      painter->drawLine(x, -100,
                        x, 100);
    for (int y = -100; y <= 100; y++)
      painter->drawLine(-100, y,
                        100, y);
  }

  painter->setPen(axisPen);

  painter->drawLine(0, -100,
                    0, 100);
  painter->drawLine(-100, 0,
                    100, 0);
}


// end of grid.cpp
