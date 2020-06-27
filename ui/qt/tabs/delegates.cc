/* ui/qt/tabs/delegates.cc --
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

#include "qt/tabs/delegates.h"

#include <cassert>

#include <QPainter>
#include <QPair>
#include <QProgressBar>
#include <QTextDocument>

namespace woinc { namespace ui { namespace qt {

// ----- ProgressBarDelegate -----

ProgressBarDelegate::ProgressBarDelegate(QWidget *parent, int precision)
    : QStyledItemDelegate(parent), precision_(precision)
{}

void ProgressBarDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const {
    if (!index.isValid())
        return;

    bool ok;
    double prct = index.data().toDouble(&ok);

    if (ok) {
        prct *= 100;

        QProgressBar bar;

        bar.resize(option.rect.size());

        bar.setMinimum(0);
        bar.setMaximum(100);
        bar.setValue(static_cast<int>(prct));

        bar.setTextVisible(true);
        bar.setFormat(QString::asprintf("%.*f%%", precision_, prct));

        painter->save();
        painter->translate(option.rect.topLeft());
        bar.render(painter);
        painter->restore();
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

// ----- ResourceShareBarDelegate -----

ResourceShareBarDelegate::ResourceShareBarDelegate(QWidget *parent)
    : QStyledItemDelegate(parent)
{}

void ResourceShareBarDelegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    if (!index.isValid())
        return;

#ifndef NDEBUG
    bool can_convert = index.data().canConvert<QPair<double, double>>();
    assert(can_convert);
#endif
    auto resource_share = index.data().value<QPair<double, double>>();

    QProgressBar bar;

    bar.resize(option.rect.size());

    bar.setMinimum(0);
    bar.setMaximum(static_cast<int>(resource_share.second));
    bar.setAlignment(Qt::AlignRight);
    bar.setTextVisible(true);

    assert(resource_share.second > 0);

    bar.setValue(static_cast<int>(resource_share.first));
    bar.setFormat(QString::asprintf("%i (%.2f%%)",
                                    static_cast<int>(resource_share.first),
                                    100*resource_share.first/resource_share.second));

    painter->save();
    painter->translate(option.rect.topLeft());
    bar.render(painter);
    painter->restore();
}

// ----- HtmlEntityDelegate -----

HtmlEntityDelegate::HtmlEntityDelegate(QWidget *parent)
    : QStyledItemDelegate(parent)
{}

void HtmlEntityDelegate::paint(QPainter *painter,
                               const QStyleOptionViewItem &opt,
                               const QModelIndex &index) const {
    if (!index.isValid())
        return;

    QStyleOptionViewItem option = opt;
    initStyleOption(&option, index);

    painter->save();

    QTextDocument doc;
    doc.setHtml(option.text);

    option.text = "";
    option.widget->style()->drawControl(QStyle::CE_ItemViewItem, &option, painter);

    painter->translate(option.rect.left(), option.rect.top() + (option.rect.height() - doc.size().height()) / 2);
    QRect clip(0, 0, option.rect.width(), option.rect.height());
    doc.drawContents(painter, clip);

    painter->restore();
}

QSize HtmlEntityDelegate::sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const {
    QStyleOptionViewItem option = opt;
    initStyleOption(&option, index);

    QTextDocument doc;
    doc.setHtml(option.text);
    doc.setTextWidth(option.rect.width());

    return QSize(static_cast<int>(doc.idealWidth()), static_cast<int>(doc.size().height()));
}

}}}
