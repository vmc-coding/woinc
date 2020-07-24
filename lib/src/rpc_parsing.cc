/* lib/rpc_parsing.cc --
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

template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
void convert_to_enum__(std::underlying_type_t<T> value, T &dest) {
    if (value < 0 || value >= static_cast<std::underlying_type_t<T>>(T::UNKNOWN_TO_WOINC))
        dest = T::UNKNOWN_TO_WOINC;
    else
        dest = static_cast<T>(value);
}

void parse__(std::underlying_type_t<woinc::NETWORK_STATUS> value, woinc::NETWORK_STATUS &dest) {
    convert_to_enum__(value, dest);
}

void parse__(std::underlying_type_t<woinc::RUN_MODE> value, woinc::RUN_MODE &dest) {
    convert_to_enum__(value - 1, dest);
}

void parse__(std::underlying_type_t<woinc::SUSPEND_REASON> value, woinc::SUSPEND_REASON &dest) {
    switch (value) {
        case    0: dest = woinc::SUSPEND_REASON::NOT_SUSPENDED; break;
        case    1: dest = woinc::SUSPEND_REASON::BATTERIES; break;
        case    2: dest = woinc::SUSPEND_REASON::USER_ACTIVE; break;
        case    4: dest = woinc::SUSPEND_REASON::USER_REQ; break;
        case    8: dest = woinc::SUSPEND_REASON::TIME_OF_DAY; break;
        case   16: dest = woinc::SUSPEND_REASON::BENCHMARKS; break;
        case   32: dest = woinc::SUSPEND_REASON::DISK_SIZE; break;
        case   64: dest = woinc::SUSPEND_REASON::CPU_THROTTLE; break;
        case  128: dest = woinc::SUSPEND_REASON::NO_RECENT_INPUT; break;
        case  256: dest = woinc::SUSPEND_REASON::INITIAL_DELAY; break;
        case  512: dest = woinc::SUSPEND_REASON::EXCLUSIVE_APP_RUNNING; break;
        case 1024: dest = woinc::SUSPEND_REASON::CPU_USAGE; break;
        case 2048: dest = woinc::SUSPEND_REASON::NETWORK_QUOTA_EXCEEDED; break;
        case 4096: dest = woinc::SUSPEND_REASON::OS; break;
        case 4097: dest = woinc::SUSPEND_REASON::WIFI_STATE; break;
        case 4098: dest = woinc::SUSPEND_REASON::BATTERY_CHARGING; break;
        case 4099: dest = woinc::SUSPEND_REASON::BATTERY_OVERHEATED; break;
        case 4100: dest = woinc::SUSPEND_REASON::NO_GUI_KEEPALIVE; break;
        default: dest = woinc::SUSPEND_REASON::UNKNOWN_TO_WOINC;
    }
}

void parse__(std::underlying_type_t<woinc::SCHEDULER_STATE> value, woinc::SCHEDULER_STATE &dest) {
    convert_to_enum__(value, dest);
}

void parse__(std::underlying_type_t<woinc::RESULT_CLIENT_STATE> value, woinc::RESULT_CLIENT_STATE &dest) {
    convert_to_enum__(value, dest);
}

void parse__(std::underlying_type_t<woinc::ACTIVE_TASK_STATE> value, woinc::ACTIVE_TASK_STATE &dest) {
    convert_to_enum__(value, dest);
}

void parse__(std::underlying_type_t<woinc::MSG_INFO> value, woinc::MSG_INFO &dest) {
    convert_to_enum__(value - 1, dest);
}

void parse__(std::underlying_type_t<woinc::RPC_REASON> value, woinc::RPC_REASON &dest) {
    convert_to_enum__(value, dest);
}

void parse__(std::underlying_type_t<woinc::DAY_OF_WEEK> value, woinc::DAY_OF_WEEK &dest) {
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
        typename std::underlying_type<T>::type value;
        parse__(child_iter->content, value);
        parse__(value, dest);
#ifndef NDEBUG
        if (dest == T::UNKNOWN_TO_WOINC)
            std::cerr << "Value of node with tag " << child_tag << " out of range\n";
        // we should adopt the unknown values, so let's fail out in dev mode
        assert(dest != T::UNKNOWN_TO_WOINC);
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

#define PARSE_CHILD_CONTENT(NODE, RESULT_STRUCT, TAG) \
    parse_child_content_(NODE, #TAG, RESULT_STRUCT . TAG)

// see ACTIVE_TASK::write_gui() in BOINC/client/app.cpp
void parse_(const wxml::Node &node, woinc::ActiveTask &active_task) {
    PARSE_CHILD_CONTENT(node, active_task, active_task_state);
    PARSE_CHILD_CONTENT(node, active_task, scheduler_state);
    PARSE_CHILD_CONTENT(node, active_task, too_large);
    PARSE_CHILD_CONTENT(node, active_task, pid);
    PARSE_CHILD_CONTENT(node, active_task, slot);
    PARSE_CHILD_CONTENT(node, active_task, needs_shmem);
    PARSE_CHILD_CONTENT(node, active_task, checkpoint_cpu_time);
    PARSE_CHILD_CONTENT(node, active_task, elapsed_time);
    PARSE_CHILD_CONTENT(node, active_task, fraction_done);
    PARSE_CHILD_CONTENT(node, active_task, current_cpu_time);
    PARSE_CHILD_CONTENT(node, active_task, progress_rate);
    PARSE_CHILD_CONTENT(node, active_task, swap_size);
    PARSE_CHILD_CONTENT(node, active_task, working_set_size_smoothed);
    PARSE_CHILD_CONTENT(node, active_task, bytes_sent);
    PARSE_CHILD_CONTENT(node, active_task, bytes_received);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, active_task, page_fault_rate);
    PARSE_CHILD_CONTENT(node, active_task, working_set_size);
    PARSE_CHILD_CONTENT(node, active_task, app_version_num);
    PARSE_CHILD_CONTENT(node, active_task, graphics_exec_path);
    PARSE_CHILD_CONTENT(node, active_task, slot_path);
    PARSE_CHILD_CONTENT(node, active_task, web_graphics_url);
    PARSE_CHILD_CONTENT(node, active_task, remote_desktop_addr);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

void parse_(const woinc::xml::Node &node, woinc::AllProjectsList &projects) {
    for (auto &project_node : node.children) {
        if (project_node.tag != "project")
            continue;
        woinc::ProjectListEntry entry;
        PARSE_CHILD_CONTENT(project_node, entry, description);
        PARSE_CHILD_CONTENT(project_node, entry, general_area);
        PARSE_CHILD_CONTENT(project_node, entry, home);
        PARSE_CHILD_CONTENT(project_node, entry, image);
        PARSE_CHILD_CONTENT(project_node, entry, name);
        PARSE_CHILD_CONTENT(project_node, entry, specific_area);
        PARSE_CHILD_CONTENT(project_node, entry, url);
        PARSE_CHILD_CONTENT(project_node, entry, web_url);
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
    PARSE_CHILD_CONTENT(node, app, non_cpu_intensive);
    PARSE_CHILD_CONTENT(node, app, name);
    PARSE_CHILD_CONTENT(node, app, user_friendly_name);
}

void parse_(const wxml::Node &node, woinc::AppVersion &app_version) {
    PARSE_CHILD_CONTENT(node, app_version, avg_ncpus);
    PARSE_CHILD_CONTENT(node, app_version, flops);
    PARSE_CHILD_CONTENT(node, app_version, version_num);
    PARSE_CHILD_CONTENT(node, app_version, app_name);
    PARSE_CHILD_CONTENT(node, app_version, plan_class);
    PARSE_CHILD_CONTENT(node, app_version, platform);

    for (auto &n : node.children) {
        if (n.tag == "file_ref") {
            woinc::FileRef f;
            parse_(n, f);
            app_version.file_refs.push_back(std::move(f));
        }
    }
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, app_version, dont_throttle);
    PARSE_CHILD_CONTENT(node, app_version, is_wrapper);
    PARSE_CHILD_CONTENT(node, app_version, needs_network);
    PARSE_CHILD_CONTENT(node, app_version, gpu_ram);
    PARSE_CHILD_CONTENT(node, app_version, api_version);
    PARSE_CHILD_CONTENT(node, app_version, cmdline);
    PARSE_CHILD_CONTENT(node, app_version, file_prefix);
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
    PARSE_CHILD_CONTENT(node, daily_statistic, host_expavg_credit);
    PARSE_CHILD_CONTENT(node, daily_statistic, host_total_credit);
    PARSE_CHILD_CONTENT(node, daily_statistic, user_expavg_credit);
    PARSE_CHILD_CONTENT(node, daily_statistic, user_total_credit);
    PARSE_CHILD_CONTENT(node, daily_statistic, day);
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
            client_state.net_stats = std::unique_ptr<woinc::NetStats>(new woinc::NetStats);
            parse_(child, *client_state.net_stats);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
        }
    }

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, client_state, executing_as_daemon);
    PARSE_CHILD_CONTENT(node, client_state, have_ati);
    PARSE_CHILD_CONTENT(node, client_state, have_cuda);
    PARSE_CHILD_CONTENT(node, client_state, platform_name);
    parse_child_content_(node, "core_client_major_version", client_state.core_client_version.major);
    parse_child_content_(node, "core_client_minor_version", client_state.core_client_version.minor);
    parse_child_content_(node, "core_client_release", client_state.core_client_version.release);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

void parse_(const wxml::Node &node, woinc::FileRef &file_ref) {
    PARSE_CHILD_CONTENT(node, file_ref, main_program);
    PARSE_CHILD_CONTENT(node, file_ref, file_name);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, file_ref, copy_file);
    PARSE_CHILD_CONTENT(node, file_ref, optional);
    PARSE_CHILD_CONTENT(node, file_ref, open_name);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

void parse_(const woinc::xml::Node &node, woinc::FileTransfer &file_transfer) {
    PARSE_CHILD_CONTENT(node, file_transfer, nbytes);
    PARSE_CHILD_CONTENT(node, file_transfer, status);
    PARSE_CHILD_CONTENT(node, file_transfer, name);
    PARSE_CHILD_CONTENT(node, file_transfer, project_name);
    PARSE_CHILD_CONTENT(node, file_transfer, project_url);
    PARSE_CHILD_CONTENT(node, file_transfer, project_backoff);

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, file_transfer, max_nbytes);
#endif

    auto persistent_file_xfer_node = node.find_child("persistent_file_xfer");
    if (node.found_child(persistent_file_xfer_node)) {
        file_transfer.persistent_file_xfer.reset(new woinc::PersistentFileXfer());
        parse_(*persistent_file_xfer_node, *file_transfer.persistent_file_xfer);
    }

    auto file_xfer_node = node.find_child("file_xfer");
    if (node.found_child(file_xfer_node)) {
        file_transfer.file_xfer.reset(new woinc::FileXfer());
        parse_(*file_xfer_node, *file_transfer.file_xfer);
    }
}

void parse_(const woinc::xml::Node &node, woinc::FileXfer &file_xfer) {
    PARSE_CHILD_CONTENT(node, file_xfer, bytes_xferred);
    PARSE_CHILD_CONTENT(node, file_xfer, xfer_speed);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, file_xfer, file_offset);
    PARSE_CHILD_CONTENT(node, file_xfer, url);
#endif
}

void parse_(const wxml::Node &node, woinc::GlobalPreferences &global_prefs) {
    PARSE_CHILD_CONTENT(node, global_prefs, confirm_before_connecting);
    PARSE_CHILD_CONTENT(node, global_prefs, dont_verify_images);
    PARSE_CHILD_CONTENT(node, global_prefs, hangup_if_dialed);
    PARSE_CHILD_CONTENT(node, global_prefs, leave_apps_in_memory);
    PARSE_CHILD_CONTENT(node, global_prefs, run_gpu_if_user_active);
    PARSE_CHILD_CONTENT(node, global_prefs, run_if_user_active);
    PARSE_CHILD_CONTENT(node, global_prefs, run_on_batteries);
    PARSE_CHILD_CONTENT(node, global_prefs, cpu_scheduling_period_minutes);
    PARSE_CHILD_CONTENT(node, global_prefs, cpu_usage_limit);
    PARSE_CHILD_CONTENT(node, global_prefs, daily_xfer_limit_mb);
    PARSE_CHILD_CONTENT(node, global_prefs, disk_interval);
    PARSE_CHILD_CONTENT(node, global_prefs, disk_max_used_gb);
    PARSE_CHILD_CONTENT(node, global_prefs, disk_max_used_pct);
    PARSE_CHILD_CONTENT(node, global_prefs, disk_min_free_gb);
    PARSE_CHILD_CONTENT(node, global_prefs, end_hour);
    PARSE_CHILD_CONTENT(node, global_prefs, idle_time_to_run);
    PARSE_CHILD_CONTENT(node, global_prefs, max_bytes_sec_down);
    PARSE_CHILD_CONTENT(node, global_prefs, max_bytes_sec_up);
    PARSE_CHILD_CONTENT(node, global_prefs, max_ncpus_pct);
    PARSE_CHILD_CONTENT(node, global_prefs, net_end_hour);
    PARSE_CHILD_CONTENT(node, global_prefs, net_start_hour);
    PARSE_CHILD_CONTENT(node, global_prefs, ram_max_used_busy_pct);
    PARSE_CHILD_CONTENT(node, global_prefs, ram_max_used_idle_pct);
    PARSE_CHILD_CONTENT(node, global_prefs, start_hour);
    PARSE_CHILD_CONTENT(node, global_prefs, suspend_cpu_usage);
    PARSE_CHILD_CONTENT(node, global_prefs, work_buf_additional_days);
    PARSE_CHILD_CONTENT(node, global_prefs, work_buf_min_days);
    PARSE_CHILD_CONTENT(node, global_prefs, vm_max_used_pct);
    PARSE_CHILD_CONTENT(node, global_prefs, daily_xfer_period_days);

    auto prefs_node = node.find_child("day_prefs");
    while (node.found_child(prefs_node)) {
        woinc::DAY_OF_WEEK day;
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
    PARSE_CHILD_CONTENT(node, global_prefs, network_wifi_only);
    PARSE_CHILD_CONTENT(node, global_prefs, override_file_present);
    PARSE_CHILD_CONTENT(node, global_prefs, battery_charge_min_pct);
    PARSE_CHILD_CONTENT(node, global_prefs, battery_max_temperature);
    PARSE_CHILD_CONTENT(node, global_prefs, mod_time);
    PARSE_CHILD_CONTENT(node, global_prefs, suspend_if_no_recent_input);
    PARSE_CHILD_CONTENT(node, global_prefs, max_cpus);
    PARSE_CHILD_CONTENT(node, global_prefs, source_project);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

void parse_(const wxml::Node &node, woinc::GuiUrl &gui_url) {
    PARSE_CHILD_CONTENT(node, gui_url, name);
    PARSE_CHILD_CONTENT(node, gui_url, description);
    PARSE_CHILD_CONTENT(node, gui_url, url);
}

void parse_(const woinc::xml::Node &node, woinc::HostInfo &info) {
    PARSE_CHILD_CONTENT(node, info, d_free);
    PARSE_CHILD_CONTENT(node, info, d_total);
    PARSE_CHILD_CONTENT(node, info, m_cache);
    PARSE_CHILD_CONTENT(node, info, m_nbytes);
    PARSE_CHILD_CONTENT(node, info, m_swap);
    PARSE_CHILD_CONTENT(node, info, p_fpops);
    PARSE_CHILD_CONTENT(node, info, p_iops);
    PARSE_CHILD_CONTENT(node, info, p_membw);
    PARSE_CHILD_CONTENT(node, info, p_ncpus);
    PARSE_CHILD_CONTENT(node, info, timezone);
    PARSE_CHILD_CONTENT(node, info, domain_name);
    PARSE_CHILD_CONTENT(node, info, ip_addr);
    PARSE_CHILD_CONTENT(node, info, os_name);
    PARSE_CHILD_CONTENT(node, info, os_version);
    PARSE_CHILD_CONTENT(node, info, p_model);
    PARSE_CHILD_CONTENT(node, info, p_vendor);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, info, p_vm_extensions_disabled);
    PARSE_CHILD_CONTENT(node, info, p_calculated);
    PARSE_CHILD_CONTENT(node, info, n_usable_coprocs);
    PARSE_CHILD_CONTENT(node, info, host_cpid);
    PARSE_CHILD_CONTENT(node, info, mac_address);
    PARSE_CHILD_CONTENT(node, info, p_features);
    PARSE_CHILD_CONTENT(node, info, product_name);
    PARSE_CHILD_CONTENT(node, info, virtualbox_version);
#endif // WOINC_EXPOSE_FULL_STRUCTURES
}

// ses MESSAGE_DESCS::write in BOINC/client/client_msgs.cpp
void parse_(const wxml::Node &node, woinc::Message &msg) {
    PARSE_CHILD_CONTENT(node, msg, body);
    PARSE_CHILD_CONTENT(node, msg, project);
    PARSE_CHILD_CONTENT(node, msg, seqno);
    parse_child_content_(node, "pri", msg.priority);
    parse_child_content_(node, "time", msg.timestamp);
}

// see NET_STATS::write() in BOINC/client/net_stats.cc
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
void parse_(const wxml::Node &node, woinc::NetStats &net_stats) {
    PARSE_CHILD_CONTENT(node, net_stats, bwup);
    PARSE_CHILD_CONTENT(node, net_stats, avg_up);
    PARSE_CHILD_CONTENT(node, net_stats, avg_time_up);
    PARSE_CHILD_CONTENT(node, net_stats, bwdown);
    PARSE_CHILD_CONTENT(node, net_stats, avg_down);
    PARSE_CHILD_CONTENT(node, net_stats, avg_time_down);
}
#endif // WOINC_EXPOSE_FULL_STRUCTURES

// see NOTICE::write in BOINC/lib/notice.cpp
void parse_(const wxml::Node &node, woinc::Notice &notice) {
    PARSE_CHILD_CONTENT(node, notice, seqno);
    PARSE_CHILD_CONTENT(node, notice, category);
    PARSE_CHILD_CONTENT(node, notice, description);
    PARSE_CHILD_CONTENT(node, notice, link);
    PARSE_CHILD_CONTENT(node, notice, project_name);
    PARSE_CHILD_CONTENT(node, notice, title);
    PARSE_CHILD_CONTENT(node, notice, create_time);

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, notice, is_private);
    PARSE_CHILD_CONTENT(node, notice, is_youtube_video);
    PARSE_CHILD_CONTENT(node, notice, arrival_time);
#endif
}

void parse_(const woinc::xml::Node &node, woinc::PersistentFileXfer &persistent_file_xfer) {
    PARSE_CHILD_CONTENT(node, persistent_file_xfer, is_upload);
    PARSE_CHILD_CONTENT(node, persistent_file_xfer, time_so_far);
    PARSE_CHILD_CONTENT(node, persistent_file_xfer, next_request_time);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, persistent_file_xfer, last_bytes_xferred);
    PARSE_CHILD_CONTENT(node, persistent_file_xfer, num_retries);
    PARSE_CHILD_CONTENT(node, persistent_file_xfer, first_request_time);
#endif
}

void parse_(const wxml::Node &node, woinc::Platform &platform) {
    platform = node.content;
}

// see PROJECT::write_state in BOINC/client/project.cpp
void parse_(const wxml::Node &node, woinc::Project &project) {
    PARSE_CHILD_CONTENT(node, project, anonymous_platform);
    PARSE_CHILD_CONTENT(node, project, attached_via_acct_mgr);
    PARSE_CHILD_CONTENT(node, project, detach_when_done);
    PARSE_CHILD_CONTENT(node, project, dont_request_more_work);
    PARSE_CHILD_CONTENT(node, project, ended);
    PARSE_CHILD_CONTENT(node, project, master_url_fetch_pending);
    PARSE_CHILD_CONTENT(node, project, non_cpu_intensive);
    PARSE_CHILD_CONTENT(node, project, scheduler_rpc_in_progress);
    PARSE_CHILD_CONTENT(node, project, suspended_via_gui);
    PARSE_CHILD_CONTENT(node, project, trickle_up_pending);

    PARSE_CHILD_CONTENT(node, project, desired_disk_usage);
    PARSE_CHILD_CONTENT(node, project, elapsed_time);
    PARSE_CHILD_CONTENT(node, project, host_expavg_credit);
    PARSE_CHILD_CONTENT(node, project, host_total_credit);
    PARSE_CHILD_CONTENT(node, project, project_files_downloaded_time);
    PARSE_CHILD_CONTENT(node, project, resource_share);
    PARSE_CHILD_CONTENT(node, project, sched_priority);
    PARSE_CHILD_CONTENT(node, project, user_expavg_credit);
    PARSE_CHILD_CONTENT(node, project, user_total_credit);

    PARSE_CHILD_CONTENT(node, project, hostid);
    PARSE_CHILD_CONTENT(node, project, master_fetch_failures);
    PARSE_CHILD_CONTENT(node, project, njobs_error);
    PARSE_CHILD_CONTENT(node, project, njobs_success);
    PARSE_CHILD_CONTENT(node, project, nrpc_failures);

    PARSE_CHILD_CONTENT(node, project, sched_rpc_pending);

    PARSE_CHILD_CONTENT(node, project, external_cpid);
    PARSE_CHILD_CONTENT(node, project, master_url);
    PARSE_CHILD_CONTENT(node, project, project_name);
    PARSE_CHILD_CONTENT(node, project, team_name);
    PARSE_CHILD_CONTENT(node, project, user_name);
    PARSE_CHILD_CONTENT(node, project, venue);

    PARSE_CHILD_CONTENT(node, project, download_backoff);
    PARSE_CHILD_CONTENT(node, project, last_rpc_time);
    PARSE_CHILD_CONTENT(node, project, min_rpc_time);
    PARSE_CHILD_CONTENT(node, project, upload_backoff);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, project, dont_use_dcf);
    PARSE_CHILD_CONTENT(node, project, send_full_workload);
    PARSE_CHILD_CONTENT(node, project, use_symlinks);
    PARSE_CHILD_CONTENT(node, project, verify_files_on_app_start);

    PARSE_CHILD_CONTENT(node, project, ams_resource_share_new);
    PARSE_CHILD_CONTENT(node, project, cpid_time);
    PARSE_CHILD_CONTENT(node, project, duration_correction_factor);
    PARSE_CHILD_CONTENT(node, project, host_create_time);
    PARSE_CHILD_CONTENT(node, project, next_rpc_time);
    PARSE_CHILD_CONTENT(node, project, rec);
    PARSE_CHILD_CONTENT(node, project, rec_time);
    PARSE_CHILD_CONTENT(node, project, user_create_time);

    PARSE_CHILD_CONTENT(node, project, rpc_seqno);
    PARSE_CHILD_CONTENT(node, project, send_job_log);
    PARSE_CHILD_CONTENT(node, project, send_time_stats_log);
    PARSE_CHILD_CONTENT(node, project, teamid);
    PARSE_CHILD_CONTENT(node, project, userid);

    PARSE_CHILD_CONTENT(node, project, cross_project_id);
    PARSE_CHILD_CONTENT(node, project, email_hash);
    PARSE_CHILD_CONTENT(node, project, host_venue);
    PARSE_CHILD_CONTENT(node, project, project_dir);
    PARSE_CHILD_CONTENT(node, project, symstore);
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
    PARSE_CHILD_CONTENT(node, project_config, error_num);
    if (project_config.error_num != 0)
        return;

    PARSE_CHILD_CONTENT(node, project_config, account_creation_disabled);
    PARSE_CHILD_CONTENT(node, project_config, client_account_creation_disabled);
    PARSE_CHILD_CONTENT(node, project_config, error_msg);
    PARSE_CHILD_CONTENT(node, project_config, master_url);
    PARSE_CHILD_CONTENT(node, project_config, min_passwd_length);
    PARSE_CHILD_CONTENT(node, project_config, name);
    PARSE_CHILD_CONTENT(node, project_config, terms_of_use);
    PARSE_CHILD_CONTENT(node, project_config, terms_of_use_is_html);
    PARSE_CHILD_CONTENT(node, project_config, uses_username);
    PARSE_CHILD_CONTENT(node, project_config, web_rpc_url_base);

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, project_config, account_manager);
    PARSE_CHILD_CONTENT(node, project_config, ldap_auth);
    PARSE_CHILD_CONTENT(node, project_config, sched_stopped);
    PARSE_CHILD_CONTENT(node, project_config, web_stopped);
    PARSE_CHILD_CONTENT(node, project_config, local_revision);
    PARSE_CHILD_CONTENT(node, project_config, min_client_version);
#endif // WOINC_EXPOSE_FULL_STRUCTURES

    auto platforms_node = node.find_child("platforms");
    if (node.found_child(platforms_node)) {
        project_config.platforms.reserve(platforms_node->children.size());
        for (auto &platform_node : platforms_node->children) {
            woinc::ProjectConfig::Platform platform;
            PARSE_CHILD_CONTENT(platform_node, platform, plan_class);
            PARSE_CHILD_CONTENT(platform_node, platform, platform_name);
            PARSE_CHILD_CONTENT(platform_node, platform, user_friendly_name);
            project_config.platforms.push_back(std::move(platform));
        }
    }
}

void parse_(const woinc::xml::Node &node, woinc::ProjectStatistics &project_statistics) {
    PARSE_CHILD_CONTENT(node, project_statistics, master_url);
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
    PARSE_CHILD_CONTENT(node, task, state);
    PARSE_CHILD_CONTENT(node, task, coproc_missing);
    PARSE_CHILD_CONTENT(node, task, got_server_ack);
    PARSE_CHILD_CONTENT(node, task, network_wait);
    PARSE_CHILD_CONTENT(node, task, project_suspended_via_gui);
    PARSE_CHILD_CONTENT(node, task, ready_to_report);
    PARSE_CHILD_CONTENT(node, task, scheduler_wait);
    PARSE_CHILD_CONTENT(node, task, suspended_via_gui);
    PARSE_CHILD_CONTENT(node, task, estimated_cpu_time_remaining);
    PARSE_CHILD_CONTENT(node, task, final_cpu_time);
    PARSE_CHILD_CONTENT(node, task, final_elapsed_time);
    PARSE_CHILD_CONTENT(node, task, exit_status);
    PARSE_CHILD_CONTENT(node, task, signal);
    PARSE_CHILD_CONTENT(node, task, version_num);
    PARSE_CHILD_CONTENT(node, task, name);
    PARSE_CHILD_CONTENT(node, task, project_url);
    PARSE_CHILD_CONTENT(node, task, resources);
    PARSE_CHILD_CONTENT(node, task, scheduler_wait_reason);
    PARSE_CHILD_CONTENT(node, task, wu_name);
    PARSE_CHILD_CONTENT(node, task, received_time);
    PARSE_CHILD_CONTENT(node, task, report_deadline);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, task, edf_scheduled);
    PARSE_CHILD_CONTENT(node, task, report_immediately);
    PARSE_CHILD_CONTENT(node, task, completed_time);
    PARSE_CHILD_CONTENT(node, task, plan_class);
    PARSE_CHILD_CONTENT(node, task, platform);
#endif // WOINC_EXPOSE_FULL_STRUCTURES

    auto active_task_node = node.find_child("active_task");
    if (node.found_child(active_task_node)) {
        task.active_task.reset(new woinc::ActiveTask());
        parse_(*active_task_node, *task.active_task);
    }
}

void parse_(const wxml::Node &node, woinc::TimeStats &time_stats) {
    PARSE_CHILD_CONTENT(node, time_stats, active_frac);
    PARSE_CHILD_CONTENT(node, time_stats, connected_frac);
    PARSE_CHILD_CONTENT(node, time_stats, cpu_and_network_available_frac);
    PARSE_CHILD_CONTENT(node, time_stats, gpu_active_frac);
    PARSE_CHILD_CONTENT(node, time_stats, now);
    PARSE_CHILD_CONTENT(node, time_stats, on_frac);
    PARSE_CHILD_CONTENT(node, time_stats, previous_uptime);
    PARSE_CHILD_CONTENT(node, time_stats, session_active_duration);
    PARSE_CHILD_CONTENT(node, time_stats, session_gpu_active_duration);
    PARSE_CHILD_CONTENT(node, time_stats, total_active_duration);
    PARSE_CHILD_CONTENT(node, time_stats, total_duration);
    PARSE_CHILD_CONTENT(node, time_stats, total_gpu_active_duration);
    PARSE_CHILD_CONTENT(node, time_stats, client_start_time);
    PARSE_CHILD_CONTENT(node, time_stats, total_start_time);
}

// see handle_exchange_versions() in BOINC/client/gui_rpc_server_ops.cpp
void parse_(const wxml::Node &node, woinc::Version &version) {
    PARSE_CHILD_CONTENT(node, version, major);
    PARSE_CHILD_CONTENT(node, version, minor);
    PARSE_CHILD_CONTENT(node, version, release);
}

void parse_(const wxml::Node &node, woinc::Workunit &workunit) {
    PARSE_CHILD_CONTENT(node, workunit, rsc_disk_bound);
    PARSE_CHILD_CONTENT(node, workunit, rsc_fpops_bound);
    PARSE_CHILD_CONTENT(node, workunit, rsc_fpops_est);
    PARSE_CHILD_CONTENT(node, workunit, rsc_memory_bound);
    PARSE_CHILD_CONTENT(node, workunit, version_num);
    PARSE_CHILD_CONTENT(node, workunit, app_name);
    PARSE_CHILD_CONTENT(node, workunit, name);

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
    PARSE_CHILD_CONTENT(node, workunit, command_line);

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
