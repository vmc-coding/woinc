/* lib/src/types.cc --
   Written and Copyright (C) 2019-2020 by vmc.

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

#include <woinc/types.h>

namespace woinc {

#define WOINC_COPY(FROM, WHAT) WHAT(FROM.WHAT)

FileTransfer::FileTransfer(const FileTransfer &ft)
    : WOINC_COPY(ft, nbytes)
    , WOINC_COPY(ft, status)
    , WOINC_COPY(ft, name)
    , WOINC_COPY(ft, project_name)
    , WOINC_COPY(ft, project_url)
    , WOINC_COPY(ft, project_backoff)
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    , WOINC_COPY(ft, max_nbytes)
#endif
    , persistent_file_xfer(nullptr)
    , file_xfer(nullptr)
{
    if (ft.persistent_file_xfer) {
        auto pfx = new PersistentFileXfer;
        *pfx = *ft.persistent_file_xfer;
        persistent_file_xfer.reset(pfx);
    }

    if (ft.file_xfer) {
        auto fx = new FileXfer;
        *fx = *ft.file_xfer;
        file_xfer.reset(fx);
    }
}

FileTransfer &FileTransfer::operator=(FileTransfer ft) {
    *this = std::move(ft);
    return *this;
}

Task::Task(const Task &task)
    : WOINC_COPY(task, state)
    , WOINC_COPY(task, coproc_missing)
    , WOINC_COPY(task, got_server_ack)
    , WOINC_COPY(task, network_wait)
    , WOINC_COPY(task, project_suspended_via_gui)
    , WOINC_COPY(task, ready_to_report)
    , WOINC_COPY(task, scheduler_wait)
    , WOINC_COPY(task, suspended_via_gui)
    , WOINC_COPY(task, estimated_cpu_time_remaining)
    , WOINC_COPY(task, final_cpu_time)
    , WOINC_COPY(task, final_elapsed_time)
    , WOINC_COPY(task, exit_status)
    , WOINC_COPY(task, signal)
    , WOINC_COPY(task, version_num)
    , WOINC_COPY(task, name)
    , WOINC_COPY(task, project_url)
    , WOINC_COPY(task, resources)
    , WOINC_COPY(task, scheduler_wait_reason)
    , WOINC_COPY(task, wu_name)
    , active_task(nullptr)
    , WOINC_COPY(task, received_time)
    , WOINC_COPY(task, report_deadline)
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    , WOINC_COPY(task, edf_scheduled)
    , WOINC_COPY(task, report_immediately)
    , WOINC_COPY(task, completed_time)
    , WOINC_COPY(task, plan_class)
    , WOINC_COPY(task, platform)
#endif
{
    if (task.active_task) {
        auto at = new ActiveTask;
        *at = *task.active_task;
        active_task.reset(at);
    }
}

Task &Task::operator=(Task task) {
    *this = std::move(task);
    return *this;
}

ClientState::ClientState(const ClientState &cs)
    : WOINC_COPY(cs, app_versions)
    , WOINC_COPY(cs, apps)
    , WOINC_COPY(cs, projects)
    , WOINC_COPY(cs, tasks)
    , WOINC_COPY(cs, time_stats)
    , WOINC_COPY(cs, workunits)
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    , WOINC_COPY(cs, global_prefs)
    , WOINC_COPY(cs, host_info)
    , WOINC_COPY(cs, platforms)
    , WOINC_COPY(cs, core_client_version)
    , WOINC_COPY(cs, executing_as_daemon)
    , WOINC_COPY(cs, have_ati)
    , WOINC_COPY(cs, have_cuda)
    , WOINC_COPY(cs, platform_name)
    , net_stats(nullptr)
#endif
{
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    if (cs.net_stats) {
        auto ns = new NetStats;
        *ns = *cs.net_stats;
        net_stats.reset(ns);
    }
#endif
}

ClientState &ClientState::operator=(ClientState cs) {
    *this = std::move(cs);
    return *this;
}

}
