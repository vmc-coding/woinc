/* ui/qt/types.cc --
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

#include "qt/types.h"

namespace woinc { namespace ui { namespace qt {

bool Event::operator==(const Event &that) const {
    return seqno == that.seqno
        && timestamp == that.timestamp
        && message == that.message
        && project_name == that.project_name
        && user_alert == that.user_alert;
}

bool Event::operator!=(const Event &that) const {
    return !(*this == that);
}

bool FileTransfer::operator==(const FileTransfer &that) const {
    return file == that.file
        && project_url == that.project_url
        && project == that.project
        && status == that.status
        && elapsed_seconds == that.elapsed_seconds
        && bytes_xferred == that.bytes_xferred
        && size == that.size
        && speed == that.speed;
}

bool FileTransfer::operator!=(const FileTransfer &that) const {
    return !(*this == that);
}

bool Project::operator==(const Project &that) const {
    return project_url == that.project_url
        && account == that.account
        && name == that.name
        && status == that.status
        && team == that.team
        && venue == that.venue
        && anonymous_platform == that.anonymous_platform
        && attached_via_acct_mgr == that.attached_via_acct_mgr
        && detach_when_done == that.detach_when_done
        && dont_request_more_work == that.dont_request_more_work
        && ended == that.ended
        && non_cpu_intensive == that.non_cpu_intensive
        && scheduler_rpc_in_progress == that.scheduler_rpc_in_progress
        && suspended_via_gui == that.suspended_via_gui
        && trickle_up_pending == that.trickle_up_pending
        && host_expavg_credit == that.host_expavg_credit
        && host_total_credit == that.host_total_credit
        && sched_priority == that.sched_priority
        && user_expavg_credit == that.user_expavg_credit
        && user_total_credit == that.user_total_credit
        && hostid == that.hostid
        && njobs_error == that.njobs_error
        && njobs_success == that.njobs_success
        && download_backoff == that.download_backoff
        && last_rpc_time == that.last_rpc_time
        && min_rpc_time == that.min_rpc_time
        && upload_backoff == that.upload_backoff
        && resource_share == that.resource_share;
}

bool Project::operator!=(const Project &that) const {
    return !(*this == that);
}

bool Task::operator==(const Task &that) const {
    return name == that.name
        && project_url == that.project_url
        && application == that.application
        && project == that.project
        && status == that.status
        && suspended == that.suspended
        && progress == that.progress
        && elapsed_seconds == that.elapsed_seconds
        && remaining_seconds == that.remaining_seconds
        && deadline == that.deadline;
}

bool Task::operator!=(const Task &that) const {
    return !(*this == that);
}

}}}
