/* ui/controller/controller.cc --
   Written and Copyright (C) 2017-2023 by vmc.

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

#include <woinc/ui/controller.h>

#include <cassert>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

#ifndef NDEBUG
#include <iostream>
#endif

#include "configuration.h"
#include "handler_registry.h"
#include "host_controller.h"
#include "periodic_tasks_scheduler.h"

#define WOINC_LOCK_GUARD std::lock_guard<decltype(mutex_)> guard(mutex_)

namespace wrpc = woinc::rpc;

namespace {

void check_not_empty__(const std::string &str, const std::string &msg) {
    if (str.empty())
        throw std::invalid_argument(msg);
}

void check_not_empty_host_name__(const std::string &host) {
    check_not_empty__(host, "Missing host name");
}

}

namespace woinc { namespace ui {

// ---- Controller::Impl ----

class WOINCUI_LOCAL Controller::Impl {
    public:
        Impl();
        ~Impl();

        Impl(const Impl &) = delete;
        Impl &operator=(const Impl &) = delete;

        Impl(Impl &&) = default;
        Impl &operator=(Impl &&) = default;

        void shutdown();

        void register_handler(HostHandler *handler);
        void deregister_handler(HostHandler *handler);

        void register_handler(PeriodicTaskHandler *handler);
        void deregister_handler(PeriodicTaskHandler *handler);

        void add_host(std::string host,
                      std::string url,
                      std::uint16_t port);
        void authorize_host(std::string host,
                            std::string password);

        void remove_host(const std::string &host);
        void async_remove_host(std::string host);

        void periodic_task_interval(const PeriodicTask task, std::chrono::milliseconds interval);
        std::chrono::milliseconds periodic_task_interval(const PeriodicTask task) const;
        void schedule_periodic_tasks(const std::string &host, bool value);
        void reschedule_now(const std::string &host, PeriodicTask task);

        void active_only_tasks(const std::string &host, bool value);

        std::future<bool> file_transfer_op(const std::string &host, FileTransferOp op,
                                           const std::string &master_url, const std::string &filename);
        std::future<bool> project_op(const std::string &host, ProjectOp op, const std::string &master_url);
        std::future<bool> task_op(const std::string &host, TaskOp op, const std::string &master_url, const std::string &task_name);

        std::future<GlobalPreferences> load_global_preferences(const std::string &host, GetGlobalPrefsMode mode);
        std::future<bool> save_global_preferences(const std::string &host, const GlobalPreferences &prefs, const GlobalPreferencesMask &mask);
        std::future<bool> read_global_prefs_override(const std::string &host);

        std::future<CCConfig> cc_config(const std::string &host);
        std::future<bool> cc_config(const std::string &host, const CCConfig &cc_config);

        std::future<bool> read_config_files(const std::string &host);

        std::future<bool> run_mode(const std::string &host, RunMode mode);
        std::future<bool> gpu_mode(const std::string &host, RunMode mode);
        std::future<bool> network_mode(const std::string &host, RunMode mode);

        std::future<AllProjectsList> all_projects_list(const std::string &host);

        std::future<bool> start_loading_project_config(const std::string &host, std::string master_url);
        std::future<ProjectConfig> poll_project_config(const std::string &host);

        std::future<bool> start_account_lookup(const std::string &host, std::string master_url,
                                               std::string email, std::string password);
        std::future<AccountOut> poll_account_lookup(const std::string &host);

        std::future<bool> attach_project(const std::string &host, std::string master_url, std::string authenticator);

        std::future<bool> network_available(const std::string &host);
        std::future<bool> run_benchmarks(const std::string &host);
        std::future<bool> quit(const std::string &host);

    private: // helper methods which assume the controller is already locked
        // use a copy of the host string as it may be the key of the host controller map
        // which will be deleted in the erase call leading to a use after free access later on
        void remove_host_(std::string host);
        void async_remove_host_(std::string host);

        bool has_host_(const std::string &name) const;

        void schedule_now_(const std::string &host, std::unique_ptr<Job> job, const char *func);

        void verify_not_shutdown_() const;
        void verify_known_host_(const std::string &host, const char *func) const;

        template<typename Command,
                 typename Result,
                 typename Getter = Result (*)(decltype(std::declval<Command>().response()) &),
                 typename Request = decltype(std::declval<Command>().request())>
        std::future<Result> create_and_schedule_async_job_(const char *func,
                                                           const std::string &host,
                                                           Getter getter,
                                                           std::string error_msg,
                                                           std::remove_reference_t<Request> request = {}) {
            typedef std::promise<Result> Promise;
            Promise promise;
            auto future = promise.get_future();

            auto job = std::make_unique<woinc::ui::AsyncJob<Result>>(
                std::make_unique<Command>(std::move(request)),
                std::move(promise),
                [error_msg, getter](woinc::rpc::Command *cmd, Promise &p, woinc::rpc::CommandStatus status) {
                    if (status == woinc::rpc::CommandStatus::Ok)
                        p.set_value(getter(static_cast<Command *>(cmd)->response()));
                    else
                        p.set_exception(std::make_exception_ptr(std::runtime_error{error_msg}));
                });

            schedule_now_(host, std::move(job), func);
            return future;
        }


    private:
        std::mutex mutex_;

        bool shutdown_ = false;

        HandlerRegistry handler_registry_;

        Configuration configuration_;

        PeriodicTasksSchedulerContext periodic_tasks_scheduler_context_;
        std::thread periodic_tasks_scheduler_thread_;

        typedef std::map<std::string, std::unique_ptr<HostController>> HostControllers;
        HostControllers host_controllers_;
};

Controller::Impl::Impl() :
    periodic_tasks_scheduler_context_(configuration_,
                                      handler_registry_,
                                      [this](const std::string &host, std::unique_ptr<Job> job) { host_controllers_.at(host)->schedule(std::move(job)); }),
    periodic_tasks_scheduler_thread_(PeriodicTasksScheduler(periodic_tasks_scheduler_context_))
{}

Controller::Impl::~Impl() {
    shutdown();
}

void Controller::Impl::shutdown() {
    WOINC_LOCK_GUARD;

    // shutdown the controller, i.e. don't accept requests anymore
    shutdown_ = true;


    // shutdown the periodic tasks scheduler
    periodic_tasks_scheduler_context_.trigger_shutdown();
    if (periodic_tasks_scheduler_thread_.joinable())
        periodic_tasks_scheduler_thread_.join();

    // shutdown the host controllers
    while (!host_controllers_.empty())
        remove_host_(host_controllers_.cbegin()->first);
}

void Controller::Impl::register_handler(HostHandler *handler) {
    handler_registry_.register_handler(handler);
}

void Controller::Impl::deregister_handler(HostHandler *handler) {
    handler_registry_.deregister_handler(handler);
}

void Controller::Impl::register_handler(PeriodicTaskHandler *handler) {
    handler_registry_.register_handler(handler);
}

void Controller::Impl::deregister_handler(PeriodicTaskHandler *handler) {
    handler_registry_.deregister_handler(handler);
}

void Controller::Impl::add_host(std::string host,
                                std::string url,
                                std::uint16_t port) {
    check_not_empty_host_name__(host);
    check_not_empty__(url, "Missing url to host");

    // to be able to use it by copy in the connect-thread
    HostController *host_controller_ptr;

    {
        WOINC_LOCK_GUARD;

        verify_not_shutdown_();

        if (has_host_(host))
            throw std::invalid_argument("Host \"" + host + "\" already registered.");

        auto host_controller = std::make_unique<HostController>(host);
        host_controller_ptr = host_controller.get();

        configuration_.add_host(host);
        host_controllers_.emplace(host, std::move(host_controller));
        // periodic tasks are not scheduled yet
        periodic_tasks_scheduler_context_.add_host(host);

        handler_registry_.for_host_handler([&](auto &handler) {
            handler.on_host_added(host);
        });
    }

    // connect asynchronously because the connect may block for a long time (see man 2 connect)
    std::thread([this, host, host_controller_ptr, port, url]() {
        bool connected = host_controller_ptr->connect(url, port);
        handler_registry_.for_host_handler([&](HostHandler &handler) {
            if (connected)
                handler.on_host_connected(host);
            else
                handler.on_host_error(host, Error::ConnectionError);
        });
    }).detach();
}

void Controller::Impl::authorize_host(std::string host,
                                      std::string password) {
    check_not_empty_host_name__(host);
    check_not_empty__(password, "Missing password");

    WOINC_LOCK_GUARD;

    verify_not_shutdown_();
    verify_known_host_(host, __func__);

    auto hc = host_controllers_.find(host);
    assert(hc != host_controllers_.end());
    hc->second->authorize(password, handler_registry_);
}

void Controller::Impl::remove_host(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;
    verify_not_shutdown_();
    verify_known_host_(host, __func__);
    remove_host_(host);
}

void Controller::Impl::async_remove_host(std::string host) {
    check_not_empty_host_name__(host);

    std::thread([this, host]() { async_remove_host_(host); }).detach();
}

void Controller::Impl::periodic_task_interval(const PeriodicTask task, std::chrono::milliseconds interval) {
    configuration_.interval(task, interval);
}

std::chrono::milliseconds Controller::Impl::periodic_task_interval(const PeriodicTask task) const {
    return configuration_.interval(task);
}

void Controller::Impl::schedule_periodic_tasks(const std::string &host, bool value) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    verify_not_shutdown_();
    verify_known_host_(host, __func__);

    configuration_.schedule_periodic_tasks(host, value);
}

void Controller::Impl::reschedule_now(const std::string &host, PeriodicTask task) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    verify_not_shutdown_();
    verify_known_host_(host, __func__);

    periodic_tasks_scheduler_context_.reschedule_now(host, task);
}

void Controller::Impl::active_only_tasks(const std::string &host, bool value) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    verify_not_shutdown_();
    verify_known_host_(host, __func__);

    configuration_.active_only_tasks(host, value);
    periodic_tasks_scheduler_context_.reschedule_now(host, PeriodicTask::GetTasks);
}

std::future<bool> Controller::Impl::file_transfer_op(const std::string &host, FileTransferOp op,
                                                     const std::string &master_url, const std::string &filename) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");
    check_not_empty__(filename, "Missing filename");

    WOINC_LOCK_GUARD;

    auto future = create_and_schedule_async_job_<wrpc::FileTransferOpCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error while executing file transfer operation",
        {op, master_url, filename});

    periodic_tasks_scheduler_context_.reschedule_now(host, PeriodicTask::GetFileTransfers);

    return future;
}

std::future<bool> Controller::Impl::project_op(const std::string &host, ProjectOp op, const std::string &master_url) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");

    WOINC_LOCK_GUARD;

    auto future = create_and_schedule_async_job_<wrpc::ProjectOpCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error while executing project operation",
        {op, master_url});

    periodic_tasks_scheduler_context_.reschedule_now(host, PeriodicTask::GetProjectStatus);

    return future;
}

std::future<bool> Controller::Impl::task_op(const std::string &host, TaskOp op,
                                            const std::string &master_url, const std::string &task_name) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");
    check_not_empty__(task_name, "Missing task name");

    WOINC_LOCK_GUARD;

    auto future = create_and_schedule_async_job_<rpc::TaskOpCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error while executing task operation",
        {op, master_url, task_name});

    periodic_tasks_scheduler_context_.reschedule_now(host, PeriodicTask::GetTasks);

    return future;
}

std::future<GlobalPreferences> Controller::Impl::load_global_preferences(const std::string &host, GetGlobalPrefsMode mode) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::GetGlobalPreferencesCommand, GlobalPreferences>(
        __func__,
        host,
        [](const auto &r) { return r.preferences; },
        "Error while loading the preferences",
        {mode});
}

std::future<bool> Controller::Impl::save_global_preferences(const std::string &host, const GlobalPreferences &prefs, const GlobalPreferencesMask &mask) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::SetGlobalPreferencesCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error while setting the preferences",
        {prefs, mask});
}

std::future<bool> Controller::Impl::read_global_prefs_override(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::ReadGlobalPreferencesOverrideCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error reading the preferences");
}

std::future<CCConfig> Controller::Impl::cc_config(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::GetCCConfigCommand, CCConfig>(
        __func__,
        host,
        [](const auto &r) { return r.cc_config; },
        "Error reading the cc_config");
}

std::future<bool> Controller::Impl::cc_config(const std::string &host, const CCConfig &cc_config) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::SetCCConfigCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error writing the cc_config",
        {cc_config});
}

std::future<bool> Controller::Impl::read_config_files(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::ReadCCConfigCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error reading the config files");
}

std::future<bool> Controller::Impl::run_mode(const std::string &host, RunMode mode) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::SetRunModeCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error setting the run mode",
        mode);
}

std::future<bool> Controller::Impl::gpu_mode(const std::string &host, RunMode mode) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::SetGpuModeCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error setting the gpu run mode",
        mode);
}

std::future<bool> Controller::Impl::network_mode(const std::string &host, RunMode mode) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::SetNetworkModeCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error setting the network mode",
        mode);
}

std::future<AllProjectsList> Controller::Impl::all_projects_list(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::GetAllProjectsListCommand, AllProjectsList>(
        __func__,
        host,
        [](const auto &r) { return r.projects; },
        "Error getting the projects list");
}

std::future<bool> Controller::Impl::start_loading_project_config(const std::string &host, std::string master_url) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::GetProjectConfigCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error loading the project config",
        {master_url});
}

std::future<ProjectConfig> Controller::Impl::poll_project_config(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::GetProjectConfigPollCommand, ProjectConfig>(
        __func__,
        host,
        [](const auto &r) { return r.project_config; },
        "Error polling the project config");
}

std::future<bool> Controller::Impl::start_account_lookup(const std::string &host, std::string master_url,
                                                         std::string email, std::string password) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");
    check_not_empty__(email, "Missing email");
    check_not_empty__(password, "Missing password");

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::LookupAccountCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error looking up the account info",
        {master_url, email, password});
}

std::future<AccountOut> Controller::Impl::poll_account_lookup(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::LookupAccountPollCommand, AccountOut>(
        __func__,
        host,
        [](const auto &r) { return r.account_out; },
        "Error polling the account info");
}

std::future<bool> Controller::Impl::attach_project(const std::string &host,
                                                   std::string master_url,
                                                   std::string authenticator) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");
    check_not_empty__(authenticator, "Missing authenticator");

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::ProjectAttachCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error attaching the project",
        {std::move(master_url), std::move(authenticator)});
}

std::future<bool> Controller::Impl::network_available(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::NetworkAvailableCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error retrying deferred network communication");
}

std::future<bool> Controller::Impl::run_benchmarks(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::RunBenchmarksCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error triggering the benchmarks run");
}

std::future<bool> Controller::Impl::quit(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::QuitCommand, bool>(
        __func__,
        host,
        [](const auto &r) { return r.success; },
        "Error quitting the client");
}

void Controller::Impl::remove_host_(std::string host) {
    periodic_tasks_scheduler_context_.remove_host(host);
    host_controllers_.at(host)->shutdown();
    host_controllers_.erase(host);
    handler_registry_.for_host_handler([&](HostHandler &handler) { handler.on_host_removed(host); });
    configuration_.remove_host(host);
}

void Controller::Impl::async_remove_host_(std::string host) {
    WOINC_LOCK_GUARD;
    verify_not_shutdown_();
    remove_host_(host);
}

bool Controller::Impl::has_host_(const std::string &name) const {
    return host_controllers_.find(name) != host_controllers_.end();
}

#ifndef NDEBUG
void Controller::Impl::schedule_now_(const std::string &host, std::unique_ptr<Job> job, const char *func) {
#else
void Controller::Impl::schedule_now_(const std::string &host, std::unique_ptr<Job> job, const char *) {
#endif
    assert(job != nullptr);
    assert(func != nullptr);

    verify_not_shutdown_();

    auto hc = host_controllers_.find(host);
    if (hc == host_controllers_.end()) {
#ifndef NDEBUG
        std::cerr << "Controller::" << func << " on non existing host \"" << host << "\" called\n";
#endif
        throw UnknownHostException{host};
    }

    hc->second->schedule_now(std::move(job));
}

void Controller::Impl::verify_not_shutdown_() const {
    if (shutdown_)
        throw ShutdownException();
}

#ifndef NDEBUG
void Controller::Impl::verify_known_host_(const std::string &host, const char *func) const {
#else
void Controller::Impl::verify_known_host_(const std::string &host, const char *) const {
#endif
    assert(func != nullptr);
    if (!has_host_(host)) {
#ifndef NDEBUG
        std::cerr << "Controller::" << func << " on non existing host \"" << host << "\" called\n";
#endif
        throw UnknownHostException{host};
    }
}

// ---- Controller ----

Controller::Controller()
    : impl_(std::make_unique<Impl>())
{}

Controller::~Controller() {
    shutdown();
}

void Controller::shutdown() {
    impl_->shutdown();
}

void Controller::register_handler(HostHandler *handler) {
    impl_->register_handler(handler);
}

void Controller::deregister_handler(HostHandler *handler) {
    impl_->deregister_handler(handler);
}

void Controller::register_handler(PeriodicTaskHandler *handler) {
    impl_->register_handler(handler);
}

void Controller::deregister_handler(PeriodicTaskHandler *handler) {
    impl_->deregister_handler(handler);
}

void Controller::add_host(const std::string &host,
                          const std::string &url,
                          std::uint16_t port) {
    impl_->add_host(host, url, port);
}

void Controller::authorize_host(const std::string &host,
                                const std::string &password) {
    impl_->authorize_host(host, password);
}

void Controller::remove_host(const std::string &host) {
    impl_->remove_host(host);
}

void Controller::async_remove_host(std::string host) {
    impl_->async_remove_host(host);
}

void Controller::periodic_task_interval(const PeriodicTask task, std::chrono::milliseconds interval) {
    impl_->periodic_task_interval(task, interval);
}

std::chrono::milliseconds Controller::periodic_task_interval(const PeriodicTask task) const {
    return impl_->periodic_task_interval(task);
}

void Controller::schedule_periodic_tasks(const std::string &host, bool value) {
    impl_->schedule_periodic_tasks(host, value);
}

void Controller::reschedule_now(const std::string &host, PeriodicTask task) {
    impl_->reschedule_now(host, task);
}

void Controller::active_only_tasks(const std::string &host, bool value) {
    impl_->active_only_tasks(host, value);
}

std::future<bool> Controller::file_transfer_op(const std::string &host, FileTransferOp op,
                                               const std::string &master_url, const std::string &filename) {
    return impl_->file_transfer_op(host, op, master_url, filename);
}

std::future<bool> Controller::project_op(const std::string &host, ProjectOp op, const std::string &master_url) {
    return impl_->project_op(host, op, master_url);
}

std::future<bool> Controller::task_op(const std::string &host, TaskOp op,
                         const std::string &master_url, const std::string &task_name) {
    return impl_->task_op(host, op, master_url, task_name);
}

std::future<GlobalPreferences> Controller::load_global_preferences(const std::string &host,
                                                                   GetGlobalPrefsMode mode) {
    return impl_->load_global_preferences(host, mode);
}

std::future<bool> Controller::save_global_preferences(const std::string &host,
                                                      const GlobalPreferences &prefs,
                                                      const GlobalPreferencesMask &mask) {
    return impl_->save_global_preferences(host, prefs, mask);
}

std::future<bool> Controller::read_global_prefs_override(const std::string &host) {
    return impl_->read_global_prefs_override(host);
}

std::future<CCConfig> Controller::cc_config(const std::string &host) {
    return impl_->cc_config(host);
}

std::future<bool> Controller::cc_config(const std::string &host, const CCConfig &cc_config) {
    return impl_->cc_config(host, cc_config);
}

std::future<bool> Controller::read_config_files(const std::string &host) {
    return impl_->read_config_files(host);
}

std::future<bool> Controller::gpu_mode(const std::string &host, RunMode mode) {
    return impl_->gpu_mode(host, mode);
}

std::future<bool> Controller::network_mode(const std::string &host, RunMode mode) {
    return impl_->network_mode(host, mode);
}

std::future<bool> Controller::run_mode(const std::string &host, RunMode mode) {
    return impl_->run_mode(host, mode);
}

std::future<AllProjectsList> Controller::all_projects_list(const std::string &host) {
    return impl_->all_projects_list(host);
}

std::future<bool> Controller::start_loading_project_config(const std::string &host, std::string master_url) {
    return impl_->start_loading_project_config(host, std::move(master_url));
}

std::future<ProjectConfig> Controller::poll_project_config(const std::string &host) {
    return impl_->poll_project_config(host);
}

std::future<bool> Controller::start_account_lookup(const std::string &host, std::string master_url,
                                                   std::string email, std::string password) {
    return impl_->start_account_lookup(host, std::move(master_url), std::move(email), std::move(password));
}

std::future<AccountOut> Controller::poll_account_lookup(const std::string &host) {
    return impl_->poll_account_lookup(host);
}

std::future<bool> Controller::attach_project(const std::string &host,
                                             std::string master_url,
                                             std::string authenticator) {
    return impl_->attach_project(host, std::move(master_url), std::move(authenticator));
}

std::future<bool> Controller::network_available(const std::string &host) {
    return impl_->network_available(host);
}

std::future<bool> Controller::run_benchmarks(const std::string &host) {
    return impl_->run_benchmarks(host);
}

std::future<bool> Controller::quit(const std::string &host) {
    return impl_->quit(host);
}

}}
