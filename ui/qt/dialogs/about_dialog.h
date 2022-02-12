/* ui/qt/dialogs/about_dialog.h --
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


#ifndef WOINC_UI_QT_ABOUT_DIALOG_H_
#define WOINC_UI_QT_ABOUT_DIALOG_H_

#include <QDialog>

namespace woinc { namespace ui { namespace qt {

class AboutDialog : public QDialog {
    Q_OBJECT

    public:
        AboutDialog(QWidget *parent = nullptr);
        virtual ~AboutDialog() = default;

        AboutDialog(const AboutDialog&) = delete;
        AboutDialog(AboutDialog &&) = delete;

        AboutDialog &operator=(const AboutDialog&) = delete;
        AboutDialog &operator=(AboutDialog &&) = delete;
};

}}}

#endif
