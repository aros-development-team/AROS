// qcomboboxx.hpp

// Copyright (C) 2016-2017 by Werner Lemberg.


#pragma once

#include <QComboBox>


// we want to grey out items in a combo box;
// since Qt doesn't provide a function for this we derive a class
class QComboBoxx
: public QComboBox
{
  Q_OBJECT

public:
  void setItemEnabled(int index,
                      bool enable);
};


// end of qcomboboxx.hpp
