/* ui/cli/main.cc --
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

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <queue>
#include <sstream>
#include <thread>
#include <type_traits>

#include <woinc/rpc_command.h>
#include <woinc/rpc_connection.h>

#include "common/types_to_string.h"

namespace wrpc = woinc::rpc;

namespace {

#if defined(WIN32) || defined(_WIN32)
static const char *EXEC_NAME = "woinccmd.exe";
#else
static const char *EXEC_NAME = "woinccmd";
#endif

typedef std::queue<std::string> Arguments;
bool matches(Arguments &args, const std::string &what);

void parse_host(std::string &hostname, std::uint16_t &port);

[[ noreturn ]] void usage(std::ostream &out, int exit_code);
[[ noreturn ]] void usage_die();
[[ noreturn ]] void die_unknown_command(const std::string &cmd);
[[ noreturn ]] void error_die(const std::string &msg);
void empty_or_die(const Arguments &args);

class Client {
    public:
        Client(std::string hostname, std::uint16_t port, std::string password);
        void do_cmd(wrpc::Command &cmd);

    private:
        void connect_();
        void authorize_();
        void execute_cmd_or_die_(wrpc::Command &cmd);

        const std::string hostname_;
        const std::uint16_t port_;
        const std::string password_;
        wrpc::Connection connection_;
        bool connected_ = false;
        bool authed_ = false;
};

typedef void *CommandContext;

struct Command {
    CommandContext (*parse)(Arguments &);
    void (*execute)(Client &, CommandContext);
};

typedef std::map<std::string, Command> CommandMap;
CommandMap command_map();

}

int main(int argc, char **argv) {
    // as we don't use the printf-functions, we don't need the streams to be synced;
    // deactivating the sync reduces the number of write-calls significantly
    std::ios_base::sync_with_stdio(false);

    Arguments args;

    for (int i = 1 /* skip name */; i < argc; ++i)
        args.push(std::string(argv[i]));

    if (args.empty())
        usage_die();

    // parse hostname

    std::string hostname("localhost");
    std::uint16_t port = wrpc::Connection::DEFAULT_PORT;

    if (matches(args, "--host")) {
        if (args.empty()) {
            std::cerr << "Missing hostname after parameter --host" << std::endl;
            exit(EXIT_FAILURE);
        }
        hostname = args.front();
        args.pop();
        parse_host(hostname, port);
    }

    // parse password

    std::string password;

    if (matches(args, "--passwd")) {
        if (args.empty()) {
            std::cerr << "Missing password after parameter --passwd" << std::endl;
            exit(EXIT_FAILURE);
        }
        password = args.front();
        args.pop();
    }

#ifndef NDEBUG
    std::cout << "Connect to host " << hostname << " on port " << port << "\n";
#endif

    // if requested show version and quit

    if (matches(args, "-v") || matches(args, "--version")) {
        std::cout << "Version: " << woinc::major_version() << "." << woinc::minor_version() << std::endl;
        return args.empty() ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    // if requested show help and quit

    if (matches(args, "-?") || matches(args, "-h") || matches(args, "--help"))
        usage(std::cout, args.empty() ? EXIT_SUCCESS : EXIT_FAILURE);

    // find command to execute

    if (args.empty())
        error_die("Nothing to do, no command given.");

    auto cmds{std::move(command_map())};
    auto cmd_iter = cmds.find(args.front());

    if (cmd_iter == cmds.end())
        die_unknown_command(args.front());

    args.pop();

    // parse the command

    CommandContext ctx = cmd_iter->second.parse(args);
    empty_or_die(args);

    // execute the command

    Client client{hostname, port, password};
    cmd_iter->second.execute(client, ctx);

    return EXIT_SUCCESS;
}


// ----------------------
// --- error handling ---
// ----------------------

namespace {

void usage(std::ostream &out, int exit_code) {
    out << "\n"
        << "Usage: " << EXEC_NAME << " [ --host <host[:port]> ] [ --passwd <password> ] <command>\n"
        << "       " << EXEC_NAME << " -v|--version -- Show the version of woinccmd\n"
        << "       " << EXEC_NAME << " -?|-h|--help -- Show this help\n"
        << R"(
  host:     The host to connect to, defaults to localhost
  password: The password to be used to connect to the host
            if the requested command needs authorization
  command:  The command to exectue, see COMMANDS for a list of available commands

COMMANDS:

  ### boinccmd compatible commands ###

  --client_version                  show client version
  --file_transfer URL filename op   file transfer operation
    op = retry | abort
  --get_cc_status                   show cc status
  --get_disk_usage                  show disk usage
  --get_file_transfers              show file transfers
  --get_host_info                   show host info
  --get_messages [ seqno ]          show messages with sequence number > seqno
  --get_notices [ seqno ]           show notices with sequence number > seqno
  --get_project_status              show status of all attached projects
  --get_state                       show entire state
  --get_tasks                       show tasks
  --network_available               retry deferred network communication
  --project URL op                  project operation
    op = reset | detach | update | suspend | resume | nomorework | allowmorework | detach_when_done | dont_detach_when_done
  --project_attach URL auth         attach to project
  --task url task_name op           task operation
    op = suspend | resume | abort
  --quit                            tell client to exit
  --read_cc_config
  --read_global_prefs_override
  --run_benchmarks
  --set_gpu_mode mode duration      set GPU run mode for given duration
    mode = always | auto | never
  --set_network_mode mode duration  set network mode for given duration
    mode = always | auto | never
  --set_run_mode mode [ duration ]  set run mode for given duration
    mode = always | auto | never
)";
#ifdef WOINC_CLI_COMMANDS
    out << R"(
  ### further woinccmd commands ###

  --sum_remaining_cpu_time          compute the sum of the remaining cpu
                                    time of all non finished WUs
  --estimate_times                  estimate the compuation time of running WUs
                                    based on the elapsed time
  --get_statistics [ "user" | "host" ]
                                    show statistics of all attached projects
)";
#endif
    exit(exit_code);
}

void usage_die() {
    usage(std::cerr, EXIT_FAILURE);
}

void die_unknown_command(const std::string &cmd) {
    std::cerr << "Unknown command: " << cmd << "\n"
        << "See '" << EXEC_NAME << " --help' for a list of available commands.\n";
    exit(EXIT_FAILURE);
}

void error_die(const std::string &msg) {
    std::cerr << "Error: " << msg << "\n"
        << "See '" << EXEC_NAME << " --help' for more information.\n";
    exit(EXIT_FAILURE);
}

} // error handling


// --------------------
// --- client impl ---
// --------------------

namespace {

Client::Client(std::string hostname, std::uint16_t port, std::string password)
    : hostname_(std::move(hostname)), port_(port), password_(std::move(password)) {}

void Client::do_cmd(wrpc::Command &cmd) {
    if (!connected_)
        connect_();

    if (!authed_ &&
        (!connection_.is_localhost() || cmd.requires_local_authorization())) {
        authorize_();
    }

    execute_cmd_or_die_(cmd);
}

void Client::connect_() {
    assert(!hostname_.empty());

    auto result = connection_.open(hostname_, port_);
    if (!result)
        error_die("Could not connect to client: " + result.error);

    connected_ = true;
}

void Client::authorize_() {
    if (password_.empty())
        error_die("Authorization needed, please set password with --passwd");

    wrpc::AuthorizeCommand cmd;
    cmd.request().password = password_;

    execute_cmd_or_die_(cmd);

    if (!cmd.response().authorized) {
        std::cerr << "Authorization failure";
        if (!cmd.error().empty())
            std::cerr << ": " << cmd.error();
        std::cerr << std::endl;
        connection_.close();
        exit(EXIT_FAILURE);
    }

    authed_ = true;
}

void Client::execute_cmd_or_die_(wrpc::Command &cmd) {
    switch (cmd.execute(connection_)) {
        case wrpc::COMMAND_STATUS::OK:
            return;
        case wrpc::COMMAND_STATUS::DISCONNECTED:
            std::cerr << "Error: not connected to BOINC-client\n";
            break;
        case wrpc::COMMAND_STATUS::UNAUTHORIZED:
            std::cerr << "Operation failed: authentication error\n";
            break;
        case wrpc::COMMAND_STATUS::CONNECTION_ERROR:
            std::cerr << "Error: could not communicate with BOINC-client: " << cmd.error() << "\n";
            break;
        case wrpc::COMMAND_STATUS::CLIENT_ERROR:
            std::cerr << "Error: " << cmd.error() << "\n";
            break;
        case wrpc::COMMAND_STATUS::PARSING_ERROR:
            if (cmd.error().empty())
                std::cerr << "Error: could not interpret the response from the BOINC-client: " << cmd.error() << "\n";
            else
                std::cerr << "Error: " << cmd.error() << "\n";
            break;
        case wrpc::COMMAND_STATUS::LOGIC_ERROR:
            std::cerr << "Logical error: " << cmd.error() << "\n";
            break;
    }

    connection_.close();
    exit(EXIT_FAILURE);
}

} // client impl


// ----------------
// --- printing ---
// ----------------

namespace {

const char NL = '\n';

const char *INDENT2 = "  ";
const char *INDENT3 = "   ";
const char *INDENT4 = "    ";

template<typename T>
constexpr T mibi(const T &t) {
    static_assert(std::is_arithmetic<T>::value, "Number needed.");
    return t / (1024 * 1024);
}

constexpr long long to_rounded_mibi(double d) {
    return static_cast<long long>(std::round(mibi(d)));
}

template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
std::ostream &operator<<(std::ostream &out, T &t) {
    return out << woinc::ui::common::to_string(t);
}

constexpr const char *bool_to_string(bool b) {
    return b ? "yes" : "no";
}

std::string time_to_string(time_t t, const char *format = "%c") {
    char buf[128];
    if (!t)
        return "---";
    auto ret = std::strftime(buf, sizeof(buf), format, std::localtime(&t));
    return ret > 0 ? buf : "";
}

std::string trimed(const std::string &str) {
    auto front = std::find_if_not(str.cbegin(), str.cend(),
                                  [](int c) { return std::isspace(c); });
    auto end = std::find_if_not(str.crbegin(), std::make_reverse_iterator(front),
                                [](int c) { return std::isspace(c); });
    return std::string(front, end.base());
}

std::string resolve_project_name(const woinc::Projects &projects, const std::string &url) {
    auto project = std::find_if(projects.cbegin(), projects.cend(),
                                [&](const woinc::Project &p) { return p.master_url == url; });
    return project == projects.cend() ? "" : project->project_name;
}

void print(std::ostream &out, const wrpc::ExchangeVersionsResponse &response) {
    out << "Client version: " << response.version.major << "."
        << response.version.minor << "." << response.version.release << NL;
}

void print(std::ostream &out, const std::string &which, const woinc::CCStatus::State &state) {
    auto indent = INDENT4;

    out << which << " status\n";

    if (state.suspend_reason == woinc::SUSPEND_REASON::NOT_SUSPENDED)
        out << indent << "not suspended";
    else
        out << indent << "suspended: " << state.suspend_reason;

    out << NL
        << indent << "current mode: " << state.mode << NL
        << indent << "perm mode: " << state.perm_mode << NL
        << indent << "perm becomes current in " << static_cast<long long>(state.delay) << " sec\n";
}

void print(std::ostream &out, const wrpc::GetCCStatusResponse &response) {
    out << "network connection status: " << response.cc_status.network_status << NL;
    print(out, "CPU", response.cc_status.cpu);
    print(out, "GPU", response.cc_status.gpu);
    print(out, "Network", response.cc_status.network);
}

void print(std::ostream &out, const woinc::Tasks &tasks) {
    auto indent = INDENT3;
    int counter = 0;

    out << std::fixed;
    out << "\n======== Tasks ========\n";

    for (const auto &task : tasks) {
        auto scheduler_state = woinc::SCHEDULER_STATE::UNINITIALIZED;
        auto active_task_state = woinc::ACTIVE_TASK_STATE::UNINITIALIZED;

        if (task.active_task != nullptr) {
            auto &active_task = *task.active_task;

            scheduler_state = active_task.scheduler_state;
            active_task_state = active_task.active_task_state;
        }

        out << ++counter << ") -----------" << NL
            << indent << "name: " << task.name << NL
            << indent << "WU name: " << task.wu_name << NL
            << indent << "project URL: " << task.project_url << NL
            << indent << "received: " << time_to_string(task.received_time) << NL
            << indent << "report deadline: " << time_to_string(task.report_deadline) << NL
            << indent << "ready to report: " << bool_to_string(task.ready_to_report) << NL
            << indent << "state: " << task.state << NL
            << indent << "scheduler state: " << scheduler_state << NL
            << indent << "active_task_state: " << active_task_state << NL
            << indent << "app version num: " << task.version_num << NL
            << indent << "resources: " << (task.resources.empty() ? "1 CPU" : task.resources) << NL;

        if (task.state <= woinc::RESULT_CLIENT_STATE::FILES_DOWNLOADED) {
            if (task.suspended_via_gui)
                out << indent << "suspended via GUI: yes" << NL;
            out << indent << "estimated CPU time remaining: " << task.estimated_cpu_time_remaining << NL;
        }

        if (scheduler_state > woinc::SCHEDULER_STATE::UNINITIALIZED && task.active_task != nullptr) {
            auto &active_task = *task.active_task;

            out << indent << "CPU time at last checkpoint: " << active_task.checkpoint_cpu_time << NL
                << indent << "current CPU time: " << active_task.current_cpu_time << NL
                << indent << "fraction done: " << active_task.fraction_done << NL
                << indent << "swap size: " << to_rounded_mibi(active_task.swap_size) << " MB" << NL
                << indent << "working set size: " << to_rounded_mibi(active_task.working_set_size_smoothed) << " MB" << NL;
            if (active_task.bytes_sent || active_task.bytes_received)
                out << indent << "bytes sent: " << round(active_task.bytes_sent)
                    << " received: " << round(active_task.bytes_received) << NL;
        }

        if (task.state > woinc::RESULT_CLIENT_STATE::FILES_DOWNLOADED) {
            out << indent << "final CPU time: " << task.final_cpu_time << NL
                << indent << "final elapsed time: " << task.final_elapsed_time << NL
                << indent << "exit_status: " << task.exit_status << NL
                << indent << "signal: " << task.signal << NL;
        }
    }
}

void print(std::ostream &out, const wrpc::GetResultsResponse &response) {
    print(out, response.tasks);
}

void print(std::ostream &out, const woinc::Projects &projects) {
    auto indent = INDENT3;
    int counter = 0;

    out << std::fixed;
    out << "======== Projects ========\n";

    for (const auto &project : projects) {
        out << ++counter << ") -----------\n"
            << indent << "name: " << project.project_name << NL
            << indent << "master URL: " << project.master_url << NL
            << indent << "user_name: " << project.user_name << NL
            << indent << "team_name: " << project.team_name << NL
            << indent << "resource share: " << project.resource_share << NL
            << indent << "user_total_credit: " << project.user_total_credit << NL
            << indent << "user_expavg_credit: " << project.user_expavg_credit << NL
            << indent << "host_total_credit: " << project.host_total_credit << NL
            << indent << "host_expavg_credit: " << project.host_expavg_credit << NL
            << indent << "nrpc_failures: " << project.nrpc_failures << NL
            << indent << "master_fetch_failures: " << project.master_fetch_failures << NL
            << indent << "master fetch pending: " << bool_to_string(project.master_url_fetch_pending) << NL
            << indent << "scheduler RPC pending: " << bool_to_string(project.sched_rpc_pending != woinc::RPC_REASON::NONE) << NL
            << indent << "trickle upload pending: " << bool_to_string(project.trickle_up_pending) << NL
            << indent << "attached via Account Manager: " << bool_to_string(project.attached_via_acct_mgr) << NL
            << indent << "ended: " << bool_to_string(project.ended) << NL
            << indent << "suspended via GUI: " << bool_to_string(project.suspended_via_gui) << NL
            << indent << "don't request more work: " << bool_to_string(project.dont_request_more_work) << NL
            << indent << "disk usage: " << project.desired_disk_usage << NL
            << indent << "last RPC: " << time_to_string(project.last_rpc_time) << NL
            << NL
            << indent << "project files downloaded: " << project.project_files_downloaded_time << NL;
        for (const auto &gui_url : project.gui_urls) {
            out << "GUI URL:" << NL
                << indent << "name: " << gui_url.name << NL
                << indent << "description: " << gui_url.description << NL
                << indent << "URL: " << gui_url.url << NL;
        }
        out << indent << "jobs succeeded: " << project.njobs_success << NL
            << indent << "jobs failed: " << project.njobs_error << NL
            << indent << "elapsed time: " << project.elapsed_time << NL
            << indent << "cross-project ID: " << project.external_cpid << NL;
    }
}

void print(std::ostream &out, const wrpc::GetProjectStatusResponse &response) {
    print(out, response.projects);
}

void print(std::ostream &out, const wrpc::GetMessagesResponse &response) {
    for (const auto &msg : response.messages)
        out << msg.seqno << ": "
            << time_to_string(msg.timestamp, "%d-%b-%Y %H:%M:%S")
            << " (" << msg.priority << ") "
            << "[" << msg.project << "] "
            << trimed(msg.body) << NL;
}

void print(std::ostream &out, const wrpc::GetNoticesResponse &response) {
    for (auto i = response.notices.crbegin(); i != response.notices.crend(); i++)
        out << i->seqno << ": ("
            << time_to_string(i->create_time, "%d-%b-%Y %H:%M:%S")
            << ") " << trimed(i->description) << NL;

}

void print(std::ostream &out, const wrpc::GetHostInfoResponse &response) {
    auto indent = INDENT2;
    out << std::fixed
        << indent << "timezone: "    << response.host_info.timezone << NL
        << indent << "domain name: " << response.host_info.domain_name << NL
        << indent << "IP addr: "     << response.host_info.ip_addr << NL
        << indent << "#CPUS: "       << response.host_info.p_ncpus << NL
        << indent << "CPU vendor: "  << response.host_info.p_vendor << NL
        << indent << "CPU model: "   << response.host_info.p_model << NL
        << indent << "CPU FP OPS: "  << response.host_info.p_fpops << NL
        << indent << "CPU int OPS: " << response.host_info.p_iops << NL
        << indent << "CPU mem BW: "  << response.host_info.p_membw << NL
        << indent << "OS name: "     << response.host_info.os_name << NL
        << indent << "OS version: "  << response.host_info.os_version << NL
        << indent << "mem size: "    << response.host_info.m_nbytes << NL
        << indent << "cache size: "  << response.host_info.m_cache << NL
        << indent << "swap size: "   << response.host_info.m_swap << NL
        << indent << "disk size: "   << response.host_info.d_total << NL
        << indent << "disk free: "   << response.host_info.d_free << NL;
}

void print(std::ostream &out, const woinc::Apps &apps, const woinc::Projects &projects) {
    auto indent = INDENT3;
    int counter = 0;

    out << std::fixed;
    out << "======== Applications ========\n";

    for (const auto &app : apps) {
        out << ++counter << ") -----------\n"
            << indent << "name: " << app.name << NL
            << indent << "Project: " << resolve_project_name(projects, app.project_url) << NL;
    }

    out << std::scientific;
}

void print(std::ostream &out, const woinc::AppVersions &app_versions, const woinc::Projects &projects) {
    auto indent = INDENT3;
    int counter = 0;

    const auto org_precision = out.precision();

    out << std::fixed;
    out << "======== Application versions ========\n";

    for (const auto &app_version : app_versions) {
        out << ++counter << ") -----------\n"
            << indent << "project: " << resolve_project_name(projects, app_version.project_url) << NL
            << indent << "application: " <<  app_version.app_name << NL
            << indent << "platform: " << app_version.platform << NL;

        if (!app_version.plan_class.empty())
            out << indent << "plan class: " << app_version.plan_class << NL;

        out << indent << "version: " << std::setprecision(2) << app_version.version_num/100.0 << NL;
        if (app_version.avg_ncpus != 1)
            out << indent << "avg #CPUS: " << std::setprecision(3) << app_version.avg_ncpus << NL;
        out << indent << "estimated GFLOPS: " << std::setprecision(2) << app_version.flops/1e9 << NL;

        auto file_ref = std::find_if(app_version.file_refs.cbegin(), app_version.file_refs.cend(),
                                     [](const woinc::FileRef &f) { return f.main_program; });

        out << indent << "filename: ";
        if (file_ref != app_version.file_refs.cend())
            out << file_ref->file_name;
        out << NL;
    }

    out.precision(org_precision);
}

void print(std::ostream &out, const woinc::Workunits &workunits) {
    auto indent = INDENT3;
    int counter = 0;

    const auto org_precision = out.precision();

    out << "======== Workunits ========\n";

    for (const auto &wu : workunits) {
        out << std::scientific
            << ++counter << ") -----------\n"
            << indent << "name: " << wu.name << NL
            << indent << "FP estimate: " << wu.rsc_fpops_est << NL
            << indent << "FP bound: " << wu.rsc_fpops_bound << NL
            << std::fixed << std::setprecision(2)
            << indent << "memory bound: " << mibi(wu.rsc_memory_bound) << " MB" << NL
            << indent << "disk bound: " << mibi(wu.rsc_disk_bound) << " MB" << NL;
        out.precision(org_precision);
    }
}

void print(std::ostream &out, const woinc::TimeStats &time_stats) {
    auto indent = INDENT3;

    out << std::fixed
        << "======== Time stats ========\n"
        << indent << "now: " << time_stats.now << NL
        << indent << "on_frac: " << time_stats.on_frac << NL
        << indent << "connected_frac: " << time_stats.connected_frac << NL
        << indent << "cpu_and_network_available_frac: " << time_stats.cpu_and_network_available_frac << NL
        << indent << "active_frac: " << time_stats.active_frac << NL
        << indent << "gpu_active_frac: " << time_stats.gpu_active_frac << NL
        << indent << "client_start_time: " << time_to_string(time_stats.client_start_time) << NL
        << NL
        << indent << "previous_uptime: " << time_stats.previous_uptime << NL
        << indent << "session_active_duration: " << time_stats.session_active_duration << NL
        << indent << "session_gpu_active_duration: " << time_stats.session_gpu_active_duration << NL
        << indent << "total_start_time: " << time_to_string(time_stats.total_start_time) << NL
        << NL
        << indent << "total_duration: " << time_stats.total_duration << NL
        << indent << "total_active_duration: " << time_stats.total_active_duration << NL
        << indent << "total_gpu_active_duration: " << time_stats.total_gpu_active_duration << NL;
}

void print(std::ostream &out, const wrpc::GetClientStateResponse &response) {
    print(out, response.client_state.projects);
    out << NL;
    print(out, response.client_state.apps, response.client_state.projects);
    out << NL;
    print(out, response.client_state.app_versions, response.client_state.projects);
    out << NL;
    print(out, response.client_state.workunits);
    print(out, response.client_state.tasks);
    out << NL;
    print(out, response.client_state.time_stats);
}

void print(std::ostream &out, const wrpc::GetFileTransfersResponse &response) {
    auto indent = INDENT3;
    int counter = 0;

    out << "\n======== File transfers ========\n";

    for (const auto &file_transfer : response.file_transfers) {
        std::string direction("unknown");
        bool is_active = false;
        double time_so_far = 0;
        double bytes_xferred = 0;
        double xfer_speed = 0;

        if (file_transfer.persistent_file_xfer.get() != nullptr) {
            direction = bool_to_string(file_transfer.persistent_file_xfer->is_upload);
            time_so_far = file_transfer.persistent_file_xfer->time_so_far;
        }

        if (file_transfer.file_xfer.get() != nullptr) {
            is_active = true;
            bytes_xferred = file_transfer.file_xfer->bytes_xferred;
            xfer_speed = file_transfer.file_xfer->xfer_speed;
        }

        out << std::scientific
            << ++counter << ") -----------\n"
            << indent << "name: " << file_transfer.name << NL
            << indent << "direction: " << direction << NL
            << indent << "sticky: no\n" // this isn't sent by the client at all ..
            << indent << "xfer active: " << bool_to_string(is_active) << NL
            << indent << "time_so_far: " << time_so_far << NL
            << indent << "bytes_xferred: " << bytes_xferred << NL
            << indent << "xfer_speed: " << xfer_speed << NL;
    }
}

void print(std::ostream &out, const wrpc::GetDiskUsageResponse &response) {
    auto indent = INDENT3;
    int counter = 0;

    const auto org_precision = out.precision();

    auto usage_formatter = [](double usage) {
        // BOINC always converts to MB
#if 0
        double factor = 0.;
        const char *unit = nullptr;
        if (usage >= 1024.*1024.*1024.*1024.) {
            factor = 1024.*1024.*1024.*1024.;
            unit = "TB";
        } else if (usage >= 1024.*1024.*1024.) {
            factor = 1024.*1024.*1024.;
            unit = "GB";
        } else if (usage >= 1024.*1024.) {
            factor = 1024.*1024.;
            unit = "MB";
        } else if (usage >= 1024.) {
            factor = 1024.;
            unit = "KB";
        }

        std::stringstream ss;

        ss << std::fixed;

        if (unit) {
            assert(factor > 0);
            ss << std::setprecision(2) << usage/factor << unit;
        } else {
            ss << std::setprecision(0) << usage << "bytes";
        }

        return ss.str();
#else
        std::stringstream ss;
        ss << std::fixed;
        ss << std::setprecision(2) << usage/(1024.*1024.) << "MB";
        return ss.str();
#endif
    };

    out << "======== Disk usage ========\n"
        << std::fixed
        << "total: " << response.disk_usage.total << NL
        << "free: " << response.disk_usage.free << NL;

    for (const auto &project : response.disk_usage.projects) {
        out << ++counter << ") -----------\n"
            << std::setprecision(2)
            << indent << "master URL: " << project.master_url << NL
            << indent << "disk usage: " << usage_formatter(project.disk_usage) << NL;
    }

    out.precision(org_precision);
}

void print(std::ostream &out, const wrpc::SuccessResponse &response) {
    if (!response.success)
        out << "Failure\n";
}

} // printing helpers


// -----------------------
// --- parsing helpers ---
// -----------------------

namespace {

std::uint16_t parse_port(const std::string &strp) {
    std::uint16_t port = wrpc::Connection::DEFAULT_PORT;
    try {
        auto p = std::stoul(strp);
        if (p > std::numeric_limits<decltype(port)>::max())
            error_die("Given port is out of range");
        port = static_cast<decltype(port)>(p);
        if (std::to_string(port) != strp)
            error_die("Invalid port");
    } catch (...) {
        error_die("Given port is not a number");
    }
    return port;
}

void parse_host(std::string &hostname, std::uint16_t &port) {
    if (hostname[0] == '[') { // ipv6 address with port
        auto idx = hostname.rfind(']');
        if (idx == hostname.npos // no closing ]
            || idx == 1 // '[]'
            || idx + 1 == hostname.size() // '[foo]' without port)
            || hostname[idx+1] != ':') // '[foo]bar'
                error_die("Invalid IPv6");
        if (idx + 2 < hostname.size()) {
            port = parse_port(hostname.substr(idx+2));
            hostname = hostname.substr(1, idx - 1);
        } else {
            hostname = hostname.substr(1, hostname.size() - 2);
        }
    } else {
        auto idx = hostname.find(':');
        auto ridx = hostname.rfind(':');
        if (idx == ridx && idx != hostname.npos) {
            port = parse_port(hostname.substr(idx+1));
            hostname = hostname.substr(0, idx);
        }
    }
}

void empty_or_die(const Arguments &args) {
    if (!args.empty())
        die_unknown_command(args.front());
}

bool matches(Arguments &args, const std::string &what) {
    if (args.front() == what) {
        args.pop();
        return true;
    }
    return false;
}

std::string parse_next_as_string(Arguments &args) {
    assert(!args.empty());
    auto s = args.front();
    args.pop();
    return s;
}

std::string need_next_as_string(Arguments &args, const std::string &error) {
    if (args.empty())
        error_die(error);
    return parse_next_as_string(args);
}

int parse_next_as_int(Arguments &args) {
    assert(!args.empty());
    auto arg = args.front();
    args.pop();
    try {
         return std::stoi(arg.c_str());
    } catch (...) {
        error_die("Parameter \"" + arg + "\" it not a valid integer number");
    }
}

double parse_next_as_double(Arguments &args) {
    assert(!args.empty());
    auto arg = args.front();
    args.pop();
    try {
         return std::stod(arg.c_str());
    } catch (...) {
        error_die("Parameter \"" + arg + "\" it not a valid floating point number");
    }
}

woinc::PROJECT_OP parse_project_op(const std::string &op) {
    if (op == "allowmorework") return woinc::PROJECT_OP::ALLOWMOREWORK;
    if (op == "detach") return woinc::PROJECT_OP::DETACH;
    if (op == "detach_when_done") return woinc::PROJECT_OP::DETACH_WHEN_DONE;
    if (op == "dont_detach_when_done") return woinc::PROJECT_OP::DONT_DETACH_WHEN_DONE;
    if (op == "nomorework") return woinc::PROJECT_OP::NOMOREWORK;
    if (op == "reset") return woinc::PROJECT_OP::RESET;
    if (op == "resume") return woinc::PROJECT_OP::RESUME;
    if (op == "suspend") return woinc::PROJECT_OP::SUSPEND;
    if (op == "update") return woinc::PROJECT_OP::UPDATE;
    std::cerr << "Error: Unkown op \"" + op + "\" for command --project\n";
    exit(EXIT_FAILURE);
}

woinc::TASK_OP parse_task_op(const std::string &op) {
    if (op == "abort") return woinc::TASK_OP::ABORT;
    if (op == "resume") return woinc::TASK_OP::RESUME;
    if (op == "suspend") return woinc::TASK_OP::SUSPEND;
    std::cerr << "Error: Unkown op \"" + op + "\" for command --task\n";
    exit(EXIT_FAILURE);
}

woinc::RUN_MODE parse_run_mode(const std::string &mode) {
    if (mode == "always") return woinc::RUN_MODE::ALWAYS;
    if (mode == "auto") return woinc::RUN_MODE::AUTO;
    if (mode == "never") return woinc::RUN_MODE::NEVER;
    std::cerr << "Error: Unkown mode \"" + mode + "\" for command --set_run_mode\n";
    exit(EXIT_FAILURE);
}

woinc::FILE_TRANSFER_OP parse_file_transfer_op(const std::string &op) {
    if (op == "retry") return woinc::FILE_TRANSFER_OP::RETRY;
    else if (op == "abort") return woinc::FILE_TRANSFER_OP::ABORT;
    std::cerr << "Error: Unkown op \"" + op + "\" for command --file_transfer\n";
    exit(EXIT_FAILURE);
}

} // parsing helpers

// --------------------------
// --- the woinc commands ---
// --------------------------

#ifdef WOINC_CLI_COMMANDS
namespace {

std::string duration_to_string(double d) {
    auto dv = std::div(static_cast<int>(round(d)), 24 * 3600);
    int days = dv.quot;
    dv = std::div(dv.rem, 3600);
    int hours = dv.quot;
    dv = std::div(dv.rem, 60);

    auto manip = [](std::ostream &out) -> std::ostream & {
        return out << std::setw(2) << std::setfill('0');
    };

    std::stringstream ss;
    ss << days << " day(s) " << manip << hours << ":"
        << manip << dv.quot << ":" << manip << dv.rem;

    return ss.str();
}

template<typename ITER>
std::ostream &print_table(std::ostream &out, ITER first, ITER last) {
    const auto num_items_per_entry = std::tuple_size<typename ITER::value_type>();
    std::array<size_t, num_items_per_entry> widths = { 0 };

    // compute width of each column
    for (auto iter = first; iter != last; ++iter)
        for (size_t i = 0; i < num_items_per_entry; ++i)
            widths[i] = std::max(widths[i], (*iter)[i].size());

    // add space for spaces to the left and/or right
    for (size_t i = 0; i < num_items_per_entry; ++i) {
        if (i == 0 || i + 1 == num_items_per_entry)
            widths[i] ++;
        else
            widths[i] += 2;
    }

    // print entries
    for (auto iter = first; iter != last; ++ iter) {
        for (size_t i = 0; i < num_items_per_entry; ++i) {
            if (i == 0)
                out << std::setw(static_cast<int>(widths[i] - 1)) << (*iter)[i] << " |";
            else if (i + 1 != num_items_per_entry)
                out << " " << std::setw(static_cast<int>(widths[i] - 2)) << (*iter)[i] << " |";
            else
                out << std::setw(static_cast<int>(widths[i])) << (*iter)[i];
        }
        out << "\n";
        // underline the header
        if (iter == first) {
            for (size_t i = 0; i < num_items_per_entry; ++i) {
                out << std::string(widths[i], '-');
                if (i + 1 != num_items_per_entry)
                    out << "|";
            }
            out << "\n";
        }
    }

    return out;
}

void do_sum_remaining_cpu_time(Client &client) {
    woinc::Projects projects;
    woinc::Tasks tasks;

    {
        wrpc::GetProjectStatusCommand project_cmd;
        client.do_cmd(project_cmd);
        projects = std::move(project_cmd.response().projects);

        wrpc::GetResultsCommand task_cmd;
        client.do_cmd(task_cmd);
        tasks = std::move(task_cmd.response().tasks);
    }

    std::map<std::string, bool> is_non_cpu_intensive;
    std::map<std::string, double> seconds_by_project;

    for (auto &&project : projects) {
        is_non_cpu_intensive[project.master_url] = project.non_cpu_intensive;
        seconds_by_project[project.project_name] = 0;
    }

    for (auto &&task : tasks)
        if (!is_non_cpu_intensive[task.project_url])
            seconds_by_project[resolve_project_name(projects, task.project_url)]
                += task.estimated_cpu_time_remaining;

    const auto num_cpus = std::thread::hardware_concurrency();

    std::vector<std::array<std::string, 3>> entries;

    // header
    std::stringstream ss;
    ss << "Time / #CPU (" << num_cpus << ")";
    entries.push_back({ "Project", "Remaining CPU time", ss.str() });

    // projects
    for (auto &&piter : seconds_by_project)
        if (piter.second != 0)
            entries.push_back({piter.first, duration_to_string(piter.second), duration_to_string(piter.second / num_cpus)});

    // sum
    double seconds = std::accumulate(seconds_by_project.cbegin(), seconds_by_project.cend(), 0.,
                                     [](const double s, const auto &i) { return s + i.second; });
    entries.push_back({"Sum", duration_to_string(seconds), duration_to_string(seconds / num_cpus)});

    print_table(std::cout, entries.cbegin(), entries.cend());
}

void do_estimate_times(Client &client) {
    auto timepoint_to_string = [](const std::chrono::system_clock::time_point &t) {
        return time_to_string(std::chrono::system_clock::to_time_t(t));
    };

    auto print_left_col = [](std::ostream &out, int width, const std::string &what) {
        out << std::left << std::setw(width) << what;
    };

    auto print_right_col = [](std::ostream &out, int width, const std::string &what) {
        out << std::right << std::setw(width) << what << NL;
    };

    wrpc::GetResultsCommand cmd;
    client.do_cmd(cmd);

    for (const auto &task : cmd.response().tasks) {
        if (task.ready_to_report || task.active_task == nullptr)
            continue;

        const auto &active_task = *task.active_task;
        if (active_task.fraction_done == 0 || active_task.current_cpu_time == 0)
            continue;

        double estimated_time = active_task.current_cpu_time / active_task.fraction_done;
        auto finished_at = std::chrono::system_clock::now() + std::chrono::seconds(
            static_cast<int>(round(estimated_time - active_task.current_cpu_time)));
        std::string finished_at_str(timepoint_to_string(finished_at));

        int lw = 16;
        int rw = static_cast<int>(finished_at_str.length());

        std::cout << NL << task.name << NL << std::string(task.name.length(), '-') << NL << NL;
        print_left_col (std::cout, lw, "Estimated time");
        print_right_col(std::cout, rw, duration_to_string(estimated_time));
        print_left_col (std::cout, lw, "Already done");
        print_right_col(std::cout, rw, duration_to_string(active_task.current_cpu_time));
        print_left_col (std::cout, lw, "Time to finish");
        print_right_col(std::cout, rw, duration_to_string(estimated_time - active_task.current_cpu_time));
        print_left_col (std::cout, lw, "Finished at");
        print_right_col(std::cout, rw, finished_at_str);
    }
}

void print(const woinc::Statistics &statistics, const woinc::Projects &projects, bool user_mode) {
    auto indent = INDENT3;
    int counter = 0;

    auto print_stats_line = [&](std::ostream &out, double max, time_t day, double value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << max;
        int width = static_cast<int>(ss.str().length());
        out << indent << indent
            << time_to_string(day, "%0d. %b")
            << " " << std::right << std::setprecision(2) << std::setw(width) << value << NL;
    };

    std::cout
        << "======== Project statistics ========\n"
        << std::fixed;

    for (const auto &project_stats : statistics) {
        auto project = std::find_if(projects.cbegin(), projects.cend(),
                                    [&](const woinc::Project &p) { return p.master_url == project_stats.master_url; });
        auto last_updated = std::chrono::system_clock::now() -
            std::chrono::system_clock::from_time_t(project_stats.daily_statistics.back().day);

        auto max_avg = std::max_element(project_stats.daily_statistics.cbegin(), project_stats.daily_statistics.cend(),
                                        [&](const auto &a, const auto &b) {
                                            return user_mode
                                                ? a.user_expavg_credit < b.user_expavg_credit
                                                : a.host_expavg_credit < b.host_expavg_credit;
                                        });

        auto max_total = std::max_element(project_stats.daily_statistics.cbegin(), project_stats.daily_statistics.cend(),
                                          [&](const auto &a, const auto &b) {
                                              return user_mode
                                                  ? a.user_total_credit < b.user_total_credit
                                                  : a.host_total_credit < b.host_total_credit;
                                          });

        std::cout << ++counter << ") -----------\n"
            << indent << "Project: " << project->project_name << NL
            << indent << "Account: " << project->user_name << NL;
        if (!project->team_name.empty())
            std::cout << indent << "Team: " << project->team_name << NL;
        std::cout << indent << "Last updated: "
            << std::setprecision(0)
            << std::floor(std::chrono::duration_cast<std::chrono::seconds>(last_updated).count() / (24*3600))
            << " days ago" << NL;

        std::cout << indent << "Average statistics: " << NL;
        for (const auto &stats : project_stats.daily_statistics)
            print_stats_line(std::cout,
                             user_mode ? max_avg->user_expavg_credit : max_avg->host_expavg_credit,
                             stats.day,
                             user_mode ? stats.user_expavg_credit : stats.host_expavg_credit);

        std::cout << indent << "Total statistics: " << NL;
        for (const auto &stats : project_stats.daily_statistics)
            print_stats_line(std::cout,
                             user_mode ? max_total->user_total_credit : max_total->user_expavg_credit,
                             stats.day,
                             user_mode ? stats.user_total_credit : stats.host_total_credit);
    }
}

void do_get_statistics(Client &client, bool user_mode) {
    wrpc::GetProjectStatusCommand projects_cmd;
    client.do_cmd(projects_cmd);

    wrpc::GetStatisticsCommand stats_cmd;
    client.do_cmd(stats_cmd);

    print(stats_cmd.response().statistics, projects_cmd.response().projects, user_mode);
}

} // woinc commands
#endif

namespace {

#define NOPARSE(CMD) [](Arguments &) -> CommandContext { return new CMD(); }
#define EXECUTE(CMD) [](Client &client, CommandContext ctx) { \
    assert(ctx != nullptr); \
    client.do_cmd(*static_cast<CMD *>(ctx)); \
    print(std::cout, static_cast<CMD *>(ctx)->response()); \
    delete static_cast<CMD *>(ctx); \
}
#define CMD_WITHOUT_REQUEST_DATA(PARAM, CMD) { PARAM, { NOPARSE(CMD), EXECUTE(CMD) } }

CommandMap command_map() {
    return {
        CMD_WITHOUT_REQUEST_DATA("--client_version", wrpc::ExchangeVersionsCommand),
        { "--file_transfer", {
            [](Arguments &args) -> CommandContext {
                auto url = need_next_as_string(args, "Missing parameter URL for command --file_transfer");
                auto filename = need_next_as_string(args, "Missing parameter filename for command --file_transfer");
                auto op = need_next_as_string(args, "Missing parameter op for command --file_transfer");
                return new wrpc::FileTransferOpCommand({
                    parse_file_transfer_op(std::move(op)), std::move(url), std::move(filename)});
            },
            EXECUTE(wrpc::FileTransferOpCommand)
        }},
        CMD_WITHOUT_REQUEST_DATA("--get_cc_status", wrpc::GetCCStatusCommand),
        CMD_WITHOUT_REQUEST_DATA("--get_disk_usage", wrpc::GetDiskUsageCommand),
        CMD_WITHOUT_REQUEST_DATA("--get_file_transfers", wrpc::GetFileTransfersCommand),
        CMD_WITHOUT_REQUEST_DATA("--get_host_info", wrpc::GetHostInfoCommand),
        { "--get_messages", {
            [](Arguments &args) -> CommandContext {
                int seqno = args.empty() ? 0 : parse_next_as_int(args);
                return new wrpc::GetMessagesCommand({seqno});
            },
            EXECUTE(wrpc::GetMessagesCommand)
        }}, { "--get_notices", {
            [](Arguments &args) -> CommandContext {
                int seqno = args.empty() ? 0 : parse_next_as_int(args);
                return new wrpc::GetNoticesCommand({seqno});
            },
            EXECUTE(wrpc::GetNoticesCommand)
        }},
        CMD_WITHOUT_REQUEST_DATA("--get_project_status", wrpc::GetProjectStatusCommand),
        CMD_WITHOUT_REQUEST_DATA("--get_tasks", wrpc::GetResultsCommand),
        CMD_WITHOUT_REQUEST_DATA("--get_state", wrpc::GetClientStateCommand),
        CMD_WITHOUT_REQUEST_DATA("--network_available", wrpc::NetworkAvailableCommand),
        { "--project", {
            [](Arguments &args) -> CommandContext {
                auto url = need_next_as_string(args, "Missing parameter URL for command --project");
                auto op = need_next_as_string(args, "Missing parameter op for command --project");
                return new wrpc::ProjectOpCommand({parse_project_op(std::move(op)), std::move(url)});
            },
            EXECUTE(wrpc::ProjectOpCommand)
        }}, { "--project_attach", {
            [](Arguments &args) -> CommandContext {
                auto url = need_next_as_string(args, "Missing parameter URL for command --project_attach");
                auto auth = need_next_as_string(args, "Missing parameter auth for command --project_attach");
                return new wrpc::ProjectAttachCommand({std::move(url), std::move(auth)});
            },
            EXECUTE(wrpc::ProjectAttachCommand)
        }}, { "--task", {
            [](Arguments &args) -> CommandContext {
                auto url = need_next_as_string(args, "Missing parameter url for command --task");
                auto name = need_next_as_string(args, "Missing parameter name for command --task");
                auto op = need_next_as_string(args, "Missing parameter op for command --task");
                return new wrpc::TaskOpCommand({parse_task_op(std::move(op)), std::move(url), std::move(name)});
            },
            EXECUTE(wrpc::TaskOpCommand)
        }},
        CMD_WITHOUT_REQUEST_DATA("--quit", wrpc::QuitCommand),
        CMD_WITHOUT_REQUEST_DATA("--read_cc_config", wrpc::ReadCCConfigCommand),
        CMD_WITHOUT_REQUEST_DATA("--read_global_prefs_override", wrpc::ReadGlobalPreferencesOverrideCommand),
        CMD_WITHOUT_REQUEST_DATA("--run_benchmarks", wrpc::RunBenchmarksCommand),
        { "--set_gpu_mode", {
            [](Arguments &args) -> CommandContext {
                auto mode = need_next_as_string(args, "Missing parameter mode for command --set_gpu_mode");
                auto duration = args.empty() ? 0 : parse_next_as_double(args);
                return new wrpc::SetGpuModeCommand({parse_run_mode(std::move(mode)), duration});
            },
            EXECUTE(wrpc::SetGpuModeCommand)
        }}, { "--set_network_mode", {
            [](Arguments &args) -> CommandContext {
                auto mode = need_next_as_string(args, "Missing parameter mode for command --set_network_mode");
                auto duration = args.empty() ? 0 : parse_next_as_double(args);
                return new wrpc::SetNetworkModeCommand({parse_run_mode(std::move(mode)), duration});
            },
            EXECUTE(wrpc::SetNetworkModeCommand)
        }}, { "--set_run_mode", {
            [](Arguments &args) -> CommandContext {
                auto mode = need_next_as_string(args, "Missing parameter mode for command --set_run_mode");
                auto duration = args.empty() ? 0 : parse_next_as_double(args);
                return new wrpc::SetRunModeCommand({parse_run_mode(std::move(mode)), duration});
            },
            EXECUTE(wrpc::SetRunModeCommand)
#ifdef WOINC_CLI_COMMANDS
        }}, { "--sum_remaining_cpu_time", {
            [](Arguments &) -> CommandContext { return nullptr; },
            [](Client &client, CommandContext) { do_sum_remaining_cpu_time(client); }
        }}, { "--estimate_times", {
            [](Arguments &) -> CommandContext { return nullptr; },
            [](Client &client, CommandContext) { do_estimate_times(client); }
        }}, { "--get_statistics", {
            [](Arguments &args) -> CommandContext {
                auto *user_mode = new bool;
                *user_mode = true;

                if (matches(args, "user"))
                    *user_mode = true;
                else if (matches(args, "host"))
                    *user_mode = false;

                return user_mode;
            },
            [](Client &client, CommandContext ctx) {
                do_get_statistics(client, *static_cast<bool *>(ctx));
                delete static_cast<bool *>(ctx);
            }
#endif
        }}
    };
}


} // commands
