// qgraphicsviewx.hpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#pragma once

#include <QGraphicsView>


// we want to anchor the view at the bottom left corner
// while the windows gets resized
class QGraphicsViewx
: public QGraphicsView
{
  Q_OBJECT

public:
  QGraphicsViewx();

protected:
  void resizeEvent(QResizeEvent* event);
  void scrollContentsBy(int dx,
                        int dy);

private:
  QPointF lastBottomLeftPoint;
  bool lastBottomLeftPointInitialized;
};


// end of qgraphicsviewx.hpp
