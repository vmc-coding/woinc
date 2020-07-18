/* ui/qt/controller.cc --
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

#include "qt/controller.h"

#include <algorithm>
#include <cassert>

#include <woinc/ui/controller.h>

#include "qt/adapter.h"

#define WOINC_LOCK_GUARD std::lock_guard<decltype(lock_)> guard(lock_)

namespace woinc { namespace ui { namespace qt {

Controller::Controller(QObject *parent)
    : QObject(parent)
    , ctrl_(new woinc::ui::Controller)
{}

Controller::~Controller() = default;

void Controller::register_handler(HostHandler *handler){
    ctrl_->register_handler(handler);
}

void Controller::deregister_handler(HostHandler *handler){
    ctrl_->deregister_handler(handler);
}


void Controller::register_handler(PeriodicTaskHandler *handler){
    ctrl_->register_handler(handler);
}

void Controller::deregister_handler(PeriodicTaskHandler *handler){
    ctrl_->deregister_handler(handler);
}


std::future<GlobalPreferences> Controller::load_global_prefs(const QString &host, GET_GLOBAL_PREFS_MODE mode) {
    return ctrl_->load_global_preferences(host.toStdString(), mode);
}

std::future<bool> Controller::save_global_prefs(const QString &host,
                                                const GlobalPreferences &prefs,
                                                const GlobalPreferencesMask &mask) {
    return ctrl_->save_global_preferences(host.toStdString(), prefs, mask);
}

std::future<bool> Controller::read_global_prefs(const QString &host) {
    return ctrl_->read_global_prefs_override(host.toStdString());
}

std::future<AllProjectsList> Controller::load_all_projects_list(const QString &host) {
    return ctrl_->all_projects_list(host.toStdString());
}

void Controller::add_host(QString host, QString url, unsigned short port, QString password) {
    {
        WOINC_LOCK_GUARD;
        pending_logins_.push_back({host, password});
    }

    try {
        ctrl_->add_host(host.toStdString(), url.toStdString(), port);
    } catch (const std::exception &e) {
        emit error_occurred("Error", e.what());
    }
}

void Controller::trigger_shutdown() {
    ctrl_->shutdown();
}

void Controller::do_active_only_tasks(QString host, bool value) {
    ctrl_->active_only_tasks(host.toStdString(), value);
}

void Controller::do_file_transfer_op(QString host, FILE_TRANSFER_OP op, QString project_url, QString filename) {
    ctrl_->file_transfer_op(host.toStdString(), op, project_url.toStdString(), filename.toStdString());
}

void Controller::do_project_op(QString host, QString project_url, PROJECT_OP op) {
    ctrl_->project_op(host.toStdString(), op, project_url.toStdString());
}

void Controller::do_task_op(QString host, QString project_url, QString name, TASK_OP op) {
    ctrl_->task_op(host.toStdString(), op, project_url.toStdString(), name.toStdString());
}

void Controller::set_gpu_mode(QString host, RUN_MODE mode) {
    ctrl_->gpu_mode(host.toStdString(), mode);
}

void Controller::set_network_mode(QString host, RUN_MODE mode) {
    ctrl_->network_mode(host.toStdString(), mode);
}

void Controller::set_run_mode(QString host, RUN_MODE mode) {
    ctrl_->run_mode(host.toStdString(), mode);
}

void Controller::schedule_disk_usage_update(QString host) {
    ctrl_->reschedule_now(host.toStdString(), PeriodicTask::GET_DISK_USAGE);
}

void Controller::schedule_projects_update(QString host) {
    ctrl_->reschedule_now(host.toStdString(), PeriodicTask::GET_PROJECT_STATUS);
}

void Controller::schedule_state_update(QString host) {
    ctrl_->reschedule_now(host.toStdString(), PeriodicTask::GET_CLIENT_STATE);
}

void Controller::schedule_tasks_update(QString host) {
    ctrl_->reschedule_now(host.toStdString(), PeriodicTask::GET_TASKS);
}

void Controller::schedule_statistics_update(QString host) {
    ctrl_->reschedule_now(host.toStdString(), PeriodicTask::GET_STATISTICS);
}

void Controller::connect(const HandlerAdapter *adapter) const {
#define WOINC_CONNECT(FROM, TO) QObject::connect(adapter, &HandlerAdapter::FROM, \
                                                 this, &Controller::TO, \
                                                 Qt::QueuedConnection)
    WOINC_CONNECT(connected, handle_host_connected);
    WOINC_CONNECT(authorized, handle_host_authorized);
    WOINC_CONNECT(authorization_failed, handle_host_authorization_failed);
    WOINC_CONNECT(error_occurred, handle_host_error);
#undef WOINC_CONNECT
}

void Controller::handle_host_connected(QString host) {
    WOINC_LOCK_GUARD;

    auto credentials = std::find_if(pending_logins_.begin(), pending_logins_.end(), [&](const auto &t) {
        return t.first == host;
    });

    bool auth = credentials != pending_logins_.end();
    assert(auth);

    if (auth)
        ctrl_->authorize_host(credentials->first.toStdString(), credentials->second.toStdString());
}

void Controller::handle_host_authorized(QString host) {
    {
        WOINC_LOCK_GUARD;
        pending_logins_.erase(std::remove_if(pending_logins_.begin(), pending_logins_.end(), [&](const auto &t) {
            return t.first == host;
        }), pending_logins_.end());
    }
    ctrl_->schedule_periodic_tasks(host.toStdString(), true);
}

void Controller::handle_host_authorization_failed(QString host) {
    {
        WOINC_LOCK_GUARD;
        pending_logins_.erase(std::remove_if(pending_logins_.begin(), pending_logins_.end(), [&](const auto &t) {
            return t.first == host;
        }), pending_logins_.end());
    }
    emit error_occurred(QString::fromUtf8("Authorization failed"),
                        QString("Authorization to host \"%1\" failed.").arg(host));
    ctrl_->async_remove_host(host.toStdString());
}

void Controller::handle_host_error(QString host, Error error) {
    QString msg;

    switch (error) {
        case Error::DISCONNECTED:
        case Error::CONNECTION_ERROR:
            msg = QString("Connection to host \"%1\" lost.").arg(host);
            break;
        case Error::UNAUTHORIZED:
            msg = QString("Not authorized to host \"%1\".").arg(host);
            break;
        case Error::CLIENT_ERROR:
        case Error::PARSING_ERROR:
            msg = QString("Communication error with host \"%1\".").arg(host);
            break;
        case Error::LOGIC_ERROR:
            msg = QString("Internal error handling host \"%1\".").arg(host);
            break;
    }

    emit error_occurred(QString::fromUtf8("An error occurred"),
                        msg /* + QString::fromUtf8(" The host will be removed from the list.") */);

    ctrl_->async_remove_host(host.toStdString());
}

}}}
