/* woinc/defs.h --
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

#ifndef WOINC_DEFS_H_
#define WOINC_DEFS_H_

namespace woinc {

// for enum reference see lib/common_defs.h in the source of BOINC;
// we use UnknownToWoinc for to us unknown values to be able to handle future clients
// which may introduce new values for the enums

enum class ActiveTaskState {
    Uninitialized,
    Executing,
    Exited,
    WasSignaled,
    ExitUnknown,
    AbortPending,
    Aborted,
    CouldntStart,
    QuitPending,
    Suspended,
    CopyPending,
    UnknownToWoinc
};

enum class MsgInfo {
    Info,
    UserAlert,
    InternalError,
    UnknownToWoinc
};

enum class NetworkStatus {
    Online,
    WantConnection,
    WantDisconnect,
    LookupPending,
    UnknownToWoinc
};

enum class ResultClientState {
    New,
    FilesDownloading,
    FilesDownloaded,
    ComputeError,
    FilesUploading,
    FilesUploaded,
    Aborted,
    UploadFailed,
    UnknownToWoinc
};

enum class RpcReason {
    None,
    UserReq,
    ResultsDue,
    NeedWork,
    TrickleUp,
    AcctMgrReq,
    Init,
    ProjectReq,
    UnknownToWoinc
};

enum class RunMode {
    Always,
    Auto,
    Never,
    Restore,
    UnknownToWoinc
};

enum class SchedulerState {
    Uninitialized,
    Preempted,
    Scheduled,
    UnknownToWoinc
};

enum class SuspendReason {
    NotSuspended,
    Batteries,
    UserActive,
    UserReq,
    TimeOfDay,
    Benchmarks,
    DiskSize,
    CpuThrottle,
    NoRecentInput,
    InitialDelay,
    ExclusiveAppRunning,
    CpuUsage,
    NetworkQuotaExceeded,
    Os,
    WifiState,
    BatteryCharging,
    BatteryOverheated,
    NoGuiKeepalive,
    UnknownToWoinc
};

// see BOINC/lib/error_numbers.h
enum class TaskExitCode {
    None = 0,
    StatefileWrite = 192,
    Signal = 193,
    AbortedByClient = 194,
    ChildFailed = 195,
    DiskLimitExceeded = 196,
    TimeLimitExceeded = 197,
    MemLimitExceeded = 198,
    ClientExiting = 199,
    UnstartedLate = 200,
    MissingCoproc = 201,
    AbortedByProject = 202,
    AbortedViaGui = 203,
    Unknown = 204,
    OutOfMemory = 205,
    InitFailure = 206,
    NoSubTasks = 207,
    SubTaskFailure = 208
};

enum class DayOfWeek {
    Sunday,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    UnknownToWoinc
};

// ----- woinc's own enums, i.e. not send by the clients -----

enum class FileTransferOp {
    Abort,
    Retry
};

enum class GetGlobalPrefsMode {
    File,
    Override,
    Working
};

enum class ProjectOp {
    Allowmorework,
    Detach,
    DetachWhenDone,
    DontDetachWhenDone,
    Nomorework,
    Reset,
    Resume,
    Suspend,
    Update
};

enum class TaskOp {
    Abort,
    Resume,
    Suspend
};


namespace rpc {

enum class ConnectionStatus {
    Ok,
    Disconnected,
    Error
};

enum class CommandStatus {
    Ok,
    Disconnected,
    Unauthorized,
    ConnectionError,
    ClientError,
    ParsingError,
    LogicError
};

}


}

#endif
