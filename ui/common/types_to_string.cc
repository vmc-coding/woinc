/* ui/common/types_to_string.cc --
   Written and Copyright (C) 2017-2020 by vmc.

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

const char *to_string(const ACTIVE_TASK_STATE ats) {
    switch (ats) {
        case ACTIVE_TASK_STATE::UNINITIALIZED: return "UNINITIALIZED";
        case ACTIVE_TASK_STATE::EXECUTING: return "EXECUTING";
        case ACTIVE_TASK_STATE::SUSPENDED: return "SUSPENDED";
        case ACTIVE_TASK_STATE::ABORT_PENDING: return "ABORT_PENDING";
        case ACTIVE_TASK_STATE::EXITED: return "EXITED";
        case ACTIVE_TASK_STATE::WAS_SIGNALED: return "WAS_SIGNALED";
        case ACTIVE_TASK_STATE::EXIT_UNKNOWN: return "EXIT_UNKNOWN";
        case ACTIVE_TASK_STATE::ABORTED: return "ABORTED";
        case ACTIVE_TASK_STATE::COULDNT_START: return "COULDNT_START";
        case ACTIVE_TASK_STATE::QUIT_PENDING: return "QUIT_PENDING";
        case ACTIVE_TASK_STATE::COPY_PENDING: return "COPY_PENDING";
        default: return "Unknown";
    }
}

const char *to_string(const MSG_INFO prio) {
    switch (prio) {
        case MSG_INFO::INFO: return "low";
        case MSG_INFO::USER_ALERT: return "user notification";
        case MSG_INFO::INTERNAL_ERROR: return "internal error";
        default: return "Unknown";
    }
}

const char *to_string(const NETWORK_STATUS ns) {
    switch (ns) {
        case NETWORK_STATUS::ONLINE: return "online";
        case NETWORK_STATUS::WANT_CONNECTION: return "need connection";
        case NETWORK_STATUS::WANT_DISCONNECT: return "don't need connection";
        case NETWORK_STATUS::LOOKUP_PENDING: return "reference site lookup pending";
        default: return "unknown";
    }
}

const char *to_string(const RESULT_CLIENT_STATE rcs) {
    switch (rcs) {
        case RESULT_CLIENT_STATE::NEW: return "new";
        case RESULT_CLIENT_STATE::FILES_DOWNLOADING: return "downloading";
        case RESULT_CLIENT_STATE::FILES_DOWNLOADED: return "downloaded";
        case RESULT_CLIENT_STATE::COMPUTE_ERROR: return "compute error";
        case RESULT_CLIENT_STATE::FILES_UPLOADING: return "uploading";
        case RESULT_CLIENT_STATE::FILES_UPLOADED: return "uploaded";
        case RESULT_CLIENT_STATE::ABORTED: return "aborted";
        case RESULT_CLIENT_STATE::UPLOAD_FAILED: return "upload failed";
        default: return "unknown";
    }
}

const char *to_string(const RPC_REASON reason) {
    switch (reason) {
        case RPC_REASON::NONE: return "";
        case RPC_REASON::USER_REQ: return "Requested by user";
        case RPC_REASON::RESULTS_DUE: return "To fetch work";
        case RPC_REASON::NEED_WORK: return "To report completed tasks";
        case RPC_REASON::TRICKLE_UP: return "To send trickle-up message";
        case RPC_REASON::ACCT_MGR_REQ: return "Requested by account manager";
        case RPC_REASON::INIT: return "Project initialization";
        case RPC_REASON::PROJECT_REQ: return "Requested by project";
        default: return "unknown";
    }
}

const char *to_string(const RUN_MODE rm) {
    switch (rm) {
        case RUN_MODE::ALWAYS: return "always";
        case RUN_MODE::AUTO: return "according to prefs";
        case RUN_MODE::NEVER: return "never";
        default: return "unknown";
    }
}

const char *to_string(const SCHEDULER_STATE ss) {
    switch (ss) {
        case SCHEDULER_STATE::UNINITIALIZED: return "uninitialized";
        case SCHEDULER_STATE::PREEMPTED: return "preempted";
        case SCHEDULER_STATE::SCHEDULED: return "scheduled";
        default: return "unknown";
    }
}

const char *to_string(const SUSPEND_REASON sr) {
    switch (sr) {
        case SUSPEND_REASON::BATTERIES: return "on batteries";
        case SUSPEND_REASON::USER_ACTIVE: return "computer is in use";
        case SUSPEND_REASON::USER_REQ: return "user request";
        case SUSPEND_REASON::TIME_OF_DAY: return "time of day";
        case SUSPEND_REASON::BENCHMARKS: return "CPU benchmarks in progress";
        case SUSPEND_REASON::DISK_SIZE: return "need disk space - check preferences";
        case SUSPEND_REASON::NO_RECENT_INPUT: return "no recent user activity";
        case SUSPEND_REASON::INITIAL_DELAY: return "initial delay";
        case SUSPEND_REASON::EXCLUSIVE_APP_RUNNING: return "an exclusive app is running";
        case SUSPEND_REASON::CPU_USAGE: return "CPU is busy";
        case SUSPEND_REASON::NETWORK_QUOTA_EXCEEDED: return "network transfer limit exceeded";
        case SUSPEND_REASON::OS: return "requested by operating system";
        case SUSPEND_REASON::WIFI_STATE: return "not connected to WiFi network";
        case SUSPEND_REASON::BATTERY_CHARGING: return "battery low";
        case SUSPEND_REASON::BATTERY_OVERHEATED: return "battery thermal protection";
        case SUSPEND_REASON::NO_GUI_KEEPALIVE: return "GUI not active";
        default: return "unknown reason";
    }
}

const char *exit_code_to_string(int exit_code) {
    switch(exit_code) {
        case static_cast<int>(TASK_EXIT_CODE::NONE): return "";
        case static_cast<int>(TASK_EXIT_CODE::ABORTED_VIA_GUI): return "Aborted by user";
        case static_cast<int>(TASK_EXIT_CODE::ABORTED_BY_PROJECT): return "Aborted by project";
        case static_cast<int>(TASK_EXIT_CODE::UNSTARTED_LATE): return "Aborted: not started by deadline";
        case static_cast<int>(TASK_EXIT_CODE::DISK_LIMIT_EXCEEDED): return "Aborted: task disk limit exceeded";
        case static_cast<int>(TASK_EXIT_CODE::TIME_LIMIT_EXCEEDED): return "Aborted: run time limit exceeded";
        case static_cast<int>(TASK_EXIT_CODE::MEM_LIMIT_EXCEEDED): return "Aborted: memory limit exceeded";
        default: return "Aborted";
    }
}

}}}
