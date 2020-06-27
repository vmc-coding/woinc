/* ui/qt/tabs/proxy_models.h --
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

#ifndef WOINC_UI_QT_TABS_PROXY_MODELS_H_
#define WOINC_UI_QT_TABS_PROXY_MODELS_H_

#include <QSortFilterProxyModel>

namespace woinc { namespace ui { namespace qt {

// set/toggle background color for rows;
// we use a proxy model to get a proper coloring even on sortable tables
// by cascading proxy models like model->sorting proxy->this proxy
class RowBackgroundProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

    public:
        RowBackgroundProxyModel(QObject *parent = nullptr);
        virtual ~RowBackgroundProxyModel() = default;

        // returns background color for role BackgroundRole, calls the super class otherwise
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

}}}

#endif
