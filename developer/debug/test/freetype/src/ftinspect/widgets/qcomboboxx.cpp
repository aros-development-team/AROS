// qcomboboxx.cpp

// Copyright (C) 2016-2017 by Werner Lemberg.


#include "qcomboboxx.hpp"

#include <QStandardItemModel>


void
QComboBoxx::setItemEnabled(int index,
                           bool enable)
{
  const QStandardItemModel* itemModel =
    qobject_cast<const QStandardItemModel*>(model());
  QStandardItem* item = itemModel->item(index);

  if (enable)
  {
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(QVariant(),
                  Qt::TextColorRole);
  }
  else
  {
    item->setFlags(item->flags()
                   & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
    // clear item data in order to use default color;
    // this visually greys out the item
    item->setData(palette().color(QPalette::Disabled, QPalette::Text),
                  Qt::TextColorRole);
  }
}


// end of qcomboboxx.cpp
