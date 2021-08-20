/* lib/rpc_parsing.cc --
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

#include "rpc_parsing.h"

#include <cassert>
#include <type_traits>

#ifndef NDEBUG
#include <iostream>
#include <map>
#endif

/*
 * Semantics of the functions in this file:
 * - parse__ functions parse POD types raising exceptions on error
 * - parse_ functions parse woinc types raising exceptions on error
 * - parse functions parse woinc types by wrapping the parse_ functions
 *   in a try-catch block and returning false on error
 */

namespace wxml = woinc::xml;

namespace {

template<typename From, typename To,
    std::enable_if_t<
        std::is_enum<To>::value && std::is_convertible<From, std::underlying_type_t<To>>::value,
        decltype(To::UnknownToWoinc) // ensures we have an enum defining UnknownToWoinc;
                                     // maybe there will be a nicer way to express this with concepts in c++20 and above
    > = To()>
void convert_to_enum__(From value, To &dest) {
    if (value < 0 || value >= static_cast<int>(To::UnknownToWoinc))
        dest = To::UnknownToWoinc;
    else
        dest = static_cast<To>(value);
}

void parse__(int value, woinc::NetworkStatus &dest) {
    convert_to_enum__(value, dest);
}

void parse__(int value, woinc::RunMode &dest) {
    convert_to_enum__(value - 1, dest);
}

void parse__(int value, woinc::SuspendReason &dest) {
    switch (value) {
        case    0: dest = woinc::SuspendReason::NotSuspended; break;
        case    1: dest = woinc::SuspendReason::Batteries; break;
        case    2: dest = woinc::SuspendReason::UserActive; break;
        case    4: dest = woinc::SuspendReason::UserReq; break;
        case    8: dest = woinc::SuspendReason::TimeOfDay; break;
        case   16: dest = woinc::SuspendReason::Benchmarks; break;
        case   32: dest = woinc::SuspendReason::DiskSize; break;
        case   64: dest = woinc::SuspendReason::CpuThrottle; break;
        case  128: dest = woinc::SuspendReason::NoRecentInput; break;
        case  256: dest = woinc::SuspendReason::InitialDelay; break;
        case  512: dest = woinc::SuspendReason::ExclusiveAppRunning; break;
        case 1024: dest = woinc::SuspendReason::CpuUsage; break;
        case 2048: dest = woinc::SuspendReason::NetworkQuotaExceeded; break;
        case 4096: dest = woinc::SuspendReason::Os; break;
        case 4097: dest = woinc::SuspendReason::WifiState; break;
        case 4098: dest = woinc::SuspendReason::BatteryCharging; break;
        case 4099: dest = woinc::SuspendReason::BatteryOverheated; break;
        case 4100: dest = woinc::SuspendReason::NoGuiKeepalive; break;
        default: dest = woinc::SuspendReason::UnknownToWoinc;
    }
}

void parse__(int value, woinc::SchedulerState &dest) {
    convert_to_enum__(value, dest);
}

void parse__(int value, woinc::ResultClientState &dest) {
    convert_to_enum__(value, dest);
}

void parse__(int value, woinc::ActiveTaskState &dest) {
    convert_to_enum__(value, dest);
}

void parse__(int value, woinc::MsgInfo &dest) {
    convert_to_enum__(value - 1, dest);
}

void parse__(int value, woinc::RpcReason &dest) {
    convert_to_enum__(value, dest);
}

void parse__(int value, woinc::DayOfWeek &dest) {
    convert_to_enum__(value, dest);
}

void parse__(const std::string &src, int &dest) {
    dest = std::stoi(src);
}

void parse__(const std::string &src, double &dest) {
    dest = std::stod(src);
}

void parse__(const std::string &src, std::string &dest) {
    dest = src;
}

void parse__(const std::string &src, time_t &dest) {
    double value; // BOINC sends time_t as double (oh, and sometimes as int ..)
    parse__(src, value);
    dest = static_cast<time_t>(value);
}

bool find_child(const wxml::Node &node, const wxml::Tag &child_tag, wxml::Nodes::const_iterator &it) {
    it = node.find_child(child_tag);
    if (!node.found_child(it)) {
#ifdef WOINC_VERBOSE_DEBUG_LOGGING
        static std::map<wxml::Tag, std::map<wxml::Tag, bool>> diag_found_nodes;
        if (!diag_found_nodes[node.tag][child_tag]) {
            std::cerr << "Child node \"" << child_tag << "\" of parent \"" << node.tag << "\" not found!\n";
            diag_found_nodes[node.tag][child_tag] = true;
        }
#endif
        return false;
    }
    return true;
}

template<typename T, std::enable_if_t<!std::is_enum<T>::value, int> = 0>
void parse_child_content_(const wxml::Node &node, const wxml::Tag &child_tag, T &dest) {
    // to be compatible with various versions of BOINC
    // we just skip non existing tags instead of failing out
    wxml::Nodes::const_iterator child_iter;
    if (!find_child(node, child_tag, child_iter))
        return;

    try {
        parse__(child_iter->content, dest);
    } catch (...) {
#ifndef NDEBUG
        std::cerr << "Value of node with tag " << child_tag << " does have wrong format\n";
#endif
        throw;
    }
}

template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
void parse_child_content_(const wxml::Node &node, const wxml::Tag &child_tag, T &dest) {
    wxml::Nodes::const_iterator child_iter;
    if (!find_child(node, child_tag, child_iter))
        return;

#ifndef NDEBUG
    try {
#endif
        int value;
        parse__(child_iter->content, value);
        parse__(value, dest);
#ifndef NDEBUG
        if (dest == T::UnknownToWoinc)
            std::cerr << "Value of node with tag " << child_tag << " out of range\n";
        // we should adopt the unknown values, so let's fail out in dev mode
        assert(dest != T::UnknownToWoinc);
    } catch (...) {
        std::cerr << "Value of node with tag " << child_tag << " does have wrong format\n";
        throw;
    }
#endif
}

template<typename T = bool>
void parse_child_content_(const wxml::Node &node, const wxml::Tag &child_tag, bool &dest) {
    // non existing bool values in the xml default to false,
    // see: BOINC/lib/parse.cpp: XML_PARSER::parse_bool()
    auto child = node.find_child(child_tag);
    dest = node.found_child(child) && child->content != "0";
}

void parse_(const woinc::xml::Node &node, woinc::AccountOut &account_out);
void parse_(const woinc::xml::Node &node, woinc::ActiveTask &active_task);
void parse_(const woinc::xml::Node &node, woinc::AllProjectsList &projects);
void parse_(const woinc::xml::Node &node, woinc::App &app);
void parse_(const woinc::xml::Node &node, woinc::AppVersion &app_version);
void parse_(const woinc::xml::Node &node, woinc::CCStatus &cc_status);
void parse_(const woinc::xml::Node &node, woinc::ClientState &client_state);
void parse_(const woinc::xml::Node &node, woinc::DailyStatistic &daily_statistic);
void parse_(const woinc::xml::Node &node, woinc::DiskUsage &disk_usage);
void parse_(const woinc::xml::Node &node, woinc::FileRef &file_ref);
void parse_(const woinc::xml::Node &node, woinc::FileTransfer &file_transfer);
void parse_(const woinc::xml::Node &node, woinc::FileXfer &file_xfer);
void parse_(const woinc::xml::Node &node, woinc::GlobalPreferences &global_prefs);
void parse_(const woinc::xml::Node &node, woinc::GuiUrl &gui_url);
void parse_(const woinc::xml::Node &node, woinc::HostInfo &info);
void parse_(const woinc::xml::Node &node, woinc::Message &msg);
void parse_(const woinc::xml::Node &node, woinc::Notice &notice);
void parse_(const woinc::xml::Node &node, woinc::PersistentFileXfer &persistent_file_xfer);
void parse_(const woinc::xml::Node &node, woinc::Platform &platform);
void parse_(const woinc::xml::Node &node, woinc::Project &project);
void parse_(const woinc::xml::Node &node, woinc::ProjectStatistics &project_statistics);
void parse_(const woinc::xml::Node &node, woinc::Statistics &statistics);
void parse_(const woinc::xml::Node &node, woinc::Task &task);
void parse_(const woinc::xml::Node &node, woinc::TimeStats &time_stats);
void parse_(const woinc::xml::Node &node, woinc::Version &version);
void parse_(const woinc::xml::Node &node, woinc::Workunit &workunit);

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
void parse_(const woinc::xml::Node &node, woinc::NetStats &net_stats);
#endif

#define WOINC_PARSE_CHILD_CONTENT(NODE, RESULT_STRUCT, TAG) \
    parse_child_content_(NODE, #TAG, RESULT_STRUCT . TAG)

void parse_(const wxml::Node &node, woinc::AccountOut &account_out) {
    account_out.error_num = 0; // it's not sent if polling is done, so let's reset it before parsing
    WOINC_PARSE_CHILD_CONTENT(node, account_out, error_num);
    if (account_out.error_num != 0)
        return;
    WOINC_PARSE_CHILD_CONTENT(node, account_out, authenticator);
    WOINC_PARSE_CHILD_CONTENT(node, account_out, error_msg);
}

// see ACTIVE_TASK::write_gui() in BOINC/client/app.cpp
void parse_(const wxml::Node &node, woinc::ActiveTask &active_task) {
    WOINC_PARSE_CHILD_CONTENT(node, active_task, active_task_state);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, scheduler_state);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, too_large);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, pid);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, slot);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, needs_shmem);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, checkpoint_cpu_time);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, elapsed_time);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, fraction_done);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, current_cpu_time);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, progress_rate);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, swap_size);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, working_set_size_smoothed);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, bytes_sent);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, bytes_received);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, active_task, page_fault_rate);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, working_set_size);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, app_version_num);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, graphics_exec_path);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, slot_path);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, web_graphics_url);
    WOINC_PARSE_CHILD_CONTENT(node, active_task, remote_desktop_addr);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

void parse_(const woinc::xml::Node &node, woinc::AllProjectsList &projects) {
    for (auto &project_node : node.children) {
        if (project_node.tag != "project")
            continue;
        woinc::ProjectListEntry entry;
        WOINC_PARSE_CHILD_CONTENT(project_node, entry, description);
        WOINC_PARSE_CHILD_CONTENT(project_node, entry, general_area);
        WOINC_PARSE_CHILD_CONTENT(project_node, entry, home);
        WOINC_PARSE_CHILD_CONTENT(project_node, entry, image);
        WOINC_PARSE_CHILD_CONTENT(project_node, entry, name);
        WOINC_PARSE_CHILD_CONTENT(project_node, entry, specific_area);
        WOINC_PARSE_CHILD_CONTENT(project_node, entry, url);
        WOINC_PARSE_CHILD_CONTENT(project_node, entry, web_url);
        auto platforms_node = project_node.find_child("platforms");
        if (project_node.found_child(platforms_node)) {
            for (auto &platform_node : platforms_node->children) {
                woinc::Platform platform;
                parse_(platform_node, platform);
                entry.platforms.push_back(std::move(platform));
            }
        }
        projects.push_back(std::move(entry));
    }
}

void parse_(const wxml::Node &node, woinc::App &app) {
    WOINC_PARSE_CHILD_CONTENT(node, app, non_cpu_intensive);
    WOINC_PARSE_CHILD_CONTENT(node, app, name);
    WOINC_PARSE_CHILD_CONTENT(node, app, user_friendly_name);
}

void parse_(const wxml::Node &node, woinc::AppVersion &app_version) {
    WOINC_PARSE_CHILD_CONTENT(node, app_version, avg_ncpus);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, flops);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, version_num);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, app_name);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, plan_class);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, platform);

    for (auto &n : node.children) {
        if (n.tag == "file_ref") {
            woinc::FileRef f;
            parse_(n, f);
            app_version.file_refs.push_back(std::move(f));
        }
    }
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, app_version, dont_throttle);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, is_wrapper);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, needs_network);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, gpu_ram);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, api_version);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, cmdline);
    WOINC_PARSE_CHILD_CONTENT(node, app_version, file_prefix);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

// see handle_get_cc_status() in BOINC/client/gui_rpc_server_ops.cpp
void parse_(const wxml::Node &node, woinc::CCStatus &cc_status) {
    parse_child_content_(node, "network_status"        , cc_status.network_status);
    parse_child_content_(node, "task_suspend_reason"   , cc_status.cpu.suspend_reason);
    parse_child_content_(node, "task_mode"             , cc_status.cpu.mode);
    parse_child_content_(node, "task_mode_perm"        , cc_status.cpu.perm_mode);
    parse_child_content_(node, "task_mode_delay"       , cc_status.cpu.delay);
    parse_child_content_(node, "gpu_suspend_reason"    , cc_status.gpu.suspend_reason);
    parse_child_content_(node, "gpu_mode"              , cc_status.gpu.mode);
    parse_child_content_(node, "gpu_mode_perm"         , cc_status.gpu.perm_mode);
    parse_child_content_(node, "gpu_mode_delay"        , cc_status.gpu.delay);
    parse_child_content_(node, "network_suspend_reason", cc_status.network.suspend_reason);
    parse_child_content_(node, "network_mode"          , cc_status.network.mode);
    parse_child_content_(node, "network_mode_perm"     , cc_status.network.perm_mode);
    parse_child_content_(node, "network_mode_delay"    , cc_status.network.delay);
}

void parse_(const woinc::xml::Node &node, woinc::DailyStatistic &daily_statistic) {
    WOINC_PARSE_CHILD_CONTENT(node, daily_statistic, host_expavg_credit);
    WOINC_PARSE_CHILD_CONTENT(node, daily_statistic, host_total_credit);
    WOINC_PARSE_CHILD_CONTENT(node, daily_statistic, user_expavg_credit);
    WOINC_PARSE_CHILD_CONTENT(node, daily_statistic, user_total_credit);
    WOINC_PARSE_CHILD_CONTENT(node, daily_statistic, day);
}

void parse_(const woinc::xml::Node &node, woinc::DiskUsage &disk_usage) {
    parse_child_content_(node, "d_allowed", disk_usage.allowed);
    parse_child_content_(node, "d_boinc", disk_usage.boinc);
    parse_child_content_(node, "d_free", disk_usage.free);
    parse_child_content_(node, "d_total", disk_usage.total);

    disk_usage.projects.reserve(node.children.size() - 4);

    for (const auto &child : node.children) {
        if (child.tag != "project")
            continue;
        woinc::DiskUsage::Project project;

        parse_child_content_(child, "master_url", project.master_url);
        parse_child_content_(child, "disk_usage", project.disk_usage);

        disk_usage.projects.push_back(project);
    }
}

void parse_(const wxml::Node &node, woinc::ClientState &client_state) {
    std::string current_project_url;

    for (const auto &child: node.children) {
        if (child.tag == "app_version") {
            woinc::AppVersion app_version;
            parse_(child, app_version);
            app_version.project_url = current_project_url;
            client_state.app_versions.push_back(std::move(app_version));
        } else if (child.tag == "app") {
            woinc::App app;
            parse_(child, app);
            app.project_url = current_project_url;
            client_state.apps.push_back(std::move(app));
        } else if (child.tag == "project") {
            woinc::Project project;
            parse_(child, project);
            current_project_url = project.master_url;
            client_state.projects.push_back(std::move(project));
        } else if (child.tag == "result") {
            woinc::Task task;
            parse_(child, task);
            client_state.tasks.push_back(std::move(task));
        } else if (child.tag == "time_stats") {
            parse_(child, client_state.time_stats);
        } else if (child.tag == "workunit") {
            woinc::Workunit workunit;
            parse_(child, workunit);
            workunit.project_url = current_project_url;
            client_state.workunits.push_back(std::move(workunit));
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
        } else if (child.tag == "global_preferences") {
            parse_(child, client_state.global_prefs);
        } else if (child.tag == "host_info") {
            parse_(child, client_state.host_info);
        } else if (child.tag == "platform") {
            woinc::Platform platform;
            parse_(child, platform);
            client_state.platforms.push_back(std::move(platform));
        } else if (child.tag == "net_stats") {
            client_state.net_stats = std::make_unique<woinc::NetStats>();
            parse_(child, *client_state.net_stats);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
        }
    }

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, client_state, executing_as_daemon);
    WOINC_PARSE_CHILD_CONTENT(node, client_state, have_ati);
    WOINC_PARSE_CHILD_CONTENT(node, client_state, have_cuda);
    WOINC_PARSE_CHILD_CONTENT(node, client_state, platform_name);
    parse_child_content_(node, "core_client_major_version", client_state.core_client_version.major);
    parse_child_content_(node, "core_client_minor_version", client_state.core_client_version.minor);
    parse_child_content_(node, "core_client_release", client_state.core_client_version.release);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

void parse_(const wxml::Node &node, woinc::FileRef &file_ref) {
    WOINC_PARSE_CHILD_CONTENT(node, file_ref, main_program);
    WOINC_PARSE_CHILD_CONTENT(node, file_ref, file_name);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, file_ref, copy_file);
    WOINC_PARSE_CHILD_CONTENT(node, file_ref, optional);
    WOINC_PARSE_CHILD_CONTENT(node, file_ref, open_name);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

void parse_(const woinc::xml::Node &node, woinc::FileTransfer &file_transfer) {
    WOINC_PARSE_CHILD_CONTENT(node, file_transfer, nbytes);
    WOINC_PARSE_CHILD_CONTENT(node, file_transfer, status);
    WOINC_PARSE_CHILD_CONTENT(node, file_transfer, name);
    WOINC_PARSE_CHILD_CONTENT(node, file_transfer, project_name);
    WOINC_PARSE_CHILD_CONTENT(node, file_transfer, project_url);
    WOINC_PARSE_CHILD_CONTENT(node, file_transfer, project_backoff);

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, file_transfer, max_nbytes);
#endif

    auto persistent_file_xfer_node = node.find_child("persistent_file_xfer");
    if (node.found_child(persistent_file_xfer_node)) {
        file_transfer.persistent_file_xfer = std::make_unique<woinc::PersistentFileXfer>();
        parse_(*persistent_file_xfer_node, *file_transfer.persistent_file_xfer);
    }

    auto file_xfer_node = node.find_child("file_xfer");
    if (node.found_child(file_xfer_node)) {
        file_transfer.file_xfer = std::make_unique<woinc::FileXfer>();
        parse_(*file_xfer_node, *file_transfer.file_xfer);
    }
}

void parse_(const woinc::xml::Node &node, woinc::FileXfer &file_xfer) {
    WOINC_PARSE_CHILD_CONTENT(node, file_xfer, bytes_xferred);
    WOINC_PARSE_CHILD_CONTENT(node, file_xfer, xfer_speed);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, file_xfer, file_offset);
    WOINC_PARSE_CHILD_CONTENT(node, file_xfer, url);
#endif
}

void parse_(const wxml::Node &node, woinc::GlobalPreferences &global_prefs) {
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, confirm_before_connecting);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, dont_verify_images);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, hangup_if_dialed);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, leave_apps_in_memory);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, run_gpu_if_user_active);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, run_if_user_active);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, run_on_batteries);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, cpu_scheduling_period_minutes);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, cpu_usage_limit);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, daily_xfer_limit_mb);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, disk_interval);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, disk_max_used_gb);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, disk_max_used_pct);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, disk_min_free_gb);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, end_hour);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, idle_time_to_run);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, max_bytes_sec_down);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, max_bytes_sec_up);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, max_ncpus_pct);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, net_end_hour);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, net_start_hour);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, ram_max_used_busy_pct);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, ram_max_used_idle_pct);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, start_hour);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, suspend_cpu_usage);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, work_buf_additional_days);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, work_buf_min_days);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, vm_max_used_pct);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, daily_xfer_period_days);

    auto prefs_node = node.find_child("day_prefs");
    while (node.found_child(prefs_node)) {
        woinc::DayOfWeek day;
        parse_child_content_(*prefs_node, "day_of_week", day);

        if (prefs_node->has_child("start_hour")) {
            assert(prefs_node->has_child("end_hour"));
            woinc::GlobalPreferences::TimeSpan span;
            parse_child_content_(*prefs_node, "start_hour", span.start);
            parse_child_content_(*prefs_node, "end_hour", span.end);
            global_prefs.cpu_times.emplace(day, std::move(span));
        }

        if (prefs_node->has_child("net_start_hour")) {
            assert(prefs_node->has_child("net_end_hour"));
            woinc::GlobalPreferences::TimeSpan span;
            parse_child_content_(*prefs_node, "net_start_hour", span.start);
            parse_child_content_(*prefs_node, "net_end_hour", span.end);
            global_prefs.net_times.emplace(day, std::move(span));
        }

        std::advance(prefs_node, 1);
        prefs_node = node.find_child(prefs_node, "day_prefs");
    }

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, network_wifi_only);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, override_file_present);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, battery_charge_min_pct);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, battery_max_temperature);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, mod_time);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, suspend_if_no_recent_input);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, max_cpus);
    WOINC_PARSE_CHILD_CONTENT(node, global_prefs, source_project);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

void parse_(const wxml::Node &node, woinc::GuiUrl &gui_url) {
    WOINC_PARSE_CHILD_CONTENT(node, gui_url, name);
    WOINC_PARSE_CHILD_CONTENT(node, gui_url, description);
    WOINC_PARSE_CHILD_CONTENT(node, gui_url, url);
}

void parse_(const woinc::xml::Node &node, woinc::HostInfo &info) {
    WOINC_PARSE_CHILD_CONTENT(node, info, d_free);
    WOINC_PARSE_CHILD_CONTENT(node, info, d_total);
    WOINC_PARSE_CHILD_CONTENT(node, info, m_cache);
    WOINC_PARSE_CHILD_CONTENT(node, info, m_nbytes);
    WOINC_PARSE_CHILD_CONTENT(node, info, m_swap);
    WOINC_PARSE_CHILD_CONTENT(node, info, p_fpops);
    WOINC_PARSE_CHILD_CONTENT(node, info, p_iops);
    WOINC_PARSE_CHILD_CONTENT(node, info, p_membw);
    WOINC_PARSE_CHILD_CONTENT(node, info, p_ncpus);
    WOINC_PARSE_CHILD_CONTENT(node, info, timezone);
    WOINC_PARSE_CHILD_CONTENT(node, info, domain_name);
    WOINC_PARSE_CHILD_CONTENT(node, info, ip_addr);
    WOINC_PARSE_CHILD_CONTENT(node, info, os_name);
    WOINC_PARSE_CHILD_CONTENT(node, info, os_version);
    WOINC_PARSE_CHILD_CONTENT(node, info, p_model);
    WOINC_PARSE_CHILD_CONTENT(node, info, p_vendor);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, info, p_vm_extensions_disabled);
    WOINC_PARSE_CHILD_CONTENT(node, info, p_calculated);
    WOINC_PARSE_CHILD_CONTENT(node, info, n_usable_coprocs);
    WOINC_PARSE_CHILD_CONTENT(node, info, host_cpid);
    WOINC_PARSE_CHILD_CONTENT(node, info, mac_address);
    WOINC_PARSE_CHILD_CONTENT(node, info, p_features);
    WOINC_PARSE_CHILD_CONTENT(node, info, product_name);
    WOINC_PARSE_CHILD_CONTENT(node, info, virtualbox_version);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

// ses MESSAGE_DESCS::write in BOINC/client/client_msgs.cpp
void parse_(const wxml::Node &node, woinc::Message &msg) {
    WOINC_PARSE_CHILD_CONTENT(node, msg, body);
    WOINC_PARSE_CHILD_CONTENT(node, msg, project);
    WOINC_PARSE_CHILD_CONTENT(node, msg, seqno);
    parse_child_content_(node, "pri", msg.priority);
    parse_child_content_(node, "time", msg.timestamp);
}

// see NET_STATS::write() in BOINC/client/net_stats.cc
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
void parse_(const wxml::Node &node, woinc::NetStats &net_stats) {
    WOINC_PARSE_CHILD_CONTENT(node, net_stats, bwup);
    WOINC_PARSE_CHILD_CONTENT(node, net_stats, avg_up);
    WOINC_PARSE_CHILD_CONTENT(node, net_stats, avg_time_up);
    WOINC_PARSE_CHILD_CONTENT(node, net_stats, bwdown);
    WOINC_PARSE_CHILD_CONTENT(node, net_stats, avg_down);
    WOINC_PARSE_CHILD_CONTENT(node, net_stats, avg_time_down);
}
#endif // WOINC_EXPOSE_FULL_STRUCTURES

// see NOTICE::write in BOINC/lib/notice.cpp
void parse_(const wxml::Node &node, woinc::Notice &notice) {
    WOINC_PARSE_CHILD_CONTENT(node, notice, seqno);
    WOINC_PARSE_CHILD_CONTENT(node, notice, category);
    WOINC_PARSE_CHILD_CONTENT(node, notice, description);
    WOINC_PARSE_CHILD_CONTENT(node, notice, link);
    WOINC_PARSE_CHILD_CONTENT(node, notice, project_name);
    WOINC_PARSE_CHILD_CONTENT(node, notice, title);
    WOINC_PARSE_CHILD_CONTENT(node, notice, create_time);

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, notice, is_private);
    WOINC_PARSE_CHILD_CONTENT(node, notice, is_youtube_video);
    WOINC_PARSE_CHILD_CONTENT(node, notice, arrival_time);
#endif
}

void parse_(const woinc::xml::Node &node, woinc::PersistentFileXfer &persistent_file_xfer) {
    WOINC_PARSE_CHILD_CONTENT(node, persistent_file_xfer, is_upload);
    WOINC_PARSE_CHILD_CONTENT(node, persistent_file_xfer, time_so_far);
    WOINC_PARSE_CHILD_CONTENT(node, persistent_file_xfer, next_request_time);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, persistent_file_xfer, last_bytes_xferred);
    WOINC_PARSE_CHILD_CONTENT(node, persistent_file_xfer, num_retries);
    WOINC_PARSE_CHILD_CONTENT(node, persistent_file_xfer, first_request_time);
#endif
}

void parse_(const wxml::Node &node, woinc::Platform &platform) {
    platform = node.content;
}

// see PROJECT::write_state in BOINC/client/project.cpp
void parse_(const wxml::Node &node, woinc::Project &project) {
    WOINC_PARSE_CHILD_CONTENT(node, project, anonymous_platform);
    WOINC_PARSE_CHILD_CONTENT(node, project, attached_via_acct_mgr);
    WOINC_PARSE_CHILD_CONTENT(node, project, detach_when_done);
    WOINC_PARSE_CHILD_CONTENT(node, project, dont_request_more_work);
    WOINC_PARSE_CHILD_CONTENT(node, project, ended);
    WOINC_PARSE_CHILD_CONTENT(node, project, master_url_fetch_pending);
    WOINC_PARSE_CHILD_CONTENT(node, project, non_cpu_intensive);
    WOINC_PARSE_CHILD_CONTENT(node, project, scheduler_rpc_in_progress);
    WOINC_PARSE_CHILD_CONTENT(node, project, suspended_via_gui);
    WOINC_PARSE_CHILD_CONTENT(node, project, trickle_up_pending);

    WOINC_PARSE_CHILD_CONTENT(node, project, desired_disk_usage);
    WOINC_PARSE_CHILD_CONTENT(node, project, elapsed_time);
    WOINC_PARSE_CHILD_CONTENT(node, project, host_expavg_credit);
    WOINC_PARSE_CHILD_CONTENT(node, project, host_total_credit);
    WOINC_PARSE_CHILD_CONTENT(node, project, project_files_downloaded_time);
    WOINC_PARSE_CHILD_CONTENT(node, project, resource_share);
    WOINC_PARSE_CHILD_CONTENT(node, project, sched_priority);
    WOINC_PARSE_CHILD_CONTENT(node, project, user_expavg_credit);
    WOINC_PARSE_CHILD_CONTENT(node, project, user_total_credit);

    WOINC_PARSE_CHILD_CONTENT(node, project, hostid);
    WOINC_PARSE_CHILD_CONTENT(node, project, master_fetch_failures);
    WOINC_PARSE_CHILD_CONTENT(node, project, njobs_error);
    WOINC_PARSE_CHILD_CONTENT(node, project, njobs_success);
    WOINC_PARSE_CHILD_CONTENT(node, project, nrpc_failures);

    WOINC_PARSE_CHILD_CONTENT(node, project, sched_rpc_pending);

    WOINC_PARSE_CHILD_CONTENT(node, project, external_cpid);
    WOINC_PARSE_CHILD_CONTENT(node, project, master_url);
    WOINC_PARSE_CHILD_CONTENT(node, project, project_name);
    WOINC_PARSE_CHILD_CONTENT(node, project, team_name);
    WOINC_PARSE_CHILD_CONTENT(node, project, user_name);
    WOINC_PARSE_CHILD_CONTENT(node, project, venue);

    WOINC_PARSE_CHILD_CONTENT(node, project, download_backoff);
    WOINC_PARSE_CHILD_CONTENT(node, project, last_rpc_time);
    WOINC_PARSE_CHILD_CONTENT(node, project, min_rpc_time);
    WOINC_PARSE_CHILD_CONTENT(node, project, upload_backoff);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, project, dont_use_dcf);
    WOINC_PARSE_CHILD_CONTENT(node, project, send_full_workload);
    WOINC_PARSE_CHILD_CONTENT(node, project, use_symlinks);
    WOINC_PARSE_CHILD_CONTENT(node, project, verify_files_on_app_start);

    WOINC_PARSE_CHILD_CONTENT(node, project, ams_resource_share_new);
    WOINC_PARSE_CHILD_CONTENT(node, project, cpid_time);
    WOINC_PARSE_CHILD_CONTENT(node, project, duration_correction_factor);
    WOINC_PARSE_CHILD_CONTENT(node, project, host_create_time);
    WOINC_PARSE_CHILD_CONTENT(node, project, next_rpc_time);
    WOINC_PARSE_CHILD_CONTENT(node, project, rec);
    WOINC_PARSE_CHILD_CONTENT(node, project, rec_time);
    WOINC_PARSE_CHILD_CONTENT(node, project, user_create_time);

    WOINC_PARSE_CHILD_CONTENT(node, project, rpc_seqno);
    WOINC_PARSE_CHILD_CONTENT(node, project, send_job_log);
    WOINC_PARSE_CHILD_CONTENT(node, project, send_time_stats_log);
    WOINC_PARSE_CHILD_CONTENT(node, project, teamid);
    WOINC_PARSE_CHILD_CONTENT(node, project, userid);

    WOINC_PARSE_CHILD_CONTENT(node, project, cross_project_id);
    WOINC_PARSE_CHILD_CONTENT(node, project, email_hash);
    WOINC_PARSE_CHILD_CONTENT(node, project, host_venue);
    WOINC_PARSE_CHILD_CONTENT(node, project, project_dir);
    WOINC_PARSE_CHILD_CONTENT(node, project, symstore);
#endif // WOINC_EXPOSE_FULL_STRUCTURES

    auto gui_urls_node = node.find_child("gui_urls");
    if (node.found_child(gui_urls_node)) {
        for (const auto &child : gui_urls_node->children) {
            woinc::GuiUrl gui_url;
            if (child.tag == "gui_url") {
                gui_url.ifteam = false;
                parse_(child, gui_url);
                project.gui_urls.push_back(gui_url);
            } else if (child.tag == "ifteam") {
                gui_url.ifteam = true;
                auto gui_url_child = child.find_child("gui_url");
                if (child.found_child(gui_url_child)) {
                    parse_(*gui_url_child, gui_url);
                    project.gui_urls.push_back(gui_url);
                }
            }
        }
    }
}

void parse_(const woinc::xml::Node &node, woinc::ProjectConfig &project_config) {
    project_config.error_num = 0; // it's not sent if polling is done, so let's reset it before parsing
    WOINC_PARSE_CHILD_CONTENT(node, project_config, error_num);
    if (project_config.error_num != 0)
        return;

    WOINC_PARSE_CHILD_CONTENT(node, project_config, account_creation_disabled);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, client_account_creation_disabled);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, error_msg);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, master_url);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, min_passwd_length);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, name);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, terms_of_use);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, terms_of_use_is_html);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, uses_username);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, web_rpc_url_base);

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, project_config, account_manager);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, ldap_auth);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, sched_stopped);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, web_stopped);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, local_revision);
    WOINC_PARSE_CHILD_CONTENT(node, project_config, min_client_version);
#endif // WOINC_EXPOSE_FULL_STRUCTURES

    auto platforms_node = node.find_child("platforms");
    if (node.found_child(platforms_node)) {
        project_config.platforms.reserve(platforms_node->children.size());
        for (auto &platform_node : platforms_node->children) {
            woinc::ProjectConfig::Platform platform;
            WOINC_PARSE_CHILD_CONTENT(platform_node, platform, plan_class);
            WOINC_PARSE_CHILD_CONTENT(platform_node, platform, platform_name);
            WOINC_PARSE_CHILD_CONTENT(platform_node, platform, user_friendly_name);
            project_config.platforms.push_back(std::move(platform));
        }
    }
}

void parse_(const woinc::xml::Node &node, woinc::ProjectStatistics &project_statistics) {
    WOINC_PARSE_CHILD_CONTENT(node, project_statistics, master_url);
    for (const auto &child : node.children) {
        if (child.tag == "daily_statistics") {
            woinc::DailyStatistic stats;
            parse_(child, stats);
            project_statistics.daily_statistics.push_back(std::move(stats));
        }
    }
}

void parse_(const woinc::xml::Node &node, woinc::Statistics &statistics) {
    for (const auto &child : node.children) {
        if (child.tag == "project_statistics") {
            woinc::ProjectStatistics stats;
            parse_(child, stats);
            statistics.push_back(std::move(stats));
        }
    }
}

// see RESULT::write_gui() in BOINC/client/result.cpp
void parse_(const wxml::Node &node, woinc::Task &task) {
    WOINC_PARSE_CHILD_CONTENT(node, task, state);
    WOINC_PARSE_CHILD_CONTENT(node, task, coproc_missing);
    WOINC_PARSE_CHILD_CONTENT(node, task, got_server_ack);
    WOINC_PARSE_CHILD_CONTENT(node, task, network_wait);
    WOINC_PARSE_CHILD_CONTENT(node, task, project_suspended_via_gui);
    WOINC_PARSE_CHILD_CONTENT(node, task, ready_to_report);
    WOINC_PARSE_CHILD_CONTENT(node, task, scheduler_wait);
    WOINC_PARSE_CHILD_CONTENT(node, task, suspended_via_gui);
    WOINC_PARSE_CHILD_CONTENT(node, task, estimated_cpu_time_remaining);
    WOINC_PARSE_CHILD_CONTENT(node, task, final_cpu_time);
    WOINC_PARSE_CHILD_CONTENT(node, task, final_elapsed_time);
    WOINC_PARSE_CHILD_CONTENT(node, task, exit_status);
    WOINC_PARSE_CHILD_CONTENT(node, task, signal);
    WOINC_PARSE_CHILD_CONTENT(node, task, version_num);
    WOINC_PARSE_CHILD_CONTENT(node, task, name);
    WOINC_PARSE_CHILD_CONTENT(node, task, project_url);
    WOINC_PARSE_CHILD_CONTENT(node, task, resources);
    WOINC_PARSE_CHILD_CONTENT(node, task, scheduler_wait_reason);
    WOINC_PARSE_CHILD_CONTENT(node, task, wu_name);
    WOINC_PARSE_CHILD_CONTENT(node, task, received_time);
    WOINC_PARSE_CHILD_CONTENT(node, task, report_deadline);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, task, edf_scheduled);
    WOINC_PARSE_CHILD_CONTENT(node, task, report_immediately);
    WOINC_PARSE_CHILD_CONTENT(node, task, completed_time);
    WOINC_PARSE_CHILD_CONTENT(node, task, plan_class);
    WOINC_PARSE_CHILD_CONTENT(node, task, platform);
#endif // WOINC_EXPOSE_FULL_STRUCTURES

    auto active_task_node = node.find_child("active_task");
    if (node.found_child(active_task_node)) {
        task.active_task = std::make_unique<woinc::ActiveTask>();
        parse_(*active_task_node, *task.active_task);
    }
}

void parse_(const wxml::Node &node, woinc::TimeStats &time_stats) {
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, active_frac);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, connected_frac);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, cpu_and_network_available_frac);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, gpu_active_frac);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, now);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, on_frac);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, previous_uptime);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, session_active_duration);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, session_gpu_active_duration);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, total_active_duration);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, total_duration);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, total_gpu_active_duration);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, client_start_time);
    WOINC_PARSE_CHILD_CONTENT(node, time_stats, total_start_time);
}

// see handle_exchange_versions() in BOINC/client/gui_rpc_server_ops.cpp
void parse_(const wxml::Node &node, woinc::Version &version) {
    WOINC_PARSE_CHILD_CONTENT(node, version, major);
    WOINC_PARSE_CHILD_CONTENT(node, version, minor);
    WOINC_PARSE_CHILD_CONTENT(node, version, release);
}

void parse_(const wxml::Node &node, woinc::Workunit &workunit) {
    WOINC_PARSE_CHILD_CONTENT(node, workunit, rsc_disk_bound);
    WOINC_PARSE_CHILD_CONTENT(node, workunit, rsc_fpops_bound);
    WOINC_PARSE_CHILD_CONTENT(node, workunit, rsc_fpops_est);
    WOINC_PARSE_CHILD_CONTENT(node, workunit, rsc_memory_bound);
    WOINC_PARSE_CHILD_CONTENT(node, workunit, version_num);
    WOINC_PARSE_CHILD_CONTENT(node, workunit, app_name);
    WOINC_PARSE_CHILD_CONTENT(node, workunit, name);

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    WOINC_PARSE_CHILD_CONTENT(node, workunit, command_line);

    for (auto &n : node.children) {
        if (n.tag == "file_ref") {
            woinc::FileRef f;
            parse_(n, f);
            workunit.input_files.push_back(std::move(f));
        }
    }
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

} // unnamed namespace

namespace woinc { namespace rpc {

#define WRAPPED_PARSE(TYPE) bool parse(const woinc::xml::Node &node, TYPE &t) { \
    try { \
        parse_(node, t); \
    } catch (...) { \
        return false; \
    } \
    return true; \
}

WRAPPED_PARSE(woinc::AccountOut)
WRAPPED_PARSE(woinc::AllProjectsList)
WRAPPED_PARSE(woinc::CCStatus)
WRAPPED_PARSE(woinc::ClientState)
WRAPPED_PARSE(woinc::DiskUsage)
WRAPPED_PARSE(woinc::FileTransfer)
WRAPPED_PARSE(woinc::GlobalPreferences)
WRAPPED_PARSE(woinc::HostInfo)
WRAPPED_PARSE(woinc::Message)
WRAPPED_PARSE(woinc::Notice)
WRAPPED_PARSE(woinc::Project)
WRAPPED_PARSE(woinc::ProjectConfig)
WRAPPED_PARSE(woinc::Statistics)
WRAPPED_PARSE(woinc::Task)
WRAPPED_PARSE(woinc::Version)
WRAPPED_PARSE(woinc::Workunit)

}}
