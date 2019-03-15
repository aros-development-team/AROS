// grid.hpp

// Copyright (C) 2016-2017 by Werner Lemberg.


#pragma once

#include <QGraphicsItem>
#include <QPen>


class Grid
: public QGraphicsItem
{
public:
  Grid(const QPen& gridPen,
       const QPen& axisPen);
  QRectF boundingRect() const;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget);

private:
  QPen gridPen;
  QPen axisPen;
};


// end of grid.hpp
