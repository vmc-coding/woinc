/* ui/qt/dialogs/event_log_options_dialog.h --
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

#ifndef WOINC_UI_QT_EVENT_LOG_OPTIONS_DIALOG_H_

#include <QDialog>
#include <QString>

#include <woinc/types.h>

struct QWidget;

namespace woinc { namespace ui { namespace qt {

struct SimpleProgressAnimation;

class EventLogOptionsDialog : public QDialog {
    Q_OBJECT

    public:
        EventLogOptionsDialog(QWidget *parent = nullptr);
        virtual ~EventLogOptionsDialog() = default;

        EventLogOptionsDialog(const EventLogOptionsDialog&) = delete;
        EventLogOptionsDialog(EventLogOptionsDialog &&) = delete;

        EventLogOptionsDialog &operator=(const EventLogOptionsDialog&) = delete;
        EventLogOptionsDialog &operator=(EventLogOptionsDialog &&) = delete;

    signals:
        void apply(woinc::LogFlags log_flags);

        // at the moment we need the dialog to show the progress animation,
        // so the dialog will not close itself after sending this signal!
        void save(woinc::LogFlags log_flags);

    public slots:
        void update(woinc::LogFlags log_flags);

        void show_progress_animation(QString base_msg);
        void stop_progress_animation();

    private:
        SimpleProgressAnimation *progress_animation_ = nullptr;
        woinc::LogFlags log_flags_;
};

}}}

#endif
