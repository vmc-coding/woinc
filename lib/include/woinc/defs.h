/* woinc/defs.h --
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

#ifndef WOINC_DEFS_H_
#define WOINC_DEFS_H_

namespace woinc {

// for enum reference see lib/common_defs.h in the source of BOINC;
// we use UNKNOWN_TO_WOINC for to us unknown values to be able to handle future clients
// which may introduce new values for the enums

enum class ACTIVE_TASK_STATE {
    UNINITIALIZED,
    EXECUTING,
    EXITED,
    WAS_SIGNALED,
    EXIT_UNKNOWN,
    ABORT_PENDING,
    ABORTED,
    COULDNT_START,
    QUIT_PENDING,
    SUSPENDED,
    COPY_PENDING,
    UNKNOWN_TO_WOINC
};

enum class MSG_INFO {
    INFO,
    USER_ALERT,
    INTERNAL_ERROR,
    UNKNOWN_TO_WOINC
};

enum class NETWORK_STATUS {
    ONLINE,
    WANT_CONNECTION,
    WANT_DISCONNECT,
    LOOKUP_PENDING,
    UNKNOWN_TO_WOINC
};

enum class RESULT_CLIENT_STATE {
    NEW,
    FILES_DOWNLOADING,
    FILES_DOWNLOADED,
    COMPUTE_ERROR,
    FILES_UPLOADING,
    FILES_UPLOADED,
    ABORTED,
    UPLOAD_FAILED,
    UNKNOWN_TO_WOINC
};

enum class RPC_REASON {
    NONE,
    USER_REQ,
    RESULTS_DUE,
    NEED_WORK,
    TRICKLE_UP,
    ACCT_MGR_REQ,
    INIT,
    PROJECT_REQ,
    UNKNOWN_TO_WOINC
};

enum class RUN_MODE {
    ALWAYS,
    AUTO,
    NEVER,
    RESTORE,
    UNKNOWN_TO_WOINC
};

enum class SCHEDULER_STATE {
    UNINITIALIZED,
    PREEMPTED,
    SCHEDULED,
    UNKNOWN_TO_WOINC
};

enum class SUSPEND_REASON {
    NOT_SUSPENDED,
    BATTERIES,
    USER_ACTIVE,
    USER_REQ,
    TIME_OF_DAY,
    BENCHMARKS,
    DISK_SIZE,
    CPU_THROTTLE,
    NO_RECENT_INPUT,
    INITIAL_DELAY,
    EXCLUSIVE_APP_RUNNING,
    CPU_USAGE,
    NETWORK_QUOTA_EXCEEDED,
    OS,
    WIFI_STATE,
    BATTERY_CHARGING,
    BATTERY_OVERHEATED,
    NO_GUI_KEEPALIVE,
    UNKNOWN_TO_WOINC
};

// see BOINC/lib/error_numbers.h
enum class TASK_EXIT_CODE {
    NONE = 0,
    STATEFILE_WRITE = 192,
	SIGNAL = 193,
	ABORTED_BY_CLIENT = 194,
	CHILD_FAILED = 195,
	DISK_LIMIT_EXCEEDED = 196,
	TIME_LIMIT_EXCEEDED = 197,
	MEM_LIMIT_EXCEEDED = 198,
	CLIENT_EXITING = 199,
	UNSTARTED_LATE = 200,
	MISSING_COPROC = 201,
	ABORTED_BY_PROJECT = 202,
	ABORTED_VIA_GUI = 203,
	UNKNOWN = 204,
	OUT_OF_MEMORY = 205,
	INIT_FAILURE = 206,
	NO_SUB_TASKS = 207,
	SUB_TASK_FAILURE = 208
};

enum class DAY_OF_WEEK {
    SUNDAY,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    UNKNOWN_TO_WOINC
};

// ----- woinc's own enums, i.e. not send by the clients -----

enum class PROJECT_OP {
    ALLOWMOREWORK,
    DETACH,
    DETACH_WHEN_DONE,
    DONT_DETACH_WHEN_DONE,
    NOMOREWORK,
    RESET,
    RESUME,
    SUSPEND,
    UPDATE
};

enum class TASK_OP {
    ABORT,
    RESUME,
    SUSPEND
};

enum class FILE_TRANSFER_OP {
    ABORT,
    RETRY
};

enum class GET_GLOBAL_PREFS_MODE {
    FILE,
    OVERRIDE,
    WORKING
};

}

#endif
