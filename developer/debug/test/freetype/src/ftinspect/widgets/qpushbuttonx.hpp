// qpushbuttonx.hpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#pragma once

#include <QPushButton>


// we want buttons that are horizontally as small as possible
class QPushButtonx
: public QPushButton
{
  Q_OBJECT

public:
  QPushButtonx(const QString& text,
               QWidget* = 0);
  virtual ~QPushButtonx(){}
};


// end of qpushbuttonx.hpp
