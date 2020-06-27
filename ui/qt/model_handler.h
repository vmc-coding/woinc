/* ui/qt/model_handler.h --
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

#ifndef WOINC_UI_QT_MODEL_HANDLER_H_
#define WOINC_UI_QT_MODEL_HANDLER_H_

#include <map>
#include <string>

#include <QString>
#include <QVariant>

#include <woinc/types.h>

#include "qt/model.h"

namespace woinc { namespace ui { namespace qt {

class ModelHandler : public Model {
    Q_OBJECT

    public:
        ModelHandler(QObject *parent = nullptr);
        virtual ~ModelHandler() = default;

        ModelHandler(const ModelHandler&) = delete;
        ModelHandler(ModelHandler &&) = delete;

        ModelHandler &operator=(const ModelHandler&) = delete;
        ModelHandler &operator=(ModelHandler &&) = delete;

    public slots: // to be threadsafe without using a mutex, these methods must never be called directly!
        void add_host(QString host);
        void remove_host(QString host);

        void update_cc_status(QString host, woinc::CCStatus cc_status);
        void update_client_state(QString host, woinc::ClientState client_state);
        void update_disk_usage(QString host, woinc::DiskUsage disk_usage);
        void update_file_transfers(QString host, woinc::FileTransfers file_transfers);
        void update_notices(QString host, woinc::Notices notices, bool refreshed);
        void update_messages(QString host, woinc::Messages messages);
        void update_projects(QString host, woinc::Projects projects);
        void update_statistics(QString host, woinc::Statistics statistics);
        void update_tasks(QString host, woinc::Tasks tasks);

        void select_host(QString host);
        void unselect_host(QString host);

    signals:
        void disk_usage_update_needed(QString host);
        void projects_update_needed(QString host);
        void state_update_needed(QString host);
        void statistics_update_needed(QString host);
        void tasks_update_needed(QString host);

    private:
        struct HostModel {
            const QString host;
            AppVersions app_versions;
            Apps apps;
            Projects projects;
            Tasks tasks;
            Workunits wus;
            woinc::CCStatus cc_status;

            explicit HostModel(const QString &host);

            QVariant resolve_app_name_by_wu(const QString &wu_name) const;
            QVariant resolve_project_name_by_url(const std::string &master_url) const;
        };

        void select_host_(const QString &host);
        void unselect_host_(const QString &host);

        const HostModel &find_host_model_(const QString &host) const;
        HostModel &find_host_model_(const QString &host);

    private:
        Apps          map_(woinc::Apps apps);
        AppVersions   map_(woinc::AppVersions app_versions);
        QVariant      map_(woinc::DiskUsage disk_usage, const HostModel &host_model);
        FileTransfers map_(woinc::FileTransfers file_transfers, const HostModel &host_model);
        Events        map_(woinc::Messages messages);
        Notices       map_(woinc::Notices notices);
        Workunits     map_(woinc::Workunits workunits);
        Projects      map_(woinc::Projects projects);
        RunModes      map_(const woinc::CCStatus &cc_status);
        QVariant      map_(woinc::Statistics statistics, const HostModel &host_model);
        QVariant      map_(woinc::Tasks tasks, const HostModel &host_model);

    private:
        QString selected_host_;
        std::map<QString, HostModel> host_models_;
};

}}}

#endif
