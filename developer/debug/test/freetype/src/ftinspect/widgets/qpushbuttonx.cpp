// qpushbuttonx.cpp

// Copyright (C) 2016-2017 by Werner Lemberg.


#include "qpushbuttonx.hpp"

#include <QStyleOptionButton>


// code derived from Qt 4.8.7, function `QPushButton::sizeHint',
// file `src/gui/widgets/qpushbutton.cpp'

QPushButtonx::QPushButtonx(const QString &text,
                           QWidget *parent)
: QPushButton(text, parent)
{
  QStyleOptionButton opt;
  opt.initFrom(this);
  QString s(this->text());
  QFontMetrics fm = fontMetrics();
  QSize sz = fm.size(Qt::TextShowMnemonic, s);
  opt.rect.setSize(sz);

  sz = style()->sizeFromContents(QStyle::CT_PushButton,
                                 &opt,
                                 sz,
                                 this);
  setFixedWidth(sz.width());
}


// end of qpushbuttonx.cpp
