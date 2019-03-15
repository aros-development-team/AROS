// glyphpointnumbers.cpp

// Copyright (C) 2016-2017 by Werner Lemberg.


#include "glyphpointnumbers.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QVector2D>


GlyphPointNumbers::GlyphPointNumbers(const QPen& onP,
                                     const QPen& offP,
                                     FT_Outline* outln)
: onPen(onP),
  offPen(offP),
  outline(outln)
{
  FT_BBox cbox;

  FT_Outline_Get_CBox(outline, &cbox);

  // XXX fix bRect size
  bRect.setCoords(qreal(cbox.xMin) / 64,
                  -qreal(cbox.yMax) / 64,
                  qreal(cbox.xMax) / 64,
                  -qreal(cbox.yMin) / 64);
}


QRectF
GlyphPointNumbers::boundingRect() const
{
  return bRect;
}


void
GlyphPointNumbers::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget*)
{
  const qreal lod = option->levelOfDetailFromTransform(
                              painter->worldTransform());

  // don't draw point numbers if magnification is too small
  if (lod >= 10)
  {
    QFont font = painter->font();

    // the following doesn't work correctly with scaling;
    // it seems that Qt doesn't allow arbitrarily small font sizes
    // that get magnified later on
#if 0
    // we want the same text size regardless of the scaling
    font.setPointSizeF(font.pointSizeF() / lod);
    painter->setFont(font);
#else
    font.setPointSizeF(font.pointSizeF() * 3 / 4);
    painter->setFont(font);

    QBrush onBrush(onPen.color());
    QBrush offBrush(offPen.color());

    painter->scale(1 / lod, 1 / lod);
#endif

    FT_Vector* points = outline->points;
    FT_Short* contours = outline->contours;
    char* tags = outline->tags;

    QVector2D octants[8] = { QVector2D(1, 0),
                             QVector2D(0.707f, -0.707f),
                             QVector2D(0, -1),
                             QVector2D(-0.707f, -0.707f),
                             QVector2D(-1, 0),
                             QVector2D(-0.707f, 0.707f),
                             QVector2D(0, 1),
                             QVector2D(0.707f, 0.707f) };


    short ptIdx = 0;
    for (int contIdx = 0; contIdx < outline->n_contours; contIdx++ )
    {
      for (;;)
      {
        short prevIdx, nextIdx;

        // find previous and next point in outline
        if (contIdx == 0)
        {
          if (contours[contIdx] == 0)
          {
            prevIdx = 0;
            nextIdx = 0;
          }
          else
          {
            prevIdx = ptIdx > 0 ? ptIdx - 1
                                : contours[contIdx];
            nextIdx = ptIdx < contours[contIdx] ? ptIdx + 1
                                                : 0;
          }
        }
        else
        {
          prevIdx = ptIdx > (contours[contIdx - 1] + 1) ? ptIdx - 1
                                                        : contours[contIdx];
          nextIdx = ptIdx < contours[contIdx] ? ptIdx + 1
                                              : contours[contIdx - 1] + 1;
        }

        // get vectors to previous and next point and normalize them;
        QVector2D in(static_cast<float>(points[prevIdx].x
                                        - points[ptIdx].x) / 64,
                     -static_cast<float>(points[prevIdx].y
                                         - points[ptIdx].y) / 64);
        QVector2D out(static_cast<float>(points[nextIdx].x
                                         - points[ptIdx].x) / 64,
                      -static_cast<float>(points[nextIdx].y
                                          - points[ptIdx].y) / 64);

        in = in.normalized();
        out = out.normalized();

        QVector2D middle = in + out;
        // check whether vector is very small, using a threshold of 1/8px
        if (qAbs(middle.x()) < 1.0f / 8
            && qAbs(middle.y()) < 1.0f / 8)
        {
          // in case of vectors in almost exactly opposite directions,
          // use a vector orthogonal to them
          middle.setX(out.y());
          middle.setY(-out.x());

          if (qAbs(middle.x()) < 1.0f / 8
              && qAbs(middle.y()) < 1.0f / 8)
          {
            // use direction based on point index for the offset
            // if we still don't have a good value
            middle = octants[ptIdx % 8];
          }
        }

        // normalize `middle' vector (which is never zero),
        // then multiply by 8 to get some distance between
        // the point and the number
        middle = middle.normalized() * 8;

        // we now position the point number in the opposite
        // direction of the `middle' vector,
        QString number = QString::number(ptIdx);

#if 0
        // this fails, see comment above
        int size = 10000;
        qreal x = qreal(points[ptIdx].x) / 64 - middle.x() / lod;
        qreal y = -qreal(points[ptIdx].y) / 64 - middle.y() / lod;
        QPointF corner(x, y);
        int flags = middle.x() > 0 ? Qt::AlignRight
                                   : Qt::AlignLeft;
        if (flags == Qt::AlignRight)
          corner.rx() -= size;
        QRectF posRect(corner, QSizeF(size, size));

        if (tags[ptIdx] & FT_CURVE_TAG_ON)
          painter->setPen(onPen);
        else
          painter->setPen(offPen);

        painter->drawText(posRect, flags, number);
#else
        // convert text string to a path object
        QPainterPath path;
        path.addText(QPointF(0, 0), font, number);
        QRectF ctrlPtRect = path.controlPointRect();

        qreal x = static_cast<qreal>(points[ptIdx].x) / 64 * lod
                  - static_cast<qreal>(middle.x());
        qreal y = -static_cast<qreal>(points[ptIdx].y) / 64 * lod
                  - static_cast<qreal>(middle.y());

        qreal heuristicOffset = 2;
        if (middle.x() > 0)
          path.translate(x - ctrlPtRect.width() - heuristicOffset,
                         y + ctrlPtRect.height() / 2);
        else
          path.translate(x,
                         y + ctrlPtRect.height() / 2);

        painter->fillPath(path,
                          tags[ptIdx] & FT_CURVE_TAG_ON ? onBrush
                                                        : offBrush);
#endif

        ptIdx++;
        if (ptIdx > contours[contIdx])
          break;
      }
    }
  }
}


// end of glyphpointnumbers.cpp
