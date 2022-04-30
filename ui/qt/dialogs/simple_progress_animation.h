/* ui/qt/dialogs/simple_progress_animation.h --
   Written and Copyright (C) 2021-2022 by vmc.

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

#ifndef WOINC_UI_QT_SIMPLE_PROGRESS_ANIMATION_H_

#include <QString>
#include <QWidget>


struct QLabel;
struct QTimer;

namespace woinc { namespace ui { namespace qt {

class SimpleProgressAnimation : public QWidget {
    Q_OBJECT

    public:
        SimpleProgressAnimation(QWidget *parent = nullptr);
        virtual ~SimpleProgressAnimation();

        void start(QString base_msg);
        void stop();

    private:
        QTimer *timer_;
        QString base_msg_;
        int counter_;
        QLabel *label_;
};

}}}

#endif
