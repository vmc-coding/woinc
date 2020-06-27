/* ui/qt/dialogs/select_computer_dialog.cc --
   Written and Copyright (C) 2020 by vmc.

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

#include "qt/dialogs/select_computer_dialog.h"

#include <QGridLayout>
#include <QMessageBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

#include "qt/defs.h"

namespace woinc { namespace ui { namespace qt {

SelectComputerDialog::SelectComputerDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("woincqt - Select computer"));

    auto *lbl_host = new QLabel(QString::fromUtf8("Host name:"));
    lbl_host->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    auto *lbl_pwd = new QLabel(QString::fromUtf8("Password:"));
    lbl_pwd->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    auto *in_host = new QLineEdit();
    in_host->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    auto *in_pwd = new QLineEdit();
    in_pwd->setEchoMode(QLineEdit::Password);
    in_pwd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    auto *btn_ok = new QPushButton(QString::fromUtf8("OK"));
    btn_ok->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(btn_ok, &QPushButton::released, [=]() {
        auto tokens = in_host->text().trimmed().split(':');
        auto host = tokens.isEmpty() ? QString() : tokens[0].trimmed();

        if (host.isEmpty()) {
            done(QDialog::Accepted); // if no host is given, just do nothing
        } else if (tokens.size() > 2) {
            QMessageBox::critical(this,
                                  QString::fromUtf8("Error"),
                                  QString::fromUtf8("Invalid host (more than one colon found)."),
                                  QMessageBox::Ok);
        } else {
            bool parsed = true;
            unsigned short port = DEFAULT_PORT;

            if (tokens.size() == 2)
                port = tokens[1].toUShort(&parsed);

            if (!parsed) {
                QMessageBox::critical(this,
                                      QString::fromUtf8("Error"),
                                      QString::fromUtf8("Invalid port."),
                                      QMessageBox::Ok);
            } else {
                emit computer_selected(host, host, port, in_pwd->text());
                done(QDialog::Accepted);
            }
        }
    });

    auto *btn_cancel = new QPushButton(QString::fromUtf8("Cancel"));
    btn_cancel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(btn_cancel, &QPushButton::released, this, &SelectComputerDialog::reject);

    auto *lyt = new QGridLayout;

    lyt->addWidget(lbl_host, 1, 1);
    lyt->addWidget(in_host, 1, 2);
    lyt->addWidget(btn_ok, 1, 3);

    lyt->addWidget(lbl_pwd, 2, 1);
    lyt->addWidget(in_pwd, 2, 2);
    lyt->addWidget(btn_cancel, 2, 3);

    setLayout(lyt);
}


}}}
