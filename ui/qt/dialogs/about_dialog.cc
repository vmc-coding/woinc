/* ui/qt/dialogs/about_dialog.cc --
   Written and Copyright (C) 2022 by vmc.

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

#include "qt/dialogs/about_dialog.h"

#ifndef NDEBUG
#include <iostream>
#endif

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QStringLiteral>
#include <QVBoxLayout>
#include <QtGlobal>

#include <woinc/version.h>

namespace {

void __add_line(QGridLayout *grid, const QString &txt) {
    auto *lbl = new QLabel(txt);
    lbl->setAlignment(Qt::AlignHCenter);
    lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    lbl->setOpenExternalLinks(true);
    grid->addWidget(lbl, grid->rowCount(), 0, 1, 2);
}

void __add_line(QGridLayout *grid, const QString &lhs, const QString &rhs) {
    auto *lbl_lhs = new QLabel(lhs);
    lbl_lhs->setAlignment(Qt::AlignRight);
    lbl_lhs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    grid->addWidget(lbl_lhs, grid->rowCount(), 0);

    auto *lbl_rhs = new QLabel(rhs);
    lbl_rhs->setAlignment(Qt::AlignLeft);
    lbl_rhs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    lbl_rhs->setOpenExternalLinks(true);
    grid->addWidget(lbl_rhs, grid->rowCount() - 1, 1);
}

}

namespace woinc { namespace ui { namespace qt {

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent, Qt::Dialog)
{
    setWindowTitle(QStringLiteral("About woincqt"));

    auto *grid = new QGridLayout;
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    setLayout(grid);

    __add_line(grid, QStringLiteral("<h1>woincqt</h1>"));
    __add_line(grid, QStringLiteral("Version:"), QString::asprintf("%d.%d", major_version(), minor_version()));
    __add_line(grid, QStringLiteral("QT version:"), qVersion());
    __add_line(grid, QStringLiteral("Copyright:"), QStringLiteral("(C) 2017-2022 by <a href=\"mailto:vmc.coding@gmail.com\">vmc</>"));
    __add_line(grid, QStringLiteral("woincqt is distributed under the GNU General Public License v3.0."));
    __add_line(grid, QStringLiteral("For more information, visit <a href=\"https://github.com/vmc-coding/woinc\">https://github.com/vmc-coding/woinc</a>"));

    auto *btn_ok = new QPushButton(QStringLiteral("OK"));
    btn_ok->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(btn_ok, &QPushButton::released, this, &AboutDialog::close);
    grid->addWidget(btn_ok, grid->rowCount(), 1, Qt::AlignRight);
}

}}}
