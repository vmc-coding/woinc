/* lib/rpc_command.cc --
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

constexpr COMMAND_STATUS map__(CONNECTION_STATUS status) {
    switch (status) {
        case CONNECTION_STATUS::OK:
            return COMMAND_STATUS::OK;
        case CONNECTION_STATUS::DISCONNECTED:
            return COMMAND_STATUS::DISCONNECTED;
        case CONNECTION_STATUS::ERROR:
            return COMMAND_STATUS::CONNECTION_ERROR;
    }
    // workaround for GCC bug 86678
#if defined(__GNUC__) && __GNUC__ >= 9
    assert(false);
#endif
    return COMMAND_STATUS::LOGIC_ERROR;
}

constexpr COMMAND_STATUS as_status__(bool b) {
    return b ? COMMAND_STATUS::OK : COMMAND_STATUS::PARSING_ERROR;
}

COMMAND_STATUS do_rpc__(Connection &connection,
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
        return COMMAND_STATUS::PARSING_ERROR;

    if (response_tree.root.children.size() == 1
            && response_tree.root.children.front().tag == "unauthorized")
        return COMMAND_STATUS::UNAUTHORIZED;

    if (response_tree.root.children.size() == 1
            && response_tree.root.children.front().tag == "error") {
        error_holder = response_tree.root.children.front().content;
        return COMMAND_STATUS::CLIENT_ERROR;
    }

    return COMMAND_STATUS::OK;
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

template<typename RESPONSE>
COMMAND_STATUS do_cmd__(Connection &connection,
                        const wxml::Tree &request_tree,
                        std::string &error_holder,
                        RESPONSE &response) {
    wxml::Tree response_tree;

    auto status = do_rpc__(connection, request_tree, response_tree, error_holder);
    if (status != COMMAND_STATUS::OK)
        return status;

    return parse__(response_tree, response) ? COMMAND_STATUS::OK : COMMAND_STATUS::PARSING_ERROR;
}

template<typename RESPONSE>
COMMAND_STATUS do_cmd__(Connection &connection,
                        const char *cmd,
                        std::string &error_holder,
                        RESPONSE &response) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    request_tree.root[cmd];
    return do_cmd__(connection, request_tree, error_holder, response);
}

wxml::Tree set_mode_request__(const char *cmd, woinc::RUN_MODE m, double duration) {
    const char *mode = nullptr;

    switch (m) {
        case woinc::RUN_MODE::ALWAYS: mode = "always"; break;
        case woinc::RUN_MODE::AUTO: mode = "auto"; break;
        case woinc::RUN_MODE::NEVER: mode = "never"; break;
        case woinc::RUN_MODE::RESTORE: mode = "restore"; break;
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
COMMAND_STATUS AuthorizeCommand::execute(Connection &connection) {
    response_.authorized = false;
    std::string nonce;

    if (request_.password.empty()) {
        error_ = "The password is missing";
        return COMMAND_STATUS::LOGIC_ERROR;
    }

    { // send auth1 request and parse the nonce response
        wxml::Tree request_tree(wxml::create_boinc_request_tree());
        request_tree.root["auth1"];

        wxml::Tree response_tree;

        auto status = do_rpc__(connection, request_tree, response_tree, error_);
        if (status != COMMAND_STATUS::OK)
            return status;

        auto nonce_node = response_tree.root.find_child("nonce");
        if (!response_tree.root.found_child(nonce_node))
            return COMMAND_STATUS::PARSING_ERROR;

        nonce = nonce_node->content;
    }

    { // send auth2
        wxml::Tree request_tree(wxml::create_boinc_request_tree());
        request_tree.root["auth2"]["nonce_hash"] = md5(nonce + request_.password);

        wxml::Tree response_tree;

        auto status = do_rpc__(connection, request_tree, response_tree, error_);
        if (status != COMMAND_STATUS::OK)
            return status;

        response_.authorized = response_tree.root.has_child("authorized");
    }

    return COMMAND_STATUS::OK;
}

template<>
COMMAND_STATUS ExchangeVersionsCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    auto &request_node = request_tree.root["exchange_versions"];
    request_node["major"]   = request_.version.major;
    request_node["minor"]   = request_.version.minor;
    request_node["release"] = request_.version.release;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
COMMAND_STATUS GetAllProjectsListCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_all_projects_list", error_, response());
}

template<>
COMMAND_STATUS GetCCStatusCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_cc_status", error_, response());
}

template<>
COMMAND_STATUS GetClientStateCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_state", error_, response());
}

template<>
COMMAND_STATUS GetDiskUsageCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_disk_usage", error_, response());
}

template<>
COMMAND_STATUS GetFileTransfersCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_file_transfers", error_, response());
}

GetGlobalPreferencesRequest::GetGlobalPreferencesRequest(GET_GLOBAL_PREFS_MODE m)
    : mode(m) {}

template<>
COMMAND_STATUS GetGlobalPreferencesCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    const char *mode = nullptr;

    switch (request().mode) {
        case GET_GLOBAL_PREFS_MODE::FILE: mode = "file"; break;
        case GET_GLOBAL_PREFS_MODE::OVERRIDE: mode = "override"; break;
        case GET_GLOBAL_PREFS_MODE::WORKING: mode = "working"; break;
    }

    assert(mode);
    request_tree.root[std::string("get_global_prefs_") + mode];

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
COMMAND_STATUS GetHostInfoCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_host_info", error_, response());
}

template<>
COMMAND_STATUS GetMessagesCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    auto &request_node = request_tree.root["get_messages"];
    request_node["seqno"] = request_.seqno;
    if (request_.translatable)
        request_node["translatable"];

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
COMMAND_STATUS GetNoticesCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    auto &request_node = request_tree.root["get_notices"];
    request_node["seqno"] = request_.seqno;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
COMMAND_STATUS GetProjectConfigCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    request_tree.root["get_project_config"]["url"] = request().url;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
COMMAND_STATUS GetProjectConfigPollCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_project_config_poll", error_, response());
}

template<>
COMMAND_STATUS GetProjectStatusCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_project_status", error_, response());
}

template<>
COMMAND_STATUS GetResultsCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());
    request_tree.root["get_results"]["active_only"] = request_.active_only ? 1 : 0;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
COMMAND_STATUS GetStatisticsCommand::execute(Connection &connection) {
    return do_cmd__(connection, "get_statistics", error_, response());
}

template<>
COMMAND_STATUS NetworkAvailableCommand::execute(Connection &connection) {
    return do_cmd__(connection, "network_available", error_, response());
}

ProjectAttachRequest::ProjectAttachRequest(std::string url, std::string auth, std::string project)
    : master_url(std::move(url)), authenticator(std::move(auth)), project_name(std::move(project)) {}

template<>
COMMAND_STATUS ProjectAttachCommand::execute(Connection &connection) {
    assert(!request().master_url.empty());
    assert(!request().authenticator.empty());

    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    auto &cmd_node = request_tree.root["project_attach"];
    cmd_node["project_url"] = request().master_url;
    cmd_node["authenticator"] = request().authenticator;
    cmd_node["project_name"] = request().project_name;

    return do_cmd__(connection, request_tree, error_, response());
}

ProjectOpRequest::ProjectOpRequest(PROJECT_OP o, std::string url)
    : op(o), master_url(std::move(url)) {}

template<>
COMMAND_STATUS ProjectOpCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    const char *op = nullptr;

    switch (request().op) {
        case PROJECT_OP::ALLOWMOREWORK: op = "allowmorework"; break;
        case PROJECT_OP::DETACH: op = "detach"; break;
        case PROJECT_OP::DETACH_WHEN_DONE: op = "detach_when_done"; break;
        case PROJECT_OP::DONT_DETACH_WHEN_DONE: op = "dont_detach_when_done"; break;
        case PROJECT_OP::NOMOREWORK: op = "nomorework"; break;
        case PROJECT_OP::RESET: op = "reset"; break;
        case PROJECT_OP::RESUME: op = "resume"; break;
        case PROJECT_OP::SUSPEND: op = "suspend"; break;
        case PROJECT_OP::UPDATE: op = "update"; break;
    }

    assert(op);
    assert(!request().master_url.empty());
    auto &cmd_node = request_tree.root[std::string("project_") + std::string(op)];
    cmd_node["project_url"] = request().master_url;

    return do_cmd__(connection, request_tree, error_, response());
}

template<>
COMMAND_STATUS QuitCommand::execute(Connection &connection) {
    return do_cmd__(connection, "quit", error_, response());
}

template<>
COMMAND_STATUS ReadCCConfigCommand::execute(Connection &connection) {
    return do_cmd__(connection, "read_cc_config", error_, response());
}

template<>
COMMAND_STATUS ReadGlobalPreferencesOverrideCommand::execute(Connection &connection) {
    return do_cmd__(connection, "read_global_prefs_override", error_, response());
}

template<>
COMMAND_STATUS RunBenchmarksCommand::execute(Connection &connection) {
    return do_cmd__(connection, "run_benchmarks", error_, response());
}

SetGpuModeRequest::SetGpuModeRequest(RUN_MODE m, double d)
    : mode(m), duration(d) {}

template<>
COMMAND_STATUS SetGpuModeCommand::execute(Connection &connection) {
    return do_cmd__(connection,
                    set_mode_request__("set_gpu_mode", request().mode, request().duration),
                    error_,
                    response());
}

SetNetworkModeRequest::SetNetworkModeRequest(RUN_MODE m, double d)
    : mode(m), duration(d) {}

template<>
COMMAND_STATUS SetNetworkModeCommand::execute(Connection &connection) {
    return do_cmd__(connection,
                    set_mode_request__("set_network_mode", request().mode, request().duration),
                    error_,
                    response());
}

SetRunModeRequest::SetRunModeRequest(RUN_MODE m, double d)
    : mode(m), duration(d) {}

template<>
COMMAND_STATUS SetRunModeCommand::execute(Connection &connection) {
    return do_cmd__(connection,
                    set_mode_request__("set_run_mode", request().mode, request().duration),
                    error_,
                    response());
}

TaskOpRequest::TaskOpRequest(TASK_OP o, std::string url, std::string n)
    : op(o), master_url(std::move(url)), name(std::move(n)) {}

template<>
COMMAND_STATUS TaskOpCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    const char *op = nullptr;

    switch (request().op) {
        case TASK_OP::ABORT: op = "abort"; break;
        case TASK_OP::RESUME: op = "resume"; break;
        case TASK_OP::SUSPEND: op = "suspend"; break;
    }

    assert(op);
    assert(!request().master_url.empty());
    assert(!request().name.empty());

    auto &cmd_node = request_tree.root[std::string(op) + std::string("_result")];
    cmd_node["project_url"] = request().master_url;
    cmd_node["name"] = request().name;

    return do_cmd__(connection, request_tree, error_, response());
}

FileTransferOpRequest::FileTransferOpRequest(FILE_TRANSFER_OP o, std::string url, std::string n)
    : op(o), master_url(std::move(url)), filename(std::move(n)) {}

template<>
COMMAND_STATUS FileTransferOpCommand::execute(Connection &connection) {
    wxml::Tree request_tree(wxml::create_boinc_request_tree());

    const char *op = nullptr;

    switch (request().op) {
        case FILE_TRANSFER_OP::RETRY: op = "retry"; break;
        case FILE_TRANSFER_OP::ABORT: op = "abort"; break;
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
COMMAND_STATUS SetGlobalPreferencesCommand::execute(Connection &connection) {
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
        std::set<woinc::DAY_OF_WEEK> days;

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
