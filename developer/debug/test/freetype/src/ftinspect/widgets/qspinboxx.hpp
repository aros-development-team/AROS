// qspinboxx.hpp

// Copyright (C) 2016-2017 by Werner Lemberg.


#pragma once

#include <QSpinBox>
#include <QString>


// we want to have our own `stepBy' function for the zoom spin box
class QSpinBoxx
: public QSpinBox
{
  Q_OBJECT

public:
  void stepBy(int val);
  int valueFromText(const QString& text) const;
};


// qspinboxx.hpp
