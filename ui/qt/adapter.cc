/* ui/qt/adapter.cc --
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

#include "qt/adapter.h"

#include <woinc/ui/handler.h>

namespace woinc { namespace ui { namespace qt {

HandlerAdapter::HandlerAdapter(QObject *parent) : QObject(parent) {}

void HandlerAdapter::on_host_added(const std::string &host) {
    emit added(QString::fromStdString(host));
}

void HandlerAdapter::on_host_removed(const std::string &host) {
    emit removed(QString::fromStdString(host));
}

void HandlerAdapter::on_host_connected(const std::string &host) {
    emit connected(QString::fromStdString(host));
}

void HandlerAdapter::on_host_authorized(const std::string &host) {
    emit authorized(QString::fromStdString(host));
}

void HandlerAdapter::on_host_authorization_failed(const std::string &host) {
    emit authorization_failed(QString::fromStdString(host));
}

void HandlerAdapter::on_host_error(const std::string &host, Error error) {
    emit error_occurred(QString::fromStdString(host), error);
}

void HandlerAdapter::on_update(const std::string &host, const woinc::CCStatus &value) {
    emit updated_cc_status(QString::fromStdString(host), value);
}

void HandlerAdapter::on_update(const std::string &host, const woinc::ClientState &value) {
    emit updated_client_state(QString::fromStdString(host), value);
}

void HandlerAdapter::on_update(const std::string &host, const woinc::DiskUsage &value) {
    emit updated_disk_usage(QString::fromStdString(host), value);
}

void HandlerAdapter::on_update(const std::string &host, const woinc::FileTransfers &value) {
    emit updated_file_transfers(QString::fromStdString(host), value);
}

void HandlerAdapter::on_update(const std::string &host, const woinc::Notices &notices, bool refreshed) {
    emit updated_notices(QString::fromStdString(host), notices, refreshed);
}

void HandlerAdapter::on_update(const std::string &host, const woinc::Messages &value) {
    emit updated_messages(QString::fromStdString(host), value);
}

void HandlerAdapter::on_update(const std::string &host, const woinc::Projects &value) {
    emit updated_projects(QString::fromStdString(host), value);
}

void HandlerAdapter::on_update(const std::string &host, const woinc::Statistics &value) {
    emit updated_statistics(QString::fromStdString(host), value);
}

void HandlerAdapter::on_update(const std::string &host, const woinc::Tasks &value) {
    emit updated_tasks(QString::fromStdString(host), value);
}

}}}
