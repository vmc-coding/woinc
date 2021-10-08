/* ui/qt/gui.h --
   Written and Copyright (C) 2017-2021 by vmc.

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

#ifndef WOINC_QT_GUI_H_
#define WOINC_QT_GUI_H_

#include <QMainWindow>

#include <woinc/ui/handler.h>

#include "qt/defs.h"

namespace woinc { namespace ui { namespace qt {

struct Controller;
struct Model;

class Gui : public QMainWindow {
    Q_OBJECT

    public:
        Gui() = default;
        virtual ~Gui() = default;

        Gui(const Gui &) = delete;
        Gui(Gui &&) = delete;

        Gui &operator=(const Gui &) = delete;
        Gui &operator=(Gui &&) = delete;

        void open(const Model &model, Controller &controller);

        void keyPressEvent(QKeyEvent *event) final;

    public slots:
        void show_info(QString title, QString message);
        void show_warning(QString title, QString message);
        void show_error(QString title, QString message);

    private:
        void create_file_menu_(const Model &model, Controller &controller);
        void create_view_menu_();
        void create_activity_menu_(const Model &model, Controller &controller);
        void create_options_menu_(const Model &model, Controller &controller);
        void create_tools_menu_(const Model &model, Controller &controller);
        void create_help_menu_();

    signals:
        void quit();
        void current_tab_to_be_changed(TAB tab);
        void computer_selected(QString host, QString url, unsigned short port, QString password);
};

}}}

#endif
