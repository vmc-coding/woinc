/* lib/rpc_command.cc --
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

#include <woinc/rpc_command.h>

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>

#ifndef NDEBUG
#include <iostream>
#endif

#include <woinc/rpc_connection.h>

#include "md5.h"
#include "rpc_parsing.h"

namespace wxml = woinc::xml;

namespace {

using namespace woinc::rpc;

constexpr CommandStatus map__(ConnectionStatus status) {
    switch (status) {
        case ConnectionStatus::Ok:
            return CommandStatus::Ok;
        case ConnectionStatus::Disconnected:
            return CommandStatus::Disconnected;
        case ConnectionStatus::Error:
            return CommandStatus::ConnectionError;
    }
    // workaround for GCC bug 86678
#if defined(__GNUC__) && __GNUC__ >= 9
    assert(false);
#endif
    return CommandStatus::LogicError;
}

constexpr CommandStatus as_status__(bool b) {
    return b ? CommandStatus::Ok : CommandStatus::ParsingError;
}

CommandStatus do_rpc__(Connection &connection,
                       const wxml::Tree &request_tree,
                       wxml::Tree &response_tree,
                       std::string &error_holder) {
    std::stringstream response;

    auto rpc_result = connection.do_rpc(request_tree.str(), response);

    if (!rpc_result) {
        error_holder = rpc_result.error;
        return map__(rpc_result.status);
    }

    if (!wxml::parse_boinc_response(response_tree, response, error_holder))
        return CommandStatus::ParsingError;

    if (response_tree.root.children.size() == 1
            && response_tree.root.children.front().tag == "unauthorized")
        return CommandStatus::Unauthorized;

    if (response_tree.root.children.size() == 1
            && response_tree.root.children.front().tag == "error") {
        error_holder = response_tree.root.children.front().content;
        return CommandStatus::ClientError;
    }

    return CommandStatus::Ok;
}

bool parse__(const wxml::Tree &response_tree, SuccessResponse &response) {
    auto result_node = response_tree.root.find_child("success");
    response.success = response_tree.root.found_child(result_node);
    return true;
}

bool parse__(const wxml::Tree &response_tree, ExchangeVersionsResponse &response) {
    auto server_version_node = response_tree.root.find_child("server_version");
    return response_tree.root.found_child(server_version_node) && parse(*server_version_node, response.version);
}

bool parse__(const wxml::Tree &response_tree, GetAllProjectsListResponse &response) {
    auto all_projects_node = response_tree.root.find_child("projects");
    return response_tree.root.found_child(all_projects_node) && parse(*all_projects_node, response.projects);
}

bool parse__(const wxml::Tree &response_tree, GetCCConfigResponse &response) {
    auto cc_config_node = response_tree.root.find_child("cc_config");
    return response_tree.root.found_child(cc_config_node) && parse(*cc_config_node, response.cc_config);
}

bool parse__(const wxml::Tree &response_tree, GetCCStatusResponse &response) {
    auto cc_status_node = response_tree.root.find_child("cc_status");
    return response_tree.root.found_child(cc_status_node) && parse(*cc_status_node, response.cc_status);
}

bool parse__(const wxml::Tree &response_tree, GetClientStateResponse &response) {
    auto client_state_node = response_tree.root.find_child("client_state");
    return response_tree.root.found_child(client_state_node) && parse(*client_state_node, response.client_state);
}

bool parse__(const wxml::Tree &response_tree, GetDiskUsageResponse &response) {
    auto disk_usage_node = response_tree.root.find_child("disk_usage_summary");
    return response_tree.root.found_child(disk_usage_node) && parse(*disk_usage_node, response.disk_usage);
}

bool parse__(const wxml::Tree &response_tree, GetFileTransfersResponse &response) {
    auto file_transfers_node = response_tree.root.find_child("file_transfers");
    if (!response_tree.root.found_child(file_transfers_node))
        return false;

    response.file_transfers.reserve(file_transfers_node->children.size());
    for (auto &result_node : file_transfers_node->children) {
        woinc::FileTransfer ft;
        if (!parse(result_node, ft))
            return false;
        response.file_transfers.push_back(std::move(ft));
    }

    return true;
}

bool parse__(const wxml::Tree &response_tree, GetHostInfoResponse &response) {
    auto host_info_node = response_tree.root.find_child("host_info");
    return response_tree.root.found_child(host_info_node) && parse(*host_info_node, response.host_info);
}

bool parse__(const wxml::Tree &response_tree, GetMessagesResponse &response) {
    auto msgs_node = response_tree.root.find_child("msgs");
    if (!response_tree.root.found_child(msgs_node))
        return false;

    response.messages.reserve(msgs_node->children.size());
    for (auto &result_node : msgs_node->children) {
        woinc::Message msg;
        if (!parse(result_node, msg))
            return false;
        response.messages.push_back(std::move(msg));
    }

    return true;
}

bool parse__(const wxml::Tree &response_tree, GetNoticesResponse &response) {
    auto notices_node = response_tree.root.find_child("notices");
    if (!response_tree.root.found_child(notices_node))
        return false;

    response.notices.reserve(notices_node->children.size());
    for (auto &result_node : notices_node->children) {
        woinc::Notice notice;
        if (!parse(result_node, notice))
            return false;
        if (notice.seqno == -1) // dummy notice to signal refresh
            response.refreshed = true;
        else
            response.notices.push_back(std::move(notice));
    }

    return true;
}

bool parse__(const wxml::Tree &response_tree, GetProjectConfigPollResponse &response) {
    auto project_config_node = response_tree.root.find_child("project_config");
    return response_tree.root.found_child(project_config_node) && parse(*project_config_node, response.project_config);
}

bool parse__(const wxml::Tree &response_tree, GetProjectStatusResponse &response) {
    auto projects_node = response_tree.root.find_child("projects");
    if (!response_tree.root.found_child(projects_node))
        return false;

    for (auto &result_node : projects_node->children) {
        woinc::Project project;
        if (!parse(result_node, project))
            return false;
        response.projects.push_back(std::move(project));
    }

    return true;
}

bool parse__(const wxml::Tree &response_tree, GetResultsResponse &response) {
    auto results_node = response_tree.root.find_child("results");
    if (!response_tree.root.found_child(results_node))
        return false;

    for (auto &result_node : results_node->children) {
        woinc::Task task;
        if (!parse(result_node, task))
            return false;
        response.tasks.push_back(std::move(task));
    }

    return true;
}

bool parse__(const wxml::Tree &response_tree, GetStatisticsResponse &response) {
    auto statistics_node = response_tree.root.find_child("statistics");
    return response_tree.root.found_child(statistics_node) && parse(*statistics_node, response.statistics);
}

bool parse__(const wxml::Tree &response_tree, GetGlobalPreferencesResponse &response) {
    auto prefs_node = response_tree.root.find_child("global_preferences");
    return response_tree.root.found_child(prefs_node) && parse(*prefs_node, response.preferences);
}

bool parse__(const wxml::Tree &response_tree, LookupAccountPollResponse &response) {
    auto account_out_node = response_tree.root.find_child("account_out");
    return response_tree.root.found_child(account_out_node) && parse(*account_out_node, response.account_out);
}

template<typename Response>
CommandStatus do_cmd__(Connection &connection,
                       const wxml::Tree &request_tree,
                       std::string &error_holder,
                       Response &response) {
    wxml::Tree response_tree;

    auto status = do_rpc__(connection, request_tree, response_tree, error_holder);
    if (status != CommandStatus::Ok)
        return status;

    return parse__(response_tree, response) ? CommandStatus::Ok : CommandStatus::ParsingError;
}

template<typename Response>
CommandStatus do_cmd__(Connection &connection,
                       const char *cmd,
                       std::string &error_holder,
                       Response &response) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    request_tree.root[cmd];
    return do_cmd__(connection, request_tree, error_holder, response);
}

wxml::Tree set_mode_request__(const char *cmd, woinc::RunMode m, double duration) {
    const char *mode = nullptr;

    switch (m) {
        case woinc::RunMode::Always: mode = "always"; break;
        case woinc::RunMode::Auto: mode = "auto"; break;
        case woinc::RunMode::Never: mode = "never"; break;
        case woinc::RunMode::Restore: mode = "restore"; break;
        default: throw std::invalid_argument("Unknown run mode: " + std::to_string(static_cast<int>(m)));
    }

    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    auto &cmd_node = request_tree.root[cmd];
    cmd_node[mode];
    cmd_node["duration"] = duration;

    return request_tree;
}

}

namespace woinc { namespace rpc {

template<>
CommandStatus AuthorizeCommand::execute(Connection &connection) {
    response_.authorized = false;
    std::string nonce;

    if (request_.password.empty()) {
        error_ = "The password is missing";
        return CommandStatus::LogicError;
    }

    { // send auth1 request and parse the nonce response
        wxml::Tree request_tree(wxml::create_boinc_request_tree());
        request_tree.root["auth1"];

        wxml::Tree response_tree;

        auto status = do_rpc__(connection, request_tree, response_tree, error_);
        if (status != CommandStatus::Ok)
            return status;

        auto nonce_node = response_tree.root.find_child("nonce");
        if (!response_tree.root.found_child(nonce_node))
            return CommandStatus::ParsingError;

        nonce = nonce_node->content;
    }

    { // send auth2
        wxml::Tree request_tree(wxml::create_boinc_request_tree());
        request_tree.root["auth2"]["nonce_hash"] = md5(nonce + request_.password);

        wxml::Tree response_tree;

        auto status = do_rpc__(connection, request_tree, response_tree, error_);
        if (status != CommandStatus::Ok)
            return status;

        response_.authorized = response_tree.root.has_child("authorized");
    }

    return CommandStatus::Ok;
}

template<>
CommandStatus ExchangeVersionsCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    auto &request_node = request_tree.root["exchange_versions"];
    request_node["major"]   = request_.version.major;
    request_node["minor"]   = request_.version.minor;
    request_node["release"] = request_.version.release;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
CommandStatus GetAllProjectsListCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_all_projects_list", error_, response());
}

template<>
CommandStatus GetCCConfigCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_cc_config", error_, response());
}

template<>
CommandStatus GetCCStatusCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_cc_status", error_, response());
}

template<>
CommandStatus GetClientStateCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_state", error_, response());
}

template<>
CommandStatus GetDiskUsageCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_disk_usage", error_, response());
}

template<>
CommandStatus GetFileTransfersCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_file_transfers", error_, response());
}

GetGlobalPreferencesRequest::GetGlobalPreferencesRequest(GetGlobalPrefsMode m)
    : mode(m) {}

template<>
CommandStatus GetGlobalPreferencesCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    const char *mode = nullptr;

    switch (request().mode) {
        case GetGlobalPrefsMode::File: mode = "file"; break;
        case GetGlobalPrefsMode::Override: mode = "override"; break;
        case GetGlobalPrefsMode::Working: mode = "working"; break;
    }

    assert(mode);
    request_tree.root[std::string("get_global_prefs_") + mode];

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
CommandStatus GetHostInfoCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_host_info", error_, response());
}

template<>
CommandStatus GetMessagesCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    auto &request_node = request_tree.root["get_messages"];
    request_node["seqno"] = request_.seqno;
    if (request_.translatable)
        request_node["translatable"];

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
CommandStatus GetNoticesCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    auto &request_node = request_tree.root["get_notices"];
    request_node["seqno"] = request_.seqno;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
CommandStatus GetProjectConfigCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    request_tree.root["get_project_config"]["url"] = request().url;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
CommandStatus GetProjectConfigPollCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_project_config_poll", error_, response());
}

template<>
CommandStatus GetProjectStatusCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_project_status", error_, response());
}

template<>
CommandStatus GetResultsCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    request_tree.root["get_results"]["active_only"] = request_.active_only ? 1 : 0;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
CommandStatus GetStatisticsCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_statistics", error_, response());
}

LookupAccountRequest::LookupAccountRequest(std::string url, std::string mail, std::string password)
    : email(std::move(mail)), master_url(std::move(url)), passwd(std::move(password)) {}

template<>
CommandStatus LookupAccountCommand::execute(Connection &connection) {
    assert(!request().master_url.empty());
    assert(!request().email.empty());
    assert(!request().passwd.empty());

    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    auto &cmd_node = request_tree.root["lookup_account"];
    cmd_node["url"] = request().master_url;
    cmd_node["email_addr"] = request().email;
    cmd_node["passwd_hash"] = md5(request().passwd + request().email);
    cmd_node["ldap_auth"] = request().ldap_auth ? 1 : 0;
    cmd_node["server_assigned_cookie"] = request().server_assigned_cookie ? 1 : 0;
    cmd_node["server_cookie"] = request().server_cookie;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
CommandStatus LookupAccountPollCommand::execute(Connection &connection) {
    return do_cmd__(connection, "lookup_account_poll", error_, response());
}

template<>
CommandStatus NetworkAvailableCommand::execute(Connection &connection) {
    return do_cmd__(connection, "network_available", error_, response());
}

ProjectAttachRequest::ProjectAttachRequest(std::string url, std::string auth, std::string project)
    : master_url(std::move(url)), authenticator(std::move(auth)), project_name(std::move(project)) {}

template<>
CommandStatus ProjectAttachCommand::execute(Connection &connection) {
    assert(!request().master_url.empty());
    assert(!request().authenticator.empty());

    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    auto &cmd_node = request_tree.root["project_attach"];
    cmd_node["project_url"] = request().master_url;
    cmd_node["authenticator"] = request().authenticator;
    cmd_node["project_name"] = request().project_name;

    return do_cmd__(connection, request_tree, error_, response());
}

ProjectOpRequest::ProjectOpRequest(ProjectOp o, std::string url)
    : op(o), master_url(std::move(url)) {}

template<>
CommandStatus ProjectOpCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    const char *op = nullptr;

    switch (request().op) {
        case ProjectOp::Allowmorework: op = "allowmorework"; break;
        case ProjectOp::Detach: op = "detach"; break;
        case ProjectOp::DetachWhenDone: op = "detach_when_done"; break;
        case ProjectOp::DontDetachWhenDone: op = "dont_detach_when_done"; break;
        case ProjectOp::Nomorework: op = "nomorework"; break;
        case ProjectOp::Reset: op = "reset"; break;
        case ProjectOp::Resume: op = "resume"; break;
        case ProjectOp::Suspend: op = "suspend"; break;
        case ProjectOp::Update: op = "update"; break;
    }

    assert(op);
    assert(!request().master_url.empty());
    auto &cmd_node = request_tree.root[std::string("project_") + std::string(op)];
    cmd_node["project_url"] = request().master_url;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
CommandStatus QuitCommand::execute(Connection &connection) {
    return do_cmd__(connection, "quit", error_, response());
}

template<>
CommandStatus ReadCCConfigCommand::execute(Connection &connection) {
    return do_cmd__(connection, "read_cc_config", error_, response());
}

template<>
CommandStatus ReadGlobalPreferencesOverrideCommand::execute(Connection &connection) {
    return do_cmd__(connection, "read_global_prefs_override", error_, response());
}

template<>
CommandStatus RunBenchmarksCommand::execute(Connection &connection) {
    return do_cmd__(connection, "run_benchmarks", error_, response());
}

template<>
CommandStatus SetCCConfigCommand::execute(Connection &connection) {
    // to avoid removing configs woinc doesn't know we read the current cc config first
    // and update all the values woinc knows in the xml tree before sending it back to the client
    wxml::Tree current_ccc_tree;

    {
        wxml::Tree request_tree(wxml::create_boinc_request_tree());
        request_tree.root["get_cc_config"];
        auto status = do_rpc__(connection, request_tree, current_ccc_tree, error_);
        if (status != CommandStatus::Ok)
            return status;
    }

    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    auto &cmd_node = request_tree.root["set_cc_config"];
    auto &ccc_node = cmd_node["cc_config"];
    ccc_node = current_ccc_tree.root["cc_config"];
    auto &options_tree = ccc_node["options"];
    auto &cc_config = request().cc_config;

#define WOINC_MAP_OPTION(OPTION) do { options_tree[#OPTION] = cc_config.OPTION; } while(false)

    WOINC_MAP_OPTION(abort_jobs_on_exit);
    WOINC_MAP_OPTION(allow_multiple_clients);
    WOINC_MAP_OPTION(allow_remote_gui_rpc);
    WOINC_MAP_OPTION(disallow_attach);
    WOINC_MAP_OPTION(dont_check_file_sizes);
    WOINC_MAP_OPTION(dont_contact_ref_site);
    WOINC_MAP_OPTION(dont_suspend_nci);
    WOINC_MAP_OPTION(dont_use_vbox);
    WOINC_MAP_OPTION(dont_use_wsl);
    WOINC_MAP_OPTION(exit_after_finish);
    WOINC_MAP_OPTION(exit_before_start);
    WOINC_MAP_OPTION(exit_when_idle);
    WOINC_MAP_OPTION(fetch_minimal_work);
    WOINC_MAP_OPTION(fetch_on_update);
    WOINC_MAP_OPTION(http_1_0);
    WOINC_MAP_OPTION(lower_client_priority);
    WOINC_MAP_OPTION(no_alt_platform);
    WOINC_MAP_OPTION(no_gpus);
    WOINC_MAP_OPTION(no_info_fetch);
    WOINC_MAP_OPTION(no_opencl);
    WOINC_MAP_OPTION(no_priority_change);
    WOINC_MAP_OPTION(os_random_only);
    WOINC_MAP_OPTION(report_results_immediately);
    WOINC_MAP_OPTION(run_apps_manually);
    WOINC_MAP_OPTION(simple_gui_only);
    WOINC_MAP_OPTION(skip_cpu_benchmarks);
    WOINC_MAP_OPTION(stderr_head);
    WOINC_MAP_OPTION(suppress_net_info);
    WOINC_MAP_OPTION(unsigned_apps_ok);
    WOINC_MAP_OPTION(use_all_gpus);
    WOINC_MAP_OPTION(use_certs);
    WOINC_MAP_OPTION(use_certs_only);
    WOINC_MAP_OPTION(vbox_window);
    WOINC_MAP_OPTION(rec_half_life_days);
    WOINC_MAP_OPTION(start_delay);
    WOINC_MAP_OPTION(http_transfer_timeout);
    WOINC_MAP_OPTION(http_transfer_timeout_bps);
    WOINC_MAP_OPTION(max_event_log_lines);
    WOINC_MAP_OPTION(max_file_xfers);
    WOINC_MAP_OPTION(max_file_xfers_per_project);
    WOINC_MAP_OPTION(max_stderr_file_size);
    WOINC_MAP_OPTION(max_stdout_file_size);
    WOINC_MAP_OPTION(max_tasks_reported);
    WOINC_MAP_OPTION(ncpus);
    WOINC_MAP_OPTION(process_priority);
    WOINC_MAP_OPTION(process_priority_special);
    WOINC_MAP_OPTION(save_stats_days);
    WOINC_MAP_OPTION(force_auth);

#undef WOINC_MAP_OPTION

    options_tree.remove_childs("alt_platform");
    for (auto &&platform : cc_config.alt_platforms)
        options_tree.add_child("alt_platform") = platform;

    options_tree.remove_childs("exclusive_app");
    for (auto &&app : cc_config.exclusive_apps)
        options_tree.add_child("exclusive__app") = app;

    options_tree.remove_childs("exclusive_gpu_app");
    for (auto &&app : cc_config.exclusive_gpu_apps)
        options_tree.add_child("exclusive_gpu_app") = app;

    return do_cmd__(connection, request_tree, error_, response());
}

SetGpuModeRequest::SetGpuModeRequest(RunMode m, double d)
    : mode(m), duration(d) {}

template<>
CommandStatus SetGpuModeCommand::execute(Connection &connection) {
    return do_cmd__(connection,
                    set_mode_request__("set_gpu_mode", request().mode, request().duration),
                    error_,
                    response());
}

SetNetworkModeRequest::SetNetworkModeRequest(RunMode m, double d)
    : mode(m), duration(d) {}

template<>
CommandStatus SetNetworkModeCommand::execute(Connection &connection) {
    return do_cmd__(connection,
                    set_mode_request__("set_network_mode", request().mode, request().duration),
                    error_,
                    response());
}

SetRunModeRequest::SetRunModeRequest(RunMode m, double d)
    : mode(m), duration(d) {}

template<>
CommandStatus SetRunModeCommand::execute(Connection &connection) {
    return do_cmd__(connection,
                    set_mode_request__("set_run_mode", request().mode, request().duration),
                    error_,
                    response());
}

TaskOpRequest::TaskOpRequest(TaskOp o, std::string url, std::string n)
    : op(o), master_url(std::move(url)), name(std::move(n)) {}

template<>
CommandStatus TaskOpCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    const char *op = nullptr;

    switch (request().op) {
        case TaskOp::Abort: op = "abort"; break;
        case TaskOp::Resume: op = "resume"; break;
        case TaskOp::Suspend: op = "suspend"; break;
    }

    assert(op);
    assert(!request().master_url.empty());
    assert(!request().name.empty());

    auto &cmd_node = request_tree.root[std::string(op) + std::string("_result")];
    cmd_node["project_url"] = request().master_url;
    cmd_node["name"] = request().name;

    return do_cmd__(connection, request_tree, error_, response());
}

FileTransferOpRequest::FileTransferOpRequest(FileTransferOp o, std::string url, std::string n)
    : op(o), master_url(std::move(url)), filename(std::move(n)) {}

template<>
CommandStatus FileTransferOpCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    const char *op = nullptr;

    switch (request().op) {
        case FileTransferOp::Retry: op = "retry"; break;
        case FileTransferOp::Abort: op = "abort"; break;
    }

    assert(op);
    assert(!request().master_url.empty());
    assert(!request().filename.empty());

    auto &cmd_node = request_tree.root[std::string(op) + std::string("_file_transfer")];
    cmd_node["project_url"] = request().master_url;
    cmd_node["filename"] = request().filename;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
CommandStatus SetGlobalPreferencesCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    const auto &prefs = request().preferences;
    const auto &mask = request().mask;

    auto &prefs_node = request_tree.root["set_global_prefs_override"]["global_preferences"];
    prefs_node.reset_indention_level = true;

#define WOINC_MAP_PREF(PREF) do { if (mask.PREF) prefs_node[#PREF] = prefs.PREF; } while(false)
    // some values must be set, otherwise the client would use some default values
#define WOINC_ALWAYS_MAP_PREF(PREF, DEFAULT) do { prefs_node[#PREF] = mask.PREF ? prefs.PREF : DEFAULT; } while(false)

    WOINC_MAP_PREF(confirm_before_connecting);
    WOINC_MAP_PREF(dont_verify_images);
    WOINC_MAP_PREF(hangup_if_dialed);
    WOINC_MAP_PREF(leave_apps_in_memory);
    WOINC_MAP_PREF(run_gpu_if_user_active);
    WOINC_MAP_PREF(run_if_user_active);
    WOINC_MAP_PREF(run_on_batteries);

    WOINC_MAP_PREF(cpu_scheduling_period_minutes);
    WOINC_MAP_PREF(cpu_usage_limit);
    WOINC_MAP_PREF(daily_xfer_limit_mb);
    WOINC_MAP_PREF(disk_interval);
    WOINC_ALWAYS_MAP_PREF(disk_max_used_gb, 0);
    WOINC_ALWAYS_MAP_PREF(disk_max_used_pct, 100);
    WOINC_ALWAYS_MAP_PREF(disk_min_free_gb, 0);
    WOINC_MAP_PREF(end_hour);
    WOINC_MAP_PREF(idle_time_to_run);
    WOINC_MAP_PREF(max_bytes_sec_down);
    WOINC_MAP_PREF(max_bytes_sec_up);
    WOINC_MAP_PREF(max_ncpus_pct);
    WOINC_MAP_PREF(net_end_hour);
    WOINC_MAP_PREF(net_start_hour);
    WOINC_MAP_PREF(ram_max_used_busy_pct);
    WOINC_MAP_PREF(ram_max_used_idle_pct);
    WOINC_MAP_PREF(start_hour);
    WOINC_ALWAYS_MAP_PREF(suspend_cpu_usage, 0);
    WOINC_MAP_PREF(vm_max_used_pct);
    WOINC_MAP_PREF(work_buf_additional_days);
    WOINC_MAP_PREF(work_buf_min_days);

    WOINC_MAP_PREF(daily_xfer_period_days);

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_MAP_PREF(network_wifi_only);
    WOINC_MAP_PREF(override_file_present);
    WOINC_MAP_PREF(battery_charge_min_pct);
    WOINC_MAP_PREF(battery_max_temperature);
    WOINC_MAP_PREF(mod_time);
    WOINC_MAP_PREF(suspend_if_no_recent_input);
    WOINC_MAP_PREF(max_cpus);
    WOINC_MAP_PREF(source_project);
#endif // WOINC_EXPOSE_FULL_STRUCTURES

    { // write day prefs
        std::set<woinc::DayOfWeek> days;

        std::transform(prefs.cpu_times.begin(), prefs.cpu_times.end(),
                       std::inserter(days, days.end()),
                       [](const auto &t) { return t.first; });
        std::transform(prefs.net_times.begin(), prefs.net_times.end(),
                       std::inserter(days, days.end()),
                       [](const auto &t) { return t.first; });

        for (auto &&day : days) {
            auto &node = prefs_node.add_child("day_prefs");
            node["day_of_week"] = static_cast<int>(day);

            auto cpu_time = prefs.cpu_times.find(day);
            if (cpu_time != prefs.cpu_times.end()) {
                node["start_hour"] = cpu_time->second.start;
                node["end_hour"] = cpu_time->second.end;
            }

            auto net_time = prefs.net_times.find(day);
            if (net_time != prefs.net_times.end()) {
                node["net_start_hour"] = net_time->second.start;
                node["net_end_hour"] = net_time->second.end;
            }
        }
    }

    return do_cmd__(connection, request_tree, error_, response());
}

}}
