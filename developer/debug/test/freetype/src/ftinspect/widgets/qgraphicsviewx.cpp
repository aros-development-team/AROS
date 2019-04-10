// qgraphicsviewx.hpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#include "qgraphicsviewx.hpp"

#include <QScrollBar>


QGraphicsViewx::QGraphicsViewx()
: lastBottomLeftPointInitialized(false)
{
  // empty
}


void
QGraphicsViewx::scrollContentsBy(int dx,
                                 int dy)
{
  QGraphicsView::scrollContentsBy(dx, dy);
  lastBottomLeftPoint = viewport()->rect().bottomLeft();
}


void
QGraphicsViewx::resizeEvent(QResizeEvent* event)
{
  QGraphicsView::resizeEvent(event);

  // XXX I don't know how to properly initialize this value,
  //     thus the hack with the boolean
  if (!lastBottomLeftPointInitialized)
  {
    lastBottomLeftPoint = viewport()->rect().bottomLeft();
    lastBottomLeftPointInitialized = true;
  }

  QPointF currentBottomLeftPoint = viewport()->rect().bottomLeft();
  int verticalPosition = verticalScrollBar()->value();
  verticalScrollBar()->setValue(static_cast<int>(
                                  verticalPosition
                                  - (currentBottomLeftPoint.y()
                                     - lastBottomLeftPoint.y())));
}


// end of qgraphicsviewx.cpp
