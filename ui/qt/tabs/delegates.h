/* ui/qt/tabs/delegates.h --
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

#ifndef WOINC_UI_QT_TABS_DELEGATES_H_
#define WOINC_UI_QT_TABS_DELEGATES_H_

#include <QStyledItemDelegate>

#include "qt/types.h"

namespace woinc { namespace ui { namespace qt {

class ProgressBarDelegate : public QStyledItemDelegate {
    Q_OBJECT

    public:
        ProgressBarDelegate(QWidget *parent = nullptr, int precision = 3);

        void paint(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    private:
        int precision_;

};

class ResourceShareBarDelegate : public QStyledItemDelegate {
    Q_OBJECT

    public:
        ResourceShareBarDelegate(QWidget *parent = nullptr);

        void paint(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

class HtmlEntityDelegate : public QStyledItemDelegate {
    Q_OBJECT

    public:
        HtmlEntityDelegate(QWidget *parent = nullptr);

        void paint(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

}}}

#endif
