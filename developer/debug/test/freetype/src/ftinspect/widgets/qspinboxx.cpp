// qspinboxx.cpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#include "qspinboxx.hpp"


// we want to mark the center of a pixel square with a single dot or a small
// cross; starting with a certain magnification we thus only use even values
// so that we can do that symmetrically

int
QSpinBoxx::valueFromText(const QString& text) const
{
  int val = QSpinBox::valueFromText(text);

  if (val > 640)
    val = val - (val % 64);
  else if (val > 320)
    val = val - (val % 32);
  else if (val > 160)
    val = val - (val % 16);
  else if (val > 80)
    val = val - (val % 8);
  else if (val > 40)
    val = val - (val % 4);
  else if (val > 20)
    val = val - (val % 2);

  return val;
}


void
QSpinBoxx::stepBy(int steps)
{
  int val = value();

  if (steps > 0)
  {
    for (int i = 0; i < steps; i++)
    {
      if (val >= 640)
        val = val + 64;
      else if (val >= 320)
        val = val + 32;
      else if (val >= 160)
        val = val + 16;
      else if (val >= 80)
        val = val + 8;
      else if (val >= 40)
        val = val + 4;
      else if (val >= 20)
        val = val + 2;
      else
        val++;
    }
  }
  else if (steps < 0)
  {
    for (int i = 0; i < -steps; i++)
    {
      if (val > 640)
        val = val - 64;
      else if (val > 320)
        val = val - 32;
      else if (val > 160)
        val = val - 16;
      else if (val > 80)
        val = val - 8;
      else if (val > 40)
        val = val - 4;
      else if (val > 20)
        val = val - 2;
      else
        val--;
    }
  }

  setValue(val);
}


// end of qspinboxx.cpp
