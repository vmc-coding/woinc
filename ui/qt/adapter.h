/* ui/qt/adapter.h --
   Written and Copyright (C) 2019 by vmc.

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

#ifndef WOINC_UI_QT_HANDLER_ADAPTER
#define WOINC_UI_QT_HANDLER_ADAPTER

#include <QMetaType>
#include <QObject>
#include <QString>

#include <woinc/ui/handler.h>

Q_DECLARE_METATYPE(woinc::ui::Error)

Q_DECLARE_METATYPE(woinc::CCStatus)
Q_DECLARE_METATYPE(woinc::ClientState)
Q_DECLARE_METATYPE(woinc::DiskUsage)
Q_DECLARE_METATYPE(woinc::FileTransfers)
Q_DECLARE_METATYPE(woinc::Notices)
Q_DECLARE_METATYPE(woinc::Messages)
Q_DECLARE_METATYPE(woinc::Projects)
Q_DECLARE_METATYPE(woinc::Statistics)
Q_DECLARE_METATYPE(woinc::Tasks)

namespace woinc { namespace ui { namespace qt {

class HandlerAdapter : public QObject, public woinc::ui::HostHandler, public woinc::ui::PeriodicTaskHandler {
    Q_OBJECT

    public:
        HandlerAdapter(QObject *parent = nullptr);

    public: // HostHandler
        void on_host_added(const std::string &host) final;
        void on_host_removed(const std::string &host) final;

        void on_host_connected(const std::string &host) final;

        void on_host_authorized(const std::string &host) final;
        void on_host_authorization_failed(const std::string &host) final;

        void on_host_error(const std::string &host, Error error) final;

    public: // PeriodicTaskHandler
        void on_update(const std::string &host, const woinc::CCStatus &cc_status) final;
        void on_update(const std::string &host, const woinc::ClientState &client_state) final;
        void on_update(const std::string &host, const woinc::DiskUsage &disk_usage) final;
        void on_update(const std::string &host, const woinc::FileTransfers &file_transfers) final;
        void on_update(const std::string &host, const woinc::Notices &notices, bool refreshed) final;
        void on_update(const std::string &host, const woinc::Messages &messages) final;
        void on_update(const std::string &host, const woinc::Projects &projects) final;
        void on_update(const std::string &host, const woinc::Statistics &statistics) final;
        void on_update(const std::string &host, const woinc::Tasks &tasks) final;

    signals:
        void added(QString host);
        void removed(QString host);

        void connected(QString host);

        void authorized(QString host);
        void authorization_failed(QString host);

        void error_occurred(QString host, woinc::ui::Error error);

        void updated_cc_status(QString host, woinc::CCStatus cc_status);
        void updated_client_state(QString host, woinc::ClientState client_state);
        void updated_disk_usage(QString host, woinc::DiskUsage disk_usage);
        void updated_file_transfers(QString host, woinc::FileTransfers file_transfers);
        void updated_notices(QString host, woinc::Notices notices, bool refreshed);
        void updated_messages(QString host, woinc::Messages messages);
        void updated_projects(QString host, woinc::Projects projects);
        void updated_statistics(QString host, woinc::Statistics statistics);
        void updated_tasks(QString host, woinc::Tasks tasks);
};

}}}

#endif
