/* ui/qt/tabs_widget.h --
   Written and Copyright (C) 2017-2020 by vmc.

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

#ifndef WOINC_UI_QT_TABS_WIDGET_H_
#define WOINC_UI_QT_TABS_WIDGET_H_

#include <map>

#include <QTabWidget>

#include "qt/defs.h"

namespace woinc { namespace ui { namespace qt {

struct Controller;
struct Model;

class TabsWidget : public QTabWidget {
    Q_OBJECT

    public:
        TabsWidget(const Model &model, Controller &controller, QWidget *parent = nullptr);

    public slots:
        void switch_to_tab(TAB tab);

    private:
        std::map<TAB, int> tab_indices_;
};

}}}

#endif
