/* ui/qt/tabs/proxy_models.cc --
   Written and Copyright (C) 2018-2019 by vmc.

   This file is part of woinc.

   woinc is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   woinc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with woinc. If not, see <http://www.gnu.org/licenses/>. */

#include "qt/tabs/proxy_models.h"

#include <QBrush>

namespace woinc { namespace ui { namespace qt {

RowBackgroundProxyModel::RowBackgroundProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent) {}

QVariant RowBackgroundProxyModel::data(const QModelIndex &index, int role) const {
    if (index.isValid() && role == Qt::BackgroundRole) {
        if (index.row() % 2 != 0) {
            return QBrush(QColor(240, 240, 240, 255));
        } else {
            return QBrush(QColor(255, 255, 255, 255));
        }
    } else {
        return QSortFilterProxyModel::data(index, role);
    }
}

}}}
