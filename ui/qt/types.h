/* ui/qt/types.h --
   Written and Copyright (C) 2017-2022 by vmc.

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

#ifndef WOINC_UI_QT_TYPES_H_
#define WOINC_UI_QT_TYPES_H_

#include <ctime>
#include <vector>

#include <QMetaType>
#include <QPair>
#include <QString>

#include <woinc/defs.h>

namespace woinc { namespace ui { namespace qt {

struct Event {
    QString message;
    QString project_name;
    int seqno;
    time_t timestamp;
    bool user_alert;

    bool operator==(const Event &that) const;
    bool operator!=(const Event &that) const;
};

typedef std::vector<Event> Events;

struct Notice {
    enum class Category { NONE, CLIENT, SCHEDULER };

    QString description;
    QString project_name;
    QString title;
    QString link;
    time_t create_time;
    Category category;
};

typedef std::vector<Notice> Notices;

struct Project {
    QString account;
    QString project_url;
    QString name;
    QString status;
    QString team;
    QString venue;

    bool anonymous_platform;
    bool attached_via_acct_mgr;
    bool detach_when_done;
    bool dont_request_more_work;
    bool ended;
    bool non_cpu_intensive;
    bool scheduler_rpc_in_progress;
    bool suspended_via_gui;
    bool trickle_up_pending;

    double host_expavg_credit;
    double host_total_credit;
    double sched_priority;
    double user_expavg_credit;
    double user_total_credit;

    int hostid;
    int njobs_error;
    int njobs_success;

    time_t download_backoff;
    time_t last_rpc_time;
    time_t min_rpc_time;
    time_t upload_backoff;

    // resource share, sum resource shares
    QPair<double, double> resource_share;

    bool operator==(const Project &that) const;
    bool operator!=(const Project &that) const;
};

typedef std::vector<Project> Projects;

struct RunModes {
    RunMode cpu = RunMode::UnknownToWoinc;
    RunMode gpu = RunMode::UnknownToWoinc;
    RunMode network = RunMode::UnknownToWoinc;
};

struct Task {
    QString application;
    QString executable;
    QString host;
    QString name;
    QString project;
    QString project_url;
    QString resources;
    QString status;
    QString wu_name;
    bool active_task = false;
    bool suspended = false;
    double estimated_computation_size = 0;
    double progress = 0;
    double progress_rate = 0;
    double virtual_mem_size = 0;
    double working_set_size = 0;
    int checkpoint_cpu_time = 0; // current_cpu_time of the last checkpoint
    int current_cpu_time = 0; // number of seconds this task is running on the CPU
    int elapsed_seconds = 0;
    int final_cpu_seconds = 0;
    int final_elapsed_seconds = 0;
    int pid = 0;
    int remaining_seconds = 0;
    int slot = 0;
    time_t deadline = 0;
    time_t received_time = 0;

    // TODO currently we use this in the tab model updater, but there we need to compare the visible one only, not all;
    // let's improve the tab model updater and provide the compare function to it; finally, remove these operators
    bool operator==(const Task &that) const;
    bool operator!=(const Task &that) const;
};

typedef std::vector<Task> Tasks;

struct Workunit {
    QString app_name;
    QString name;
    QString project_url;
    double rsc_fpops_est = 0;
    int version_num;
};

typedef std::vector<Workunit> Workunits;

struct App {
    QString name;
    QString project_url;
    QString user_friendly_name;
    bool non_cpu_intensive = false;
};

typedef std::vector<App> Apps;

struct AppVersion {
    QString app_name;
    QString executable;
    QString plan_class;
    QString project_url;
    int version_num;
};

typedef std::vector<AppVersion> AppVersions;

struct FileTransfer {
    QString file;
    QString project;
    QString project_url;
    QString status;
    double elapsed_seconds;
    double bytes_xferred;
    double size;
    double speed;

    bool operator==(const FileTransfer &that) const;
    bool operator!=(const FileTransfer &that) const;
};

typedef std::vector<FileTransfer> FileTransfers;

struct DiskUsage {
    double used = 0;
    double available = 0;
    double not_available = 0;
    double others = 0;

    struct Project {
        QString name;
        double usage;
    };

    std::vector<Project> projects;
};

struct DailyStatistic {
    double host_expavg_credit;
    double host_total_credit;
    double user_expavg_credit;
    double user_total_credit;
    time_t day;
};

typedef std::vector<DailyStatistic> DailyStatistics;

struct ProjectStatistics {
    struct {
        QString project_url;
        QString name;
        QString account;
        QString team;
    } project;
    DailyStatistics daily_statistics;
};

typedef std::vector<ProjectStatistics> Statistics;

}}}

Q_DECLARE_METATYPE(woinc::ui::qt::AppVersions)
Q_DECLARE_METATYPE(woinc::ui::qt::DiskUsage)
Q_DECLARE_METATYPE(woinc::ui::qt::Events)
Q_DECLARE_METATYPE(woinc::ui::qt::FileTransfers)
Q_DECLARE_METATYPE(woinc::ui::qt::Notices)
Q_DECLARE_METATYPE(woinc::ui::qt::Project)
Q_DECLARE_METATYPE(woinc::ui::qt::Projects)
Q_DECLARE_METATYPE(woinc::ui::qt::RunModes)
Q_DECLARE_METATYPE(woinc::ui::qt::Statistics)
Q_DECLARE_METATYPE(woinc::ui::qt::Tasks)
Q_DECLARE_METATYPE(woinc::ui::qt::Workunits)

#endif
