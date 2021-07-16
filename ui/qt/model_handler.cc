/* ui/qt/model_handler.cc --
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

#include "qt/model_handler.h"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <type_traits>

#ifndef NDEBUG
#include <iostream>
#endif

#include <QTextStream>

#include "common/types_to_string.h"

#include "qt/utils.h"

namespace {

namespace wqt = woinc::ui::qt;

template<typename T>
int compare(const T &a, const T &b) {
    static_assert(std::is_arithmetic<T>::value, "Arithmetic type needed");
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

template<>
int compare(const QString &a, const QString &b) {
    return a.localeAwareCompare(b);
}

template<>
int compare(const woinc::ui::qt::Task &a, const woinc::ui::qt::Task &b) {
    int result = compare(a.project, b.project);
    if (!result)
        result = compare(a.name, b.name);
    return result;
}

template<>
int compare(const woinc::ui::qt::Project &a,
            const woinc::ui::qt::Project &b) {
    return compare(a.name, b.name);
}

bool uses_gpu(const woinc::Task &task) {
    // TODO is there another way to determine this?
    return task.resources.find("GPU") != task.resources.npos;
}

// TODO see BOINC's clientgui/MainDocument.cpp result_description(RESULT* result, bool show_resources)
QString resolve_task_status(const woinc::Task &task,
                            const woinc::CCStatus &cc_status,
                            bool non_cpu_intensive) {
    QString result;
    QTextStream ss(&result);

    bool throttled = cc_status.cpu.suspend_reason == woinc::SuspendReason::CpuThrottle;

    auto handle_state_new = [&]() { ss << "New"; };

    auto handle_state_files_downloading = [&]() {
        if (task.ready_to_report) {
            ss << "Download failed";
        } else {
            ss << "Downloading";
            if (cc_status.network.suspend_reason != woinc::SuspendReason::NotSuspended)
                ss << " (suspended - " << woinc::ui::common::to_string(cc_status.network.suspend_reason) << ")";
        }
    };

    auto handle_state_files_downloaded = [&]() {
        if (task.project_suspended_via_gui) {
            ss << "Project suspended by user";
        } else if (task.suspended_via_gui) {
            ss << "Task suspended by user";
        } else if (cc_status.cpu.suspend_reason != woinc::SuspendReason::NotSuspended
                   && !throttled
                   && task.active_task != nullptr
                   && task.active_task->active_task_state != woinc::ActiveTaskState::Executing) {
            ss << "Suspended - " << woinc::ui::common::to_string(cc_status.cpu.suspend_reason);
        } else if (cc_status.gpu.suspend_reason != woinc::SuspendReason::NotSuspended && uses_gpu(task)) {
            ss << "GPU suspended - " << woinc::ui::common::to_string(cc_status.gpu.suspend_reason);
        } else if (task.active_task != nullptr) {
            if (task.active_task->too_large) {
                ss << "Waiting for memory";
            } else if (task.active_task->needs_shmem) {
                ss << "Waiting for shared memory";
            } else if (task.active_task->scheduler_state == woinc::SchedulerState::Scheduled) {
                ss << "Running";
                if (non_cpu_intensive)
                    ss << " (non-CPU-intensive)";
            } else if (task.active_task->scheduler_state == woinc::SchedulerState::Preempted) {
                ss << "Waiting to run";
            } else if (task.active_task->scheduler_state == woinc::SchedulerState::Uninitialized) {
                ss << "Ready to start";
            }
        } else {
            ss << "Ready to start";
        }
        if (task.scheduler_wait) {
            // TODO: this is what BOINC does, but I'm not yet sure, if it's the right thing to do here ..
            // why delete the maybe existing 'GPU missing' string?
            result = "";
            ss << "Postponed";
            if (!task.scheduler_wait_reason.empty())
                ss << ": " << QString::fromStdString(task.scheduler_wait_reason);
        }
        if (task.network_wait) {
            // TODO: this is what BOINC does, but I'm not yet sure, if it's the right thing to do here ..
            // why delete the maybe existing 'GPU missing' string?
            result = "";
            ss << "Waiting for network access";
        }
    };

    auto handle_state_compute_error = [&]() {
        ss << "Computation error";
    };

    auto handle_state_files_uploading = [&]() {
        if (task.ready_to_report) {
            ss << "Upload failed";
        } else {
            ss << "Uploading";
            if (cc_status.network.suspend_reason != woinc::SuspendReason::NotSuspended)
                ss << " (suspended - " << woinc::ui::common::to_string(cc_status.network.suspend_reason) << ")";
        }
    };

    auto handle_state_aborted = [&]() {
        ss << woinc::ui::common::exit_code_to_string(task.exit_status);
    };

    auto handle_state_default = [&]() {
        if (task.got_server_ack) {
            ss << "Acknowledged";
        } else if (task.ready_to_report) {
            ss << "Ready to report";
        } else {
            result = "";
            ss << "Error: invalid state '" << static_cast<int>(task.state) << "'";
        }
    };

    if (task.coproc_missing)
        ss << "GPU missing, ";

    switch (task.state) {
        case woinc::ResultClientState::New:              handle_state_new(); break;
        case woinc::ResultClientState::FilesDownloading: handle_state_files_downloading(); break;
        case woinc::ResultClientState::FilesDownloaded:  handle_state_files_downloaded(); break;
        case woinc::ResultClientState::ComputeError:     handle_state_compute_error(); break;
        case woinc::ResultClientState::FilesUploading:   handle_state_files_uploading(); break;
        case woinc::ResultClientState::Aborted:          handle_state_aborted(); break;
        default: handle_state_default();
    }

    if (!task.resources.empty())
        ss << " (" << QString::fromStdString(task.resources) << ")";

    return result;
}

QString duration_to_string(const std::chrono::seconds &duration) {
    auto dv = std::div(static_cast<int>(duration.count()), 3600);
    int hours = dv.quot;
    dv = std::div(dv.rem, 60);

    return QString::asprintf("%02d:%02d:%02d", hours, dv.quot, dv.rem);
}

QString resolve_project_status(const woinc::Project &project) {
    QStringList status;

    if (project.suspended_via_gui)
        status << QString::fromUtf8("Suspended by user");

    if (project.dont_request_more_work)
        status << QString::fromUtf8("Won't get new tasks");

    if (project.ended)
        status << QString::fromUtf8("Project ended - OK to remove");

    if (project.detach_when_done)
        status << QString::fromUtf8("Will remove when tasks done");

    if (project.sched_rpc_pending != woinc::RpcReason::None) {
        status << QString::fromUtf8("Scheduler request pending");
        status << QString::fromUtf8(woinc::ui::common::to_string(project.sched_rpc_pending));
    }

    if (project.scheduler_rpc_in_progress)
        status << QString::fromUtf8("Scheduler request in progress");

    if (project.trickle_up_pending)
        status << QString::fromUtf8("Trickle up message pending");

    auto next_rpc = std::chrono::system_clock::from_time_t(project.min_rpc_time);
    auto now = std::chrono::system_clock::now();

    if (next_rpc > now)
        status << (QString::fromUtf8("Communication deferred ") + duration_to_string(
                std::chrono::duration_cast<std::chrono::seconds>(next_rpc - now)));

    return status.join(", ");
}

woinc::ui::qt::Notice::Category resolve_notice_category(const woinc::Notice &notice) {
    if (notice.category == "client")
        return woinc::ui::qt::Notice::Category::CLIENT;
    else if (notice.category == "scheduler")
        return woinc::ui::qt::Notice::Category::SCHEDULER;
    else
        return woinc::ui::qt::Notice::Category::NONE;
}

QString remove_localization_marker(QString str) {
    str.remove("_(\"");
    str.remove("\")");
    return str;
}

QString resolve_transfer_status(const woinc::FileTransfer &file_transfer, const woinc::CCStatus &cc_status) {
    enum { ERR_GIVEUP_DOWNLOAD = -114, ERR_GIVEUP_UPLOAD = -115 };

    QString result;
    QTextStream ss(&result);

    if (file_transfer.persistent_file_xfer == nullptr) {
        ss << "Could not resolve status";
        return result;
    }

    auto persistent_file_xfer = *file_transfer.persistent_file_xfer;

    auto now = std::chrono::system_clock::now();
    auto next_request = std::chrono::system_clock::from_time_t(persistent_file_xfer.next_request_time);

    ss << (persistent_file_xfer.is_upload ? "Upload" : "Download") << ": ";

    if (next_request > now) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(next_request - now);
        ss << "retry in " << duration_to_string(duration);
    } else if (file_transfer.status == ERR_GIVEUP_DOWNLOAD || file_transfer.status == ERR_GIVEUP_UPLOAD) {
        ss << "failed";
    } else {
        if (cc_status.network.suspend_reason != woinc::SuspendReason::NotSuspended) {
            ss << "suspended - ";
            if (cc_status.network.suspend_reason != woinc::SuspendReason::UnknownToWoinc)
                ss << woinc::ui::common::to_string(cc_status.network.suspend_reason);
            else
                ss << "unknown reason";
        } else {
            if (file_transfer.file_xfer != nullptr)
                ss << "active";
            else
                ss << "pending";
        }
    }
    if (file_transfer.project_backoff)
        ss << " (project backoff: " << wqt::seconds_as_time_string(
                static_cast<long>(file_transfer.project_backoff)) << ")";
    return result;
}

}

namespace woinc { namespace ui { namespace qt {

ModelHandler::HostModel::HostModel(const QString &h) : host(h) {}

QVariant ModelHandler::HostModel::resolve_app_name_by_wu(const QString &wu_name) const {
    auto wup = std::find_if(wus.cbegin(), wus.cend(), [&](const Workunit &w) {
        return w.name == wu_name;
    });

    if (wup == wus.cend())
        return {};

    auto app = std::find_if(apps.cbegin(), apps.cend(), [&](const App &a) {
        return a.project_url == wup->project_url && a.name == wup->app_name;
    });

    if (app == apps.cend())
        return {};

    auto app_version = std::find_if(app_versions.cbegin(), app_versions.cend(), [&](const AppVersion &av) {
        return av.project_url == wup->project_url && av.app_name == wup->app_name;
    });

    if (app_version == app_versions.cend())
        return {};

    auto project = std::find_if(projects.cbegin(), projects.cend(), [&](const Project &p) {
        return p.project_url == wup->project_url;
    });

    if (project == projects.cend())
        return {};

    QString app_name;
    if (!app->user_friendly_name.isEmpty())
        app_name = app->user_friendly_name;
    else
        app_name = app_version->app_name;

    QString plan_class;
    if (!app_version->plan_class.isEmpty())
        QTextStream(&plan_class) << " (" << app_version->plan_class << ")";

    QString anon_platform;
    if (project->anonymous_platform)
        anon_platform = QString::fromUtf8("Local: ");

    QString version = QString::asprintf(" %d.%02d", app_version->version_num / 100, app_version->version_num % 100);

    QString result;
    QTextStream(&result) << anon_platform << app_name << version << plan_class;
    return {result.trimmed()};
}

QVariant ModelHandler::HostModel::resolve_project_name_by_url(const std::string &master_url) const {
    QString qurl = QString::fromStdString(master_url);
    auto it = std::find_if(projects.cbegin(), projects.cend(), [&](const Project &project) {
        return project.project_url == qurl;
    });
    return it == projects.cend() ? QVariant{} : QVariant{it->name};
}

// ---- ModelHandler ----

ModelHandler::ModelHandler(QObject *parent)
    : Model(parent)
{}

void ModelHandler::add_host(QString host) {
    host_models_.emplace(host, HostModel(host));

    if (selected_host_.isEmpty())
        select_host_(host);
}

void ModelHandler::remove_host(QString host) {
    unselect_host_(host);
    host_models_.erase(host);
}

void ModelHandler::update_cc_status(QString host, woinc::CCStatus cc_status) {
    find_host_model_(host).cc_status = std::move(cc_status);
    if (selected_host_ == host)
        emit run_modes_updated(map_(find_host_model_(host).cc_status));

}

void ModelHandler::update_client_state(QString host, woinc::ClientState wclient_state) {
    auto &host_model = find_host_model_(host);

    host_model.app_versions  = map_(std::move(wclient_state.app_versions));
    host_model.apps          = map_(std::move(wclient_state.apps));
    host_model.projects      = map_(std::move(wclient_state.projects));
    host_model.wus           = map_(std::move(wclient_state.workunits));

    auto tasks               = map_(std::move(wclient_state.tasks), host_model);
    assert(tasks.isValid());
    host_model.tasks         = tasks.value<Tasks>();
}

void ModelHandler::update_disk_usage(QString host, woinc::DiskUsage wdisk_usage) {
    if (selected_host_ == host) {
        auto usage = map_(std::move(wdisk_usage), find_host_model_(host));
        if (usage.isValid()) {
            emit disk_usage_updated(usage.value<DiskUsage>());
        } else {
            emit projects_update_needed(host);
            emit disk_usage_update_needed(host);
        }
    }
}

void ModelHandler::update_file_transfers(QString host, woinc::FileTransfers wfile_transfers) {
    if (selected_host_ == host)
        emit file_transfers_updated(map_(std::move(wfile_transfers), find_host_model_(host)));
}

void ModelHandler::update_messages(QString host, woinc::Messages wmessages) {
    if (wmessages.empty())
        return;

    if (selected_host_ == host)
        emit events_appended(map_(std::move(wmessages)));
}

void ModelHandler::update_notices(QString host, woinc::Notices wnotices, bool refreshed) {
    if (selected_host_ != host)
        return;

    if (refreshed)
        emit notices_refreshed(map_(std::move(wnotices)));
    else
        emit notices_appended(map_(std::move(wnotices)));
}

// TODO update tasks if needed, i.e. the suspension state of projects changed
void ModelHandler::update_projects(QString host, woinc::Projects wprojects) {
#if 0
#ifndef NDEBUG
    static size_t cnt = 0;

    size_t to_drop = (cnt % wprojects.size()) + 1;

    while (to_drop-- > 0)
        wprojects.pop_back();

    ++ cnt;
#endif
#endif
    Projects projects(map_(std::move(wprojects)));

    find_host_model_(host).projects = std::move(projects);
    if (selected_host_ == host)
        emit projects_updated(find_host_model_(host).projects);
}

void ModelHandler::update_statistics(QString host, woinc::Statistics wstatistics) {
#if 0
#ifndef NDEBUG
    static size_t last_size = 0;

    size_t to_drop = statistics.project_statistics.size() - last_size;
    to_drop = (to_drop + 1) % statistics.project_statistics.size();

    while (to_drop-- > 0)
        statistics.project_statistics.pop_back();

    last_size = statistics.project_statistics.size();
#endif
#endif
    if (selected_host_ == host) {
        auto statistics = map_(std::move(wstatistics), find_host_model_(host));
        if (statistics.isValid()) {
            emit statistics_updated(statistics.value<Statistics>());
        } else {
            emit projects_update_needed(host);
            emit statistics_update_needed(host);
        }
    }
}

void ModelHandler::update_tasks(QString host, woinc::Tasks wtasks) {
    // the controller ensures that the host won't be removed while another handler
    // method is executed, i.e. we don't need to lock for accessing the host;
    // furthermore there is only one worker thread per host, i.e. the (reading) access the host model without locking is fine
    auto &host_model = find_host_model_(host);
    auto tasks = map_(std::move(wtasks), host_model);
    if (!tasks.isValid()) {
        emit state_update_needed(host);
    } else {
        host_model.tasks = tasks.value<Tasks>();
        if (selected_host_ == host)
            emit tasks_updated(host_model.tasks);
    }
}

void ModelHandler::select_host(QString host) {
    select_host_(host);
}

void ModelHandler::unselect_host(QString host) {
    unselect_host_(host);
}

// TODO support multiple selected hosts
void ModelHandler::select_host_(const QString &host) {
    unselect_host_(selected_host_);
    selected_host_ = host;
    emit host_selected(host);
}

void ModelHandler::unselect_host_(const QString &host) {
    if (selected_host_.isEmpty() || selected_host_ != host)
        return;
    selected_host_.clear();

    emit host_unselected(host);

    // TODO all widgets should connect to the host_unselected signal and update the view
    emit disk_usage_updated({});
    emit events_appended({});
    emit file_transfers_updated({});
    emit notices_appended({});
    emit notices_refreshed({});
    emit projects_updated({});
    emit statistics_updated({});
    emit tasks_updated({});
}

const ModelHandler::HostModel &ModelHandler::find_host_model_(const QString &host) const {
    const auto iter = host_models_.find(host);
    assert(iter != host_models_.end());
    return iter->second;
}

ModelHandler::HostModel &ModelHandler::find_host_model_(const QString &host) {
    auto iter = host_models_.find(host);
    assert(iter != host_models_.end());
    return iter->second;
}

Apps ModelHandler::map_(woinc::Apps wapps) {
    Apps apps;
    apps.reserve(wapps.size());

    for (auto &&source : wapps) {
        App dest;

        dest.name = QString::fromStdString(source.name);
        dest.project_url = QString::fromStdString(source.project_url);
        dest.user_friendly_name = QString::fromStdString(source.user_friendly_name);
        dest.non_cpu_intensive = source.non_cpu_intensive;

        apps.push_back(std::move(dest));
    }

    return apps;
}

AppVersions ModelHandler::map_(woinc::AppVersions wapp_versions) {
    AppVersions app_versions;
    app_versions.reserve(wapp_versions.size());

    for (auto &&source : wapp_versions) {
        AppVersion dest;

        dest.app_name = QString::fromStdString(source.app_name);

        for (auto &&fr : source.file_refs) {
            if (fr.main_program) {
                dest.executable = QString::fromStdString(fr.file_name);
                break;
            }
        }

        dest.project_url  = QString::fromStdString(source.project_url);
        dest.plan_class   = QString::fromStdString(source.plan_class);
        dest.version_num  = source.version_num;

        app_versions.push_back(std::move(dest));
    }

    return app_versions;
}

QVariant ModelHandler::map_(woinc::DiskUsage wdisk_usage, const HostModel &host_model) {
    double total = wdisk_usage.boinc;
    for (auto &&project : wdisk_usage.projects)
        total += project.disk_usage;

    double available = std::min(std::max(wdisk_usage.allowed - total, 0.), wdisk_usage.free);
    double not_available = wdisk_usage.free - available;
    double others = wdisk_usage.total - total - wdisk_usage.free;

    DiskUsage disk_usage;
    disk_usage.projects.reserve(wdisk_usage.projects.size());

    disk_usage.used          = total;
    disk_usage.available     = available;
    disk_usage.not_available = not_available;
    disk_usage.others        = others;

    for (auto &&source : wdisk_usage.projects) {
        DiskUsage::Project dest;

        auto name = host_model.resolve_project_name_by_url(source.master_url);
        if (!name.isValid())
            return name;
        dest.name = name.toString();
        dest.usage = source.disk_usage;

        disk_usage.projects.push_back(std::move(dest));
    }

    return QVariant::fromValue(std::move(disk_usage));
}

FileTransfers ModelHandler::map_(woinc::FileTransfers wfile_transfers, const HostModel &host_model) {
    FileTransfers file_transfers;
    file_transfers.reserve(wfile_transfers.size());

    for (auto &&source : wfile_transfers) {
        FileTransfer dest;

        dest.file = QString::fromStdString(source.name);
        dest.project = QString::fromStdString(source.project_name);
        dest.project_url = QString::fromStdString(source.project_url);
        dest.status = resolve_transfer_status(source, host_model.cc_status);

        dest.elapsed_seconds = 0;
        if (source.persistent_file_xfer.get() != nullptr)
            dest.elapsed_seconds = source.persistent_file_xfer->time_so_far;

        dest.size = source.nbytes;

        dest.bytes_xferred = 0;
        dest.speed = 0;
        if (source.file_xfer.get() != nullptr) {
            dest.bytes_xferred = source.file_xfer->bytes_xferred;
            dest.speed = source.file_xfer->xfer_speed;
        }

        file_transfers.push_back(std::move(dest));
    }

    return file_transfers;
}

Events ModelHandler::map_(woinc::Messages wmessages) {
    Events events;
    events.reserve(wmessages.size());

    for (auto &&source : wmessages) {
        Event dest;

        dest.message      = QString::fromStdString(source.body).trimmed();
        dest.project_name = QString::fromStdString(source.project);
        dest.seqno        = source.seqno;
        dest.user_alert   = source.priority == MsgInfo::UserAlert;
        dest.timestamp    = source.timestamp;

        events.push_back(dest);
    }

    return events;
}

Notices ModelHandler::map_(woinc::Notices wnotices) {
    Notices notices;
    notices.reserve(wnotices.size());

    for (auto &&source : wnotices) {
        Notice dest;

        dest.title        = QString::fromStdString(source.title);
        dest.project_name = QString::fromStdString(source.project_name);
        dest.description  = remove_localization_marker(QString::fromStdString(source.description));
        dest.category     = resolve_notice_category(source);
        dest.link         = QString::fromStdString(source.link);
        dest.create_time  = source.create_time;

        // convert newlines to HTML
        dest.description.replace("<br />", "<br>");
        dest.description.replace("<br/>", "<br>");
        dest.description.replace("<br>\r\n", "<br>");
        dest.description.replace("<br>\n", "<br>");
        dest.description.replace("\r\n", "<br>");
        dest.description.replace("\n", "<br>");
        // BOINC does this, but why delete newlines the project (presumably) intended?
        //dest.description.replace("<br><br>", "<br>");

        notices.push_back(dest);
    }

    return notices;
}

Workunits ModelHandler::map_(woinc::Workunits wwus) {
    Workunits wus;
    wus.reserve(wwus.size());

    for (auto &&source : wwus) {
        Workunit dest;

        dest.app_name      = QString::fromStdString(source.app_name);
        dest.name          = QString::fromStdString(source.name);
        dest.project_url   = QString::fromStdString(source.project_url);
        dest.rsc_fpops_est = source.rsc_fpops_est;
        dest.version_num   = source.version_num;

        wus.push_back(std::move(dest));
    }

    return wus;
}

Projects ModelHandler::map_(woinc::Projects wprojects) {
    Projects projects;
    projects.reserve(wprojects.size());

    double sum_resource_share = std::accumulate(wprojects.cbegin(), wprojects.cend(), 0.,
                                                [](double sum, auto &proj) { return sum + proj.resource_share; });

    // convert the response to our data structure
    for (auto &&source : wprojects) {
        Project dest;

        dest.account     = QString::fromStdString(source.user_name);
        dest.project_url = QString::fromStdString(source.master_url);
        dest.name        = QString::fromStdString(source.project_name);
        dest.status      = resolve_project_status(source);
        dest.team        = QString::fromStdString(source.team_name);
        dest.venue       = QString::fromStdString(source.venue);

        dest.anonymous_platform        = source.anonymous_platform;
        dest.attached_via_acct_mgr     = source.attached_via_acct_mgr;
        dest.detach_when_done          = source.detach_when_done;
        dest.dont_request_more_work    = source.dont_request_more_work;
        dest.ended                     = source.ended;
        dest.non_cpu_intensive         = source.non_cpu_intensive;
        dest.scheduler_rpc_in_progress = source.scheduler_rpc_in_progress;
        dest.suspended_via_gui         = source.suspended_via_gui;
        dest.trickle_up_pending        = source.trickle_up_pending;

        dest.host_expavg_credit = source.host_expavg_credit;
        dest.host_total_credit  = source.host_total_credit;
        dest.sched_priority     = source.sched_priority;
        dest.user_expavg_credit = source.user_expavg_credit;
        dest.user_total_credit  = source.user_total_credit;

        dest.hostid        = source.hostid;
        dest.njobs_error   = source.njobs_error;
        dest.njobs_success = source.njobs_success;

        dest.download_backoff = source.download_backoff;
        dest.last_rpc_time    = source.last_rpc_time;
        dest.min_rpc_time     = source.min_rpc_time;
        dest.upload_backoff   = source.upload_backoff;

        dest.resource_share = qMakePair(source.resource_share, sum_resource_share);

        projects.push_back(std::move(dest));
    }

    // ensure the tasks are always in the same order
    std::sort(projects.begin(), projects.end(), [](const Project &a, const Project &b) {
        int result = compare(a, b);
        assert(result != 0);
        return result < 0;
    });

    return projects;
}

RunModes ModelHandler::map_(const woinc::CCStatus &cc_status) {
    RunModes modes;
    modes.cpu = cc_status.cpu.mode;
    modes.gpu = cc_status.gpu.mode;
    modes.network = cc_status.network.mode;
    return modes;
}

QVariant ModelHandler::map_(woinc::Statistics wstatistics, const HostModel &host_model) {
    Statistics statistics;
    statistics.reserve(wstatistics.size());

    for (auto &&source : wstatistics) {
        auto project = std::find_if(host_model.projects.cbegin(), host_model.projects.cend(), [&](const Project &p) {
            return p.project_url == QString::fromStdString(source.master_url);
        });

        if (project == host_model.projects.cend())
            return {};

        ProjectStatistics dest;
        dest.project.project_url = project->project_url;
        dest.project.name = project->name;
        dest.project.account = project->account;
        dest.project.team = project->team;
        dest.daily_statistics.reserve(source.daily_statistics.size());

        for (auto &source_ds : source.daily_statistics) {
            DailyStatistic dest_ds;

            dest_ds.host_expavg_credit = source_ds.host_expavg_credit;
            dest_ds.host_total_credit  = source_ds.host_total_credit;
            dest_ds.user_expavg_credit = source_ds.user_expavg_credit;
            dest_ds.user_total_credit  = source_ds.user_total_credit;
            dest_ds.day = source_ds.day;

            dest.daily_statistics.push_back(std::move(dest_ds));
        }

        statistics.push_back(std::move(dest));
    }

    return QVariant::fromValue(std::move(statistics));
}

QVariant ModelHandler::map_(woinc::Tasks wtasks, const HostModel &host_model) {
    Tasks tasks;
    tasks.reserve(wtasks.size());

    // convert the response to our data structure
    for (auto &&source : wtasks) {
        Task dest;

        auto project = std::find_if(host_model.projects.cbegin(), host_model.projects.cend(), [&](const Project &p) {
            return p.project_url == QString::fromStdString(source.project_url);
        });

        auto wup = std::find_if(host_model.wus.cbegin(), host_model.wus.cend(), [&](const Workunit &w) {
            return w.name == QString::fromStdString(source.wu_name);
        });

        if (wup == host_model.wus.cend())
            return {};

        auto app_version = std::find_if(host_model.app_versions.cbegin(), host_model.app_versions.cend(), [&](const AppVersion &av) {
            return av.project_url == wup->project_url && av.app_name == wup->app_name;
        });

        auto app = host_model.resolve_app_name_by_wu(QString::fromStdString(source.wu_name));

        if (project == host_model.projects.cend() || app_version == host_model.app_versions.cend() || !app.isValid())
            return {};

        dest.application = app.toString();
        dest.executable  = app_version->executable;
        dest.host        = host_model.host;
        dest.project     = project->name;
        dest.project_url = QString::fromStdString(source.project_url);
        dest.resources   = QString::fromStdString(source.resources);
        dest.status      = resolve_task_status(source, host_model.cc_status, project->non_cpu_intensive);
        dest.name        = QString::fromStdString(source.name);
        dest.wu_name     = QString::fromStdString(source.wu_name);

        dest.active_task = source.active_task != nullptr;
        dest.suspended   = source.suspended_via_gui;

        dest.estimated_computation_size = wup->rsc_fpops_est;

        if (dest.active_task) {
            dest.progress         = source.active_task->fraction_done;
            dest.elapsed_seconds  = static_cast<int>(round(source.active_task->elapsed_time));
            dest.progress_rate    = source.active_task->progress_rate;
            dest.virtual_mem_size = source.active_task->swap_size;
            dest.working_set_size = source.active_task->working_set_size_smoothed;

            dest.checkpoint_cpu_time = static_cast<int>(round(source.active_task->checkpoint_cpu_time));
            dest.current_cpu_time    = static_cast<int>(round(source.active_task->current_cpu_time));
        }

        dest.final_cpu_seconds     = static_cast<int>(round(source.final_cpu_time));
        dest.final_elapsed_seconds = static_cast<int>(round(source.final_elapsed_time));
        dest.pid                   = dest.active_task ? source.active_task->pid : 0;
        dest.remaining_seconds     = static_cast<int>(round(source.estimated_cpu_time_remaining));
        dest.slot                  = dest.active_task ? source.active_task->slot : -1;

        dest.deadline      = source.report_deadline;
        dest.received_time = source.received_time;

        if (!dest.active_task && (
                source.state == woinc::ResultClientState::ComputeError ||
                source.state == woinc::ResultClientState::FilesUploading ||
                source.state == woinc::ResultClientState::FilesUploaded ||
                source.state == woinc::ResultClientState::Aborted ||
                source.state == woinc::ResultClientState::UploadFailed)) {
            dest.progress = 1;
            dest.elapsed_seconds = static_cast<int>(round(source.final_elapsed_time));
            if (!dest.elapsed_seconds)
                dest.elapsed_seconds = static_cast<int>(round(source.final_cpu_time));
        }

        tasks.push_back(std::move(dest));
    }

    // ensure the tasks are always in the same order
    std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
        int result = compare(a, b);
        assert(result != 0);
        return result < 0;
    });

    return QVariant::fromValue(std::move(tasks));
}

}}}
