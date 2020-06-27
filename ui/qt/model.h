/* ui/qt/model.h --
   Written and Copyright (C) 2017-2019 by vmc.

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

#ifndef WOINC_UI_QT_MODEL_H_
#define WOINC_UI_QT_MODEL_H_

#include <QObject>
#include <QString>

#include "qt/types.h"

namespace woinc { namespace ui { namespace qt {

class Model : public QObject {
    Q_OBJECT

    public:
        Model(QObject *parent = nullptr);
        virtual ~Model() = default;

        Model(const Model&) = delete;
        Model(Model &&) = delete;

        Model &operator=(const Model&) = delete;
        Model &operator=(Model &&) = delete;

    signals:
        void host_selected(QString host);
        void host_unselected(QString host);

        void disk_usage_updated(woinc::ui::qt::DiskUsage disk_usage);
        void events_appended(woinc::ui::qt::Events events);
        void file_transfers_updated(woinc::ui::qt::FileTransfers transfers);
        void notices_appended(woinc::ui::qt::Notices notices);
        void notices_refreshed(woinc::ui::qt::Notices notices);
        void projects_updated(woinc::ui::qt::Projects projects);
        void run_modes_updated(woinc::ui::qt::RunModes run_modes);
        void statistics_updated(woinc::ui::qt::Statistics statistics);
        void tasks_updated(woinc::ui::qt::Tasks tasks);

};

}}}

#endif
