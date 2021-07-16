/* ui/common/types_to_string.cc --
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

// More or less copied from BOINC's v7.6.33 file lib/str_util.cpp.
// BOINC also uses GPL v3.

#include "common/types_to_string.h"

namespace woinc { namespace ui { namespace common {

const char *to_string(const ActiveTaskState ats) {
    switch (ats) {
        case ActiveTaskState::Uninitialized: return "Uninitialized";
        case ActiveTaskState::Executing: return "Executing";
        case ActiveTaskState::Suspended: return "Suspended";
        case ActiveTaskState::AbortPending: return "AbortPending";
        case ActiveTaskState::Exited: return "Exited";
        case ActiveTaskState::WasSignaled: return "WasSignaled";
        case ActiveTaskState::ExitUnknown: return "EXIT_Unknown";
        case ActiveTaskState::Aborted: return "Aborted";
        case ActiveTaskState::CouldntStart: return "CouldntStart";
        case ActiveTaskState::QuitPending: return "QuitPending";
        case ActiveTaskState::CopyPending: return "CopyPending";
        default: return "Unknown";
    }
}

const char *to_string(const MsgInfo prio) {
    switch (prio) {
        case MsgInfo::Info: return "low";
        case MsgInfo::UserAlert: return "user notification";
        case MsgInfo::InternalError: return "internal error";
        default: return "Unknown";
    }
}

const char *to_string(const NetworkStatus ns) {
    switch (ns) {
        case NetworkStatus::Online: return "online";
        case NetworkStatus::WantConnection: return "need connection";
        case NetworkStatus::WantDisconnect: return "don't need connection";
        case NetworkStatus::LookupPending: return "reference site lookup pending";
        default: return "unknown";
    }
}

const char *to_string(const ResultClientState rcs) {
    switch (rcs) {
        case ResultClientState::New: return "new";
        case ResultClientState::FilesDownloading: return "downloading";
        case ResultClientState::FilesDownloaded: return "downloaded";
        case ResultClientState::ComputeError: return "compute error";
        case ResultClientState::FilesUploading: return "uploading";
        case ResultClientState::FilesUploaded: return "uploaded";
        case ResultClientState::Aborted: return "aborted";
        case ResultClientState::UploadFailed: return "upload failed";
        default: return "unknown";
    }
}

const char *to_string(const RpcReason reason) {
    switch (reason) {
        case RpcReason::None: return "";
        case RpcReason::UserReq: return "Requested by user";
        case RpcReason::ResultsDue: return "To fetch work";
        case RpcReason::NeedWork: return "To report completed tasks";
        case RpcReason::TrickleUp: return "To send trickle-up message";
        case RpcReason::AcctMgrReq: return "Requested by account manager";
        case RpcReason::Init: return "Project initialization";
        case RpcReason::ProjectReq: return "Requested by project";
        default: return "unknown";
    }
}

const char *to_string(const RunMode rm) {
    switch (rm) {
        case RunMode::Always: return "always";
        case RunMode::Auto: return "according to prefs";
        case RunMode::Never: return "never";
        default: return "unknown";
    }
}

const char *to_string(const SchedulerState ss) {
    switch (ss) {
        case SchedulerState::Uninitialized: return "uninitialized";
        case SchedulerState::Preempted: return "preempted";
        case SchedulerState::Scheduled: return "scheduled";
        default: return "unknown";
    }
}

const char *to_string(const SuspendReason sr) {
    switch (sr) {
        case SuspendReason::Batteries: return "on batteries";
        case SuspendReason::UserActive: return "computer is in use";
        case SuspendReason::UserReq: return "user request";
        case SuspendReason::TimeOfDay: return "time of day";
        case SuspendReason::Benchmarks: return "CPU benchmarks in progress";
        case SuspendReason::DiskSize: return "need disk space - check preferences";
        case SuspendReason::NoRecentInput: return "no recent user activity";
        case SuspendReason::InitialDelay: return "initial delay";
        case SuspendReason::ExclusiveAppRunning: return "an exclusive app is running";
        case SuspendReason::CpuUsage: return "CPU is busy";
        case SuspendReason::NetworkQuotaExceeded: return "network transfer limit exceeded";
        case SuspendReason::Os: return "requested by operating system";
        case SuspendReason::WifiState: return "not connected to WiFi network";
        case SuspendReason::BatteryCharging: return "battery low";
        case SuspendReason::BatteryOverheated: return "battery thermal protection";
        case SuspendReason::NoGuiKeepalive: return "GUI not active";
        default: return "unknown reason";
    }
}

const char *exit_code_to_string(int exit_code) {
    switch(exit_code) {
        case static_cast<int>(TaskExitCode::None): return "";
        case static_cast<int>(TaskExitCode::AbortedViaGui): return "Aborted by user";
        case static_cast<int>(TaskExitCode::AbortedByProject): return "Aborted by project";
        case static_cast<int>(TaskExitCode::UnstartedLate): return "Aborted: not started by deadline";
        case static_cast<int>(TaskExitCode::DiskLimitExceeded): return "Aborted: task disk limit exceeded";
        case static_cast<int>(TaskExitCode::TimeLimitExceeded): return "Aborted: run time limit exceeded";
        case static_cast<int>(TaskExitCode::MemLimitExceeded): return "Aborted: memory limit exceeded";
        default: return "Aborted";
    }
}

}}}
