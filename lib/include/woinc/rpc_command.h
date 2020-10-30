/* woinc/rpc_command.h --
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

#ifndef WOINC_RPC_COMMAND_H_
#define WOINC_RPC_COMMAND_H_

#include <woinc/defs.h>
#include <woinc/types.h>
#include <woinc/version.h>

namespace woinc { namespace rpc {

struct Connection;

enum class COMMAND_STATUS {
    OK,
    DISCONNECTED,
    UNAUTHORIZED,
    CONNECTION_ERROR,
    CLIENT_ERROR,
    PARSING_ERROR,
    LOGIC_ERROR
};

struct Command {
    virtual ~Command() = default;

    virtual COMMAND_STATUS execute(Connection &) = 0;

    bool operator()(Connection &connection) {
        return execute(connection) == COMMAND_STATUS::OK;
    }

    const std::string &error() const { return error_; }

    // see gui_rpcs[] in BOINC/client/gui_rpc_server_ops.cpp
    virtual bool requires_local_authorization() const = 0;

    protected:
        std::string error_;
};

template<typename REQUEST_TYPE, typename RESPONSE_TYPE, bool REQUIRE_LOCAL_AUTH>
struct BOINCCommand : public Command {
    BOINCCommand() = default;
    explicit BOINCCommand(REQUEST_TYPE rq) : request_(std::move(rq)) {}
    virtual ~BOINCCommand() = default;

    COMMAND_STATUS execute(Connection &connection) override;

    bool requires_local_authorization() const final { return REQUIRE_LOCAL_AUTH; }

    REQUEST_TYPE &request() { return request_; }
    const REQUEST_TYPE &request() const { return request_; }

    RESPONSE_TYPE &response() { return response_; }
    const RESPONSE_TYPE &response() const { return response_; }

    protected:
        REQUEST_TYPE request_;
        RESPONSE_TYPE response_;
};

struct SuccessResponse {
    bool success = false;
};

// --- Authorize command ---

struct AuthorizeRequest {
    std::string password;
};

struct AuthorizeResponse {
    bool authorized = false;
};

typedef BOINCCommand<AuthorizeRequest, AuthorizeResponse, false> AuthorizeCommand;

// --- ExchangeVersionsCommand ---

struct ExchangeVersionsRequest {
    explicit ExchangeVersionsRequest(const Version &v = {
        boinc_major_version(),
        boinc_minor_version(),
        boinc_release_version() }) : version(v) {}
    Version version;
};

struct ExchangeVersionsResponse {
    Version version;
};

typedef BOINCCommand<ExchangeVersionsRequest, ExchangeVersionsResponse, false> ExchangeVersionsCommand;

// --- GetAllProjectsListCommand ---

struct GetAllProjectsListRequest {};

struct GetAllProjectsListResponse {
	AllProjectsList projects;
};

typedef BOINCCommand<GetAllProjectsListRequest, GetAllProjectsListResponse, false> GetAllProjectsListCommand;

// --- GetCCStatusCommand ---

struct GetCCStatusRequest {};

struct GetCCStatusResponse {
    CCStatus cc_status;
};

typedef BOINCCommand<GetCCStatusRequest, GetCCStatusResponse, false> GetCCStatusCommand;

// --- GetClientState ---

struct GetClientStateRequest {};

struct GetClientStateResponse {
    ClientState client_state;
};

typedef BOINCCommand<GetClientStateRequest, GetClientStateResponse, false> GetClientStateCommand;

// --- GetDiskUsage ---

struct GetDiskUsageRequest {};

struct GetDiskUsageResponse {
    DiskUsage disk_usage;
};

typedef BOINCCommand<GetDiskUsageRequest, GetDiskUsageResponse, false> GetDiskUsageCommand;

// --- GetGlobalPreferences ---

struct GetGlobalPreferencesRequest {
    GetGlobalPreferencesRequest(GET_GLOBAL_PREFS_MODE mode);
    const GET_GLOBAL_PREFS_MODE mode;
};

struct GetGlobalPreferencesResponse {
    GlobalPreferences preferences;
};

typedef BOINCCommand<GetGlobalPreferencesRequest, GetGlobalPreferencesResponse, true> GetGlobalPreferencesCommand;

// --- GetFileTransfers ---

struct GetFileTransfersRequest {};

struct GetFileTransfersResponse {
    FileTransfers file_transfers;
};

typedef BOINCCommand<GetFileTransfersRequest, GetFileTransfersResponse, false> GetFileTransfersCommand;

// --- GetHostInfo ---

struct GetHostInfoRequest {};

struct GetHostInfoResponse {
    HostInfo host_info;
};

typedef BOINCCommand<GetHostInfoRequest, GetHostInfoResponse, false> GetHostInfoCommand;

// --- GetMessagesCommand ---

struct GetMessagesRequest {
    int seqno = 0;
    bool translatable = false;
};

struct GetMessagesResponse {
    Messages messages;
};

typedef BOINCCommand<GetMessagesRequest, GetMessagesResponse, false> GetMessagesCommand;

// --- GetNoticesCommand ---

struct GetNoticesRequest {
    int seqno = 0;
};

struct GetNoticesResponse {
    // when true, the client ignored the requested seqno and sent a full new list of notices
    bool refreshed = false;
    Notices notices;
};

typedef BOINCCommand<GetNoticesRequest, GetNoticesResponse, true> GetNoticesCommand;

// --- GetProjectConfigCommand ---

struct GetProjectConfigRequest {
    std::string url;
};

struct GetProjectConfigResponse : public SuccessResponse {};

typedef BOINCCommand<GetProjectConfigRequest, GetProjectConfigResponse, true> GetProjectConfigCommand;

// --- GetProjectConfigPollCommand ---

struct GetProjectConfigPollRequest {};

struct GetProjectConfigPollResponse {
    ProjectConfig project_config; // only valid if project_config.error_num == 0
};

typedef BOINCCommand<GetProjectConfigPollRequest, GetProjectConfigPollResponse, true> GetProjectConfigPollCommand;

// --- GetProjectStatusCommand ---

struct GetProjectStatusRequest {};

struct GetProjectStatusResponse {
    Projects projects;
};

typedef BOINCCommand<GetProjectStatusRequest, GetProjectStatusResponse, false> GetProjectStatusCommand;

// --- GetResultsCommand ---

struct GetResultsRequest {
    bool active_only = false;
};

struct GetResultsResponse {
    Tasks tasks;
};

typedef BOINCCommand<GetResultsRequest, GetResultsResponse, false> GetResultsCommand;

// --- GetStatistics ---

struct GetStatisticsRequest {};

struct GetStatisticsResponse {
    Statistics statistics;
};

typedef BOINCCommand<GetStatisticsRequest, GetStatisticsResponse, false> GetStatisticsCommand;

// --- FileTransferOp ---

struct FileTransferOpRequest {
    FileTransferOpRequest(FILE_TRANSFER_OP op, std::string url = "", std::string name = "");
    const FILE_TRANSFER_OP op;
    std::string master_url;
    std::string filename;
};

struct FileTransferOpResponse : public SuccessResponse {};

typedef BOINCCommand<FileTransferOpRequest, FileTransferOpResponse, true> FileTransferOpCommand;

// --- LookupAccount ---

struct LookupAccountRequest {
    bool ldap_auth = false;
    bool server_assigned_cookie = false;
    std::string email;
    std::string master_url;
    std::string passwd;
    std::string server_cookie;
};

struct LookupAccountResponse : public SuccessResponse {};

typedef BOINCCommand<LookupAccountRequest, LookupAccountResponse, true> LookupAccountCommand;

// --- LookupAccountPollCommand ---

struct LookupAccountPollRequest {};

struct LookupAccountPollResponse {
    AccountOut account_out;
};

typedef BOINCCommand<LookupAccountPollRequest, LookupAccountPollResponse, true> LookupAccountPollCommand;

// --- NetworkAvailable ---

struct NetworkAvailableRequest {};

struct NetworkAvailableResponse : public SuccessResponse {};

typedef BOINCCommand<NetworkAvailableRequest, NetworkAvailableResponse, true> NetworkAvailableCommand;

// --- ProjectAttach ---

struct ProjectAttachRequest {
    ProjectAttachRequest(std::string master_url = "", std::string authentication = "", std::string project_name = "");
    std::string master_url;
    std::string authenticator;
    std::string project_name;
};

struct ProjectAttachResponse : public SuccessResponse {};

typedef BOINCCommand<ProjectAttachRequest, ProjectAttachResponse, true> ProjectAttachCommand;

// --- ProjectOp ---

struct ProjectOpRequest {
    ProjectOpRequest(PROJECT_OP o, std::string url = "");
    const PROJECT_OP op;
    std::string master_url;
};

struct ProjectOpResponse : public SuccessResponse {};

typedef BOINCCommand<ProjectOpRequest, ProjectOpResponse, true> ProjectOpCommand;

// --- Quit ---

struct QuitRequest {};

struct QuitResponse : public SuccessResponse {};

typedef BOINCCommand<QuitRequest, QuitResponse, true> QuitCommand;

// --- ReadCCConfig ---

struct ReadCCConfigRequest {};

struct ReadCCConfigResponse : public SuccessResponse {};

typedef BOINCCommand<ReadCCConfigRequest, ReadCCConfigResponse, true> ReadCCConfigCommand;

// --- ReadGlobalPreferencesOverride ---

struct ReadGlobalPreferencesOverrideRequest {};

struct ReadGlobalPreferencesOverrideResponse : public SuccessResponse {};

typedef BOINCCommand<ReadGlobalPreferencesOverrideRequest, ReadGlobalPreferencesOverrideResponse, true> ReadGlobalPreferencesOverrideCommand;

// --- RunBenchmarks ---

struct RunBenchmarksRequest {};

struct RunBenchmarksResponse : public SuccessResponse {};

typedef BOINCCommand<RunBenchmarksRequest, RunBenchmarksResponse, true> RunBenchmarksCommand;

// --- SetGlobalPreferences ---

struct SetGlobalPreferencesRequest {
    GlobalPreferences preferences;
    GlobalPreferencesMask mask;
};

struct SetGlobalPreferencesResponse : public SuccessResponse {};

typedef BOINCCommand<SetGlobalPreferencesRequest, SetGlobalPreferencesResponse, true> SetGlobalPreferencesCommand;

// --- SetGpuModeCommand ---

struct SetGpuModeRequest {
    SetGpuModeRequest(RUN_MODE mode, double duration = 0);
    const RUN_MODE mode;
    double duration;
};

struct SetGpuModeResponse : public SuccessResponse {};

typedef BOINCCommand<SetGpuModeRequest, SetGpuModeResponse, true> SetGpuModeCommand;

// --- SetNetworkModeCommand ---

struct SetNetworkModeRequest {
    SetNetworkModeRequest(RUN_MODE mode, double duration = 0);
    const RUN_MODE mode;
    double duration;
};

struct SetNetworkModeResponse : public SuccessResponse {};

typedef BOINCCommand<SetNetworkModeRequest, SetNetworkModeResponse, true> SetNetworkModeCommand;

// --- SetRunModeCommand ---

struct SetRunModeRequest {
    SetRunModeRequest(RUN_MODE mode, double duration = 0);
    const RUN_MODE mode;
    double duration;
};

struct SetRunModeResponse : public SuccessResponse {};

typedef BOINCCommand<SetRunModeRequest, SetRunModeResponse, true> SetRunModeCommand;

// --- TaskOp ---

struct TaskOpRequest {
    TaskOpRequest(TASK_OP o, std::string url = "", std::string name = "");
    const TASK_OP op;
    std::string master_url;
    std::string name;
};

struct TaskOpResponse : public SuccessResponse {};

typedef BOINCCommand<TaskOpRequest, TaskOpResponse, true> TaskOpCommand;

}}

#endif
